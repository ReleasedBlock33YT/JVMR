#include <stdint.h>
#include "bytecode.h"
#include "frame.h"
#include "handlers.h"

static int16_t offset(void) { return (int16_t)((fetch_bytecode() << 8) | fetch_bytecode()); }
static int32_t word(void) { return (int32_t)(((uint32_t)fetch_bytecode() << 24) | ((uint32_t)fetch_bytecode() << 16) | ((uint32_t)fetch_bytecode() << 8) | fetch_bytecode()); }
static void branch_if(int condition) { int16_t jump=offset(); if (condition) current_frame.pc = (uint32_t)((int32_t)current_frame.pc - 3 + jump); }
void handler_ifeq(void) { branch_if((int32_t)current_frame.stack[--current_frame.sp] == 0); }
void handler_ifne(void) { branch_if((int32_t)current_frame.stack[--current_frame.sp] != 0); }
void handler_iflt(void) { branch_if((int32_t)current_frame.stack[--current_frame.sp] < 0); }
void handler_ifge(void) { branch_if((int32_t)current_frame.stack[--current_frame.sp] >= 0); }
void handler_ifgt(void) { branch_if((int32_t)current_frame.stack[--current_frame.sp] > 0); }
void handler_ifle(void) { branch_if((int32_t)current_frame.stack[--current_frame.sp] <= 0); }
void handler_if_icmpeq(void) { uint64_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; branch_if((int32_t)a==(int32_t)b); }
void handler_if_icmpne(void) { uint64_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; branch_if((int32_t)a!=(int32_t)b); }
void handler_if_icmplt(void) { uint64_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; branch_if((int32_t)a<(int32_t)b); }
void handler_if_icmpge(void) { uint64_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; branch_if((int32_t)a>=(int32_t)b); }
void handler_if_icmpgt(void) { uint64_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; branch_if((int32_t)a>(int32_t)b); }
void handler_if_icmple(void) { uint64_t b=current_frame.stack[--current_frame.sp],a=current_frame.stack[--current_frame.sp]; branch_if((int32_t)a<=(int32_t)b); }
void handler_goto(void) { int16_t jump=offset(); current_frame.pc=(uint32_t)((int32_t)current_frame.pc-3+jump); }

void handler_tableswitch(void) {
	uint32_t instruction = current_frame.pc - 1;
	while ((current_frame.pc & 3u) != 0) (void)fetch_bytecode();
	int32_t default_jump = word(), low = word(), high = word();
	int32_t key = (int32_t)current_frame.stack[--current_frame.sp];
	int32_t jump = default_jump;
	if (key >= low && key <= high) {
		uint32_t count = (uint32_t)(high - low + 1);
		for (uint32_t i = 0; i < count; i++) { int32_t candidate = word(); if (key == low + (int32_t)i) jump = candidate; }
	} else {
		/* Still consume the table so the next instruction is correctly located. */
		for (int32_t i = low; i <= high; i++) (void)word();
	}
	current_frame.pc = (uint32_t)((int32_t)instruction + jump);
}

void handler_lookupswitch(void) {
	uint32_t instruction = current_frame.pc - 1;
	while ((current_frame.pc & 3u) != 0) (void)fetch_bytecode();
	int32_t default_jump = word(), pairs = word();
	int32_t key = (int32_t)current_frame.stack[--current_frame.sp], jump = default_jump;
	for (int32_t i = 0; i < pairs; i++) { int32_t match = word(), candidate = word(); if (match == key) jump = candidate; }
	current_frame.pc = (uint32_t)((int32_t)instruction + jump);
}
