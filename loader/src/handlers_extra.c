#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bytecode.h"
#include "frame.h"
#include "handlers.h"
#include "runtime.h"
#include "heap.h"

#define FIXED_LOAD(name, index) void handler_##name(void) { current_frame.stack[current_frame.sp++] = current_frame.locals[index]; }
#define FIXED_STORE(name, index) void handler_##name(void) { current_frame.locals[index] = current_frame.stack[--current_frame.sp]; }
FIXED_LOAD(lload_0,0) FIXED_LOAD(lload_1,1) FIXED_LOAD(lload_2,2) FIXED_LOAD(lload_3,3)
FIXED_LOAD(fload_0,0) FIXED_LOAD(fload_1,1) FIXED_LOAD(fload_2,2) FIXED_LOAD(fload_3,3)
FIXED_LOAD(dload_0,0) FIXED_LOAD(dload_1,1) FIXED_LOAD(dload_2,2) FIXED_LOAD(dload_3,3)
FIXED_STORE(lstore_0,0) FIXED_STORE(lstore_1,1) FIXED_STORE(lstore_2,2) FIXED_STORE(lstore_3,3)
FIXED_STORE(fstore_0,0) FIXED_STORE(fstore_1,1) FIXED_STORE(fstore_2,2) FIXED_STORE(fstore_3,3)
FIXED_STORE(dstore_0,0) FIXED_STORE(dstore_1,1) FIXED_STORE(dstore_2,2) FIXED_STORE(dstore_3,3)

void handler_iinc(void) {
	uint8_t index = fetch_bytecode();
	int8_t amount = (int8_t)fetch_bytecode();
	current_frame.locals[index] = (uint32_t)current_frame.locals[index] + amount;
}
void handler_i2b(void) { current_frame.stack[current_frame.sp-1] = (int8_t)(int32_t)current_frame.stack[current_frame.sp-1]; }
void handler_i2c(void) { current_frame.stack[current_frame.sp-1] = (uint16_t)(int32_t)current_frame.stack[current_frame.sp-1]; }
void handler_i2s(void) { current_frame.stack[current_frame.sp-1] = (int16_t)(int32_t)current_frame.stack[current_frame.sp-1]; }

static void compare_int(int32_t value) { current_frame.stack[current_frame.sp++] = (uint64_t)(int64_t)value; }
void handler_lcmp(void) { int64_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; compare_int(a>b?1:a<b?-1:0); }
static int float_compare(float a,float b,int nan_value) { return isnan(a)||isnan(b) ? nan_value : (a>b?1:a<b?-1:0); }
void handler_fcmpl(void) { uint32_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; float x,y; memcpy(&x,&a,4); memcpy(&y,&b,4); compare_int(float_compare(x,y,-1)); }
void handler_fcmpg(void) { uint32_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; float x,y; memcpy(&x,&a,4); memcpy(&y,&b,4); compare_int(float_compare(x,y,1)); }
void handler_dcmpl(void) { double b,a; uint64_t bb=current_frame.stack[--current_frame.sp],aa=current_frame.stack[--current_frame.sp]; memcpy(&a,&aa,8); memcpy(&b,&bb,8); compare_int(isnan(a)||isnan(b)?-1:(a>b?1:a<b?-1:0)); }
void handler_dcmpg(void) { double b,a; uint64_t bb=current_frame.stack[--current_frame.sp],aa=current_frame.stack[--current_frame.sp]; memcpy(&a,&aa,8); memcpy(&b,&bb,8); compare_int(isnan(a)||isnan(b)?1:(a>b?1:a<b?-1:0)); }

static int32_t word(void) { return (int32_t)(((uint32_t)fetch_bytecode()<<24)|((uint32_t)fetch_bytecode()<<16)|((uint32_t)fetch_bytecode()<<8)|fetch_bytecode()); }
static void ref_branch(int condition) { int16_t jump=(int16_t)((fetch_bytecode()<<8)|fetch_bytecode()); if(condition) current_frame.pc=(uint32_t)((int32_t)current_frame.pc-3+jump); }
void handler_if_acmpeq(void) { uint64_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; ref_branch(a==b); }
void handler_if_acmpne(void) { uint64_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; ref_branch(a!=b); }
void handler_ifnull(void) { ref_branch(current_frame.stack[--current_frame.sp]==0); }
void handler_ifnonnull(void) { ref_branch(current_frame.stack[--current_frame.sp]!=0); }
void handler_goto_w(void) { uint32_t instruction=current_frame.pc-1; current_frame.pc=(uint32_t)((int32_t)instruction+word()); }
void handler_jsr(void) { fprintf(stderr,"[JVMR/ERROR] jsr is obsolete and unsupported.\n"); (void)fetch_bytecode(); (void)fetch_bytecode(); }
void handler_jsr_w(void) { fprintf(stderr,"[JVMR/ERROR] jsr_w is obsolete and unsupported.\n"); (void)word(); }
void handler_ret(void) { fprintf(stderr,"[JVMR/ERROR] ret is obsolete and unsupported.\n"); (void)fetch_bytecode(); }
void handler_invokedynamic(void) { uint16_t index=(uint16_t)((fetch_bytecode()<<8)|fetch_bytecode()); (void)fetch_bytecode(); (void)fetch_bytecode(); (void)jvmr_invoke_dynamic(index); }
static int new_multi_array(const int32_t *counts, int depth, int dimensions) {
	int reference=jvmr_heap_new_array(JVMR_HEAP_REF_ARRAY,counts[depth]);
	if (reference<0) return -1;
	if (depth+1<dimensions) for (int32_t i=0;i<counts[depth];i++) {
		int child=new_multi_array(counts,depth+1,dimensions);
		if (child<0 || jvmr_heap_array_store((uint64_t)reference,i,(uint64_t)child)) return -1;
	}
	return reference;
}
void handler_multianewarray(void) {
	(void)fetch_bytecode(); (void)fetch_bytecode();
	uint8_t dimensions=fetch_bytecode();
	if (!dimensions || dimensions>16 || current_frame.sp<dimensions) { current_frame.pc=UINT32_MAX; return; }
	int32_t counts[16];
	for (int i=dimensions-1;i>=0;i--) counts[i]=(int32_t)current_frame.stack[--current_frame.sp];
	int reference=new_multi_array(counts,0,dimensions);
	if (reference<0) { current_frame.pc=UINT32_MAX; return; }
	current_frame.stack[current_frame.sp++]=(uint64_t)reference;
}
void handler_wide(void) {
	uint8_t opcode=fetch_bytecode();
	uint16_t index=(uint16_t)((fetch_bytecode()<<8)|fetch_bytecode());
	switch (opcode) {
	case ILOAD: case LLOAD: case FLOAD: case DLOAD: case ALOAD: current_frame.stack[current_frame.sp++]=current_frame.locals[index]; break;
	case ISTORE: case LSTORE: case FSTORE: case DSTORE: case ASTORE: current_frame.locals[index]=current_frame.stack[--current_frame.sp]; break;
	case IINC: { int16_t amount=(int16_t)((fetch_bytecode()<<8)|fetch_bytecode()); current_frame.locals[index]=(uint32_t)current_frame.locals[index]+amount; break; }
	case RET: (void)fetch_bytecode(); (void)fetch_bytecode(); break;
	default: fprintf(stderr,"[JVMR/ERROR] unsupported wide opcode %u.\n",opcode); current_frame.pc=UINT32_MAX; break;
	}
}
