# Variables del compilador
CC = gcc

CFLAGS = -Wall -Wextra -Werror -g -fPIC
RPCGEN_CFLAGS = -w -g -fPIC

CPPFLAGS = -I/usr/include/tirpc
LDFLAGS = -L. -Wl,-rpath=.
RPCLIBS = -ltirpc

.PHONY: all clean rpc

# Regla por defecto que compila todo
all: rpc libclaves.so libproxyclaves.so clavesRPC_server cliente

# 1. Generación automática de ficheros RPC a partir del .x
rpc: clavesRPC.h clavesRPC_clnt.c clavesRPC_svc.c clavesRPC_xdr.c clavesRPC_client.c clavesRPC_server.c

clavesRPC.h clavesRPC_clnt.c clavesRPC_svc.c clavesRPC_xdr.c clavesRPC_client.c clavesRPC_server.c: clavesRPC.x
	rm -f Makefile.clavesRPC clavesRPC.h clavesRPC_clnt.c clavesRPC_svc.c clavesRPC_xdr.c clavesRPC_client.c clavesRPC_server.c
	rpcgen -aNM clavesRPC.x

# 2. Creación de las bibliotecas dinámicas (.so)
libclaves.so: claves.o
	$(CC) -shared -o $@ $^ -lpthread

claves.o: claves.c claves.h
	$(CC) $(CFLAGS) -c $<

libproxyclaves.so: proxy-rpc.o clavesRPC_clnt.o clavesRPC_xdr.o
	$(CC) -shared -o $@ $^ $(RPCLIBS)

proxy-rpc.o: proxy-rpc.c claves.h clavesRPC.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

# 3. Compilación de los ficheros generados por rpcgen
clavesRPC_clnt.o: clavesRPC_clnt.c clavesRPC.h
	$(CC) $(CPPFLAGS) $(RPCGEN_CFLAGS) -c $<

clavesRPC_xdr.o: clavesRPC_xdr.c clavesRPC.h
	$(CC) $(CPPFLAGS) $(RPCGEN_CFLAGS) -c $<

clavesRPC_svc.o: clavesRPC_svc.c clavesRPC.h
	$(CC) $(CPPFLAGS) $(RPCGEN_CFLAGS) -c $<

rpc_service.o: rpc_service.c claves.h clavesRPC.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

# 4. Creación del ejecutable del servidor RPC
clavesRPC_server: clavesRPC_svc.o rpc_service.o clavesRPC_xdr.o libclaves.so
	$(CC) -o $@ clavesRPC_svc.o rpc_service.o clavesRPC_xdr.o $(LDFLAGS) -lclaves $(RPCLIBS) -lpthread

app-cliente.o: app-cliente.c claves.h
	$(CC) $(CFLAGS) -c $<

# 5. Creación del ejecutable del cliente de prueba
cliente: app-cliente.o libproxyclaves.so
	$(CC) -o $@ app-cliente.o $(LDFLAGS) -lproxyclaves $(RPCLIBS)

# 6. Limpieza de archivos compilados y generados automáticamente
clean:
	rm -f *.o *.so clavesRPC_server cliente clavesRPC.h clavesRPC_clnt.c clavesRPC_svc.c clavesRPC_xdr.c clavesRPC_client.c clavesRPC_server.c Makefile.clavesRPC