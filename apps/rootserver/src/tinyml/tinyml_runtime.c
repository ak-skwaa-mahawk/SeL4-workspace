/*
 * tinyml_runtime.c - Freestanding, Capability-Isolated Inference Stub
 * No stdlib, no dynamic allocation, deterministic fixed-point Q16.16 math.
 */

#include <stdint.h>
#include <stddef.h>

#define INPUT_DIM   128
#define HIDDEN_DIM  16
#define OUTPUT_DIM  2

static const int16_t W1[HIDDEN_DIM][INPUT_DIM] = {
    #include "weights_w1.inc"
};
static const int16_t B1[HIDDEN_DIM] = {
    #include "biases_b1.inc"
};
static const int16_t W2[OUTPUT_DIM][HIDDEN_DIM] = {
    #include "weights_w2.inc"
};
static const int16_t B2[OUTPUT_DIM] = {
    #include "biases_b2.inc"
};

static inline int32_t relu(int32_t x) {
    return (x > 0) ? x : 0;
}

int tinyml_infer(const uint8_t input[INPUT_DIM], int32_t *classification, uint32_t *confidence_q16) {
    int32_t hidden[HIDDEN_DIM];
    int32_t output[OUTPUT_DIM];
    
    if (!input || !classification || !confidence_q16) {
        return -1;
    }

    for (size_t i = 0; i < HIDDEN_DIM; i++) {
        int32_t acc = ((int32_t)B1[i]) << 8;
        for (size_t j = 0; j < INPUT_DIM; j++) {
            acc += ((int32_t)(int8_t)input[j]) * ((int32_t)W1[i][j]);
        }
        hidden[i] = relu(acc >> 8);
    }

    for (size_t i = 0; i < OUTPUT_DIM; i++) {
        int32_t acc = ((int32_t)B2[i]) << 8;
        for (size_t j = 0; j < HIDDEN_DIM; j++) {
            acc += hidden[j] * ((int32_t)W2[i][j]);
        }
        output[i] = acc >> 8;
    }

    if (output[0] >= output[1]) {
        *classification = 0;
        int32_t diff = output[0] - output[1];
        *confidence_q16 = (diff > 65535) ? 65535 : (uint32_t)diff;
    } else {
        *classification = 1;
        int32_t diff = output[1] - output[0];
        *confidence_q16 = (diff > 65535) ? 65535 : (uint32_t)diff;
    }

    return 0;
}

int infer_run_inference(const uint8_t input_features[128], int32_t *classification, uint32_t *confidence_q16) {
    return tinyml_infer(input_features, classification, confidence_q16);
}
