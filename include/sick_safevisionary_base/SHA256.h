/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 */
#include <cstdint>
#ifdef __cplusplus
extern "C"
{
#endif
  typedef uint8_t ulong8;
  typedef uint32_t ulong32;
  typedef uint64_t ulong64;

  struct sha256_state
  {
    ulong64 length;
    ulong32 state[8], curlen;
    unsigned char buf[64];
  };

  /* error codes [will be expanded in future releases] */
  enum
  {
    CRYPT_OK = 0,          /* Result OK */
    CRYPT_FAIL_TESTVECTOR, /* Algorithm failed test vectors */
    CRYPT_INVALID_ARG,     /* Generic invalid argument */
    CRYPT_HASH_OVERFLOW    /* Hash applied to too many bits */
  };

  typedef union Hash_state
  {
    struct sha256_state sha256;
    void* data;
  } hash_state;

  int sha256_init(hash_state* md);
  int sha256_process(hash_state* md, const ulong8* in, ulong32 inlen);
  int sha256_done(hash_state* md, ulong8* hash);
#ifdef __cplusplus
}
#endif