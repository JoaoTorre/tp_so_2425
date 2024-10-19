CC=gcc

all: manager feed


#compilar manager
manager : manager.o utils.o
		$(CC) manager.o utils.o -o manager

manager.o : manager.c manager.h utils.h
		$(CC) manager.c -c

#compilar feed
feed : feed.o utils.o
		$(CC) feed.o utils.o -o feed

feed.o : feed.c feed.h utils.h
		$(CC) feed.c -c


#compilar utils
utils.o : utils.c
		$(CC) utils.c -c


limpa : 
		rm *.o