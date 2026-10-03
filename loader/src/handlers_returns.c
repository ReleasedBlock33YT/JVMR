#include <stdint.h>
#include "frame.h"
#include "handlers.h"
#include "runtime.h"
#define RETURN_HANDLER(name) void handler_##name(void) { uint64_t value = current_frame.sp ? current_frame.stack[--current_frame.sp] : 0; jvmr_return_value(value, 1); }
RETURN_HANDLER(ireturn) RETURN_HANDLER(lreturn) RETURN_HANDLER(freturn)
RETURN_HANDLER(dreturn) RETURN_HANDLER(areturn)
void handler_return(void) { jvmr_return_value(0, 0); }
