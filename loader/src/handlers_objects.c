#include <stdio.h>
#include "handlers.h"
#include "bytecode.h"
#include "frame.h"
#include "heap.h"
#include "runtime.h"
static void unsupported(const char *name) { fprintf(stderr, "[JVMR/ERROR] %s is unavailable without a class/object runtime.\n", name); }
#define UNSUPPORTED(name) void handler_##name(void) { unsupported(#name); }
void handler_new(void) { uint16_t index=(uint16_t)((fetch_bytecode()<<8)|fetch_bytecode()); int reference=jvmr_heap_new_object(jvmr_runtime_resolve_class(index)); if(reference<0){unsupported("new");current_frame.pc=UINT32_MAX;return;} current_frame.stack[current_frame.sp++]=(uint64_t)reference; }
void handler_newarray(void) { (void)fetch_bytecode(); uint64_t count=current_frame.stack[--current_frame.sp]; int reference=jvmr_heap_new_array(JVMR_HEAP_PRIMITIVE_ARRAY,(int32_t)count); if(reference<0){unsupported("newarray");current_frame.pc=UINT32_MAX;return;} current_frame.stack[current_frame.sp++]=(uint64_t)reference; }
void handler_anewarray(void) { (void)fetch_bytecode(); (void)fetch_bytecode(); uint64_t count=current_frame.stack[--current_frame.sp]; int reference=jvmr_heap_new_array(JVMR_HEAP_REF_ARRAY,(int32_t)count); if(reference<0){unsupported("anewarray");current_frame.pc=UINT32_MAX;return;} current_frame.stack[current_frame.sp++]=(uint64_t)reference; }
void handler_arraylength(void) { int32_t length; uint64_t reference=current_frame.stack[current_frame.sp-1]; if(jvmr_heap_array_length(reference,&length)){unsupported("arraylength");current_frame.pc=UINT32_MAX;return;} current_frame.stack[current_frame.sp-1]=(uint64_t)(int64_t)length; }
void handler_checkcast(void) {
	uint16_t index=(uint16_t)((fetch_bytecode()<<8)|fetch_bytecode());
	uint64_t reference=current_frame.stack[current_frame.sp-1];
	const JVMR_Class *target=jvmr_runtime_resolve_class(index);
	int array_match=0;int32_t length=0;if(reference&&((target&&target->name&&target->name[0]=='[')||!target))array_match=jvmr_heap_array_length(reference,&length)==0;
	if (reference && ((!target&&!array_match)||(target&&!array_match&&!jvmr_runtime_is_instance(reference,target)))) jvmr_throw_named("java/lang/ClassCastException");
}
void handler_instanceof(void) {
	uint16_t index=(uint16_t)((fetch_bytecode()<<8)|fetch_bytecode());
	uint64_t reference=current_frame.stack[--current_frame.sp];
	const JVMR_Class *target=jvmr_runtime_resolve_class(index);int array_match=0;int32_t length=0;if(reference&&((target&&target->name&&target->name[0]=='[')||!target))array_match=jvmr_heap_array_length(reference,&length)==0;
	current_frame.stack[current_frame.sp++]=(uint64_t)(reference && (array_match||jvmr_runtime_is_instance(reference,target)));
}
