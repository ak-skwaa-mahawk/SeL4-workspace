#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#define MAX_NODES 8
#define MAX_DOCKET_STR_LEN 48
#define SHA256_DIGEST_LENGTH 32

#define SOVR_MAGIC 0x534F5652
#define SOVA_MAGIC 0x534F5641
#define TITLE_ABORIGINAL_SOVEREIGN 1

#define SOVR_STATUS_SUCCESS 0x0000
#define SOVR_STATUS_ERR_BOUNDS 0xE003

#define SOVR_FLAG_STATUTORY_DUTY      (1 << 0)
#define SOVR_FLAG_CORP_DEFENSE_VALID  (1 << 1)
#define SOVR_FLAG_CAN_BE_ADMINISTERED (1 << 2)
#define SOVR_FLAG_ANOMALY_DETECTED    (1 << 3)

#pragma pack(push, 1)
typedef struct {
    char     name[32];
    uint16_t era_year;
    char     territorial_hub[30];
    uint32_t title_type;
} c_lineage_node_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t fiduciary_role;
    uint8_t  veteran_verified;
    uint8_t  node_count;
    uint8_t  reserved[6];
    char claimant[64];
    char dockets[3][MAX_DOCKET_STR_LEN];
    c_lineage_node_t nodes[MAX_NODES];
    uint8_t  computed_root_hash[SHA256_DIGEST_LENGTH];
    uint32_t statutory_duty;
    uint8_t  corporate_defense_valid;
    uint8_t  can_be_administered_away;
    uint8_t  status_padding[2];
} sovereign_audit_frame_t;

typedef struct {
    uint32_t magic;
    uint16_t status_code;
    uint16_t flags;
    uint8_t  root_hash[32];
} sovereign_response_frame_t;
#pragma pack(pop)

/* Model check stub for tinyml_infer bounded behavior */
int nondet_int(void);
uint8_t nondet_uint8(void);
uint32_t nondet_uint32(void);

int tinyml_infer_stub(const uint8_t input[128], int32_t *classification, uint32_t *confidence_q16) {
    __CPROVER_assert(input != NULL, "tinyml_infer input non-null");
    __CPROVER_assert(classification != NULL, "tinyml_infer classification non-null");
    __CPROVER_assert(confidence_q16 != NULL, "tinyml_infer confidence non-null");
    
    /* Symbolic non-deterministic outcome constrained to binary classification */
    *classification = nondet_int() ? 1 : 0;
    *confidence_q16 = nondet_uint32() & 0xFFFF;
    return 0;
}

int main(void) {
    sovereign_audit_frame_t frame;
    sovereign_response_frame_t resp;
    memset(&resp, 0, sizeof(resp));

    /* Initialize symbolic inputs */
    frame.magic = SOVR_MAGIC;
    frame.node_count = nondet_uint8();

    /* Microkernel Invariant Guard: Bound checking */
    if (frame.node_count > MAX_NODES) {
        resp.status_code = SOVR_STATUS_ERR_BOUNDS;
        __CPROVER_assert(resp.status_code == SOVR_STATUS_ERR_BOUNDS, "Bounds rejection invariant");
        return 0;
    }

    __CPROVER_assume(frame.node_count <= MAX_NODES);

    /* --- Bounded Multi-Node TinyML Inference Sweep Invariant --- */
    uint32_t iterations_executed = 0;
    int anomaly_seen = 0;

    for (uint32_t n = 0; n < frame.node_count && n < MAX_NODES; n++) {
        __CPROVER_assert(n < MAX_NODES, "Array index n strictly less than MAX_NODES");
        __CPROVER_assert(n < frame.node_count, "Array index n strictly less than node_count");

        int32_t ml_cls = 0;
        uint32_t ml_conf = 0;

        uint8_t feature_buf[128] = {0};
        size_t cpy_len = sizeof(feature_buf) < sizeof(frame.nodes[n]) ? sizeof(feature_buf) : sizeof(frame.nodes[n]);
        
        __CPROVER_assert(cpy_len <= sizeof(feature_buf), "Buffer copy does not overflow feature_buf");
        __CPROVER_assert(cpy_len <= sizeof(frame.nodes[n]), "Buffer copy does not overrun source node");

        memcpy(feature_buf, (const uint8_t *)&frame.nodes[n], cpy_len);

        tinyml_infer_stub(feature_buf, &ml_cls, &ml_conf);

        if (ml_cls != 0) {
            resp.flags |= SOVR_FLAG_ANOMALY_DETECTED;
            anomaly_seen = 1;
        }

        iterations_executed++;
    }

    /* Post-loop Invariant Assertions */
    __CPROVER_assert(iterations_executed == frame.node_count, "All valid nodes evaluated");
    __CPROVER_assert(iterations_executed <= MAX_NODES, "Iterations never exceed MAX_NODES");

    if (anomaly_seen) {
        __CPROVER_assert((resp.flags & SOVR_FLAG_ANOMALY_DETECTED) != 0, "Anomaly flag preserved");
    }

    /* Bit collision check: Anomaly flag must never contaminate contract bits */
    __CPROVER_assert((resp.flags & (SOVR_FLAG_STATUTORY_DUTY | SOVR_FLAG_CORP_DEFENSE_VALID | SOVR_FLAG_CAN_BE_ADMINISTERED)) == 0,
                     "ML inference does not corrupt baseline authority flags");

    return 0;
}
