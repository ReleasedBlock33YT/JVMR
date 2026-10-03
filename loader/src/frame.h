#ifndef JVMR_FRAME_H
#define JVMR_FRAME_H

#include <stdint.h>

#define JVMR_STACK_MAX 1024
#define JVMR_LOCALS_MAX 256

typedef struct {
	uint8_t *code;
	uint32_t pc;
	uint32_t code_length;

	uint64_t stack[JVMR_STACK_MAX];
	int32_t sp;

	uint64_t locals[JVMR_LOCALS_MAX];
} JVMR_Frame;

extern JVMR_Frame current_frame;

#endif
