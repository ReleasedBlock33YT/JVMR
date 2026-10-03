#ifndef JVMR_RUNTIME_H
#define JVMR_RUNTIME_H

#include <stdint.h>
#include "classfile.h"
#include "classloader.h"

#define JVMR_CALL_STACK_MAX 256

int jvmr_execute_class_method(const JVMR_Class *klass, const char *name, const char *descriptor);
int jvmr_execute_class_method_args(const JVMR_Class *klass, const char *name, const char *descriptor, const uint64_t *arguments, uint16_t argument_count);
void jvmr_runtime_set_classloader(JVMR_ClassLoader *loader);
int jvmr_invoke_static(uint16_t constant_pool_index);
int jvmr_invoke_instance(uint16_t constant_pool_index);
int jvmr_invoke_interface(uint16_t constant_pool_index);
int jvmr_invoke_special(uint16_t constant_pool_index);
int jvmr_invoke_dynamic(uint16_t constant_pool_index);
const JVMR_Field *jvmr_runtime_resolve_field(uint16_t constant_pool_index, const JVMR_Class **owner_out);
void jvmr_return_value(uint64_t value, int has_value);
void jvmr_throw_reference(uint64_t reference);
void jvmr_throw_named(const char *class_name);
const JVMR_Class *jvmr_runtime_class(void);
const JVMR_Class *jvmr_runtime_resolve_class(uint16_t constant_pool_index);
const JVMR_Class *jvmr_runtime_load_class_name(const char *name);
int jvmr_runtime_class_is_assignable(const JVMR_Class *candidate, const JVMR_Class *target);
int jvmr_invoke_reflected(const JVMR_Class *declaring_class, const JVMR_Method *method, uint64_t receiver, const uint64_t *arguments, uint16_t argument_count);
int jvmr_runtime_is_instance(uint64_t reference, const JVMR_Class *target);
int jvmr_ensure_initialized(const JVMR_Class *klass);

#endif
