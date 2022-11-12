ROOT := $(CURDIR)

CXXFLAGS := -Iopenssl -Wl,$(ROOT)/openssl/libcrypto.a

all: integrity

integrity: main.c aes.c
	clang $^ $(CXXFLAGS) -o $@

clean:
	rm integrity
