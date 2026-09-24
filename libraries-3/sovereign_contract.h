#pragma once

#include <stdint.h>
#include <sel4/sel4.h>

#define SOVR_MAGIC              0x534F5652  /* "SOVR" */
#define SOVA_MAGIC              0x534F5641  /* "SOVA" - Structured Ack */
#define SOVR_VERSION            1
#define SHA256_DIGEST_LENGTH    32
#define MAX_DOCKET_STR_LEN      48
#define MAX_NODES               8

/* IPC Command Codes */
#define OP_SOVR_EVALUATE        0x10
#define OP_SOVR_HASH            0x11

/* Status & Role Values */
#define TITLE_ABORIGINAL_SOVEREIGN 1
#define TITLE_ADMINISTRATIVE_CORP  2

#define ROLE_FIDUCIARY_PR          0xC001  /* Matches verified capability badge */
#define ROLE_PASSIVE_SHAREHOLDER   0x0000

/* Status / Error Codes */
#define SOVR_STATUS_SUCCESS         0x0000
#define SOVR_STATUS_ERR_UNAUTH      0xE001
#define SOVR_STATUS_ERR_MAGIC       0xE002
#define SOVR_STATUS_ERR_BOUNDS      0xE003

/* Flags Bitmask */
#define SOVR_FLAG_STATUTORY_DUTY         (1 << 0)
#define SOVR_FLAG_CORP_DEFENSE_VALID     (1 << 1)
#define SOVR_FLAG_CAN_BE_ADMINISTERED    (1 << 2)
#define SOVR_FLAG_ANOMALY_DETECTED       (1 << 3)

#pragma pack(push, 1)

typedef struct {
    char     name[32];
    uint16_t era_year;
    char     territorial_hub[30];
    uint32_t title_type;
} c_lineage_node_t;

typedef struct {
    /* Header & Authority */
    uint32_t magic;
    uint16_t version;
    uint16_t fiduciary_role;
    uint8_t  veteran_verified;
    uint8_t  node_count;
    uint8_t  reserved[6];

    /* Identity & Docket Data */
    char claimant[64];
    char dockets[3][MAX_DOCKET_STR_LEN];

    /* Lineage Graph */
    c_lineage_node_t nodes[MAX_NODES];

    /* Evaluation Output */
    uint8_t  computed_root_hash[SHA256_DIGEST_LENGTH];
    uint32_t statutory_duty;            /* 1 = MANDATORY_ACCOUNTING, 0 = DEFER */
    uint8_t  corporate_defense_valid;   /* 0 = false, 1 = true */
    uint8_t  can_be_administered_away;  /* 0 = false, 1 = true */
    uint8_t  status_padding[2];
} sovereign_audit_frame_t;

/* 40-byte Structured Response Frame */
typedef struct {
    uint32_t magic;                      /* 0x534F5641 ("SOVA") */
    uint16_t status_code;                /* SOVR_STATUS_* */
    uint16_t flags;                      /* Evaluation bitmask */
    uint8_t  root_hash[32];              /* SHA-256 Digest */
} sovereign_response_frame_t;

#pragma pack(pop)

_Static_assert(sizeof(sovereign_audit_frame_t) == 808, "sovereign_audit_frame_t size must exactly match 808 bytes");
_Static_assert(sizeof(sovereign_response_frame_t) == 40, "sovereign_response_frame_t size must exactly match 40 bytes");
