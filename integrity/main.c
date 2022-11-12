#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include "aes.h"

void print_hexstr(const unsigned char *data, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        printf("%c%c ", data[i*2], data[i*2+1]);
    }

    printf("\n");
}

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

    in = (unsigned char *) argv;
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

char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

char *convert_chararr_to_hexarr(char *arr, int leng, unsigned char *out)
{
    int i, j;
    for (i = 0; i < leng; i++) {
      for (j = 0; j < 16; j++) {
        if (arr[i] == hex_chars[j]) {
          if (i % 2 == 0) {
            out[i/2] |= (j << 4);
          }
          else {
            out[i/2] |= (j);
          }
          break;
        }
      }
    }
    return out;
}

char test[] = "Hell World!";
char result[] = "d41dfe6726e9b099548518086d0ce9d39b8691317792d1eafd4aa9755462e3f5";
void testSHA256()
{
  unsigned char digest[SHA256_DIGEST_LENGTH] = {0,};
  unsigned char cmp[SHA256_DIGEST_LENGTH] = {0,};

  SHA256((unsigned char *)test, strlen(test), (unsigned char *)&digest);
  printf("TEST SHA256 \n");
  print_hex(digest, SHA256_DIGEST_LENGTH);
  convert_chararr_to_hexarr(result, strlen(result), &cmp);
  if (memcmp(cmp, digest, SHA256_DIGEST_LENGTH) == 0)
    printf("SHA256 AVAILIABLE\n");
}

int main(int argc, char** argv)
{
  testAES(argv[1]);
  testSHA256();

  return 0;
}