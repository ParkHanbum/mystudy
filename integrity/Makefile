ROOT := $(CURDIR)

override CXXFLAGS += -Iopenssl -Wl,$(ROOT)/openssl/libcrypto.a,$(ROOT)/openssl/libssl.a
override LDFLAGS += -lpthread -ldl

all: integrity

bench: main.c aes.c
	gcc $^ $(CXXFLAGS) -g -pg -DBENCH -o integrity $(LDFLAGS)

integrity: main.c aes.c
	gcc $^ $(CXXFLAGS) -o integrity $(LDFLAGS)

clean:
	rm integrity
