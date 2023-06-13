CFLAGS ?= -O0 -ggdb3

%.o: %.c
	gcc -m32 $(CFLAGS) -c $< -o $@ -I ../ltc/headers

LTC_OBJS :=  ../ltc/linux_x86/libtomcrypt_debug.a \
			 ../ltc/linux_x86/libtommath.a

.PHONY: progs
progs: verify resign

verify: verify.o $(LTC_OBJS)
	gcc -m32 $^ -o $@

resign: resign.o $(LTC_OBJS)
	gcc -m32 $^ -o $@

clean:
	rm -f verify resign verify.o resign.o
