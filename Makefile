define \n


endef

CFLAGS := $(subst ${\n}, , $(file < compiler_flags.txt))
CC := clang

.PHONY: all
all: demo.out

.PHONY: run
run: demo.out
	./$^

demo.out: mws.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.o *.a *.out
