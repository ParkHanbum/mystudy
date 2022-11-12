#include <openssl/sha.h>

#if !defined(DEFINED_FILE_COUNT)
#error "DEFINED_FILE_COUNT MUST SPECIFIED"
#endif

struct hash_pair {
  unsigned char namehash[SHA256_DIGEST_LENGTH];
  unsigned char datahash[SHA256_DIGEST_LENGTH];
};

__attribute__((section("myhashes"),used))
struct hash_pair hashes[DEFINED_FILE_COUNT];

struct index {
    int key;
    int collision;
};

__attribute__((section("myhashes_index"),used))
struct index ind[DEFINED_FILE_COUNT];

