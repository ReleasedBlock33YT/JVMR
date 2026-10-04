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

static int run_classpath(int argc, char **argv) {
	if (argc < 4) return INIT_FAIL;
	JVMR_ClassLoader *loader = jvmr_classloader_create();
	if (!loader) return INIT_FAIL;
	char *paths = strdup(argv[2]);
	if (!paths) { jvmr_classloader_destroy(loader); return INIT_FAIL; }
	for (char *path = strtok(paths, ":"); path; path = strtok(NULL, ":"))
		if (jvmr_classloader_add_path(loader, path) != 0) {
			free(paths); jvmr_classloader_destroy(loader); return INIT_FAIL;
		}
	const char *java_home = getenv("JAVA_HOME");
	if (!java_home) java_home = "/usr/lib/jvm/java-21-openjdk-amd64";
	(void)jvmr_classloader_add_java_runtime(loader, java_home);
	char class_name[1024];
	if (strlen(argv[3]) >= sizeof(class_name)) {
		free(paths); jvmr_classloader_destroy(loader); return INIT_FAIL;
	}
	strcpy(class_name, argv[3]);
	for (char *p = class_name; *p; p++) if (*p == '.') *p = '/';
	const JVMR_Class *klass = jvmr_classloader_load(loader, class_name, NULL, 0);
	if (!klass) {
		fprintf(stderr, "[JVMR/ERROR] class %s was not found on the class path.\n", argv[3]);
		free(paths); jvmr_classloader_destroy(loader); return INIT_FAIL;
	}
	jvmr_runtime_set_classloader(loader);
	const char *method_name = argc > 4 ? argv[4] : "main";
	const char *descriptor = argc > 5 ? argv[5] : "([Ljava/lang/String;)V";
	uint64_t arguments[1] = { 0 };
	uint16_t argument_count = 0;
	if (!strcmp(method_name, "main") && !strcmp(descriptor, "([Ljava/lang/String;)V")) {
		int array = jvmr_heap_new_array(JVMR_HEAP_REF_ARRAY, argc > 6 ? argc - 6 : 0);
		if (array < 0) { jvmr_runtime_set_classloader(NULL); free(paths); jvmr_classloader_destroy(loader); return INIT_FAIL; }
		for (int i = 6; i < argc; i++) {
			int string = jvmr_heap_new_string((const uint8_t *)argv[i], (uint16_t)strlen(argv[i]), klass);
			if (string < 0 || jvmr_heap_array_store((uint64_t)array, i - 6, (uint64_t)string)) {
				jvmr_runtime_set_classloader(NULL); free(paths); jvmr_classloader_destroy(loader); return INIT_FAIL;
			}
		}
		arguments[0] = (uint64_t)array;
		argument_count = 1;
	}
	int result = jvmr_execute_class_method_args(klass, method_name, descriptor, arguments, argument_count);
	if (result != 0) fprintf(stderr, "[JVMR/ERROR] method %s%s was not found or failed.\n", method_name, descriptor);
	else if (current_frame.sp > 0) printf("[JVMR/INFO] result: %llu\n", (unsigned long long)current_frame.stack[current_frame.sp - 1]);
	jvmr_runtime_set_classloader(NULL);
	free(paths);
	jvmr_classloader_destroy(loader);
	return result == 0 ? JVM_SUCCESS : INIT_FAIL;
}

static int run_jar(int argc, char **argv) {
	if (argc < 2) return INIT_FAIL;
	JVMR_ClassLoader *loader=jvmr_classloader_create();
	if (!loader || jvmr_classloader_add_path(loader,argv[1])) { jvmr_classloader_destroy(loader); return INIT_FAIL; }
	const char *java_home=getenv("JAVA_HOME");
	if (!java_home) java_home="/usr/lib/jvm/java-21-openjdk-amd64";
	(void)jvmr_classloader_add_java_runtime(loader,java_home);
	uint8_t *manifest=NULL; size_t manifest_size=0;
	if (jvmr_classloader_read_resource(loader,"META-INF/MANIFEST.MF",&manifest,&manifest_size)) { fprintf(stderr,"[JVMR/ERROR] executable JAR has no manifest.\n"); jvmr_classloader_destroy(loader); return INIT_FAIL; }
	char main_class[1024]={0};
	const char *key="Main-Class:";
	for (size_t i=0;i+strlen(key)<=manifest_size;i++) if ((i==0||manifest[i-1]=='\n')&&!memcmp(manifest+i,key,strlen(key))) {
		size_t start=i+strlen(key); while (start<manifest_size&&(manifest[start]==' '||manifest[start]=='\t')) start++;
		size_t end=start; while (end<manifest_size&&manifest[end]!='\r'&&manifest[end]!='\n') end++;
		if (end-start>=sizeof(main_class)) { free(manifest); jvmr_classloader_destroy(loader); return INIT_FAIL; }
		memcpy(main_class,manifest+start,end-start); main_class[end-start]=0; break;
	}
	free(manifest);
	if (!main_class[0]) { fprintf(stderr,"[JVMR/ERROR] executable JAR manifest has no Main-Class.\n"); jvmr_classloader_destroy(loader); return INIT_FAIL; }
	for (char *p=main_class;*p;p++) if (*p=='.') *p='/';
	const JVMR_Class *klass=jvmr_classloader_load(loader,main_class,NULL,0);
	if (!klass) { fprintf(stderr,"[JVMR/ERROR] executable JAR main class %s was not found.\n",main_class); jvmr_classloader_destroy(loader); return INIT_FAIL; }
	jvmr_runtime_set_classloader(loader);
	int argument_count=argc>2?argc-2:0;
	int array=jvmr_heap_new_array(JVMR_HEAP_REF_ARRAY,argument_count);
	if (array<0) { jvmr_runtime_set_classloader(NULL); jvmr_classloader_destroy(loader); return INIT_FAIL; }
	for (int i=0;i<argument_count;i++) { int string=jvmr_heap_new_string((const uint8_t *)argv[i+2],(uint16_t)strlen(argv[i+2]),klass); if (string<0||jvmr_heap_array_store((uint64_t)array,i,(uint64_t)string)) { jvmr_runtime_set_classloader(NULL); jvmr_classloader_destroy(loader); return INIT_FAIL; } }
	uint64_t arguments[1]={(uint64_t)array};
	int result=jvmr_execute_class_method_args(klass,"main","([Ljava/lang/String;)V",arguments,1);
	if (result) fprintf(stderr,"[JVMR/ERROR] executable JAR main method failed.\n");
	jvmr_runtime_set_classloader(NULL); jvmr_classloader_destroy(loader);
	return result?INIT_FAIL:JVM_SUCCESS;
}

int main(int argc, char *argv[]) {
	// JVM initializer
	int bci = bytecode_init();
	if (bci != 0) {
		fprintf(stderr, "FATAL: Bytecode initialization failed.\n       Check Earlier logs.\n");
		return INIT_FAIL;
	}
	if (argc >= 3 && !strcmp(argv[1], "--jar")) return run_jar(argc-1,argv+1);
	if (argc >= 4 && !strcmp(argv[1], "--cp")) return run_classpath(argc, argv);
	if (argc >= 3) return run_class_file(argc,argv,argv[1], argv[2], argc >= 4 ? argv[3] : "()I");

	printf("Hello, JVM World!\n");

	// JVM bytecode test
	uint8_t test_code[] = { ICONST_3, ICONST_4, SWAP, ISUB, ICONST_4, IADD, ICONST_2, IMUL, DUP, IMUL };
	jvm_execute(test_code, sizeof(test_code));

	return JVM_SUCCESS;
}
