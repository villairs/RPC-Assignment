all: lib binder

lib: common.o rpc_server.o rpc_client.o
	ar rc librpc.a common.o rpc_server.o rpc_client.o
	cp -p librpc.a ../test/

binder: common.o rpc_binder.o
	g++ common.o rpc_binder.o -o binder

.PHONY: clean

clean:
	rm -f *.o *.a binder
