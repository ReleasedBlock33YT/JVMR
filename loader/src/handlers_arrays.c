#include <stdio.h>
#include "heap.h"
#include "runtime.h"
#include "frame.h"
#include "handlers.h"

static void array_error(const char *name) { fprintf(stderr, "[JVMR/ERROR] %s received an invalid array reference or index.\n", name); }
#define ARRAY_LOAD(name, type) void handler_##name(void) { uint64_t index=current_frame.stack[--current_frame.sp], ref=current_frame.stack[--current_frame.sp], value; if(jvmr_heap_array_load(ref,(int32_t)index,&value)){array_error(#name);jvmr_throw_named(ref?"java/lang/ArrayIndexOutOfBoundsException":"java/lang/NullPointerException");return;} current_frame.stack[current_frame.sp++]=(type)value; }
#define ARRAY_STORE(name) void handler_##name(void) { uint64_t value=current_frame.stack[--current_frame.sp],index=current_frame.stack[--current_frame.sp],ref=current_frame.stack[--current_frame.sp]; if(jvmr_heap_array_store(ref,(int32_t)index,value)){array_error(#name);jvmr_throw_named(ref?"java/lang/ArrayIndexOutOfBoundsException":"java/lang/NullPointerException");} }
ARRAY_LOAD(iaload, int32_t) ARRAY_LOAD(laload, uint64_t) ARRAY_LOAD(faload, uint32_t) ARRAY_LOAD(daload, uint64_t) ARRAY_LOAD(aaload, uint64_t)
ARRAY_LOAD(baload, int8_t) ARRAY_LOAD(caload, uint16_t) ARRAY_LOAD(saload, int16_t)
ARRAY_STORE(iastore) ARRAY_STORE(lastore) ARRAY_STORE(fastore) ARRAY_STORE(dastore) ARRAY_STORE(aastore)
ARRAY_STORE(bastore) ARRAY_STORE(castore) ARRAY_STORE(sastore)
