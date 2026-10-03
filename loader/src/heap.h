#ifndef JVMR_HEAP_H
#define JVMR_HEAP_H

#include <stdint.h>
#include <stddef.h>
struct JVMR_Class;
struct JVMR_Method;
struct JVMR_Field;

typedef enum { JVMR_HEAP_OBJECT, JVMR_HEAP_PRIMITIVE_ARRAY, JVMR_HEAP_INT_ARRAY, JVMR_HEAP_REF_ARRAY, JVMR_HEAP_STRING } JVMR_HeapKind;

int jvmr_heap_new_object(const struct JVMR_Class *klass);
int jvmr_heap_new_class_mirror(const struct JVMR_Class *represented_class, const struct JVMR_Class *class_class);
int jvmr_heap_class_mirror(uint64_t reference, const struct JVMR_Class **represented_class);
int jvmr_heap_new_method_mirror(const struct JVMR_Class *reflection_class, const struct JVMR_Class *declaring_class, const struct JVMR_Method *method);
int jvmr_heap_method_mirror(uint64_t reference, const struct JVMR_Class **declaring_class, const struct JVMR_Method **method);
int jvmr_heap_new_field_mirror(const struct JVMR_Class *reflection_class, const struct JVMR_Class *declaring_class, const struct JVMR_Field *field);
int jvmr_heap_field_mirror(uint64_t reference, const struct JVMR_Class **declaring_class, const struct JVMR_Field **field);
int jvmr_heap_new_lambda(const struct JVMR_Class *interface_class, const struct JVMR_Class *implementation_class, const struct JVMR_Method *method, const uint64_t *captured, uint16_t captured_count);
int jvmr_heap_lambda_info(uint64_t reference, const struct JVMR_Class **implementation_class, const struct JVMR_Method **method, const uint64_t **captured, uint16_t *captured_count);
int jvmr_heap_new_string(const uint8_t *bytes, uint16_t length, const struct JVMR_Class *klass);
int jvmr_heap_new_array(JVMR_HeapKind kind, int32_t length);
int jvmr_heap_array_length(uint64_t reference, int32_t *length);
int jvmr_heap_array_load(uint64_t reference, int32_t index, uint64_t *value);
int jvmr_heap_array_store(uint64_t reference, int32_t index, uint64_t value);
int jvmr_heap_field_load(uint64_t reference, uint16_t slot, uint64_t *value);
int jvmr_heap_field_store(uint64_t reference, uint16_t slot, uint64_t value);
const struct JVMR_Class *jvmr_heap_object_class(uint64_t reference);
int jvmr_heap_string_length(uint64_t reference, int32_t *length);
int jvmr_heap_string_char_at(uint64_t reference, int32_t index, uint16_t *value);
int jvmr_heap_string_equals(uint64_t left, uint64_t right, int *equal);
int32_t jvmr_heap_string_hash(uint64_t reference);
int jvmr_heap_string_copy(uint64_t reference, char *buffer, size_t capacity, size_t *length);
int jvmr_heap_builder_append_string(uint64_t builder, uint64_t string);
int jvmr_heap_builder_append_int(uint64_t builder, int32_t value);
int jvmr_heap_builder_to_string(uint64_t builder, int *reference);
void jvmr_heap_destroy(void);

#endif
