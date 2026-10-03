#include <stdio.h>
#include <stdint.h>
#include "handlers.h"
#include "frame.h"

// ===========================
// STACK OPERATIONS
// ===========================

void handler_pop(void) {
        current_frame.sp--;
}

void handler_pop2(void) {
        current_frame.sp -= 2;
}

void handler_dup(void) {
        int64_t value = current_frame.stack[current_frame.sp - 1];
        current_frame.stack[current_frame.sp++] = value;

	printf("[JVMR/INFO] DUP EXECUTED.\n");
}

void handler_dup_x1(void) {
        int64_t value1 = current_frame.stack[current_frame.sp - 1];
        int64_t value2 = current_frame.stack[current_frame.sp - 2];

        current_frame.stack[current_frame.sp - 2] = value1;
        current_frame.stack[current_frame.sp - 1] = value2;
        current_frame.stack[current_frame.sp++] = value1;
}

void handler_dup_x2(void) {
        int64_t value1 = current_frame.stack[current_frame.sp - 1];
        int64_t value2 = current_frame.stack[current_frame.sp - 2];
        int64_t value3 = current_frame.stack[current_frame.sp - 3];

        current_frame.stack[current_frame.sp - 3] = value1;
        current_frame.stack[current_frame.sp - 2] = value3;
        current_frame.stack[current_frame.sp - 1] = value2;
        current_frame.stack[current_frame.sp++] = value1;
}

void handler_dup2(void) {
        int64_t value1 = current_frame.stack[current_frame.sp - 2];
        int64_t value2 = current_frame.stack[current_frame.sp - 1];

        current_frame.stack[current_frame.sp++] = value1;
        current_frame.stack[current_frame.sp++] = value2;
}

void handler_dup2_x1(void) {
        int64_t a = current_frame.stack[current_frame.sp - 2];
        int64_t b = current_frame.stack[current_frame.sp - 1];
        int64_t c = current_frame.stack[current_frame.sp - 3];
        current_frame.stack[current_frame.sp - 3] = a;
        current_frame.stack[current_frame.sp - 2] = b;
        current_frame.stack[current_frame.sp - 1] = c;
        current_frame.stack[current_frame.sp++] = a;
        current_frame.stack[current_frame.sp++] = b;
}

void handler_dup2_x2(void) {
        int64_t a = current_frame.stack[current_frame.sp - 2];
        int64_t b = current_frame.stack[current_frame.sp - 1];
        int64_t c = current_frame.stack[current_frame.sp - 3];
        int64_t d = current_frame.stack[current_frame.sp - 4];
        current_frame.stack[current_frame.sp - 4] = a;
        current_frame.stack[current_frame.sp - 3] = b;
        current_frame.stack[current_frame.sp - 2] = c;
        current_frame.stack[current_frame.sp - 1] = d;
        current_frame.stack[current_frame.sp++] = a;
        current_frame.stack[current_frame.sp++] = b;
}

void handler_swap(void) {
        int64_t value1 = current_frame.stack[current_frame.sp - 1];
        int64_t value2 = current_frame.stack[current_frame.sp - 2];

        current_frame.stack[current_frame.sp - 2] = value1;
        current_frame.stack[current_frame.sp - 1] = value2;

	printf("[JVMR/INFO] SWAP EXECUTED.\n");
}
