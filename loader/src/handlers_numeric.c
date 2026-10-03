#include <stdint.h>
#include <string.h>
#include "frame.h"
#include "handlers.h"

static float as_float(uint64_t x) { float v; uint32_t b = (uint32_t)x; memcpy(&v, &b, 4); return v; }
static uint64_t as_float_bits(float v) { uint32_t b; memcpy(&b, &v, 4); return b; }
static double as_double(uint64_t x) { double v; memcpy(&v, &x, 8); return v; }
static uint64_t as_double_bits(double v) { uint64_t b; memcpy(&b, &v, 8); return b; }
#define BIN_INT(name, op) void handler_##name(void) { uint64_t b=current_frame.stack[--current_frame.sp], a=current_frame.stack[--current_frame.sp]; current_frame.stack[current_frame.sp++]=(uint64_t)((int64_t)a op (int64_t)b); }
#define BIN_UINT(name, op) void handler_##name(void) { uint64_t b=current_frame.stack[--current_frame.sp], a=current_frame.stack[--current_frame.sp]; current_frame.stack[current_frame.sp++]=a op b; }
#define SHIFT_INT(name, type, op, mask) void handler_##name(void) { uint64_t n=current_frame.stack[--current_frame.sp], v=current_frame.stack[--current_frame.sp]; current_frame.stack[current_frame.sp++]=(uint64_t)((type)v op (n & mask)); }
BIN_INT(iand, &) BIN_INT(ior, |) BIN_INT(ixor, ^)
BIN_UINT(land, &) BIN_UINT(lor, |) BIN_UINT(lxor, ^)
SHIFT_INT(ishl, int32_t, <<, 0x1f) SHIFT_INT(ishr, int32_t, >>, 0x1f)
SHIFT_INT(iushr, uint32_t, >>, 0x1f) SHIFT_INT(lshl, int64_t, <<, 0x3f)
SHIFT_INT(lshr, int64_t, >>, 0x3f) SHIFT_INT(lushr, uint64_t, >>, 0x3f)

void handler_i2l(void) { current_frame.stack[current_frame.sp-1]=(int64_t)(int32_t)current_frame.stack[current_frame.sp-1]; }
void handler_i2f(void) { current_frame.stack[current_frame.sp-1]=as_float_bits((float)(int32_t)current_frame.stack[current_frame.sp-1]); }
void handler_i2d(void) { current_frame.stack[current_frame.sp-1]=as_double_bits((double)(int32_t)current_frame.stack[current_frame.sp-1]); }
void handler_l2i(void) { current_frame.stack[current_frame.sp-1]=(int32_t)current_frame.stack[current_frame.sp-1]; }
void handler_l2f(void) { current_frame.stack[current_frame.sp-1]=as_float_bits((float)(int64_t)current_frame.stack[current_frame.sp-1]); }
void handler_l2d(void) { current_frame.stack[current_frame.sp-1]=as_double_bits((double)(int64_t)current_frame.stack[current_frame.sp-1]); }
void handler_f2i(void) { current_frame.stack[current_frame.sp-1]=(int32_t)as_float(current_frame.stack[current_frame.sp-1]); }
void handler_f2l(void) { current_frame.stack[current_frame.sp-1]=(int64_t)as_float(current_frame.stack[current_frame.sp-1]); }
void handler_f2d(void) { current_frame.stack[current_frame.sp-1]=as_double_bits((double)as_float(current_frame.stack[current_frame.sp-1])); }
void handler_d2i(void) { current_frame.stack[current_frame.sp-1]=(int32_t)as_double(current_frame.stack[current_frame.sp-1]); }
void handler_d2l(void) { current_frame.stack[current_frame.sp-1]=(int64_t)as_double(current_frame.stack[current_frame.sp-1]); }
void handler_d2f(void) { current_frame.stack[current_frame.sp-1]=as_float_bits((float)as_double(current_frame.stack[current_frame.sp-1])); }
