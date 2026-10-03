#include "bytecode.h"
#include "frame.h"
#include "handlers.h"

void handler_istore(void) { current_frame.locals[fetch_bytecode()] = current_frame.stack[--current_frame.sp]; }
void handler_lstore(void) { current_frame.locals[fetch_bytecode()] = current_frame.stack[--current_frame.sp]; }
void handler_fstore(void) { current_frame.locals[fetch_bytecode()] = current_frame.stack[--current_frame.sp]; }
void handler_dstore(void) { current_frame.locals[fetch_bytecode()] = current_frame.stack[--current_frame.sp]; }
void handler_astore(void) { current_frame.locals[fetch_bytecode()] = current_frame.stack[--current_frame.sp]; }
#define FIXED_STORE(name, index) void handler_##name(void) { current_frame.locals[index] = current_frame.stack[--current_frame.sp]; }
FIXED_STORE(istore_0, 0) FIXED_STORE(istore_1, 1) FIXED_STORE(istore_2, 2) FIXED_STORE(istore_3, 3)
FIXED_STORE(astore_0, 0) FIXED_STORE(astore_1, 1) FIXED_STORE(astore_2, 2) FIXED_STORE(astore_3, 3)
