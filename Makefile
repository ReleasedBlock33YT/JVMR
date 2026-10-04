CC = gcc
CFLAGS = -Wall -Wextra
CLIBS = -lm -lz

LOADER_SRCDIR = loader/src
LOADER_RUNTIME_SRCS = $(filter-out $(LOADER_SRCDIR)/main.c,$(wildcard $(LOADER_SRCDIR)/*.c))

build_loader:
	mkdir -p loader/build
	$(CC) $(CFLAGS) -o loader/build/loader $(LOADER_SRCDIR)/*.c $(CLIBS)

test_loader:
	mkdir -p loader/build
	$(CC) $(CFLAGS) -I$(LOADER_SRCDIR) -o loader/build/test_loader loader/tests/test_loader.c $(LOADER_RUNTIME_SRCS) $(CLIBS)
	javac --release 21 -d loader/build loader/tests/Fixture.java loader/tests/Other.java loader/tests/Adder.java loader/tests/Impl.java loader/tests/Base.java loader/tests/Derived.java
	jar --create --file loader/build/fixture.jar -C loader/build Fixture.class -C loader/build Other.class -C loader/build Adder.class -C loader/build Impl.class -C loader/build Base.class -C loader/build Derived.class
	./loader/build/test_loader loader/build/Fixture.class
	$(MAKE) build_loader >/dev/null
	./loader/build/loader loader/build/Fixture.class caller '()I'
	./loader/build/loader loader/build/Fixture.class nativeMath '()I'
	./loader/build/loader loader/build/Fixture.class main '([Ljava/lang/String;)V' smoke test
	./loader/build/loader --cp loader/build Fixture main '([Ljava/lang/String;)V' smoke test
	./loader/build/loader --cp loader/build/fixture.jar Fixture caller '()I'

clean_loader:
	rm -rfv loader/build
