#setting compiler
CC=gcc
#setting compiler flags
CFLAGS=-c -Wall -O2
#setting filename
FILE=prethreads_chatserver.c
OUTPUT=chat_server
LINKER=-lpthread
all: build

build: 
	$(CC) $(FILE) $(LINKER) -o $(OUTPUT)

clean:
	rm -rf *~ chat_server *.o

