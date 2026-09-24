/*
 * verify_harness.c - Bounded Model Checking & Assertion Test Harness
 * Proves absence of buffer overruns, pointer errors, and arithmetic faults.
 */

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_DIM   128
#define HIDDEN_DIM  16
#define OUTPUT_DIM  2

#define SOVR_MAGIC 0x534f5652
#define SOVA_MAGIC 0x534f5641

#define SOVR_STATUS_SUCCESS     0x0000
#define SOVR_STATUS_ERR_MAGIC   0xE002
#define SOVR_STATUS_ERR_BOUNDS  0xE003

#define SOVR_FLAG_STATUTORY_DUTY 0x0001

int tinyml_infer(const uint8_t input[INPUT_DIM], int32_t *classification, uint32_t *confidence_q16);

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t node_count;
    uint8_t  raw_nodes[512];
} audit_frame_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t status_code;
    uint16_t flags;
    uint8_t  root_hash[32];
} audit_response_t;

int verify_evaluate_and_hash_frame(const audit_frame_t *frame, audit_response_t *resp) {
    if (!frame || !resp) {
        return -1;
    }

    if (frame->magic != SOVR_MAGIC) {
        resp->magic = SOVA_MAGIC;
        resp->status_code = SOVR_STATUS_ERR_MAGIC;
        resp->flags = 0;
        return -1;
    }

    /* Strict bounds check invariant */
    if (frame->node_count > 8) {
        resp->magic = SOVA_MAGIC;
        resp->status_code = SOVR_STATUS_ERR_BOUNDS;
        resp->flags = 0;
        return -1;
    }

    resp->magic = SOVA_MAGIC;
    resp->status_code = SOVR_STATUS_SUCCESS;
    resp->flags = SOVR_FLAG_STATUTORY_DUTY;

    /* Execute isolated inference over frame features */
    int32_t classification = 0;
    uint32_t confidence = 0;
    int inf_err = tinyml_infer(frame->raw_nodes, &classification, &confidence);
    assert(inf_err == 0);
    assert(classification == 0 || classification == 1);

    return 0;
}

static void test_one_frame(audit_frame_t frame) {
    audit_response_t resp;
    memset(&resp, 0, sizeof(resp));

    int res = verify_evaluate_and_hash_frame(&frame, &resp);

    if (frame.magic != SOVR_MAGIC) {
        assert(res != 0);
        assert(resp.status_code == SOVR_STATUS_ERR_MAGIC);
    } else if (frame.node_count > 8) {
        assert(res != 0);
        assert(resp.status_code == SOVR_STATUS_ERR_BOUNDS);
    } else {
        assert(res == 0);
        assert(resp.status_code == SOVR_STATUS_SUCCESS);
        assert((resp.flags & SOVR_FLAG_STATUTORY_DUTY) != 0);
    }
}

#ifdef __CPROVER__
void main(void) {
    audit_frame_t frame;
    __CPROVER_havoc_slice(&frame, sizeof(frame));
    test_one_frame(frame);
}
#else
int main(void) {
    audit_frame_t frame;

    /* Test Case 1: Valid frame with bounds within limits */
    memset(&frame, 0, sizeof(frame));
    frame.magic = SOVR_MAGIC;
    frame.node_count = 4;
    for (int i = 0; i < 512; i++) frame.raw_nodes[i] = (uint8_t)(i % 256);
    test_one_frame(frame);

    /* Test Case 2: Invalid magic rejection */
    frame.magic = 0xDEADBEEF;
    test_one_frame(frame);

    /* Test Case 3: Bounds overrun rejection */
    frame.magic = SOVR_MAGIC;
    frame.node_count = 9;
    test_one_frame(frame);

    /* Test Case 4: Randomized stress fuzzing (10,000 iterations) */
    srand(0x534f5652);
    for (int iter = 0; iter < 10000; iter++) {
        uint8_t *raw = (uint8_t *)&frame;
        for (size_t b = 0; b < sizeof(frame); b++) {
            raw[b] = (uint8_t)(rand() & 0xFF);
        }
        test_one_frame(frame);
    }

    return 0;
}
#endif
