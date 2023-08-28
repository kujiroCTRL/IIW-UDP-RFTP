CC = gcc
CFLAGS = -Wall -Wextra

CLIENTDIR = ./client-side
SERVERDIR = ./server-side

DFLAGS = -DUDP_RFTP_DYN_TOUT

all : ./client ./server ./clientd ./serverd

./client : $(CLIENTDIR)/client.c $(CLIENTDIR)/defs-client.h $(CLIENTDIR)/defs.h
	$(CC) $< $(CFLAGS) -o $@

./server : $(SERVERDIR)/server.c $(SERVERDIR)/defs-server.h $(SERVERDIR)/defs.h
	$(CC) $< $(CFLAGS) -o $@

./clientd : $(CLIENTDIR)/client.c $(CLIENTDIR)/defs-client.h $(CLIENTDIR)/defs.h
	$(CC) $< $(CFLAGS) $(DFLAGS) -o $@

./serverd : $(SERVERDIR)/server.c $(SERVERDIR)/defs-server.h $(SERVERDIR)/defs.h
	$(CC) $< $(CFLAGS) $(DFLAGS) -o $@

clean :
	rm -rf ./client ./server ./clientd ./serverd
