#ifndef SOVEREIGN_CRYPTO_H
#define SOVEREIGN_CRYPTO_H

#include <stdint.h>
#include <stddef.h>

/* SHA-512 State Context */
typedef struct {
    uint64_t state[8];
    uint64_t count[2];
    uint8_t  buffer[128];
} sovereign_sha512_ctx_t;

/* SHA-512 API */
void sovereign_sha512_init(sovereign_sha512_ctx_t *ctx);
void sovereign_sha512_update(sovereign_sha512_ctx_t *ctx, const uint8_t *data, size_t len);
void sovereign_sha512_final(sovereign_sha512_ctx_t *ctx, uint8_t digest[64]);
void sovereign_sha512(const uint8_t *data, size_t len, uint8_t digest[64]);

/* Ed25519 Detached Verification API */
int sovereign_ed25519_verify(
    const uint8_t signature[64],
    const uint8_t *message,
    size_t message_len,
    const uint8_t public_key[32]
);

/* Constant-Time Memory Utilities */
int sovereign_crypto_verify_32(const uint8_t a[32], const uint8_t b[32]);

#endif /* SOVEREIGN_CRYPTO_H */
