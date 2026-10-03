#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include "handlers.h"
#include "frame.h"
#include "runtime.h"

// ===========================
// INT ARITHMETIC
// ===========================
void handler_iadd(void) {
        int32_t b = current_frame.stack[--current_frame.sp];
        int32_t a = current_frame.stack[--current_frame.sp];
        current_frame.stack[current_frame.sp++] = a + b;

        printf("[JVMR/INFO] iadd: %d + %d = %d\n", a, b, a + b);
}

void handler_isub(void) {
        int32_t b = current_frame.stack[--current_frame.sp];
        int32_t a = current_frame.stack[--current_frame.sp];
        current_frame.stack[current_frame.sp++] = a - b;

        printf("[JVMR/INFO] isub: %d - %d = %d\n", a, b, a - b);
}

void handler_imul(void) {
        int32_t b = current_frame.stack[--current_frame.sp];
        int32_t a = current_frame.stack[--current_frame.sp];
        current_frame.stack[current_frame.sp++] = a * b;

        printf("[JVMR/INFO] imul: %d * %d = %d\n", a, b, a * b);
}

void handler_idiv(void) {
        int32_t b = current_frame.stack[--current_frame.sp];
        int32_t a = current_frame.stack[--current_frame.sp];

        if (b == 0) {
		jvmr_throw_named("java/lang/ArithmeticException");
                return;
        }

        current_frame.stack[current_frame.sp++] = a / b;

        printf("[JVMR/INFO] idiv: %d / %d = %d\n", a, b, a / b);
}

void handler_irem(void) {
        int32_t b = current_frame.stack[--current_frame.sp];
        int32_t a = current_frame.stack[--current_frame.sp];

        if (b == 0) {
		jvmr_throw_named("java/lang/ArithmeticException");
                return;
        }

        current_frame.stack[current_frame.sp++] = a % b;

        printf("[JVMR/INFO] irem: %d %% %d = %d\n", a, b, a % b);
}

void handler_ineg(void) {
        int32_t a = current_frame.stack[current_frame.sp - 1];
        current_frame.stack[current_frame.sp - 1] = -a;

        printf("[JVMR/INFO] ineg: %d -> %d\n", a, -a);
}

// ============================
// LONG ARITHMETIC
// ============================
void handler_ladd(void) {
        int64_t b = current_frame.stack[--current_frame.sp];
        int64_t a = current_frame.stack[--current_frame.sp];
        current_frame.stack[current_frame.sp++] = a + b;
}

void handler_lsub(void) {
        int64_t b = current_frame.stack[--current_frame.sp];
        int64_t a = current_frame.stack[--current_frame.sp];
        current_frame.stack[current_frame.sp++] = a - b;
}

void handler_lmul(void) {
        int64_t b = current_frame.stack[--current_frame.sp];
        int64_t a = current_frame.stack[--current_frame.sp];
        current_frame.stack[current_frame.sp++] = a * b;
}

void handler_ldiv(void) {
        int64_t b = current_frame.stack[--current_frame.sp];
        int64_t a = current_frame.stack[--current_frame.sp];

        if (b == 0) {
		jvmr_throw_named("java/lang/ArithmeticException");
                return;
        }

        current_frame.stack[current_frame.sp++] = a / b;
}

void handler_lrem(void) {
        int64_t b = current_frame.stack[--current_frame.sp];
        int64_t a = current_frame.stack[--current_frame.sp];

        if (b == 0) {
		jvmr_throw_named("java/lang/ArithmeticException");
                return;
        }

        current_frame.stack[current_frame.sp++] = a % b;
}

void handler_lneg(void) {
        int64_t a = current_frame.stack[current_frame.sp - 1];
        current_frame.stack[current_frame.sp - 1] = -a;
}

// =============================
// FLOAT ARITHMETIC
// =============================
static float stack_to_float(int64_t value) {
        float result;
        uint32_t bits = (uint32_t)value;
        memcpy(&result, &bits, sizeof(result));
        return result;
}

static int64_t float_to_stack(float value) {
        uint32_t bits;
        memcpy(&bits, &value, sizeof(bits));
        return (int64_t)bits;
}

void handler_fadd(void) {
        float b = stack_to_float(current_frame.stack[--current_frame.sp]);
        float a = stack_to_float(current_frame.stack[--current_frame.sp]);

        current_frame.stack[current_frame.sp++] = float_to_stack(a + b);
}

void handler_fsub(void) {
        float b = stack_to_float(current_frame.stack[--current_frame.sp]);
        float a = stack_to_float(current_frame.stack[--current_frame.sp]);

        current_frame.stack[current_frame.sp++] = float_to_stack(a - b);
}

void handler_fmul(void) {
        float b = stack_to_float(current_frame.stack[--current_frame.sp]);
        float a = stack_to_float(current_frame.stack[--current_frame.sp]);

        current_frame.stack[current_frame.sp++] = float_to_stack(a * b);
}

void handler_fdiv(void) {
        float b = stack_to_float(current_frame.stack[--current_frame.sp]);
        float a = stack_to_float(current_frame.stack[--current_frame.sp]);

        // JVM float division follows IEEE-754.
        // Division by zero produces Infinity or NaN.
        current_frame.stack[current_frame.sp++] = float_to_stack(a / b);
}

void handler_frem(void) {
        float b = stack_to_float(current_frame.stack[--current_frame.sp]);
        float a = stack_to_float(current_frame.stack[--current_frame.sp]);

        current_frame.stack[current_frame.sp++] = float_to_stack(fmodf(a, b));
}

void handler_fneg(void) {
        float a = stack_to_float(current_frame.stack[current_frame.sp - 1]);

        current_frame.stack[current_frame.sp - 1] = float_to_stack(-a);
}

// ==============================
// DOUBLE ARITHMETIC
// ==============================
static double stack_to_double(int64_t value) {
        double result;
        uint64_t bits = (uint64_t)value;
        memcpy(&result, &bits, sizeof(result));
        return result;
}

static int64_t double_to_stack(double value) {
        uint64_t bits;
        memcpy(&bits, &value, sizeof(bits));
        return (int64_t)bits;
}

void handler_dadd(void) {
        double b = stack_to_double(current_frame.stack[--current_frame.sp]);
        double a = stack_to_double(current_frame.stack[--current_frame.sp]);

        current_frame.stack[current_frame.sp++] = double_to_stack(a + b);
}

void handler_dsub(void) {
        double b = stack_to_double(current_frame.stack[--current_frame.sp]);
        double a = stack_to_double(current_frame.stack[--current_frame.sp]);

        current_frame.stack[current_frame.sp++] = double_to_stack(a - b);
}

void handler_dmul(void) {
        double b = stack_to_double(current_frame.stack[--current_frame.sp]);
        double a = stack_to_double(current_frame.stack[--current_frame.sp]);

        current_frame.stack[current_frame.sp++] = double_to_stack(a * b);
}

void handler_ddiv(void) {
        double b = stack_to_double(current_frame.stack[--current_frame.sp]);
        double a = stack_to_double(current_frame.stack[--current_frame.sp]);

        // JVM double division follows IEEE-754.
        // Division by zero produces Infinity or NaN.
        current_frame.stack[current_frame.sp++] = double_to_stack(a / b);
}

void handler_drem(void) {
        double b = stack_to_double(current_frame.stack[--current_frame.sp]);
        double a = stack_to_double(current_frame.stack[--current_frame.sp]);

        current_frame.stack[current_frame.sp++] = double_to_stack(fmod(a, b));
}

void handler_dneg(void) {
        double a = stack_to_double(current_frame.stack[current_frame.sp - 1]);

        current_frame.stack[current_frame.sp - 1] = double_to_stack(-a);
}
