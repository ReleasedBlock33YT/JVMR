#ifndef JVMR_NATIVE_H
#define JVMR_NATIVE_H

#include "classfile.h"

/* Returns 0 when handled, 1 when no native implementation exists, -1 on error. */
int jvmr_native_invoke(const JVMR_Class *klass, const JVMR_Method *method,
	int is_static);

#endif
