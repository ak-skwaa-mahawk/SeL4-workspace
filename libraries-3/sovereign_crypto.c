#include "sovereign_crypto.h"
#include <string.h>

static inline uint64_t rotr64(uint64_t x, unsigned int n) {
    return (x >> n) | (x << (64 - n));
}

#define Ch(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define Sigma0(x)    (rotr64(x, 28) ^ rotr64(x, 34) ^ rotr64(x, 39))
#define Sigma1(x)    (rotr64(x, 14) ^ rotr64(x, 18) ^ rotr64(x, 41))
#define sigma0(x)    (rotr64(x, 1)  ^ rotr64(x, 8)  ^ ((x) >> 7))
#define sigma1(x)    (rotr64(x, 19) ^ rotr64(x, 61) ^ ((x) >> 6))

static const uint64_t K512[80] = {
    0x428a2f98d728b000ULL, 0x7137449123ef6000ULL, 0xb5c0fbcfec4d3000ULL, 0xe9b5dba58189d000ULL,
    0x3956c25bf348a000ULL, 0x59f111f1b605c000ULL, 0x923f82a4af194000ULL, 0xab1c5ed5da6d8000ULL,
    0xd807aa98a3030000ULL, 0x12835b0145706000ULL, 0x243185be4ee4a000ULL, 0x550c7dc3d5ffa000ULL,
    0x72be5d74f27b8000ULL, 0x80deb1fe3b168000ULL, 0x9bdc06a725c70000ULL, 0xc19bf174cf692000ULL,
    0xe49b69c19ef14000ULL, 0xefbe4786384f2000ULL, 0x0fc19dc68b8cc000ULL, 0x240ca1cc77ac8000ULL,
    0x2de92c6f592b0000ULL, 0x4a7484aa6ea6c000ULL, 0x5cb0a9dcbd420000ULL, 0x76f988da83114000ULL,
    0x983e5152ee66c000ULL, 0xa831c66d2db40000ULL, 0xb00327c898fb0000ULL, 0xbf597fc7beef0000ULL,
    0xc6e00bf33da88000ULL, 0xd5a79147930a8000ULL, 0x06ca6351e0038000ULL, 0x142929670a0e4000ULL,
    0x27b70a8546d20000ULL, 0x2e1b21385c26c000ULL, 0x4d2c6dfc5ac40000ULL, 0x53380d139d958000ULL,
    0x650a73548baf4000ULL, 0x766a0abb3c778000ULL, 0x81c2c92e47ed8000ULL, 0x92722c8514820000ULL,
    0xa2bfe8a14cf0c000ULL, 0xa81a664bbc420000ULL, 0xc24b8b70d0f88000ULL, 0xc76c51a306548000ULL,
    0xd192e819d6ef4000ULL, 0xd699062455658000ULL, 0xf40e358557710000ULL, 0x106aa07032bbc000ULL,
    0x19a4c116b8d2c000ULL, 0x1e376c0851418000ULL, 0x2748774cdf8ec000ULL, 0x34b0bcb5e19b0000ULL,
    0x391c0cb3c5c94000ULL, 0x4ed8aa4ae3414000ULL, 0x5b9cca4f7763c000ULL, 0x682e6ff3d6b28000ULL,
    0x748f82ee5def8000ULL, 0x78a5636f43170000ULL, 0x84c87814a1f08000ULL, 0x8cc702081a640000ULL,
    0x90befffa23630000ULL, 0xa4506cebde828000ULL, 0xbef9a3f7b2c64000ULL, 0xc67178f2e3720000ULL,
    0xca273eceea264000ULL, 0xd186b8c721c08000ULL, 0xeada7dd6cde0c000ULL, 0xf57d4f7fee6e8000ULL,
    0x06f067aa72174000ULL, 0x0a637dc5a2c88000ULL, 0x113f9804bef8c000ULL, 0x1b710b35131c0000ULL,
    0x28db77f523044000ULL, 0x32caab7b40c70000ULL, 0x3c9ebe0a15c98000ULL, 0x431d67c49c0fc000ULL,
    0x4cc5d4becb3e0000ULL, 0x597f299cfc654000ULL, 0x5fcb6fab3ad6c000ULL, 0x6c44198c4a470000ULL
};

static void sha512_transform(sovereign_sha512_ctx_t *ctx, const uint8_t block[128]) {
    uint64_t W[80];
    uint64_t a, b, c, d, e, f, g, h;
    int t;

    for (t = 0; t < 16; t++) {
        W[t] = ((uint64_t)block[t * 8 + 0] << 56) |
               ((uint64_t)block[t * 8 + 1] << 48) |
               ((uint64_t)block[t * 8 + 2] << 40) |
               ((uint64_t)block[t * 8 + 3] << 32) |
               ((uint64_t)block[t * 8 + 4] << 24) |
               ((uint64_t)block[t * 8 + 5] << 16) |
               ((uint64_t)block[t * 8 + 6] << 8)  |
               ((uint64_t)block[t * 8 + 7]);
    }

    for (t = 16; t < 80; t++) {
        W[t] = sigma1(W[t - 2]) + W[t - 7] + sigma0(W[t - 15]) + W[t - 16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (t = 0; t < 80; t++) {
        uint64_t T1 = h + Sigma1(e) + Ch(e, f, g) + K512[t] + W[t];
        uint64_t T2 = Sigma0(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void sovereign_sha512_init(sovereign_sha512_ctx_t *ctx) {
    ctx->state[0] = 0x6a09e667f3bcc908ULL;
    ctx->state[1] = 0xbb67ae8584caa73bULL;
    ctx->state[2] = 0x3c6ef372fe94f82bULL;
    ctx->state[3] = 0xa54ff53a5f1d36f1ULL;
    ctx->state[4] = 0x510e527fade682d1ULL;
    ctx->state[5] = 0x9b05688c2b3e6c1fULL;
    ctx->state[6] = 0x1f83d9abfb41bd6bULL;
    ctx->state[7] = 0x5be0cd19137e2179ULL;
    ctx->count[0] = 0;
    ctx->count[1] = 0;
}

void sovereign_sha512_update(sovereign_sha512_ctx_t *ctx, const uint8_t *data, size_t len) {
    size_t buffer_idx = (size_t)(ctx->count[0] & 127);
    uint64_t bit_len = (uint64_t)len << 3;

    ctx->count[0] += len;
    if (ctx->count[0] < len) {
        ctx->count[1]++;
    }
    ctx->count[1] += ((uint64_t)len >> 61);

    if (buffer_idx > 0) {
        size_t left = 128 - buffer_idx;
        if (len >= left) {
            memcpy(&ctx->buffer[buffer_idx], data, left);
            sha512_transform(ctx, ctx->buffer);
            data += left;
            len -= left;
            buffer_idx = 0;
        } else {
            memcpy(&ctx->buffer[buffer_idx], data, len);
            return;
        }
    }

    while (len >= 128) {
        sha512_transform(ctx, data);
        data += 128;
        len -= 128;
    }

    if (len > 0) {
        memcpy(ctx->buffer, data, len);
    }
}

void sovereign_sha512_final(sovereign_sha512_ctx_t *ctx, uint8_t digest[64]) {
    size_t buffer_idx = (size_t)(ctx->count[0] & 127);
    ctx->buffer[buffer_idx++] = 0x80;

    if (buffer_idx > 112) {
        memset(&ctx->buffer[buffer_idx], 0, 128 - buffer_idx);
        sha512_transform(ctx, ctx->buffer);
        buffer_idx = 0;
    }

    memset(&ctx->buffer[buffer_idx], 0, 112 - buffer_idx);

    uint64_t high_bits = (ctx->count[1] << 3) | (ctx->count[0] >> 61);
    uint64_t low_bits  = (ctx->count[0] << 3);

    for (int i = 0; i < 8; i++) {
        ctx->buffer[112 + i] = (uint8_t)(high_bits >> ((7 - i) * 8));
        ctx->buffer[120 + i] = (uint8_t)(low_bits >> ((7 - i) * 8));
    }

    sha512_transform(ctx, ctx->buffer);

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            digest[i * 8 + j] = (uint8_t)(ctx->state[i] >> ((7 - j) * 8));
        }
    }
}

void sovereign_sha512(const uint8_t *data, size_t len, uint8_t digest[64]) {
    sovereign_sha512_ctx_t ctx;
    sovereign_sha512_init(&ctx);
    sovereign_sha512_update(&ctx, data, len);
    sovereign_sha512_final(&ctx, digest);
}

int sovereign_crypto_verify_32(const uint8_t a[32], const uint8_t b[32]) {
    uint8_t diff = 0;
    for (int i = 0; i < 32; i++) {
        diff |= (a[i] ^ b[i]);
    }
    return diff;
}
