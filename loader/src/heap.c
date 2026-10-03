#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "heap.h"
#include "classfile.h"

typedef struct {
	JVMR_HeapKind kind;
	int32_t length;
	uint64_t *elements;
	const JVMR_Class *klass;
	uint64_t *fields;
	char *text;
	size_t text_length;
	const JVMR_Class *implementation_class;
	const JVMR_Method *lambda_method;
	uint64_t *captured;
	uint16_t captured_count;
	const JVMR_Class *represented_class;
	const JVMR_Class *declaring_class;
	const JVMR_Method *reflected_method;
	const JVMR_Field *reflected_field;
} HeapObject;

static HeapObject *objects;
static uint32_t object_count;
static uint32_t object_capacity;

static HeapObject *lookup(uint64_t reference) {
	if (reference == 0 || reference > object_count) return NULL;
	return &objects[reference - 1];
}

static int allocate(JVMR_HeapKind kind, int32_t length, const JVMR_Class *klass) {
	if (length < 0) return -1;
	if (object_count == object_capacity) {
		uint32_t capacity = object_capacity ? object_capacity * 2 : 64;
		HeapObject *next = realloc(objects, (size_t)capacity * sizeof(*objects));
		if (!next) return -1;
		objects = next; object_capacity = capacity;
	}
	HeapObject *object = &objects[object_count];
	object->kind = kind; object->length = length; object->elements = NULL; object->klass = klass; object->fields = NULL; object->text=NULL; object->text_length=0; object->implementation_class=NULL; object->lambda_method=NULL; object->captured=NULL; object->captured_count=0; object->represented_class=NULL; object->declaring_class=NULL; object->reflected_method=NULL; object->reflected_field=NULL;
	if (kind == JVMR_HEAP_OBJECT && klass && klass->name && (!strcmp(klass->name,"java/lang/StringBuilder") || !strcmp(klass->name,"java/lang/AbstractStringBuilder"))) { object->text=strdup(""); if(!object->text)return -1; }
	if (kind == JVMR_HEAP_OBJECT && klass && klass->field_count) { object->fields=calloc(klass->field_count,sizeof(*object->fields)); if(!object->fields)return -1; }
	if (kind != JVMR_HEAP_OBJECT && length > 0) {
		object->elements = calloc((size_t)length, sizeof(*object->elements));
		if (!object->elements) return -1;
	}
	return (int)++object_count;
}

int jvmr_heap_new_object(const struct JVMR_Class *klass) { return allocate(JVMR_HEAP_OBJECT, 0, klass); }
int jvmr_heap_new_class_mirror(const struct JVMR_Class *represented_class,const struct JVMR_Class *class_class) { if(!represented_class)return -1;for(uint32_t i=0;i<object_count;i++)if(objects[i].represented_class==represented_class&&objects[i].klass==class_class)return (int)i+1;int reference=allocate(JVMR_HEAP_OBJECT,0,class_class);if(reference<0)return -1;lookup((uint64_t)reference)->represented_class=represented_class;return reference; }
int jvmr_heap_class_mirror(uint64_t reference,const struct JVMR_Class **represented_class) { HeapObject *object=lookup(reference);if(!object||object->kind!=JVMR_HEAP_OBJECT||!object->represented_class||!represented_class)return -1;*represented_class=object->represented_class;return 0; }
int jvmr_heap_new_method_mirror(const struct JVMR_Class *reflection_class,const struct JVMR_Class *declaring_class,const struct JVMR_Method *method) { int reference=allocate(JVMR_HEAP_OBJECT,0,reflection_class);if(reference<0)return -1;HeapObject *object=lookup((uint64_t)reference);object->declaring_class=declaring_class;object->reflected_method=method;return reference; }
int jvmr_heap_method_mirror(uint64_t reference,const struct JVMR_Class **declaring_class,const struct JVMR_Method **method) { HeapObject *object=lookup(reference);if(!object||object->kind!=JVMR_HEAP_OBJECT||!object->reflected_method)return -1;if(declaring_class)*declaring_class=object->declaring_class;if(method)*method=object->reflected_method;return 0; }
int jvmr_heap_new_field_mirror(const struct JVMR_Class *reflection_class,const struct JVMR_Class *declaring_class,const struct JVMR_Field *field) { int reference=allocate(JVMR_HEAP_OBJECT,0,reflection_class);if(reference<0)return -1;HeapObject *object=lookup((uint64_t)reference);object->declaring_class=declaring_class;object->reflected_field=field;return reference; }
int jvmr_heap_field_mirror(uint64_t reference,const struct JVMR_Class **declaring_class,const struct JVMR_Field **field) { HeapObject *object=lookup(reference);if(!object||object->kind!=JVMR_HEAP_OBJECT||!object->reflected_field)return -1;if(declaring_class)*declaring_class=object->declaring_class;if(field)*field=object->reflected_field;return 0; }
int jvmr_heap_new_lambda(const struct JVMR_Class *interface_class,const struct JVMR_Class *implementation_class,const struct JVMR_Method *method,const uint64_t *captured,uint16_t captured_count) { int reference=allocate(JVMR_HEAP_OBJECT,0,interface_class);if(reference<0)return -1;HeapObject *object=lookup((uint64_t)reference);object->implementation_class=implementation_class;object->lambda_method=method;object->captured_count=captured_count;if(captured_count){object->captured=malloc((size_t)captured_count*sizeof(*object->captured));if(!object->captured)return -1;memcpy(object->captured,captured,(size_t)captured_count*sizeof(*captured));}return reference; }
int jvmr_heap_lambda_info(uint64_t reference,const struct JVMR_Class **implementation_class,const struct JVMR_Method **method,const uint64_t **captured,uint16_t *captured_count) { HeapObject *object=lookup(reference);if(!object||object->kind!=JVMR_HEAP_OBJECT||!object->lambda_method)return -1;if(implementation_class)*implementation_class=object->implementation_class;if(method)*method=object->lambda_method;if(captured)*captured=object->captured;if(captured_count)*captured_count=object->captured_count;return 0; }
int jvmr_heap_new_string(const uint8_t *bytes,uint16_t length,const struct JVMR_Class *klass) { int reference=allocate(JVMR_HEAP_STRING,length,klass); if(reference<0)return -1; HeapObject *object=lookup((uint64_t)reference); if(length)for(uint16_t i=0;i<length;i++)object->elements[i]=bytes[i]; return reference; }
int jvmr_heap_new_array(JVMR_HeapKind kind, int32_t length) {
	return kind == JVMR_HEAP_PRIMITIVE_ARRAY || kind == JVMR_HEAP_INT_ARRAY || kind == JVMR_HEAP_REF_ARRAY ? allocate(kind, length, NULL) : -1;
}
int jvmr_heap_array_length(uint64_t reference, int32_t *length) {
	HeapObject *object = lookup(reference);
	if (!object || object->kind == JVMR_HEAP_OBJECT || !length) return -1;
	*length = object->length; return 0;
}
int jvmr_heap_array_load(uint64_t reference, int32_t index, uint64_t *value) {
	HeapObject *object = lookup(reference);
	if (!object || object->kind == JVMR_HEAP_OBJECT || index < 0 || index >= object->length || !value) return -1;
	*value = object->elements[index]; return 0;
}
int jvmr_heap_array_store(uint64_t reference, int32_t index, uint64_t value) {
	HeapObject *object = lookup(reference);
	if (!object || object->kind == JVMR_HEAP_OBJECT || index < 0 || index >= object->length) return -1;
	object->elements[index] = value; return 0;
}
void jvmr_heap_destroy(void) {
	for (uint32_t i = 0; i < object_count; i++) { free(objects[i].elements); free(objects[i].text); free(objects[i].captured); }
	for (uint32_t i = 0; i < object_count; i++) free(objects[i].fields);
	free(objects); objects = NULL; object_count = object_capacity = 0;
}
const struct JVMR_Class *jvmr_heap_object_class(uint64_t reference) { HeapObject *object=lookup(reference); return object&&(object->kind==JVMR_HEAP_OBJECT||object->kind==JVMR_HEAP_STRING)?object->klass:NULL; }
int jvmr_heap_string_length(uint64_t reference,int32_t *length){HeapObject *o=lookup(reference);if(!o||o->kind!=JVMR_HEAP_STRING||!length)return -1;*length=o->length;return 0;}
int jvmr_heap_string_char_at(uint64_t reference,int32_t index,uint16_t *value){HeapObject *o=lookup(reference);if(!o||o->kind!=JVMR_HEAP_STRING||index<0||index>=o->length||!value)return -1;*value=(uint16_t)o->elements[index];return 0;}
int jvmr_heap_string_equals(uint64_t left,uint64_t right,int *equal){HeapObject *a=lookup(left),*b=lookup(right);if(!a||!b||a->kind!=JVMR_HEAP_STRING||b->kind!=JVMR_HEAP_STRING||!equal)return -1;if(a->length!=b->length){*equal=0;return 0;}*equal=memcmp(a->elements,b->elements,(size_t)a->length*sizeof(*a->elements))==0;return 0;}
int32_t jvmr_heap_string_hash(uint64_t reference){HeapObject *o=lookup(reference);if(!o||o->kind!=JVMR_HEAP_STRING)return 0;int32_t hash=0;for(int32_t i=0;i<o->length;i++)hash=31*hash+(int32_t)o->elements[i];return hash;}
int jvmr_heap_string_copy(uint64_t reference,char *buffer,size_t capacity,size_t *length){HeapObject *o=lookup(reference);if(!o||o->kind!=JVMR_HEAP_STRING||!length)return -1;*length=(size_t)o->length;if(buffer&&capacity){size_t n=*length<capacity-1?*length:capacity-1;for(size_t i=0;i<n;i++)buffer[i]=(char)o->elements[i];buffer[n]=0;}return 0;}
static HeapObject *builder(uint64_t reference){HeapObject *o=lookup(reference);return o&&o->kind==JVMR_HEAP_OBJECT&&o->text?o:NULL;}
int jvmr_heap_builder_append_string(uint64_t reference,uint64_t string){HeapObject *b=builder(reference),*s=lookup(string);if(!b||!s||s->kind!=JVMR_HEAP_STRING)return -1;char *next=realloc(b->text,b->text_length+(size_t)s->length+1);if(!next)return -1;b->text=next;for(int32_t i=0;i<s->length;i++)b->text[b->text_length+i]=(char)s->elements[i];b->text_length+=(size_t)s->length;b->text[b->text_length]=0;return 0;}
int jvmr_heap_builder_append_int(uint64_t reference,int32_t value){char text[32];int length=snprintf(text,sizeof(text),"%d",value);if(length<0)return -1;int string=jvmr_heap_new_string((const uint8_t*)text,(uint16_t)length,NULL);if(string<0)return -1;int result=jvmr_heap_builder_append_string(reference,(uint64_t)string);return result;}
int jvmr_heap_builder_to_string(uint64_t reference,int *result){HeapObject *b=builder(reference);if(!b||!result)return -1;*result=jvmr_heap_new_string((const uint8_t*)b->text,(uint16_t)b->text_length,NULL);return *result<0?-1:0;}
int jvmr_heap_field_load(uint64_t reference,uint16_t slot,uint64_t *value) { HeapObject *o=lookup(reference); if(!o||o->kind!=JVMR_HEAP_OBJECT||!o->fields||slot>=o->klass->field_count||!value)return -1; *value=o->fields[slot]; return 0; }
int jvmr_heap_field_store(uint64_t reference,uint16_t slot,uint64_t value) { HeapObject *o=lookup(reference); if(!o||o->kind!=JVMR_HEAP_OBJECT||!o->fields||slot>=o->klass->field_count)return -1; o->fields[slot]=value; return 0; }
