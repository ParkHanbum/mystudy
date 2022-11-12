ROOT := $(CURDIR)

override CXXFLAGS += -Iopenssl -Wl,$(ROOT)/openssl/libcrypto.a,$(ROOT)/openssl/libssl.a
override LDFLAGS += -lpthread -ldl

all: integrity

integrity: main.c aes.c
	clang $^ $(CXXFLAGS) -o $@ $(LDFLAGS)

clean:
	rm integrity
