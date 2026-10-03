#include <stdio.h>
#include <stdint.h>
#include "frame.h"
#include "bytecode.h"
#include "runtime.h"
#include "heap.h"
#include "handlers.h"

void handler_aconst_null(void) { current_frame.stack[current_frame.sp++] = 0; }
void handler_iconst_m1(void) { current_frame.stack[current_frame.sp++] = (uint64_t)(int64_t)-1; }
void handler_iconst_0(void) { current_frame.stack[current_frame.sp++] = 0; }
void handler_iconst_1(void) { current_frame.stack[current_frame.sp++] = 1; }

void handler_iconst_2(void) {
	current_frame.stack[current_frame.sp++] = 2;

	printf("[JVMR/INFO] ICONST2\n");
}

void handler_iconst_3(void) {
	current_frame.stack[current_frame.sp++] = 3;

	printf("[JVMR/INFO] ICONST3\n");
}

void handler_iconst_4(void) {
	current_frame.stack[current_frame.sp++] = 4;

	printf("[JVMR/INFO] ICONST4\n");
}

void handler_iconst_5(void) { current_frame.stack[current_frame.sp++] = 5; }
void handler_lconst_0(void) { current_frame.stack[current_frame.sp++] = 0; }
void handler_lconst_1(void) { current_frame.stack[current_frame.sp++] = 1; }
void handler_fconst_0(void) { current_frame.stack[current_frame.sp++] = 0; }
void handler_fconst_1(void) { current_frame.stack[current_frame.sp++] = 0x3f800000u; }
void handler_fconst_2(void) { current_frame.stack[current_frame.sp++] = 0x40000000u; }
void handler_dconst_0(void) { current_frame.stack[current_frame.sp++] = 0; }
void handler_dconst_1(void) { current_frame.stack[current_frame.sp++] = 0x3ff0000000000000ULL; }

void handler_bipush(void) {
	int8_t value = (int8_t)fetch_bytecode();
	current_frame.stack[current_frame.sp++] = (uint64_t)(int64_t)value;
}

void handler_sipush(void) {
	int16_t value = (int16_t)((fetch_bytecode() << 8) | fetch_bytecode());
	current_frame.stack[current_frame.sp++] = (uint64_t)(int64_t)value;
}

static void load_constant(uint16_t index) {
	const JVMR_Class *klass=jvmr_runtime_class();
	if(!klass||index==0||index>=klass->constant_pool_count){current_frame.pc=UINT32_MAX;return;}
	const JVMR_Constant *constant=&klass->constant_pool[index];
	if(constant->tag==JVMR_CP_INTEGER) current_frame.stack[current_frame.sp++]=(uint64_t)(int64_t)(int32_t)constant->value.integer;
	else if(constant->tag==JVMR_CP_FLOAT) current_frame.stack[current_frame.sp++]=constant->value.float_bits;
	else if(constant->tag==JVMR_CP_LONG||constant->tag==JVMR_CP_DOUBLE) current_frame.stack[current_frame.sp++]=constant->value.wide;
	else if(constant->tag==JVMR_CP_CLASS){const JVMR_Class *represented=jvmr_runtime_resolve_class(index);const JVMR_Class *class_class=jvmr_runtime_load_class_name("java/lang/Class");int reference=represented?jvmr_heap_new_class_mirror(represented,class_class): -1;if(reference<0){current_frame.pc=UINT32_MAX;return;}current_frame.stack[current_frame.sp++]=(uint64_t)reference;}
	else if(constant->tag==JVMR_CP_STRING&&constant->value.index<klass->constant_pool_count&&klass->constant_pool[constant->value.index].tag==JVMR_CP_UTF8){const JVMR_Constant *text=&klass->constant_pool[constant->value.index];const JVMR_Class *string_class=jvmr_runtime_load_class_name("java/lang/String");int reference=jvmr_heap_new_string(text->value.utf8.bytes,text->value.utf8.length,string_class?string_class:klass);if(reference<0){current_frame.pc=UINT32_MAX;return;}current_frame.stack[current_frame.sp++]=(uint64_t)reference;}
	else { fprintf(stderr,"[JVMR/ERROR] ldc reference constant is unsupported.\n"); current_frame.pc=UINT32_MAX; }
}
void handler_ldc(void) { load_constant(fetch_bytecode()); }
void handler_ldc_w(void) { load_constant((uint16_t)((fetch_bytecode()<<8)|fetch_bytecode())); }
void handler_ldc2_w(void) { load_constant((uint16_t)((fetch_bytecode()<<8)|fetch_bytecode())); }
