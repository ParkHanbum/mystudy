#include "aes.h"

AES_KEY enc_key, dec_key;

void enc(unsigned char *_in, unsigned char *_out, int length) 
{
  unsigned char iv[AES_BLOCK_SIZE];
  memcpy(iv, aes_iv, AES_BLOCK_SIZE);
  AES_set_encrypt_key(aes_key, sizeof(aes_key) * 8, &enc_key);
  AES_cbc_encrypt(_in, _out, length, &enc_key, iv, AES_ENCRYPT);
}

void dec(unsigned char *_in, unsigned char *_out, int length) 
{
  unsigned char iv[AES_BLOCK_SIZE];
  memcpy(iv, aes_iv, AES_BLOCK_SIZE);
  AES_set_decrypt_key(aes_key, sizeof(aes_key) * 8, &dec_key);
  AES_cbc_encrypt(_in, _out, length, &dec_key, iv, AES_DECRYPT);
}