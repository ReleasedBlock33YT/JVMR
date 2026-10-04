#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bytecode.h"
#include "classfile.h"
#include "classloader.h"
#include "errors.h"
#include "frame.h"
#include "heap.h"
#include "runtime.h"

static int failures;

#define CHECK(condition, message) do { \
	if (!(condition)) { fprintf(stderr, "FAIL: %s\n", message); failures++; } \
} while (0)

static void reset_frame(void) {
	memset(&current_frame, 0, sizeof(current_frame));
}

static uint64_t float_bits(float value) {
	uint32_t bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static uint64_t double_bits(double value) {
	uint64_t bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static void test_registration(void) {
	for (int opcode = 0; opcode < JVMR_OPCODE_COUNT; opcode++) {
		char message[64];
		snprintf(message, sizeof(message), "opcode %d has a handler", opcode);
		CHECK(bytecode[opcode] != NULL, message);
	}
}

static void test_every_handler_is_callable(void) {
	uint8_t operands[64] = { 0 };
	for (int opcode = 0; opcode < JVMR_OPCODE_COUNT; opcode++) {
		reset_frame();
		current_frame.code = operands;
		current_frame.pc = 1;
		/* Give stack-consuming instructions valid operands. */
		current_frame.sp = 8;
		for (int i = 0; i < current_frame.sp; i++) current_frame.stack[i] = 1;
		bytecode[opcode]();
	}
	CHECK(1, "every registered instruction handler executes without crashing");
}

static void test_constants(void) {
	uint8_t code[] = {
		ACONST_NULL, ICONST_M1, ICONST_0, ICONST_1, ICONST_2, ICONST_3,
		ICONST_4, ICONST_5, LCONST_0, LCONST_1, FCONST_0, FCONST_1,
		FCONST_2, DCONST_0, DCONST_1, BIPUSH, (uint8_t)-7, SIPUSH, 0x01, 0xf4
	};
	reset_frame();
	jvm_execute(code, sizeof(code));
	CHECK(current_frame.sp == 17, "constant instructions push one value each");
	CHECK((int64_t)current_frame.stack[1] == -1, "iconst_m1 sign extends");
	CHECK((int64_t)current_frame.stack[15] == -7, "bipush sign extends");
	CHECK((int64_t)current_frame.stack[16] == 500, "sipush decodes big-endian signed operand");
	CHECK(current_frame.stack[11] == float_bits(1.0f), "fconst_1 has the correct bits");
	CHECK(current_frame.stack[14] == double_bits(1.0), "dconst_1 has the correct bits");
}

static void test_locals(void) {
	uint8_t code[] = { ICONST_5, ISTORE, 7, ILOAD, 7, ISTORE_0, ILOAD_0,
		ASTORE_1, ALOAD_1 };
	reset_frame();
	jvm_execute(code, sizeof(code));
	CHECK(current_frame.sp == 1, "load/store instructions preserve stack values");
	CHECK(current_frame.stack[0] == 5,
		"indexed and fixed local loads/stores work");
}

static void test_stack(void) {
	uint8_t code[] = { ICONST_1, ICONST_2, DUP, SWAP, DUP_X1, POP, DUP2,
		POP2, ICONST_3, ICONST_4, DUP2_X1, POP2, POP, ICONST_1, ICONST_2,
		ICONST_3, ICONST_4, DUP2_X2, POP2, POP2, POP2, POP2, POP2 };
	reset_frame();
	jvm_execute(code, sizeof(code));
	CHECK(current_frame.sp == 0, "all stack manipulation instructions balance");
}

static void test_arithmetic(void) {
	uint8_t integer_code[] = { ICONST_3, ICONST_4, IADD, ICONST_2, ISUB,
		ICONST_5, IMUL, ICONST_3, IDIV, ICONST_2, IREM, INEG };
	reset_frame();
	jvm_execute(integer_code, sizeof(integer_code));
	CHECK(current_frame.sp == 1 && (int32_t)current_frame.stack[0] == 0,
		"integer arithmetic instructions work");

	reset_frame();
	current_frame.stack[0] = float_bits(1.5f);
	current_frame.stack[1] = float_bits(2.0f);
	current_frame.sp = 2;
	bytecode[FADD]();
	CHECK(current_frame.stack[0] == float_bits(3.5f), "float addition works");

	reset_frame();
	current_frame.stack[0] = double_bits(7.0);
	current_frame.stack[1] = double_bits(2.0);
	current_frame.sp = 2;
	bytecode[DREM]();
	CHECK(current_frame.stack[0] == double_bits(1.0), "double remainder works");
}

static void test_numeric(void) {
	reset_frame();
	current_frame.stack[0] = 1;
	current_frame.stack[1] = 3;
	current_frame.sp = 2;
	bytecode[IAND]();
	CHECK(current_frame.stack[0] == 1, "integer bitwise and works");

	current_frame.stack[0] = (uint32_t)8;
	current_frame.stack[1] = 2;
	current_frame.sp = 2;
	bytecode[IUSHR]();
	CHECK(current_frame.stack[0] == 2, "unsigned integer shift works");

	current_frame.stack[0] = (uint64_t)(int64_t)-12;
	current_frame.sp = 1;
	bytecode[I2F]();
	CHECK(current_frame.stack[0] == float_bits(-12.0f), "integer-to-float conversion works");
}

static void test_extended_instructions(void) {
	uint8_t wide_load[] = { WIDE, ILOAD, 0, 200 };
	reset_frame();
	current_frame.locals[200] = 17;
	jvm_execute(wide_load, sizeof(wide_load));
	CHECK(current_frame.sp == 1 && current_frame.stack[0] == 17,
		"wide loads address locals above byte-sized indexes");

	uint8_t wide_increment[] = { WIDE, IINC, 0, 200, 0xff, 0xf9 };
	reset_frame();
	current_frame.locals[200] = 17;
	jvm_execute(wide_increment, sizeof(wide_increment));
	CHECK(current_frame.locals[200] == 10,
		"wide iinc applies a signed 16-bit increment");

	uint8_t multidimensional[] = { ICONST_2, ICONST_3, MULTIANEWARRAY, 0, 0, 2, ARRAYLENGTH };
	reset_frame();
	jvm_execute(multidimensional, sizeof(multidimensional));
	CHECK(current_frame.sp == 1 && current_frame.stack[0] == 2,
		"multianewarray creates the requested outer dimension");

	uint8_t monitors[] = { ACONST_NULL, MONITORENTER, ACONST_NULL, MONITOREXIT };
	reset_frame();
	jvm_execute(monitors, sizeof(monitors));
	CHECK(current_frame.sp == 0,
		"monitor instructions consume their object references");
}

static void test_control_flow(void) {
	/* ifeq at offset 1 jumps to the final nop at offset 6. */
	uint8_t taken[] = { ICONST_0, IFEQ, 0, 5, ICONST_1, RETURN, NOP };
	reset_frame();
	jvm_execute(taken, sizeof(taken));
	CHECK(current_frame.sp == 0, "ifeq consumes its operand and branches");

	/* goto skips ICONST_1 and lands on RETURN. */
	uint8_t jump[] = { GOTO, 0, 4, ICONST_1, NOP, RETURN };
	reset_frame();
	jvm_execute(jump, sizeof(jump));
	CHECK(current_frame.sp == 0, "goto branches relative to its opcode");
}

static void test_returns(void) {
	uint8_t code[] = { ICONST_1, IRETURN, ICONST_2 };
	reset_frame();
	jvm_execute(code, sizeof(code));
	CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
		"return instructions stop execution");
}

static void test_class_file(const char *path) {
	FILE *file = fopen(path, "rb");
	long size;
	uint8_t *data;
	char error[160];
	JVMR_Class klass;
	CHECK(file != NULL, "Java 21 fixture opens");
	if (!file) return;
	fseek(file, 0, SEEK_END); size = ftell(file); rewind(file);
	data = malloc((size_t)size);
	CHECK(data != NULL && fread(data, 1, (size_t)size, file) == (size_t)size,
		"class fixture reads completely");
	fclose(file);
	if (!data) return;
	int parsed = jvmr_class_parse(data, (size_t)size, &klass, error, sizeof(error));
	CHECK(parsed == 0, parsed == 0 ? "class parser succeeds" : error);
	if (parsed == 0) {
		const JVMR_Method *method = jvmr_class_find_method(&klass, "answer", "()I");
		CHECK(klass.major_version == JVMR_JAVA21_MAJOR, "class parser accepts Java 21 version");
		CHECK(klass.name && strcmp(klass.name, "Fixture") == 0, "class name resolves from constant pool");
		CHECK(method && method->code_length > 0, "Code attribute and method bytecode are extracted");
		if (method) {
			CHECK(jvm_execute_method(method) == 0, "parsed method executes");
			CHECK(current_frame.sp == 1 && current_frame.stack[0] == 42,
				"Java 21 method returns its expected value");
		}
		CHECK(jvmr_execute_class_method(&klass, "caller", "()I") == 0,
			"class method with invokestatic executes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 42,
			"invokestatic transfers the nested return value");
		CHECK(jvmr_execute_class_method(&klass, "arraySum", "()I") == 0,
			"class method with array bytecode executes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 12,
			"managed integer arrays load and store values");
		CHECK(jvmr_execute_class_method(&klass, "instanceValue", "()I") == 0,
			"class method with object construction executes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 37,
			"constructors and instance fields preserve object state");
		CHECK(jvmr_execute_class_method(&klass, "constantValue", "()I") == 0,
			"method using ldc executes");
		CHECK(current_frame.sp == 1 && (int32_t)current_frame.stack[0] == 123456,
			"ldc resolves integer constants from the constant pool");
		CHECK(jvmr_execute_class_method(&klass, "staticValue", "()I") == 0,
			"first static access runs class initialization");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 11,
			"getstatic observes values written by clinit");
		JVMR_ClassLoader *loader=jvmr_classloader_create();
		CHECK(loader && jvmr_classloader_add_path(loader,"loader/build")==0,
			"cross-class directory path is configured");
		jvmr_runtime_set_classloader(loader);
		CHECK(jvmr_execute_class_method(&klass, "crossClass", "()I") == 0,
			"cross-class invokestatic executes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 12,
			"cross-class invocation resolves the external class");
		jvmr_runtime_set_classloader(NULL);
		jvmr_classloader_destroy(loader);
		JVMR_ClassLoader *jar_loader=jvmr_classloader_create();
		CHECK(jar_loader && jvmr_classloader_add_path(jar_loader,"loader/build/fixture.jar")==0,
			"JAR class path supports execution");
		jvmr_runtime_set_classloader(jar_loader);
		CHECK(jvmr_execute_class_method(&klass, "crossClass", "()I") == 0,
			"cross-class invocation works through a JAR");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 12,
			"JAR-loaded method returns its value");
		jvmr_runtime_set_classloader(NULL);
		jvmr_classloader_destroy(jar_loader);
		JVMR_ClassLoader *jdk=jvmr_classloader_create();
		CHECK(jdk && jvmr_classloader_add_java_runtime(jdk,"/usr/lib/jvm/java-21-openjdk-amd64")==0,
			"Java runtime modules are configured for native calls");
		CHECK(jdk && jvmr_classloader_add_path(jdk,"loader/build")==0,
			"application classes can be used with the Java runtime path");
		CHECK(jdk && jvmr_classloader_load(jdk,"java/lang/Math",error,sizeof(error))!=NULL,
			"class loader finds java.lang.Math in java.base");
		jvmr_runtime_set_classloader(jdk);
		CHECK(jvmr_execute_class_method(&klass, "nativeMath", "()I") == 0,
			"class method can call a Java platform method");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 9,
			"native Math method returns its value");
		CHECK(jvmr_execute_class_method(&klass, "nativeArrayCopy", "()I") == 0,
			"class method can call System.arraycopy");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 2,
			"native arraycopy transfers array elements");
		CHECK(jvmr_execute_class_method(&klass, "nativeClock", "()I") == 0,
			"class method can call System.currentTimeMillis");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"native clock returns a usable Java long value");
		CHECK(jvmr_execute_class_method(&klass, "platformValue", "()I") == 0,
			"platform bootstrap calls return host runtime information");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 3,
			"Java version and processor-count properties are available");
		CHECK(jvmr_execute_class_method(&klass, "threadValue", "()I") == 0,
			"Thread compatibility methods execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 11,
			"Thread compatibility methods return expected values");
		CHECK(jvmr_execute_class_method(&klass, "coreNativeValue", "()I") == 0,
			"core Object and Class native methods execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"core Object monitor and assertion natives return expected values");
		CHECK(jvmr_execute_class_method(&klass, "numericNativeValue", "()I") == 0,
			"floating-point library natives execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 6,
			"floating-point conversion and Math natives return expected values");
		CHECK(jvmr_execute_class_method(&klass, "propertyValue", "()I") == 0,
			"system property overloads and environment lookup execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 7,
			"system property defaults and standard properties return expected values");
		CHECK(jvmr_execute_class_method(&klass, "platformLibraryValue", "()I") == 0,
			"platform library-name mapping executes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"platform library-name mapping returns a native library suffix");
		CHECK(jvmr_execute_class_method(&klass, "fileValue", "()I") == 0,
			"Java File filesystem methods execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 15,
			"Java File existence, type, and length checks return expected values");
		CHECK(jvmr_execute_class_method(&klass, "streamValue", "()I") == 0,
			"Java FileInputStream reads fixture bytes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"Java FileInputStream preserves byte order and close semantics");
		CHECK(jvmr_execute_class_method(&klass, "outputStreamValue", "()I") == 0,
			"Java FileOutputStream writes fixture data");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"Java FileOutputStream round-trips bytes through the host filesystem");
		CHECK(jvmr_execute_class_method(&klass, "nioValue", "()I") == 0,
			"Java NIO path and file methods execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 7,
			"Java NIO existence, read, and size operations return expected values");
		CHECK(jvmr_execute_class_method(&klass, "nioExtendedValue", "()I") == 0,
			"extended Java NIO file operations execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 7,
			"Java NIO type checks and readString return expected values");
		CHECK(jvmr_execute_class_method(&klass, "nioWriteValue", "()I") == 0,
			"Java NIO directory and write methods execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"Java NIO writeString and readString round-trip host data");
		CHECK(jvmr_execute_class_method(&klass, "zipValue", "()I") == 0,
			"Java ZipFile and ZipEntry methods execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"Java ZipFile streams return archive entry bytes");
		CHECK(jvmr_execute_class_method(&klass, "resourceValue", "()I") == 0,
			"Class resource streams execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"Class resource streams return classpath bytes");
		CHECK(jvmr_execute_class_method(&klass, "stringExtendedValue", "()I") == 0,
			"extended String operations execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"String search, replacement, trimming, concatenation, and conversion work");
		CHECK(jvmr_execute_class_method(&klass, "classLoaderValue", "()I") == 0,
			"context ClassLoader lookup executes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"context ClassLoader resolves an application class");
		CHECK(jvmr_execute_class_method(&klass, "pathValue", "()I") == 0,
			"NIO Path composition and inspection execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"NIO Path resolve, filename, parent, and equality work");
		CHECK(jvmr_execute_class_method(&klass, "nioStreamValue", "()I") == 0,
			"NIO stream factories execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"NIO input and output streams round-trip host bytes");
		CHECK(jvmr_execute_class_method(&klass, "directoryValue", "()I") == 0,
			"NIO directory streams execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"NIO directory iteration finds a classpath entry");
		CHECK(jvmr_execute_class_method(&klass, "identityValue", "()I") == 0,
			"identityHashCode executes for managed objects");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"identityHashCode returns a stable nonzero object identity");
		CHECK(jvmr_execute_class_method(&klass, "classValue", "()I") == 0,
			"class literals create managed Class mirrors");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 18,
			"Class name, assignability, and instance checks work");
		CHECK(jvmr_execute_class_method(&klass, "objectClassValue", "()I") == 0,
			"Object.getClass and Class superclass metadata execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 17,
			"managed reflection exposes object type metadata");
		CHECK(jvmr_execute_class_method(&klass, "forNameValue", "()I") == 0,
			"Class.forName resolves binary Java class names");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"Class.forName returns the canonical class mirror");
		CHECK(jvmr_execute_class_method(&klass, "memberReflectionValue", "()I") == 0,
			"Class member reflection returns managed Method and Field objects");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 3,
			"reflected members expose names and descriptors");
		CHECK(jvmr_execute_class_method(&klass, "methodLookupValue", "()I") == 0,
			"Class method lookup resolves a declared signature");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 0,
			"resolved methods expose their parameter count");
		CHECK(jvmr_execute_class_method(&klass, "fieldReflectionValue", "()I") == 0,
			"reflected fields support lookup and value access");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 11,
			"Field.getInt reads static managed values");
		CHECK(jvmr_execute_class_method(&klass, "reflectedInvokeValue", "()I") == 0,
			"Method.invoke executes an interpreted static method");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 42,
			"Method.invoke boxes primitive return values");
		CHECK(jvmr_execute_class_method(&klass, "reflectedInstanceInvokeValue", "()I") == 0,
			"Method.invoke passes receivers to instance methods");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 37,
			"reflective instance invocation preserves object state");
		int argument_array=jvmr_heap_new_array(JVMR_HEAP_REF_ARRAY,3);
		uint64_t method_arguments[1]={(uint64_t)argument_array};
		CHECK(argument_array>0 && jvmr_execute_class_method_args(&klass,"argLength","([Ljava/lang/String;)I",method_arguments,1)==0,
			"argument-aware entry point invokes String[] methods");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 3,
			"reference-array method arguments are visible to Java code");
		CHECK(jvmr_execute_class_method(&klass, "catchesArithmetic", "()I") == 0,
			"arithmetic exceptions transfer to a Java catch handler");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 7,
			"exception tables select the matching handler");
		CHECK(jvmr_execute_class_method(&klass, "catchesRuntime", "()I") == 0,
			"exception matching walks superclass chains");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 8,
			"RuntimeException catches ArithmeticException");
		CHECK(jvmr_execute_class_method(&klass, "stringValue", "()I") == 0,
			"string constants and methods execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 106,
			"String length and charAt return Java values");
		CHECK(jvmr_execute_class_method(&klass, "stringEquals", "()I") == 0,
			"String equals executes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"equal string constants compare equal");
		CHECK(jvmr_execute_class_method(&klass, "stringOpsValue", "()I") == 0,
			"common String search and slicing methods execute");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 22,
			"String operations preserve Java text semantics");
		CHECK(jvmr_execute_class_method(&klass, "builderLength", "()I") == 0,
			"StringBuilder construction and chaining executes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 6,
			"StringBuilder append and toString preserve text");
		CHECK(jvmr_execute_class_method(&klass, "builderRangeValue", "()I") == 0,
			"StringBuilder range append executes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 1,
			"StringBuilder range append preserves the requested subsequence");
		CHECK(jvmr_execute_class_method(&klass, "concatValue", "()Ljava/lang/String;") == 0,
			"invokedynamic string concatenation executes");
		int32_t string_length=0;
		CHECK(current_frame.sp == 1 && jvmr_heap_string_length(current_frame.stack[0], &string_length) == 0 && string_length == 7,
			"StringConcatFactory recipe produces the expected string");
		CHECK(jvmr_execute_class_method(&klass, "interfaceValue", "()I") == 0,
			"invokeinterface selects a concrete receiver implementation");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 12,
			"interface dispatch returns the implementation result");
		CHECK(jvmr_execute_class_method(&klass, "inheritedValue", "()I") == 0,
			"virtual dispatch searches superclass methods");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 42,
			"inherited method execution returns its result");
		CHECK(jvmr_execute_class_method(&klass, "inheritedFieldValue", "()I") == 0,
			"inherited instance fields execute through a subclass object");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 12,
			"inherited instance fields use the correct object layout offset");
		CHECK(jvmr_execute_class_method(&klass, "constantStringLength", "()I") == 0,
			"static string fields execute through getstatic");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 8,
			"static strings are materialized on the managed heap");
		CHECK(jvmr_execute_class_method(&klass, "listValue", "()I") == 0,
			"Java 21 JMOD library classes execute through the runtime");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 10,
			"ArrayList construction, interface calls, and field-backed methods work");
		CHECK(jvmr_execute_class_method(&klass, "mapValue", "()I") == 0,
			"HashMap library bytecode executes through JMOD classes");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 40,
			"HashMap put/get uses reference keys and boxed values");
		CHECK(jvmr_execute_class_method(&klass, "lambdaValue", "()I") == 0,
			"invokedynamic creates a functional-interface lambda");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 7,
			"lambda implementation methods receive interface arguments");
		CHECK(jvmr_execute_class_method(&klass, "capturedLambdaValue", "()I") == 0,
			"invokedynamic captures enclosing values");
		CHECK(current_frame.sp == 1 && current_frame.stack[0] == 9,
			"captured lambda values are passed to the implementation method");
		jvmr_runtime_set_classloader(NULL);
		jvmr_classloader_destroy(jdk);
		jvmr_class_destroy(&klass);
	}
	free(data);
}

static void test_class_loader(void) {
	char error[160];
	JVMR_ClassLoader *loader=jvmr_classloader_create();
	CHECK(loader && jvmr_classloader_add_path(loader,"loader/build") == 0,"directory class path is accepted");
	const JVMR_Class *klass=loader?jvmr_classloader_load(loader,"Fixture",error,sizeof(error)):NULL;
	CHECK(klass!=NULL,"class loader finds a class in a directory");
	JVMR_ClassLoader *jar_loader=jvmr_classloader_create();
	CHECK(jar_loader && jvmr_classloader_add_path(jar_loader,"loader/build/fixture.jar") == 0,"JAR class path is accepted");
	const JVMR_Class *jar_klass=jar_loader?jvmr_classloader_load(jar_loader,"Fixture",error,sizeof(error)):NULL;
	CHECK(jar_klass!=NULL,"class loader extracts a class from a JAR");
	if (access("/usr/lib/jvm/java-21-openjdk-amd64/jmods", F_OK) == 0) {
		JVMR_ClassLoader *jdk=jvmr_classloader_create();
		CHECK(jdk && jvmr_classloader_add_java_runtime(jdk,"/usr/lib/jvm/java-21-openjdk-amd64")==0,
			"Java 21 jmod paths are accepted");
		const JVMR_Class *string_class=jdk?jvmr_classloader_load(jdk,"java/lang/String",error,sizeof(error)):NULL;
		CHECK(string_class!=NULL,"class loader finds java.base classes in a jmod");
		jvmr_classloader_destroy(jdk);
	}
	jvmr_classloader_destroy(jar_loader);
	jvmr_classloader_destroy(loader);
}

int main(int argc, char **argv) {
	CHECK(bytecode_init() == JVM_SUCCESS, "bytecode initialization succeeds");
	test_registration();
	test_every_handler_is_callable();
	test_constants();
	test_locals();
	test_stack();
	test_arithmetic();
	test_numeric();
	test_extended_instructions();
	test_control_flow();
	test_returns();
	if (argc > 1) test_class_file(argv[1]);
	if (argc > 1) test_class_loader();

	if (failures != 0) {
		fprintf(stderr, "%d loader test(s) failed.\n", failures);
		return 1;
	}
	puts("All loader instruction tests passed.");
	return 0;
}
