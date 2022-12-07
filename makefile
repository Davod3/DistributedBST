# Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
#   - André Dias    , nº 55314
#   - David Pereira , nº 56361
#   - Miguel Cut    , nº 56339

SRC_dir := source
INCLUDE_dir := include
OBJ_dir = object
LIB_dir = lib

CC := gcc
LINKER := ld 
OBJETOS_SERVER := tree_server.o network_server.o message.o sdmessage.pb-c.o tree_skel.o data.o entry.o
OBJETOS_CLIENT := tree_client.o sdmessage.pb-c.o
CLIENT_LIB := network_client.o client_stub.o data.o entry.o message.o
OBJETOS := $(OBJETOS_CLIENT) $(OBJETOS_SERVER) $(CLIENT_LIB)

all: tree-client tree-server

client-lib.o: sdmessage.pb-c.o $(CLIENT_LIB)
	$(LINKER) -r $(addprefix $(OBJ_dir)/,$(CLIENT_LIB)) -o lib/client-lib.o

tree-client: client-lib.o client

tree-server: sdmessage.pb-c.o server 

server: $(OBJETOS_SERVER)
	$(CC) -D THREADED $(addprefix $(OBJ_dir)/,$(OBJETOS_SERVER)) $(OBJ_dir)/tree.o -g -o binary/tree-server -L/usr/lib -std=gnu99 -lprotobuf-c -pthread -lzookeeper_mt

client: $(OBJETOS_CLIENT)
	$(CC) -D THREADED $(addprefix $(OBJ_dir)/,$(OBJETOS_CLIENT)) $(LIB_dir)/client-lib.o -g -o binary/tree-client -L/usr/lib -std=gnu99 -lprotobuf-c -lzookeeper_mt

sdmessage.pb-c.o:
	protoc --c_out=. sdmessage.proto
	mv sdmessage.pb-c.c $(SRC_dir)
	mv sdmessage.pb-c.h $(INCLUDE_dir)
	$(CC) -I include -o $(OBJ_dir)/sdmessage.pb-c.o -c $(SRC_dir)/sdmessage.pb-c.c

%.o: source/%.c $($@)
	$(CC) -I $(INCLUDE_dir) -o $(OBJ_dir)/$@ -c $< -g

clean:
	rm -f $(addprefix $(OBJ_dir)/,$(OBJETOS))
	rm -f lib/*
	rm -f binary/*
	rm -f $(INCLUDE_dir)/sdmessage.pb-c.h
	rm -f $(SRC_dir)/sdmessage.pb-c.c
	
