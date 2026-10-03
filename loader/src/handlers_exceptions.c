#include <stdio.h>
#include "handlers.h"
#include "frame.h"
#include "runtime.h"
void handler_athrow(void) { uint64_t reference=current_frame.sp?current_frame.stack[--current_frame.sp]:0; jvmr_throw_reference(reference); }
