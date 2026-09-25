#ifndef SOVEREIGN_CONTRACT_H
#define SOVEREIGN_CONTRACT_H

#include <stdint.h>
#include <stddef.h>

#define SOVR_MAGIC                  0x534F5652  /* 'SOVR' */
#define SOVA_MAGIC                  0x534F5641  /* 'SOVA' */

#define SOVR_VERSION                2
#define ROLE_FIDUCIARY_PR           0xC001
#define OP_SOVR_EVALUATE            1

#define MAX_NODES                   8
#define MAX_DOCKET_STR_LEN          32
#define MAX_QUORUM_SIGNERS          4
#define QUORUM_THRESHOLD            3

/* Status Codes */
#define SOVR_STATUS_SUCCESS         0x0000
#define SOVR_STATUS_ERR_MAGIC       0xE001
#define SOVR_STATUS_REJECT_REPLAY   0xE002
#define SOVR_STATUS_ERR_BOUNDS      0xE003
#define SOVR_STATUS_ERR_QUORUM      0xE004
#define SOVR_STATUS_ERR_UNAUTH      0xE005

/* Title Types */
#define TITLE_ABORIGINAL_SOVEREIGN  1
#define TITLE_ALLOTMENT_PATENT      2
#define TITLE_STATUTORY_HEIRSHIP    3

/* Response Authority Flags */
#define SOVR_FLAG_STATUTORY_DUTY     0x0001
#define SOVR_FLAG_CORP_DEFENSE_VALID 0x0002
#define SOVR_FLAG_CAN_BE_ADMINISTERED 0x0004
#define SOVR_FLAG_ANOMALY_DETECTED   0x0008
#define SOVR_FLAG_QUORUM_VERIFIED    0x0010

#pragma pack(push, 1)

/* 96-byte Cryptographic Witness Struct */
typedef struct {
    uint8_t signer_pubkey[32];   /* 32-byte public key */
    uint8_t signature[64];       /* 64-byte signature */
} sovereign_witness_t;

/* 64-byte Lineage Node with dual union access for domain & raw feature extraction */
typedef struct {
    union {
        struct {
            char     name[32];
            uint32_t era_year;
            char     territorial_hub[24];
            uint32_t title_type;
        };
        struct {
            uint32_t node_id;
            uint32_t flags;
            uint64_t timestamp;
            uint8_t  node_digest[32];
            uint8_t  reserved[16];
        };
        uint8_t raw[64];
    };
} c_lineage_node_t;

/* Sequence 28 Ingress Frame (1152 bytes) */
typedef struct {
    uint32_t magic;                     /* 0x000: 0x534F5652 (4B) */
    uint16_t version;                   /* 0x004: 2 (2B) */
    uint16_t fiduciary_role;            /* 0x006: 0xC001 (2B) */
    uint64_t sequence_id;               /* 0x008: Monotonic counter (8B) */
    uint8_t  veteran_verified;          /* 0x010: (1B) */
    uint8_t  statutory_duty;            /* 0x011: (1B) */
    uint8_t  corporate_defense_valid;   /* 0x012: (1B) */
    uint8_t  can_be_administered_away;  /* 0x013: (1B) */
    uint32_t node_count;                /* 0x014: 1..8 (4B) */
    char     claimant[64];              /* 0x018: (64B) */
    char     dockets[4][32];            /* 0x058: 4 * 32 = 128B */
    c_lineage_node_t nodes[MAX_NODES];  /* 0x0D8: 8 * 64 = 512B */
    uint8_t  computed_root_hash[32];    /* 0x2D8: (32B) -> Total base = 760B */

    /* Sequence 28 Quorum Extension */
    uint8_t  signer_bitmap;             /* 0x2F8: (1B) */
    uint8_t  quorum_count;              /* 0x2F9: (1B) */
    uint8_t  reserved_pad[6];           /* 0x2FA: (6B) -> Subtotal = 768B */
    sovereign_witness_t witnesses[MAX_QUORUM_SIGNERS]; /* 0x300: 4 * 96 = 384B */
} sovereign_audit_frame_t;

/* Response Frame (40 bytes) */
typedef struct {
    uint32_t magic;                     /* 0x534F5641 */
    uint16_t status_code;               /* 0x0000, 0xE001..0xE005 */
    uint16_t flags;                     /* Bitmask */
    uint8_t  root_hash[32];             /* SHA-256 Digest */
} sovereign_response_frame_t;

#pragma pack(pop)

_Static_assert(sizeof(sovereign_witness_t) == 96, "sovereign_witness_t size must be 96 bytes");
_Static_assert(sizeof(c_lineage_node_t) == 64, "c_lineage_node_t size must be 64 bytes");
_Static_assert(sizeof(sovereign_audit_frame_t) == 1152, "sovereign_audit_frame_t size must be 1152 bytes");
_Static_assert(sizeof(sovereign_response_frame_t) == 40, "sovereign_response_frame_t size must be 40 bytes");

#endif /* SOVEREIGN_CONTRACT_H */
