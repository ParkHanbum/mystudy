#include <openssl/sha.h>

#if !defined(DEFINED_FILE_COUNT)
#error "DEFINED_FILE_COUNT MUST SPECIFIED"
#endif

#if !defined(BUCKET_COUNT)
#error "BUCKET_COUNT MUST SPECIFIED"
#endif

struct hash_pair {
  unsigned char namehash[SHA256_DIGEST_LENGTH];
  unsigned char datahash[SHA256_DIGEST_LENGTH];
};

__attribute__((section("myhashes"),used))
struct hash_pair hashes[DEFINED_FILE_COUNT];

struct index {
    int hash_idx;
    int count;
};

__attribute__((section("myhashes_index"),used))
struct index ind[BUCKET_COUNT];

