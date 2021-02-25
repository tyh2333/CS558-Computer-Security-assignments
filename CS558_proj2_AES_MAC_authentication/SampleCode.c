//+++++++++++++++++++++++++++++++++++++
/*  I assume that you get the error when you try to compile 
 * your code.
 *  You would need to specify the include path 
    (using option -I/usr/local/include/openssl)
 * and the library path (using option -L followed by the path
  where the library files are installed).
 * You also need to include -lcrypt to indicate that you need
to link the crypto library).*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <openssl/aes.h>
#include <openssl/modes.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>
#define u8 unsigned char
struct ctr_state
{
    unsigned char ivec[AES_BLOCK_SIZE]; unsigned int num;
    unsigned char ecount[AES_BLOCK_SIZE];
};
void init_ctr(struct ctr_state *state, const unsigned char iv[16])
{
    state->num = 0;
    memset(state->ecount, 0, 16);
    memcpy(state->ivec, iv, 16); }
void crypt_message(const u8* src, u8* dst, unsigned int src_len, const AES_KEY* key, const u8* iv)
{
    struct ctr_state state;
    init_ctr(&state, iv);
    CRYPTO_ctr128_encrypt(src, dst, src_len, key, state.ivec, state.ecount,
                          &state.num, (block128_f)AES_encrypt); 
}
int main()
{
    int len;
    char source[128];
    char cipher[128];
    char recovered[128];
    unsigned char iv[AES_BLOCK_SIZE];
    const unsigned char* enc_key = (const unsigned char*)"123456789abcdef0";
    if(!RAND_bytes(iv, AES_BLOCK_SIZE)) {

        fprintf(stderr, "Could not create random bytes.");
        exit(1); }
    AES_KEY key; AES_set_encrypt_key(enc_key, 128, &key);
    strcpy(source, "quick brown fox jumped over the lazy dog what."); len = strlen(source);
    memset(recovered, 0, sizeof(recovered));
    crypt_message((const u8*)source, (u8*)cipher, len, &key, iv); crypt_message((const u8*)cipher, (u8*)recovered, len, &key, iv); printf("Recovered text:%s\n", recovered);
    strcpy(source, "a dog he is idiot who is the genius.");
    len = strlen(source);
    memset(recovered, 0, sizeof(recovered));
    crypt_message((const u8*)source, (u8*)cipher, len, &key, iv); crypt_message((const u8*)cipher, (u8*)recovered, len, &key, iv); printf("Recovered text:%s\n", recovered);
    return 0; }
//++++++++++++++++++++++++++++++++