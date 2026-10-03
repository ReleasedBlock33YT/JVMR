#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "classfile.h"

typedef struct { const uint8_t *data, *end; } Reader;
static int fail(char *error, size_t n, const char *fmt, ...) {
	if (error && n) { va_list ap; va_start(ap, fmt); vsnprintf(error, n, fmt, ap); va_end(ap); }
	return -1;
}
static int take(Reader *r, size_t n, const uint8_t **p) {
	if (n > (size_t)(r->end - r->data)) return -1;
	*p = r->data; r->data += n; return 0;
}
static int u1(Reader *r, uint8_t *v) { const uint8_t *p; if (take(r,1,&p)) return -1; *v=p[0]; return 0; }
static int u2(Reader *r, uint16_t *v) { const uint8_t *p; if (take(r,2,&p)) return -1; *v=(uint16_t)((p[0]<<8)|p[1]); return 0; }
static int u4(Reader *r, uint32_t *v) { const uint8_t *p; if (take(r,4,&p)) return -1; *v=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; return 0; }
static int skip_attributes(Reader *r, char *error, size_t n) {
	uint16_t count; if (u2(r,&count)) return fail(error,n,"truncated attribute count");
	for (uint16_t i=0;i<count;i++) { uint16_t name; uint32_t length; const uint8_t *ignored;
		if (u2(r,&name)||u4(r,&length)||take(r,length,&ignored)) return fail(error,n,"truncated attribute");
	}
	return 0;
}
static const char *utf8(const JVMR_Class *c, uint16_t index) {
	if (!index || index >= c->constant_pool_count || c->constant_pool[index].tag != JVMR_CP_UTF8) return NULL;
	return (const char *)c->constant_pool[index].value.utf8.bytes;
}
static const char *class_name(const JVMR_Class *c, uint16_t index) {
	if (!index || index >= c->constant_pool_count || c->constant_pool[index].tag != JVMR_CP_CLASS) return NULL;
	return utf8(c, c->constant_pool[index].value.index);
}

void jvmr_class_destroy(JVMR_Class *c) {
	if (!c) return;
	for (uint16_t i=1;i<c->constant_pool_count;i++) if (c->constant_pool[i].tag == JVMR_CP_UTF8) free((void *)c->constant_pool[i].value.utf8.bytes);
	for(uint16_t i=0;i<c->method_count;i++) free(c->methods[i].exceptions);
	for(uint16_t i=0;i<c->bootstrap_count;i++) free(c->bootstraps[i].arguments);
	free(c->constant_pool); free(c->methods); free(c->fields); free(c->static_values); free(c->bootstraps); free(c->interfaces); memset(c,0,sizeof(*c));
}

int jvmr_class_parse(const uint8_t *data, size_t length, JVMR_Class *out, char *error, size_t error_length) {
	Reader r={data,data+length}; uint32_t magic; uint16_t cp_count;
	if (!out || !data) return fail(error,error_length,"null class input");
	memset(out,0,sizeof(*out));
	if (u4(&r,&magic)||magic!=JVMR_CLASS_MAGIC) return fail(error,error_length,"invalid class magic");
	if (u2(&r,&out->minor_version)||u2(&r,&out->major_version)) return fail(error,error_length,"truncated version");
	if (out->major_version != JVMR_JAVA21_MAJOR) return fail(error,error_length,"unsupported class version %u.%u (Java 21 requires major %u)",out->major_version,out->minor_version,JVMR_JAVA21_MAJOR);
	if (u2(&r,&cp_count)||cp_count<1) return fail(error,error_length,"invalid constant-pool count");
	out->constant_pool_count=cp_count; out->constant_pool=calloc(cp_count,sizeof(*out->constant_pool));
	if (!out->constant_pool) return fail(error,error_length,"out of memory");
	for (uint16_t i=1;i<cp_count;i++) {
		JVMR_Constant *c=&out->constant_pool[i]; uint8_t tag; uint32_t value;
		if (u1(&r,&tag)) goto trunc;
		c->tag=tag;
		switch(tag) {
		case JVMR_CP_UTF8: { uint16_t n; const uint8_t *p; if(u2(&r,&n)||take(&r,n,&p)) goto trunc; uint8_t *copy=malloc((size_t)n+1); if(!copy) goto oom; memcpy(copy,p,n); copy[n]=0; c->value.utf8=(typeof(c->value.utf8)){n,copy}; break; }
		case JVMR_CP_INTEGER: case JVMR_CP_FLOAT: if(u4(&r,&value)) goto trunc; if(tag==JVMR_CP_INTEGER)c->value.integer=value;else c->value.float_bits=value; break;
		case JVMR_CP_LONG: case JVMR_CP_DOUBLE: { uint32_t hi,lo; if(u4(&r,&hi)||u4(&r,&lo))goto trunc; c->value.wide=((uint64_t)hi<<32)|lo; if(++i<cp_count) out->constant_pool[i].tag=0; break; }
		case JVMR_CP_CLASS: case JVMR_CP_STRING: case JVMR_CP_METHODTYPE: case JVMR_CP_MODULE: case JVMR_CP_PACKAGE: if(u2(&r,&c->value.index))goto trunc; break;
		case JVMR_CP_FIELDREF: case JVMR_CP_METHODREF: case JVMR_CP_INTERFACEMETHODREF: case JVMR_CP_NAMEANDTYPE: if(u2(&r,&c->value.pair.left)||u2(&r,&c->value.pair.right))goto trunc; break;
		case JVMR_CP_METHODHANDLE: if(u1(&r,&c->value.handle.reference_kind)||u2(&r,&c->value.handle.reference_index))goto trunc; break;
		case JVMR_CP_DYNAMIC: case JVMR_CP_INVOKEDYNAMIC: if(u2(&r,&c->value.dynamic.bootstrap)||u2(&r,&c->value.dynamic.name_and_type))goto trunc; break;
		default: return fail(error,error_length,"unsupported constant-pool tag %u",tag);
		}
	}
	{ uint16_t this_class, super_class; if(u2(&r,&out->access_flags)||u2(&r,&this_class)||u2(&r,&super_class))goto trunc; out->name=class_name(out,this_class); out->super_name=class_name(out,super_class); }
	{ uint16_t interfaces; if(u2(&r,&interfaces))goto trunc; out->interface_count=interfaces; out->interfaces=calloc(interfaces,sizeof(*out->interfaces)); if(interfaces&&!out->interfaces)goto oom; for(uint16_t i=0;i<interfaces;i++) { uint16_t index; if(u2(&r,&index))goto trunc; out->interfaces[i]=class_name(out,index); } }
	if(u2(&r,&out->field_count))goto trunc;
	out->fields=calloc(out->field_count,sizeof(*out->fields)); if(out->field_count&&!out->fields)goto oom;
	for(uint16_t i=0;i<out->field_count;i++) { uint16_t name,desc,attrs; JVMR_Field *field=&out->fields[i]; if(u2(&r,&field->access_flags)||u2(&r,&name)||u2(&r,&desc)||u2(&r,&attrs))goto trunc; field->name=utf8(out,name); field->descriptor=utf8(out,desc); field->slot=i; for(uint16_t j=0;j<attrs;j++){uint16_t attr_name;uint32_t len;const uint8_t *body;if(u2(&r,&attr_name)||u4(&r,&len)||take(&r,len,&body))goto trunc;const char *attribute=utf8(out,attr_name);if(attribute&&!strcmp(attribute,"ConstantValue")){Reader cr={body,body+len};uint16_t value;if(len!=2||u2(&cr,&value))goto trunc;field->constant_value_index=value;}} }
	out->static_values=calloc(out->field_count,sizeof(*out->static_values)); if(out->field_count&&!out->static_values)goto oom;
	for(uint16_t i=0;i<out->field_count;i++) if(out->fields[i].constant_value_index<out->constant_pool_count) { const JVMR_Constant *constant=&out->constant_pool[out->fields[i].constant_value_index]; if(constant->tag==JVMR_CP_INTEGER) out->static_values[i]=(uint64_t)(int64_t)(int32_t)constant->value.integer; else if(constant->tag==JVMR_CP_FLOAT) out->static_values[i]=constant->value.float_bits; else if(constant->tag==JVMR_CP_LONG||constant->tag==JVMR_CP_DOUBLE) out->static_values[i]=constant->value.wide; }
	if(u2(&r,&out->method_count))goto trunc;
	out->methods=calloc(out->method_count,sizeof(*out->methods));
	if(out->method_count&&!out->methods)goto oom;
	for(uint16_t i=0;i<out->method_count;i++) { JVMR_Method *m=&out->methods[i]; uint16_t name,desc,attrs; if(u2(&r,&m->access_flags)||u2(&r,&name)||u2(&r,&desc)||u2(&r,&attrs))goto trunc; m->name=utf8(out,name); m->descriptor=utf8(out,desc);
		for(uint16_t j=0;j<attrs;j++) { uint16_t attr_name; uint32_t len; if(u2(&r,&attr_name)||u4(&r,&len))goto trunc; const char *an=utf8(out,attr_name); const uint8_t *body; if(take(&r,len,&body))goto trunc; if(an&&strcmp(an,"Code")==0) { Reader cr={body,body+len}; uint16_t ex; uint32_t code_len; if(u2(&cr,&m->max_stack)||u2(&cr,&m->max_locals)||u4(&cr,&code_len)||take(&cr,code_len,&m->code)||u2(&cr,&ex))goto trunc; m->code_length=code_len;m->exception_count=ex;m->exceptions=calloc(ex,sizeof(*m->exceptions));if(ex&&!m->exceptions)goto oom;for(uint16_t k=0;k<ex;k++){JVMR_ExceptionHandler *handler=&m->exceptions[k];if(u2(&cr,&handler->start_pc)||u2(&cr,&handler->end_pc)||u2(&cr,&handler->handler_pc)||u2(&cr,&handler->catch_type))goto trunc;}if(skip_attributes(&cr,error,error_length))goto trunc; } }
	}
	{ uint16_t attrs; if(u2(&r,&attrs))goto trunc; for(uint16_t i=0;i<attrs;i++){uint16_t name;uint32_t length;const uint8_t *body;if(u2(&r,&name)||u4(&r,&length)||take(&r,length,&body))goto trunc;const char *attribute=utf8(out,name);if(attribute&&!strcmp(attribute,"BootstrapMethods")){Reader br={body,body+length};if(u2(&br,&out->bootstrap_count))goto trunc;out->bootstraps=calloc(out->bootstrap_count,sizeof(*out->bootstraps));if(out->bootstrap_count&&!out->bootstraps)goto oom;for(uint16_t j=0;j<out->bootstrap_count;j++){JVMR_BootstrapMethod *bootstrap=&out->bootstraps[j];if(u2(&br,&bootstrap->method_ref)||u2(&br,&bootstrap->argument_count))goto trunc;bootstrap->arguments=calloc(bootstrap->argument_count,sizeof(*bootstrap->arguments));if(bootstrap->argument_count&&!bootstrap->arguments)goto oom;for(uint16_t k=0;k<bootstrap->argument_count;k++)if(u2(&br,&bootstrap->arguments[k]))goto trunc;}}} }
	return 0;
trunc: jvmr_class_destroy(out); return fail(error,error_length,"truncated class file");
oom: jvmr_class_destroy(out); return fail(error,error_length,"out of memory");
}

const JVMR_Method *jvmr_class_find_method(const JVMR_Class *c,const char *name,const char *descriptor) { if(!c||!name||!descriptor)return NULL; for(uint16_t i=0;i<c->method_count;i++)if(c->methods[i].name&&c->methods[i].descriptor&&!strcmp(c->methods[i].name,name)&&!strcmp(c->methods[i].descriptor,descriptor))return &c->methods[i]; return NULL; }

const JVMR_Method *jvmr_class_resolve_method(const JVMR_Class *c, uint16_t index) {
	if (!c || index == 0 || index >= c->constant_pool_count) return NULL;
	const JVMR_Constant *ref = &c->constant_pool[index];
	if (ref->tag != JVMR_CP_METHODREF && ref->tag != JVMR_CP_INTERFACEMETHODREF) return NULL;
	uint16_t class_index = ref->value.pair.left;
	uint16_t name_type_index = ref->value.pair.right;
	if (class_index == 0 || class_index >= c->constant_pool_count || name_type_index == 0 || name_type_index >= c->constant_pool_count) return NULL;
	const JVMR_Constant *owner = &c->constant_pool[class_index];
	const JVMR_Constant *name_type = &c->constant_pool[name_type_index];
	if (owner->tag != JVMR_CP_CLASS || name_type->tag != JVMR_CP_NAMEANDTYPE) return NULL;
	const char *owner_name = utf8(c, owner->value.index);
	const char *name = utf8(c, name_type->value.pair.left);
	const char *descriptor = utf8(c, name_type->value.pair.right);
	if (!owner_name || !name || !descriptor || !c->name || strcmp(owner_name, c->name) != 0) return NULL;
	return jvmr_class_find_method(c, name, descriptor);
}

const JVMR_Field *jvmr_class_find_field(const JVMR_Class *c,const char *name,const char *descriptor) { if(!c||!name||!descriptor)return NULL; for(uint16_t i=0;i<c->field_count;i++)if(c->fields[i].name&&c->fields[i].descriptor&&!strcmp(c->fields[i].name,name)&&!strcmp(c->fields[i].descriptor,descriptor))return &c->fields[i]; return NULL; }
const JVMR_Field *jvmr_class_resolve_field(const JVMR_Class *c,uint16_t index) { if(!c||index==0||index>=c->constant_pool_count)return NULL; const JVMR_Constant *ref=&c->constant_pool[index]; if(ref->tag!=JVMR_CP_FIELDREF)return NULL; uint16_t ci=ref->value.pair.left,ni=ref->value.pair.right; if(ci>=c->constant_pool_count||ni>=c->constant_pool_count)return NULL; const JVMR_Constant *owner=&c->constant_pool[ci],*nt=&c->constant_pool[ni]; if(owner->tag!=JVMR_CP_CLASS||nt->tag!=JVMR_CP_NAMEANDTYPE)return NULL; const char *on=utf8(c,owner->value.index),*name=utf8(c,nt->value.pair.left),*desc=utf8(c,nt->value.pair.right); if(!on||!name||!desc||!c->name||strcmp(on,c->name))return NULL; return jvmr_class_find_field(c,name,desc); }
const JVMR_Class *jvmr_class_resolve_class(const JVMR_Class *c,uint16_t index) { if(!c||index==0||index>=c->constant_pool_count||c->constant_pool[index].tag!=JVMR_CP_CLASS)return NULL; const char *name=utf8(c,c->constant_pool[index].value.index); return name&&c->name&&!strcmp(name,c->name)?c:NULL; }
