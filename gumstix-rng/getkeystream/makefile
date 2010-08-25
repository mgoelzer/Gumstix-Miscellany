SOURCES = common.cpp getkeystream.cpp

CC = /usr/bin/gcc
CFLAGS = -I/usr/include/mysql -I. -c
LDFLAGS = -lmysqlclient -lnsl -lm -lz -lstdc++ -lgcc_s -lgcc -lssl
SHELL = /bin/sh

.SUFFIXES: .cpp .o

all: getkeystream

#install:  getkeystream
#	mv getkeystream ../bin
	
getkeystream:  getkeystream.o
        $(CC) $(LDFLAGS) $< -o $@

.cpp.o: 
        $(CC) $(CFLAGS) $< -o $@

clean:
	rm -f getkeystream *.o *~

