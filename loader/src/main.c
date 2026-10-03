#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "bytecode.h"
#include "errors.h"
#include "frame.h"
#include "classfile.h"
#include "classloader.h"
#include "runtime.h"
#include "heap.h"

static int run_class_file(int argc, char **argv, const char *path, const char *method_name, const char *descriptor) {
	JVMR_ClassLoader *loader=jvmr_classloader_create();
	if (!loader) return INIT_FAIL;
	const char *slash=strrchr(path,'/');
	char directory[2048];
	if (slash) { size_t length=(size_t)(slash-path); if(length>=sizeof(directory)){jvmr_classloader_destroy(loader);return INIT_FAIL;} memcpy(directory,path,length);directory[length]=0; }
	else strcpy(directory,".");
	(void)jvmr_classloader_add_path(loader,directory);
	const char *java_home=getenv("JAVA_HOME");
	if (!java_home) java_home="/usr/lib/jvm/java-21-openjdk-amd64";
	(void)jvmr_classloader_add_java_runtime(loader,java_home);
	FILE *file = fopen(path, "rb");
	if (!file) { perror(path); jvmr_classloader_destroy(loader); return INIT_FAIL; }
	if (fseek(file, 0, SEEK_END) != 0) { fclose(file); jvmr_classloader_destroy(loader); return INIT_FAIL; }
	long size = ftell(file);
	if (size <= 0) { fclose(file); jvmr_classloader_destroy(loader); return INIT_FAIL; }
	rewind(file);
	uint8_t *data = malloc((size_t)size);
	if (!data || fread(data, 1, (size_t)size, file) != (size_t)size) { free(data); fclose(file); jvmr_classloader_destroy(loader); return INIT_FAIL; }
	fclose(file);
	JVMR_Class klass;
	char error[256];
	if (jvmr_class_parse(data, (size_t)size, &klass, error, sizeof(error)) != 0) {
		fprintf(stderr, "[JVMR/ERROR] %s\n", error);
		free(data);
		jvmr_classloader_destroy(loader);
		return INIT_FAIL;
	}
	jvmr_runtime_set_classloader(loader);
	uint64_t arguments[1]={0}; uint16_t argument_count=0;
	if (!strcmp(method_name,"main") && !strcmp(descriptor,"([Ljava/lang/String;)V")) {
		int array=jvmr_heap_new_array(JVMR_HEAP_REF_ARRAY,argc>3?argc-3:0);
		if(array<0){jvmr_class_destroy(&klass);free(data);jvmr_runtime_set_classloader(NULL);jvmr_classloader_destroy(loader);return INIT_FAIL;}
		for(int i=3;i<argc;i++){int string=jvmr_heap_new_string((const uint8_t *)argv[i],(uint16_t)strlen(argv[i]),&klass);if(string<0||jvmr_heap_array_store((uint64_t)array,i-3,(uint64_t)string)){jvmr_class_destroy(&klass);free(data);jvmr_runtime_set_classloader(NULL);jvmr_classloader_destroy(loader);return INIT_FAIL;}}
		arguments[0]=(uint64_t)array;argument_count=1;
	}
	int result = jvmr_execute_class_method_args(&klass, method_name, descriptor, arguments, argument_count);
	if (result != 0) fprintf(stderr, "[JVMR/ERROR] method %s%s was not found or has no Code attribute.\n", method_name, descriptor);
	else if (current_frame.sp > 0) printf("[JVMR/INFO] result: %llu\n", (unsigned long long)current_frame.stack[current_frame.sp - 1]);
	jvmr_class_destroy(&klass);
	free(data);
	jvmr_runtime_set_classloader(NULL);
	jvmr_classloader_destroy(loader);
	return result == 0 ? JVM_SUCCESS : INIT_FAIL;
}

int main(int argc, char *argv[]) {
	// JVM initializer
	int bci = bytecode_init();
	if (bci != 0) {
		fprintf(stderr, "FATAL: Bytecode initialization failed.\n       Check Earlier logs.\n");
		return INIT_FAIL;
	}
	if (argc >= 3) return run_class_file(argc,argv,argv[1], argv[2], argc >= 4 ? argv[3] : "()I");

	printf("Hello, JVM World!\n");

	// JVM bytecode test
	uint8_t test_code[] = { ICONST_3, ICONST_4, SWAP, ISUB, ICONST_4, IADD, ICONST_2, IMUL, DUP, IMUL };
	jvm_execute(test_code, sizeof(test_code));

	return JVM_SUCCESS;
}
