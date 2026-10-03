#ifndef JVMR_CLASSLOADER_H
#define JVMR_CLASSLOADER_H

#include <stddef.h>
#include "classfile.h"

typedef struct JVMR_ClassLoader JVMR_ClassLoader;

JVMR_ClassLoader *jvmr_classloader_create(void);
void jvmr_classloader_destroy(JVMR_ClassLoader *loader);
int jvmr_classloader_add_path(JVMR_ClassLoader *loader, const char *path);
int jvmr_classloader_add_java_runtime(JVMR_ClassLoader *loader, const char *java_home);
const JVMR_Class *jvmr_classloader_load(JVMR_ClassLoader *loader, const char *internal_name,
	char *error, size_t error_length);

#endif
