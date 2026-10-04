#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bytecode.h"
#include "frame.h"
#include "heap.h"
#include "native.h"
#include "runtime.h"

typedef struct { JVMR_Frame frame; const JVMR_Method *method; const JVMR_Class *klass; uint8_t box_return; const char *return_descriptor; } SavedFrame;
static const JVMR_Class *runtime_class;
static const JVMR_Method *runtime_method;
static JVMR_ClassLoader *runtime_loader;
static SavedFrame call_stack[JVMR_CALL_STACK_MAX];
static uint32_t call_depth;
static int invoke_declared_method;

static int descriptor_arguments(const char *descriptor) {
	if (!descriptor || descriptor[0] != '(') return -1;
	int slots = 0;
	for (const char *p = descriptor + 1; *p && *p != ')'; p++) {
		if (*p == '[') { while (*p == '[') p++; if (*p == 'L') while (*p && *p != ';') p++; if (!*p) return -1; slots++; }
		else if (*p == 'L') { while (*p && *p != ';') p++; if (!*p) return -1; slots++; }
		else if (*p == 'B' || *p == 'C' || *p == 'D' || *p == 'F' || *p == 'I' || *p == 'J' || *p == 'S' || *p == 'Z') slots++;
		else return -1;
	}
	return strchr(descriptor, ')') ? slots : -1;
}

int jvmr_ensure_initialized(const JVMR_Class *klass) {
	if (!klass || klass->initialized) return 0;
	JVMR_Class *mutable_class=(JVMR_Class *)klass;
	if (mutable_class->initializing) return 0;
	mutable_class->initializing=1;
	const JVMR_Method *clinit=jvmr_class_find_method(klass,"<clinit>","()V");
	if (clinit && clinit->code) {
		SavedFrame *saved=call_depth?malloc((size_t)call_depth*sizeof(*saved)):NULL;
		if(call_depth&&!saved){mutable_class->initializing=0;return -1;}
		if(saved)memcpy(saved,call_stack,(size_t)call_depth*sizeof(*saved));
		JVMR_Frame caller_frame=current_frame;const JVMR_Method *caller_method=runtime_method;const JVMR_Class *caller_class=runtime_class;uint32_t caller_depth=call_depth;
		call_depth=0;memset(&current_frame,0,sizeof(current_frame));runtime_method=clinit;runtime_class=klass;
		jvm_execute((uint8_t *)clinit->code,clinit->code_length);
		current_frame=caller_frame;runtime_method=caller_method;runtime_class=caller_class;call_depth=caller_depth;
		if(saved){memcpy(call_stack,saved,(size_t)call_depth*sizeof(*saved));free(saved);}
	}
	mutable_class->initializing=0;mutable_class->initialized=1;return 0;
}

static const char *cp_utf8(const JVMR_Class *klass,uint16_t index) { return klass&&index<klass->constant_pool_count&&klass->constant_pool[index].tag==JVMR_CP_UTF8 ? (const char *)klass->constant_pool[index].value.utf8.bytes : NULL; }
static int append_text(char *output,size_t capacity,size_t *used,const char *text){size_t length=strlen(text);if(*used+length+1>capacity)return -1;memcpy(output+*used,text,length);*used+=length;output[*used]=0;return 0;}
static const char *dynamic_recipe(const JVMR_Class *klass,uint16_t bootstrap){if(!klass||bootstrap>=klass->bootstrap_count||!klass->bootstraps[bootstrap].argument_count)return NULL;uint16_t index=klass->bootstraps[bootstrap].arguments[0];if(index>=klass->constant_pool_count||klass->constant_pool[index].tag!=JVMR_CP_STRING)return NULL;return cp_utf8(klass,klass->constant_pool[index].value.index);}
static int exception_is_a(const JVMR_Class *thrown,const char *wanted) { for(const JVMR_Class *current=thrown;current;){if(current->name&&!strcmp(current->name,wanted))return 1;if(!current->super_name||!runtime_loader)return 0;current=jvmr_classloader_load(runtime_loader,current->super_name,NULL,0);}return 0; }
static int exception_matches(const JVMR_Class *klass,const JVMR_ExceptionHandler *handler,const JVMR_Class *thrown) { if(!handler->catch_type)return 1;if(!klass||!thrown||handler->catch_type>=klass->constant_pool_count)return 0;const JVMR_Constant *entry=&klass->constant_pool[handler->catch_type];if(entry->tag!=JVMR_CP_CLASS)return 0;const char *name=cp_utf8(klass,entry->value.index);return name&&exception_is_a(thrown,name); }
static int reference_info(uint16_t index,const char **owner,const char **name,const char **descriptor) {
	if(!runtime_class||index>=runtime_class->constant_pool_count)return -1;
	const JVMR_Constant *ref=&runtime_class->constant_pool[index];
	if(ref->tag!=JVMR_CP_METHODREF&&ref->tag!=JVMR_CP_INTERFACEMETHODREF)return -1;
	if(ref->value.pair.left>=runtime_class->constant_pool_count||ref->value.pair.right>=runtime_class->constant_pool_count)return -1;
	const JVMR_Constant *oc=&runtime_class->constant_pool[ref->value.pair.left],*nt=&runtime_class->constant_pool[ref->value.pair.right];
	if(oc->tag!=JVMR_CP_CLASS||nt->tag!=JVMR_CP_NAMEANDTYPE)return -1;
	*owner=cp_utf8(runtime_class,oc->value.index);*name=cp_utf8(runtime_class,nt->value.pair.left);*descriptor=cp_utf8(runtime_class,nt->value.pair.right);
	return *owner&&*name&&*descriptor?0:-1;
}
static const JVMR_Method *resolve_method(uint16_t index,const JVMR_Class **owner_out,const char **name_out,const char **descriptor_out) {
	const char *owner,*name,*descriptor;if(reference_info(index,&owner,&name,&descriptor))return NULL;const JVMR_Class *klass=NULL;if(runtime_class->name&&!strcmp(owner,runtime_class->name))klass=runtime_class;else if(runtime_loader)klass=jvmr_classloader_load(runtime_loader,owner,NULL,0);if(!klass)return NULL;for(const JVMR_Class *current=klass;current;){const JVMR_Method *method=jvmr_class_find_method(current,name,descriptor);if(method){if(owner_out)*owner_out=current;if(name_out)*name_out=name;if(descriptor_out)*descriptor_out=descriptor;return method;}if(!current->super_name||!runtime_loader)break;current=jvmr_classloader_load(runtime_loader,current->super_name,NULL,0);}return NULL;
}
const JVMR_Field *jvmr_runtime_resolve_field(uint16_t index, const JVMR_Class **owner_out) {
	if (!runtime_class || index >= runtime_class->constant_pool_count) return NULL;
	const JVMR_Constant *ref=&runtime_class->constant_pool[index];
	if (ref->tag!=JVMR_CP_FIELDREF || ref->value.pair.left>=runtime_class->constant_pool_count || ref->value.pair.right>=runtime_class->constant_pool_count) return NULL;
	const JVMR_Constant *owner=&runtime_class->constant_pool[ref->value.pair.left], *type=&runtime_class->constant_pool[ref->value.pair.right];
	if (owner->tag!=JVMR_CP_CLASS || type->tag!=JVMR_CP_NAMEANDTYPE) return NULL;
	const char *owner_name=cp_utf8(runtime_class,owner->value.index), *name=cp_utf8(runtime_class,type->value.pair.left), *descriptor=cp_utf8(runtime_class,type->value.pair.right);
	if (!owner_name || !name || !descriptor) return NULL;
	const JVMR_Class *klass=runtime_class->name&&!strcmp(runtime_class->name,owner_name)?runtime_class:(runtime_loader?jvmr_classloader_load(runtime_loader,owner_name,NULL,0):NULL);
	for (const JVMR_Class *current=klass;current;){const JVMR_Field *field=jvmr_class_find_field(current,name,descriptor);if(field){if(owner_out)*owner_out=current;return field;}if(!current->super_name||!runtime_loader)break;current=jvmr_classloader_load(runtime_loader,current->super_name,NULL,0);}
	return NULL;
}

int jvmr_invoke_static(uint16_t constant_pool_index) {
	if (!runtime_class) { fprintf(stderr, "[JVMR/ERROR] invokestatic has no active class.\n"); current_frame.pc = UINT32_MAX; return -1; }
	const JVMR_Class *owner_class; const JVMR_Method *method = resolve_method(constant_pool_index,&owner_class,NULL,NULL);
	int arguments = descriptor_arguments(method ? method->descriptor : NULL);
	if (!method || arguments < 0) { fprintf(stderr, "[JVMR/ERROR] cannot resolve invokestatic constant-pool entry #%u.\n", constant_pool_index); current_frame.pc = UINT32_MAX; return -1; }
	if (jvmr_ensure_initialized(owner_class)) { current_frame.pc=UINT32_MAX; return -1; }
	if (arguments > current_frame.sp || call_depth >= JVMR_CALL_STACK_MAX) { fprintf(stderr, "[JVMR/ERROR] invalid invokestatic arguments or call-stack overflow.\n"); current_frame.pc = UINT32_MAX; return -1; }
	if (jvmr_native_invoke(owner_class,method,1)==0) return 0;
	if (!method->code) { fprintf(stderr,"[JVMR/ERROR] native method %s.%s%s is not implemented.\n",owner_class&&owner_class->name?owner_class->name:"?",method->name,method->descriptor); current_frame.pc=UINT32_MAX; return -1; }
	int caller_sp = current_frame.sp;
	call_stack[call_depth++] = (SavedFrame){ current_frame, runtime_method, runtime_class, 0, NULL };
	JVMR_Frame callee; memset(&callee, 0, sizeof(callee)); callee.code = (uint8_t *)method->code; callee.code_length = method->code_length;
	for (int i = 0; i < arguments; i++) callee.locals[i] = call_stack[call_depth - 1].frame.stack[caller_sp - arguments + i];
	call_stack[call_depth - 1].frame.sp -= arguments;
	current_frame = callee; runtime_method = method; runtime_class = owner_class;
	return 0;
}

int jvmr_invoke_instance(uint16_t constant_pool_index) {
	if (!runtime_class) { current_frame.pc=UINT32_MAX; return -1; }
	const JVMR_Class *owner_class; const char *name=NULL,*descriptor=NULL; const JVMR_Method *method=resolve_method(constant_pool_index,&owner_class,&name,&descriptor);
	int arguments=descriptor_arguments(method?method->descriptor:NULL);
	if(!method||arguments<0||current_frame.sp<arguments+1||call_depth>=JVMR_CALL_STACK_MAX){ fprintf(stderr,"[JVMR/ERROR] cannot resolve instance method #%u.\n",constant_pool_index); current_frame.pc=UINT32_MAX; return -1; }
	uint64_t receiver=current_frame.stack[current_frame.sp-arguments-1];
	const JVMR_Class *receiver_class=jvmr_heap_object_class(receiver);
	int32_t string_length;int string_object=jvmr_heap_string_length(receiver,&string_length)==0;
	if (!invoke_declared_method && string_object && runtime_loader && name && descriptor) { const JVMR_Class *string_class=jvmr_classloader_load(runtime_loader,"java/lang/String",NULL,0);const JVMR_Method *candidate=string_class?jvmr_class_find_method(string_class,name,descriptor):NULL;if(candidate){method=candidate;owner_class=string_class;} }
	if (!invoke_declared_method && !string_object && receiver_class && name && descriptor) for (const JVMR_Class *current=receiver_class;current;){const JVMR_Method *candidate=jvmr_class_find_method(current,name,descriptor);if(candidate){method=candidate;owner_class=current;break;}if(!current->super_name||!runtime_loader)break;current=jvmr_classloader_load(runtime_loader,current->super_name,NULL,0);}
	if (jvmr_native_invoke(owner_class,method,0)==0) return 0;
	if(!method->code) { fprintf(stderr,"[JVMR/ERROR] native method %s.%s%s is not implemented.\n",owner_class&&owner_class->name?owner_class->name:"?",method->name,method->descriptor); current_frame.pc=UINT32_MAX; return -1; }
	int caller_sp=current_frame.sp;
	call_stack[call_depth++]=(SavedFrame){current_frame,runtime_method,runtime_class,0,NULL};
	SavedFrame *saved=&call_stack[call_depth-1];
	receiver=saved->frame.stack[caller_sp-arguments-1];
	JVMR_Frame callee; memset(&callee,0,sizeof(callee)); callee.code=(uint8_t*)method->code; callee.code_length=method->code_length; callee.locals[0]=receiver;
	for(int i=0;i<arguments;i++)callee.locals[i+1]=saved->frame.stack[caller_sp-arguments+i];
	saved->frame.sp-=arguments+1;
	current_frame=callee; runtime_method=method; runtime_class=owner_class; return 0;
}

int jvmr_invoke_interface(uint16_t constant_pool_index) {
	if(!runtime_class)return -1;
	const char *owner_name,*name,*descriptor;if(reference_info(constant_pool_index,&owner_name,&name,&descriptor))return -1;
	int arguments=descriptor_arguments(descriptor);if(arguments<0||current_frame.sp<arguments+1||call_depth>=JVMR_CALL_STACK_MAX){current_frame.pc=UINT32_MAX;return -1;}
	uint64_t receiver=current_frame.stack[current_frame.sp-arguments-1];const JVMR_Class *receiver_class=jvmr_heap_object_class(receiver);const JVMR_Class *implementation_class=receiver_class;const JVMR_Method *method=receiver_class?jvmr_class_find_method(receiver_class,name,descriptor):NULL;const uint64_t *captured=NULL;uint16_t captured_count=0;int lambda=jvmr_heap_lambda_info(receiver,&implementation_class,&method,&captured,&captured_count)==0;
	if(!method&&owner_name&&!strcmp(owner_name,"jdk/internal/access/JavaIOFileDescriptorAccess")){
		if(!strcmp(name,"getAppend")&&!strcmp(descriptor,"(Ljava/io/FileDescriptor;)Z")){current_frame.sp-=arguments+1;current_frame.stack[current_frame.sp++]=0;return 0;}
		if(!strcmp(name,"get")&&!strcmp(descriptor,"(Ljava/io/FileDescriptor;)I")){current_frame.sp-=arguments+1;current_frame.stack[current_frame.sp++]=(uint32_t)-1;return 0;}
		if(!strcmp(name,"getHandle")&&!strcmp(descriptor,"(Ljava/io/FileDescriptor;)J")){current_frame.sp-=arguments+1;current_frame.stack[current_frame.sp++]=0;return 0;}
		if(!strcmp(name,"set")||!strcmp(name,"setAppend")||!strcmp(name,"close")||!strcmp(name,"registerCleanup")||!strcmp(name,"unregisterCleanup")||!strcmp(name,"setHandle")){current_frame.sp-=arguments+1;if(descriptor[strlen(descriptor)-1]=='V')return 0;}
	}
	const JVMR_Class *native_owner=owner_name&&runtime_loader?jvmr_classloader_load(runtime_loader,owner_name,NULL,0):implementation_class;
	const JVMR_Method *native_method=method;
	if(!native_method&&native_owner)native_method=jvmr_class_find_method(native_owner,name,descriptor);
	if(jvmr_native_invoke(native_owner,native_method,0)==0)return 0;
	if(!method||!method->code){fprintf(stderr,"[JVMR/ERROR] interface method %s%s has no concrete implementation.\n",name,descriptor);current_frame.pc=UINT32_MAX;return -1;}
	int caller_sp=current_frame.sp;call_stack[call_depth++]=(SavedFrame){current_frame,runtime_method,runtime_class,0,NULL};SavedFrame *saved=&call_stack[call_depth-1];saved->frame.sp-=arguments+1;JVMR_Frame callee;memset(&callee,0,sizeof(callee));callee.code=(uint8_t*)method->code;callee.code_length=method->code_length;int local=0;if(!lambda||!(method->access_flags&0x0008))callee.locals[local++]=receiver;if(lambda)for(uint16_t i=0;i<captured_count;i++)callee.locals[local++]=captured[i];for(int i=0;i<arguments;i++)callee.locals[local+i]=saved->frame.stack[caller_sp-arguments+i];current_frame=callee;runtime_method=method;runtime_class=implementation_class;return 0;
}

int jvmr_invoke_reflected(const JVMR_Class *declaring_class,const JVMR_Method *method,uint64_t receiver,const uint64_t *arguments,uint16_t argument_count) {
	if(!declaring_class||!method||!method->code||argument_count>JVMR_LOCALS_MAX|| (argument_count&&!arguments)||call_depth>=JVMR_CALL_STACK_MAX)return -1;
	int expected=descriptor_arguments(method->descriptor);if(expected<0||expected!=argument_count)return -1;
	call_stack[call_depth++]=(SavedFrame){current_frame,runtime_method,runtime_class,1,method->descriptor};
	SavedFrame *saved=&call_stack[call_depth-1];saved->frame.sp-=0;
	JVMR_Frame callee;memset(&callee,0,sizeof(callee));callee.code=(uint8_t *)method->code;callee.code_length=method->code_length;int local=0;if(!(method->access_flags&0x0008))callee.locals[local++]=receiver;for(uint16_t i=0;i<argument_count;i++)callee.locals[local+i]=arguments[i];
	current_frame=callee;runtime_method=method;runtime_class=declaring_class;return 0;
}

int jvmr_invoke_special(uint16_t constant_pool_index) {
	const JVMR_Constant *ref = runtime_class && constant_pool_index < runtime_class->constant_pool_count ? &runtime_class->constant_pool[constant_pool_index] : NULL;
	if (!ref || (ref->tag != JVMR_CP_METHODREF && ref->tag != JVMR_CP_INTERFACEMETHODREF)) { current_frame.pc=UINT32_MAX; return -1; }
	const JVMR_Constant *owner = ref->value.pair.left < runtime_class->constant_pool_count ? &runtime_class->constant_pool[ref->value.pair.left] : NULL;
	const JVMR_Constant *nt = ref->value.pair.right < runtime_class->constant_pool_count ? &runtime_class->constant_pool[ref->value.pair.right] : NULL;
	const char *owner_name = owner && owner->tag == JVMR_CP_CLASS && owner->value.index < runtime_class->constant_pool_count && runtime_class->constant_pool[owner->value.index].tag == JVMR_CP_UTF8 ? (const char *)runtime_class->constant_pool[owner->value.index].value.utf8.bytes : NULL;
	const char *name = nt && nt->tag == JVMR_CP_NAMEANDTYPE && nt->value.pair.left < runtime_class->constant_pool_count && runtime_class->constant_pool[nt->value.pair.left].tag == JVMR_CP_UTF8 ? (const char *)runtime_class->constant_pool[nt->value.pair.left].value.utf8.bytes : NULL;
	const char *descriptor = nt && nt->tag == JVMR_CP_NAMEANDTYPE && nt->value.pair.right < runtime_class->constant_pool_count && runtime_class->constant_pool[nt->value.pair.right].tag == JVMR_CP_UTF8 ? (const char *)runtime_class->constant_pool[nt->value.pair.right].value.utf8.bytes : NULL;
	const JVMR_Class *owner_class; const JVMR_Method *method = resolve_method(constant_pool_index,&owner_class,&name,&descriptor);
	if (method) { int previous=invoke_declared_method;invoke_declared_method=1;int result=jvmr_invoke_instance(constant_pool_index);invoke_declared_method=previous;return result; }
	/* java/lang/Object.<init> has no state in the minimal object model. */
	int arguments=descriptor_arguments(descriptor);
	if (owner_name && name && descriptor && !strcmp(owner_name,"java/lang/Object") && !strcmp(name,"<init>") && arguments>=0 && current_frame.sp>=arguments+1) { current_frame.sp-=arguments+1; return 0; }
	fprintf(stderr,"[JVMR/ERROR] cannot resolve invokespecial constant-pool entry #%u.\n",constant_pool_index); current_frame.pc=UINT32_MAX; return -1;
}

int jvmr_invoke_dynamic(uint16_t index) {
	if(!runtime_class||index>=runtime_class->constant_pool_count||runtime_class->constant_pool[index].tag!=JVMR_CP_INVOKEDYNAMIC){current_frame.pc=UINT32_MAX;return -1;}
	const JVMR_Constant *dynamic=&runtime_class->constant_pool[index];if(dynamic->value.dynamic.name_and_type>=runtime_class->constant_pool_count)return -1;const JVMR_Constant *name_type=&runtime_class->constant_pool[dynamic->value.dynamic.name_and_type];if(name_type->tag!=JVMR_CP_NAMEANDTYPE)return -1;const char *descriptor=cp_utf8(runtime_class,name_type->value.pair.right);if(!descriptor||descriptor[0]!='(')return -1;
	const char *result=strchr(descriptor,')');
	if(result&&result[1]=='L'&&runtime_class->bootstrap_count>dynamic->value.dynamic.bootstrap){
		JVMR_BootstrapMethod *bootstrap=&runtime_class->bootstraps[dynamic->value.dynamic.bootstrap];
		if(bootstrap->argument_count>1&&bootstrap->arguments[1]<runtime_class->constant_pool_count&&runtime_class->constant_pool[bootstrap->arguments[1]].tag==JVMR_CP_METHODHANDLE){
			uint16_t implementation_ref=runtime_class->constant_pool[bootstrap->arguments[1]].value.handle.reference_index;const JVMR_Class *implementation_class=NULL;const char *implementation_name=NULL,*implementation_descriptor=NULL;const JVMR_Method *implementation=resolve_method(implementation_ref,&implementation_class,&implementation_name,&implementation_descriptor);const char *end=strchr(result+2,';');size_t interface_length=end?(size_t)(end-(result+2)):0;char interface_name[256];
			if(implementation&&end&&interface_length>0&&interface_length<sizeof(interface_name)){memcpy(interface_name,result+2,interface_length);interface_name[interface_length]=0;const JVMR_Class *interface_class=runtime_class->name&&!strcmp(runtime_class->name,interface_name)?runtime_class:(runtime_loader?jvmr_classloader_load(runtime_loader,interface_name,NULL,0):NULL);if(interface_class){int capture_count=0;for(const char *p=descriptor+1;*p&&*p!=')';p++){if(*p=='['){while(*p=='[')p++;if(*p=='L')while(*p&&*p!=';')p++;}else if(*p=='L'){while(*p&&*p!=';')p++;}if(!*p||capture_count>=256)return -1;capture_count++;}if(current_frame.sp>=capture_count){uint64_t captures[256];for(int i=0;i<capture_count;i++)captures[i]=current_frame.stack[current_frame.sp-capture_count+i];int reference=jvmr_heap_new_lambda(interface_class,implementation_class,implementation,captures,(uint16_t)capture_count);if(reference<0)return -1;current_frame.sp-=capture_count;current_frame.stack[current_frame.sp++]=(uint64_t)reference;return 0;}}}
		}
	}
	char types[256];int count=0;for(const char *p=descriptor+1;*p&&*p!=')';p++){if(*p=='['){while(*p=='[')p++;if(*p=='L')while(*p&&*p!=';')p++;if(!*p)return -1;types[count++]='L';}else if(*p=='L'){while(*p&&*p!=';')p++;if(!*p)return -1;types[count++]='L';}else if(strchr("BCDFIJSZ",*p))types[count++]=*p;else return -1;if(count>=256)return -1;}
	if(current_frame.sp<count)return -1;
	char output[4096]={0};size_t used=0;const char *recipe=dynamic_recipe(runtime_class,dynamic->value.dynamic.bootstrap);int argument=0;
	if(!recipe){recipe="";for(int i=0;i<count;i++){char text[128];uint64_t value=current_frame.stack[current_frame.sp-count+i];if(types[i]=='L'){size_t length;if(jvmr_heap_string_copy(value,text,sizeof(text),&length))snprintf(text,sizeof(text),"null");}else if(types[i]=='F')snprintf(text,sizeof(text),"%g",(double)(float)(uint32_t)value);else if(types[i]=='D'){double number;memcpy(&number,&value,8);snprintf(text,sizeof(text),"%g",number);}else if(types[i]=='J')snprintf(text,sizeof(text),"%lld",(long long)(int64_t)value);else snprintf(text,sizeof(text),"%d",(int32_t)value);if(i&&append_text(output,sizeof(output),&used,"")<0)return -1;if(append_text(output,sizeof(output),&used,text)<0)return -1;}}
	else for(const uint8_t *p=(const uint8_t *)recipe;*p;p++){if(*p==1&&argument<count){char text[128];uint64_t value=current_frame.stack[current_frame.sp-count+argument];if(types[argument]=='L'){size_t length;if(jvmr_heap_string_copy(value,text,sizeof(text),&length))snprintf(text,sizeof(text),"null");}else if(types[argument]=='F')snprintf(text,sizeof(text),"%g",(double)(float)(uint32_t)value);else if(types[argument]=='D'){double number;memcpy(&number,&value,8);snprintf(text,sizeof(text),"%g",number);}else if(types[argument]=='J')snprintf(text,sizeof(text),"%lld",(long long)(int64_t)value);else snprintf(text,sizeof(text),"%d",(int32_t)value);if(append_text(output,sizeof(output),&used,text)<0)return -1;argument++;}else if(*p!=2&&*p!=3){char literal[2]={(char)*p,0};if(append_text(output,sizeof(output),&used,literal)<0)return -1;}}
	int reference=jvmr_heap_new_string((const uint8_t *)output,(uint16_t)used,runtime_class);if(reference<0)return -1;current_frame.sp-=count;current_frame.stack[current_frame.sp++]=(uint64_t)reference;return 0;
}

void jvmr_return_value(uint64_t value, int has_value) {
	if (call_depth == 0) { current_frame.sp=has_value?1:0; if (has_value) current_frame.stack[0]=value; current_frame.pc = UINT32_MAX; return; }
	SavedFrame saved = call_stack[--call_depth]; current_frame = saved.frame; runtime_method = saved.method; runtime_class = saved.klass;
	if (has_value) { if(saved.box_return){const char *return_type=strchr(saved.return_descriptor,')');if(return_type&&return_type[1]=='I'){const JVMR_Class *wrapper=jvmr_runtime_load_class_name("java/lang/Integer");const JVMR_Field *field=wrapper?jvmr_class_find_field(wrapper,"value","I"):NULL;int reference=field?jvmr_heap_new_object(wrapper):-1;if(reference>=0&&!jvmr_heap_field_store((uint64_t)reference,field->slot,value))value=(uint64_t)reference;}else if(return_type&&return_type[1]=='Z'){const JVMR_Class *wrapper=jvmr_runtime_load_class_name("java/lang/Boolean");const JVMR_Field *field=wrapper?jvmr_class_find_field(wrapper,"value","Z"):NULL;int reference=field?jvmr_heap_new_object(wrapper):-1;if(reference>=0&&!jvmr_heap_field_store((uint64_t)reference,field->slot,value))value=(uint64_t)reference;}else if(return_type&&return_type[1]=='J'){const JVMR_Class *wrapper=jvmr_runtime_load_class_name("java/lang/Long");const JVMR_Field *field=wrapper?jvmr_class_find_field(wrapper,"value","J"):NULL;int reference=field?jvmr_heap_new_object(wrapper):-1;if(reference>=0&&!jvmr_heap_field_store((uint64_t)reference,field->slot,value))value=(uint64_t)reference;}} current_frame.stack[current_frame.sp++]=value;} else if(saved.box_return) current_frame.stack[current_frame.sp++]=0;
}

void jvmr_throw_reference(uint64_t reference) {
	for (;;) {
		uint32_t throw_pc=current_frame.pc?current_frame.pc-1:0;
		if(runtime_method)for(uint16_t i=0;i<runtime_method->exception_count;i++){JVMR_ExceptionHandler *handler=&runtime_method->exceptions[i];if(throw_pc>=handler->start_pc&&throw_pc<handler->end_pc&&exception_matches(runtime_class,handler,jvmr_heap_object_class(reference))){current_frame.sp=0;current_frame.stack[current_frame.sp++]=reference;current_frame.pc=handler->handler_pc;return;}}
		if(!call_depth){fprintf(stderr,"[JVMR/ERROR] uncaught Java exception.\n");current_frame.pc=UINT32_MAX;return;}
		SavedFrame saved=call_stack[--call_depth];current_frame=saved.frame;runtime_method=saved.method;runtime_class=saved.klass;
	}
}
void jvmr_throw_named(const char *class_name) { const JVMR_Class *klass=runtime_loader&&class_name?jvmr_classloader_load(runtime_loader,class_name,NULL,0):NULL;int reference=jvmr_heap_new_object(klass);if(reference<0){current_frame.pc=UINT32_MAX;return;}jvmr_throw_reference((uint64_t)reference); }

int jvmr_execute_class_method(const JVMR_Class *klass, const char *name, const char *descriptor) {
	return jvmr_execute_class_method_args(klass,name,descriptor,NULL,0);
}
int jvmr_execute_class_method_args(const JVMR_Class *klass, const char *name, const char *descriptor, const uint64_t *arguments, uint16_t argument_count) {
	if (!klass) return -1;
	const JVMR_Method *method = jvmr_class_find_method(klass, name, descriptor);
	if (!method || !method->code) return -1;
	runtime_class = klass; runtime_method = method; call_depth = 0;
	memset(&current_frame, 0, sizeof(current_frame));
	if (argument_count > JVMR_STACK_MAX || (argument_count && !arguments)) return -1;
	for (uint16_t i=0;i<argument_count;i++) current_frame.locals[i]=arguments[i];
	current_frame.sp=0;
	jvm_execute((uint8_t *)method->code, method->code_length);
	runtime_class = NULL; runtime_method = NULL;
	return 0;
}

const JVMR_Class *jvmr_runtime_class(void) { return runtime_class; }
const JVMR_Class *jvmr_runtime_resolve_class(uint16_t index) {
	if (!runtime_class || index >= runtime_class->constant_pool_count || runtime_class->constant_pool[index].tag != JVMR_CP_CLASS) return NULL;
	const JVMR_Constant *entry=&runtime_class->constant_pool[index];
	const char *name=cp_utf8(runtime_class,entry->value.index);
	if (name && runtime_class->name && !strcmp(name,runtime_class->name)) return runtime_class;
	return runtime_loader && name ? jvmr_classloader_load(runtime_loader,name,NULL,0) : NULL;
}
const JVMR_Class *jvmr_runtime_load_class_name(const char *name) {
	if (!name || !runtime_class) return NULL;
	if (runtime_class->name && !strcmp(runtime_class->name,name)) return runtime_class;
	return runtime_loader ? jvmr_classloader_load(runtime_loader,name,NULL,0) : NULL;
}
int jvmr_runtime_class_is_assignable(const JVMR_Class *candidate,const JVMR_Class *target) {
	for(const JVMR_Class *current=candidate;current;){if(current==target||(current->name&&target&&target->name&&!strcmp(current->name,target->name)))return 1;for(uint16_t i=0;i<current->interface_count;i++){const JVMR_Class *interface_class=jvmr_runtime_load_class_name(current->interfaces[i]);if(interface_class&&jvmr_runtime_class_is_assignable(interface_class,target))return 1;}if(!current->super_name)break;current=jvmr_runtime_load_class_name(current->super_name);}return 0;
}
int jvmr_runtime_field_offset(const JVMR_Class *object_class,const JVMR_Class *declaring_class,const JVMR_Field *field) { if(!object_class||!declaring_class||!field)return -1;int offset=field->slot;const JVMR_Class *current=object_class;while(current&&current!=declaring_class){offset+=current->field_count;if(!current->super_name)break;current=jvmr_runtime_load_class_name(current->super_name);}return current==declaring_class?offset:-1; }
int jvmr_runtime_is_instance(uint64_t reference, const JVMR_Class *target) {
	const JVMR_Class *current;
	if (!reference || !target) return 0;
	current = jvmr_heap_object_class(reference);
	while (current) {
		if (current == target || (current->name && target->name && !strcmp(current->name, target->name))) return 1;
		for (uint16_t i=0;i<current->interface_count;i++) {
			const char *name=current->interfaces[i];
			if (name && target->name && !strcmp(name,target->name)) return 1;
			if (name && runtime_loader) {
				const JVMR_Class *interface_class=jvmr_classloader_load(runtime_loader,name,NULL,0);
				if (interface_class && interface_class != current && jvmr_runtime_is_instance(reference,interface_class)) return 1;
			}
		}
		if (!current->super_name || !runtime_loader) break;
		current = jvmr_classloader_load(runtime_loader, current->super_name, NULL, 0);
	}
	return 0;
}
void jvmr_runtime_set_classloader(JVMR_ClassLoader *loader) { runtime_loader=loader; if(loader){const JVMR_Class *klass=jvmr_classloader_load(loader,"java/lang/String",NULL,0);jvmr_heap_set_string_class(klass);} }
int jvmr_runtime_read_resource(const char *name,uint8_t **data,size_t *size){return jvmr_classloader_read_resource(runtime_loader,name,data,size);}
