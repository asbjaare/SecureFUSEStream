# From the official optee_examples repository
# https://github.com/linaro-swg/optee_examples
CC      ?= $(CROSS_COMPILE)gcc
LD      ?= $(CROSS_COMPILE)ld
AR      ?= $(CROSS_COMPILE)ar
NM      ?= $(CROSS_COMPILE)nm
OBJCOPY ?= $(CROSS_COMPILE)objcopy
OBJDUMP ?= $(CROSS_COMPILE)objdump
READELF ?= $(CROSS_COMPILE)readelf

OBJS = main.o ../lib/bmp/bmp.o ../lib/libbmp/libbmp.o

CFLAGS += -Wall -I../ta/include -I./include
CFLAGS += -I$(TEEC_EXPORT)/include
CFLAGS += -I../lib/bmp
CFLAGS += -I../lib/libbmp
LDADD += -lteec -L$(TEEC_EXPORT)/lib

BINARY = video_tee

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDADD)

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
