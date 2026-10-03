#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "bytecode.h"
#include "frame.h"
#include "runtime.h"

void jvm_execute(uint8_t *code, uint32_t length) {
	current_frame.code = code;
	current_frame.pc = 0;
	current_frame.code_length = length;

	while (current_frame.pc < current_frame.code_length) {
		uint8_t opcode = fetch_bytecode();

		if (opcode >= JVMR_OPCODE_COUNT || bytecode[opcode] == NULL) {
			printf("[JVMR/ERROR] Invalid opcode: %u\n", opcode);
			return;
		}

		bytecode[opcode]();
	}
}

int jvm_execute_method(const JVMR_Method *method) {
	if (!method || !method->code || method->code_length == 0) return -1;
	current_frame.sp = 0;
	current_frame.pc = 0;
	jvm_execute((uint8_t *)method->code, method->code_length);
	return 0;
}
