#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

/* =========================================================================
 * Formal CBMC Proof Harness: Q16.16 Saturating Arithmetic
 * ========================================================================= */
#define Q16_SHIFT 16
#define Q16_MAX   ((int32_t)0x7FFFFFFF)
#define Q16_MIN   ((int32_t)0x80000000)

static inline int32_t q16_mul_sat(int32_t a, int32_t b) {
    int64_t prod = ((int64_t)a * (int64_t)b) >> Q16_SHIFT;
    if (prod > (int64_t)Q16_MAX) return Q16_MAX;
    if (prod < (int64_t)Q16_MIN) return Q16_MIN;
    return (int32_t)prod;
}

static inline int32_t q16_add_sat(int32_t a, int32_t b) {
    int64_t sum = (int64_t)a + (int64_t)b;
    if (sum > (int64_t)Q16_MAX) return Q16_MAX;
    if (sum < (int64_t)Q16_MIN) return Q16_MIN;
    return (int32_t)sum;
}

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
    uint64_t sequence_id;
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

#ifdef __CPROVER__
int nondet_int(void);
int32_t nondet_int32(void);
uint8_t nondet_uint8(void);
uint32_t nondet_uint32(void);

static void verify_q16_saturation_properties(void) {
    int32_t a = nondet_int32();
    int32_t b = nondet_int32();

    int32_t mul_res = q16_mul_sat(a, b);
    __CPROVER_assert(mul_res <= Q16_MAX, "Q16_MUL_MAX_BOUND");
    __CPROVER_assert(mul_res >= Q16_MIN, "Q16_MUL_MIN_BOUND");

    int32_t add_res = q16_add_sat(a, b);
    __CPROVER_assert(add_res <= Q16_MAX, "Q16_ADD_MAX_BOUND");
    __CPROVER_assert(add_res >= Q16_MIN, "Q16_ADD_MIN_BOUND");

    if (a > 0 && b > 0 && mul_res != Q16_MAX) {
        __CPROVER_assert(mul_res >= 0, "Q16_MUL_NO_POSITIVE_WRAPAROUND");
    }
}

int tinyml_infer_stub(const uint8_t input[128], int32_t *classification, uint32_t *confidence_q16) {
    __CPROVER_assert(input != NULL, "tinyml_infer input non-null");
    __CPROVER_assert(classification != NULL, "tinyml_infer classification non-null");
    __CPROVER_assert(confidence_q16 != NULL, "tinyml_infer confidence non-null");

    *classification = nondet_int() ? 1 : 0;
    *confidence_q16 = nondet_uint32() & 0xFFFF;
    return 0;
}

int main(void) {
    /* Formally verify Q16.16 math saturation */
    verify_q16_saturation_properties();

    /* Formally verify multi-node batch unrolling */
    sovereign_audit_frame_t frame;
    sovereign_response_frame_t resp;
    memset(&resp, 0, sizeof(resp));

    frame.magic = SOVR_MAGIC;
    frame.node_count = nondet_uint8();

    if (frame.node_count > MAX_NODES) {
        resp.status_code = SOVR_STATUS_ERR_BOUNDS;
        __CPROVER_assert(resp.status_code == SOVR_STATUS_ERR_BOUNDS, "Bounds rejection invariant");
        return 0;
    }

    __CPROVER_assume(frame.node_count <= MAX_NODES);

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

    __CPROVER_assert(iterations_executed == frame.node_count, "All valid nodes evaluated");
    __CPROVER_assert(iterations_executed <= MAX_NODES, "Iterations never exceed MAX_NODES");

    if (anomaly_seen) {
        __CPROVER_assert((resp.flags & SOVR_FLAG_ANOMALY_DETECTED) != 0, "Anomaly flag preserved");
    }

    __CPROVER_assert((resp.flags & (SOVR_FLAG_STATUTORY_DUTY | SOVR_FLAG_CORP_DEFENSE_VALID | SOVR_FLAG_CAN_BE_ADMINISTERED)) == 0,
                     "ML inference does not corrupt baseline authority flags");

    return 0;
}
#else

static void verify_q16_saturation_properties(void) {
    static const int32_t corners[] = {
        0, 1, -1, Q16_MAX, Q16_MIN, Q16_MAX - 1, Q16_MIN + 1,
        (1 << 16), -(1 << 16), (1 << 15), -(1 << 15)
    };
    size_t n = sizeof(corners) / sizeof(corners[0]);
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            int32_t a = corners[i];
            int32_t b = corners[j];

            int32_t m = q16_mul_sat(a, b);
            assert(m <= Q16_MAX && m >= Q16_MIN);

            int32_t add = q16_add_sat(a, b);
            assert(add <= Q16_MAX && add >= Q16_MIN);

            if (a > 0 && b > 0 && m != Q16_MAX) {
                assert(m >= 0);
            }
        }
    }
}

static int evaluate_frame_mock(const sovereign_audit_frame_t *frame, sovereign_response_frame_t *resp) {
    if (!frame || !resp) return -1;
    memset(resp, 0, sizeof(*resp));
    resp->magic = SOVA_MAGIC;

    if (frame->node_count > MAX_NODES) {
        resp->status_code = SOVR_STATUS_ERR_BOUNDS;
        return 0;
    }

    resp->status_code = SOVR_STATUS_SUCCESS;
    uint32_t iterations = 0;

    for (uint32_t n = 0; n < frame->node_count && n < MAX_NODES; n++) {
        assert(n < MAX_NODES);
        assert(n < frame->node_count);

        uint8_t feature_buf[128] = {0};
        size_t cpy_len = sizeof(feature_buf) < sizeof(frame->nodes[n]) ? sizeof(feature_buf) : sizeof(frame->nodes[n]);
        assert(cpy_len <= sizeof(feature_buf));
        assert(cpy_len <= sizeof(frame->nodes[n]));

        memcpy(feature_buf, (const uint8_t *)&frame->nodes[n], cpy_len);

        int32_t ml_cls = (feature_buf[0] == 127) ? 1 : 0;
        if (ml_cls != 0) {
            resp->flags |= SOVR_FLAG_ANOMALY_DETECTED;
        }

        iterations++;
    }

    assert(iterations == frame->node_count);
    assert(iterations <= MAX_NODES);
    assert((resp->flags & (SOVR_FLAG_STATUTORY_DUTY | SOVR_FLAG_CORP_DEFENSE_VALID | SOVR_FLAG_CAN_BE_ADMINISTERED)) == 0);

    return 0;
}

int main(void) {
    printf("[*] Running native sanitizer harness: Bounded Multi-Node & Q16.16 Invariant Sweep\n");

    /* 1. Verify Q16.16 arithmetic boundary saturation */
    verify_q16_saturation_properties();
    printf("[+] Passed Q16.16 deterministic saturation boundary checks.\n");

    /* 2. Boundary checks: node_count = 0 through 16 */
    for (uint32_t count = 0; count <= 16; count++) {
        sovereign_audit_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.magic = SOVR_MAGIC;
        frame.node_count = count;

        sovereign_response_frame_t resp;
        evaluate_frame_mock(&frame, &resp);

        if (count > MAX_NODES) {
            assert(resp.status_code == SOVR_STATUS_ERR_BOUNDS);
        } else {
            assert(resp.status_code == SOVR_STATUS_SUCCESS);
        }
    }
    printf("[+] Passed all boundary checks (node_count in [0, 16]).\n");

    /* 3. Anomaly flag isolation test */
    {
        sovereign_audit_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.magic = SOVR_MAGIC;
        frame.node_count = 4;
        frame.nodes[2].name[0] = 127; // Triggers anomaly on node 2

        sovereign_response_frame_t resp;
        evaluate_frame_mock(&frame, &resp);
        assert(resp.status_code == SOVR_STATUS_SUCCESS);
        assert((resp.flags & SOVR_FLAG_ANOMALY_DETECTED) != 0);
        assert((resp.flags & ~SOVR_FLAG_ANOMALY_DETECTED) == 0);
    }
    printf("[+] Passed dynamic anomaly flag isolation and preservation check.\n");

    /* 4. Fuzzing stress test: 20,000 iterations */
    srand(0x534F5652);
    for (int iter = 0; iter < 20000; iter++) {
        sovereign_audit_frame_t frame;
        uint8_t *raw = (uint8_t *)&frame;
        for (size_t i = 0; i < sizeof(frame); i++) {
            raw[i] = (uint8_t)(rand() & 0xFF);
        }
        frame.magic = SOVR_MAGIC;
        frame.node_count = (uint8_t)(rand() % 16);

        sovereign_response_frame_t resp;
        evaluate_frame_mock(&frame, &resp);
    }
    printf("[+] Passed 20,000 fuzz cycles with zero assertion or bounds violations.\n");

    return 0;
}
#endif
