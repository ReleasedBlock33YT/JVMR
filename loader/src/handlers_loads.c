#include "bytecode.h"
#include "frame.h"
#include "handlers.h"

void handler_iload(void) { current_frame.stack[current_frame.sp++] = current_frame.locals[fetch_bytecode()]; }
void handler_lload(void) { current_frame.stack[current_frame.sp++] = current_frame.locals[fetch_bytecode()]; }
void handler_fload(void) { current_frame.stack[current_frame.sp++] = current_frame.locals[fetch_bytecode()]; }
void handler_dload(void) { current_frame.stack[current_frame.sp++] = current_frame.locals[fetch_bytecode()]; }
void handler_aload(void) { current_frame.stack[current_frame.sp++] = current_frame.locals[fetch_bytecode()]; }

#define FIXED_LOAD(name, index) void handler_##name(void) { current_frame.stack[current_frame.sp++] = current_frame.locals[index]; }
FIXED_LOAD(iload_0, 0) FIXED_LOAD(iload_1, 1) FIXED_LOAD(iload_2, 2) FIXED_LOAD(iload_3, 3)
FIXED_LOAD(aload_0, 0) FIXED_LOAD(aload_1, 1) FIXED_LOAD(aload_2, 2) FIXED_LOAD(aload_3, 3)
