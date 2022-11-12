#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include "aes.h"


void print_hex(const unsigned char *data, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }

    printf("\n");
}

void testAES(char *argv)
{
    unsigned char iv[AES_BLOCK_SIZE] = { 0, };
    unsigned char *_enc, *_dec, *in;
    int in_length;

    in = argv;
    in_length = strlen(in);
    in_length = ((in_length / AES_BLOCK_SIZE) * AES_BLOCK_SIZE) + AES_BLOCK_SIZE;

    _enc = (unsigned char *)malloc(in_length);
    _dec = (unsigned char *)malloc(in_length);

    print_hex(in, in_length);
    enc(in, _enc, in_length);
    print_hex(_enc, in_length);

    dec(in, _dec, in_length);
    print_hex(_dec, in_length);
}

int main(int argc, char** argv)
{
  testAES(argv[1]);
  return 0;
}