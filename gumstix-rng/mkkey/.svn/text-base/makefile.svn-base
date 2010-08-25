SOURCES = mkkey.cpp common.cpp

CC = /usr/bin/gcc
CFLAGS = -I. -c
LDFLAGS = -lstdc++ -lgcc_s -lgcc -lssl
SHELL = /bin/sh

.SUFFIXES: .cpp .o

all:  mkkey

install:  mkkey
	mv mkkey ../bin

mkkey:  mkkey.o
	$(CC) $(LDFLAGS) $< -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f mkkey *.o *~

