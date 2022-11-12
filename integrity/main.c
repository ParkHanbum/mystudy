#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <dirent.h>
#include <sys/types.h>
#include <limits.h>
#include "aes.h"
#include "integrity.h"

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

void testINTEGRITY1()
{
    int i;
    for (i = 0; i < DEFINED_FILE_COUNT; i++) {
        struct hash_pair curr = hashes[i];
        print_hex(curr.namehash, SHA256_DIGEST_LENGTH);
        print_hex(curr.datahash, SHA256_DIGEST_LENGTH);
    }
}

void testINTEGRITY2()
{
  unsigned char namehash[SHA256_DIGEST_LENGTH] = {0,};
  unsigned char datahash[SHA256_DIGEST_LENGTH] = {0,};
  unsigned char path[PATH_MAX];
  int i;
  char *buf;
  FILE *fp;
  long size;

  DIR *dp;
  struct dirent *ep;
  dp = opendir("temp");
  while ((ep = readdir(dp)) != NULL) {
    if (ep->d_type == DT_REG) {
      snprintf(path, PATH_MAX, "temp/%s", ep->d_name);
      puts(path);

      fp = fopen(path, "r");
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      rewind(fp);

      buf = malloc(size);
      fread(buf, 1, size, fp);
      fclose(fp);

      SHA256((unsigned char *)&path, strlen(path), (unsigned char *)&namehash);
      SHA256((unsigned char *)buf, size, (unsigned char *)&datahash);
      free(buf);

      for (i = 0; i < DEFINED_FILE_COUNT; i++) {
        struct hash_pair curr = hashes[i];
        if (memcmp(namehash, curr.namehash, SHA256_DIGEST_LENGTH) == 0) {
          printf("hash found! : ");
          if (memcmp(datahash, curr.datahash, SHA256_DIGEST_LENGTH) == 0) {
            printf("correct!\n");
          } else {
            printf("hash tampering : ");
            print_hex(datahash, SHA256_DIGEST_LENGTH);
            print_hex(curr.datahash, SHA256_DIGEST_LENGTH);
          }
        }
      }
    }
  }
  closedir(dp);
}

int main(int argc, char** argv)
{
  testAES(argv[1]);
  testSHA256();
  testINTEGRITY1();
  testINTEGRITY2();
  return 0;
}