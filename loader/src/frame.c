#include <stdio.h>
#include <stdlib.h>
#include "frame.h"

JVMR_Frame current_frame = {
	.code = NULL,
	.pc = 0,
	.sp = 0
};
