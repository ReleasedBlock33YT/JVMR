#ifndef JVMR_CLASSFILE_H
#define JVMR_CLASSFILE_H

#include <stdint.h>
#include <stddef.h>

#define JVMR_CLASS_MAGIC 0xcafebabeu
#define JVMR_JAVA21_MAJOR 65

typedef enum {
	JVMR_CP_UTF8 = 1, JVMR_CP_INTEGER = 3, JVMR_CP_FLOAT = 4,
	JVMR_CP_LONG = 5, JVMR_CP_DOUBLE = 6, JVMR_CP_CLASS = 7,
	JVMR_CP_STRING = 8, JVMR_CP_FIELDREF = 9, JVMR_CP_METHODREF = 10,
	JVMR_CP_INTERFACEMETHODREF = 11, JVMR_CP_NAMEANDTYPE = 12,
	JVMR_CP_METHODHANDLE = 15, JVMR_CP_METHODTYPE = 16,
	JVMR_CP_DYNAMIC = 17, JVMR_CP_INVOKEDYNAMIC = 18,
	JVMR_CP_MODULE = 19, JVMR_CP_PACKAGE = 20
} JVMR_ConstantTag;

typedef struct {
	uint8_t tag;
	union {
		struct { uint16_t length; const uint8_t *bytes; } utf8;
		uint32_t integer;
		uint32_t float_bits;
		uint64_t wide;
		uint16_t index;
		struct { uint16_t left, right; } pair;
		struct { uint8_t reference_kind; uint16_t reference_index; } handle;
		struct { uint16_t bootstrap, name_and_type; } dynamic;
	} value;
} JVMR_Constant;

typedef struct JVMR_Method {
	const char *name;
	const char *descriptor;
	uint16_t access_flags;
	uint16_t max_stack;
	uint16_t max_locals;
	uint32_t code_length;
	const uint8_t *code;
	struct JVMR_ExceptionHandler *exceptions;
	uint16_t exception_count;
} JVMR_Method;

typedef struct JVMR_ExceptionHandler {
	uint16_t start_pc, end_pc, handler_pc, catch_type;
} JVMR_ExceptionHandler;
typedef struct { uint16_t method_ref; uint16_t argument_count; uint16_t *arguments; } JVMR_BootstrapMethod;

typedef struct JVMR_Field {
	const char *name;
	const char *descriptor;
	uint16_t access_flags;
	uint16_t slot;
	uint16_t constant_value_index;
} JVMR_Field;

typedef struct JVMR_Class {
	uint16_t minor_version;
	uint16_t major_version;
	uint16_t access_flags;
	const char *name;
	const char *super_name;
	uint16_t interface_count;
	const char **interfaces;
	uint16_t constant_pool_count;
	JVMR_Constant *constant_pool;
	uint16_t method_count;
	JVMR_Method *methods;
	uint16_t field_count;
	JVMR_Field *fields;
	uint64_t *static_values;
	uint8_t initialized;
	uint8_t initializing;
	uint16_t bootstrap_count;
	JVMR_BootstrapMethod *bootstraps;
} JVMR_Class;

int jvmr_class_parse(const uint8_t *data, size_t length, JVMR_Class *out,
	char *error, size_t error_length);
void jvmr_class_destroy(JVMR_Class *klass);
const JVMR_Method *jvmr_class_find_method(const JVMR_Class *klass,
	const char *name, const char *descriptor);
const JVMR_Method *jvmr_class_resolve_method(const JVMR_Class *klass,
	uint16_t constant_pool_index);
const JVMR_Field *jvmr_class_find_field(const JVMR_Class *klass,
	const char *name, const char *descriptor);
const JVMR_Field *jvmr_class_resolve_field(const JVMR_Class *klass,
	uint16_t constant_pool_index);
const JVMR_Class *jvmr_class_resolve_class(const JVMR_Class *klass,
	uint16_t constant_pool_index);

#endif
