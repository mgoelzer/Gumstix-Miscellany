SOURCES = push-rand.cpp pop-rand.cpp status-rng.cpp hourly.cpp common.cpp

CC = /usr/bin/gcc
CFLAGS = -I/usr/include/mysql -I. -c
LDFLAGS = -lmysqlclient -lnsl -lm -lz -lstdc++ -lgcc_s -lgcc
SHELL = /bin/sh

.SUFFIXES: .cpp .o

all:  push-rand pop-rand status-rng hourly

install:  push-rand pop-rand status-rng hourly
	mv push-rand ../bin
	mv pop-rand ../bin
	mv status-rng ../bin
	mv hourly ../bin

hourly:  hourly.o common.o
	$(CC) $(LDFLAGS) hourly.o common.o -o $@

push-rand:  push-rand.o common.o
	$(CC) $(LDFLAGS) common.o push-rand.o -o $@

pop-rand:  pop-rand.o common.o
	$(CC) $(LDFLAGS) pop-rand.o common.o -o $@

status-rng:  status-rng.o common.o
	$(CC) $(LDFLAGS) status-rng.o common.o -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f push-rand pop-rand hourly status-rng *.o *~

