SOURCES = push-rand.cpp pop-rand.cpp status-rng.cpp common.cpp mkkey.cpp

CC = /usr/bin/gcc
CFLAGS = -I/usr/include/mysql -I. -c
LDFLAGS = -lmysqlclient -lnsl -lm -lz -lstdc++ -lgcc_s -lgcc -lssl
SHELL = /bin/sh

.SUFFIXES: .cpp .o

all:  push-rand pop-rand status-rng mkkey

install:  push-rand pop-rand status-rng mkkey
	mv push-rand ../bin
	mv pop-rand ../bin
	mv status-rng ../bin
	mv mkkey ../bin
	
push-rand:  push-rand.o common.o
	$(CC) $(LDFLAGS) common.o push-rand.o -o $@

pop-rand:  pop-rand.o common.o
	$(CC) $(LDFLAGS) pop-rand.o common.o -o $@

status-rng:  status-rng.o common.o
	$(CC) $(LDFLAGS) status-rng.o common.o -o $@

mkkey:  mkkey.o common.o
	$(CC) $(LDFLAGS) mkkey.o common.o -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f push-rand pop-rand status-rng mkkey *.o *~

