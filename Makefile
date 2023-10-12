define \n


endef

CFLAGS := $(subst ${\n}, , $(file < compiler_flags.txt))
CC := clang

.PHONY: all
all: meows

.PHONY: run
run: meows
	./$^

meows: mws.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.o *.a *.out meows
