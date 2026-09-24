#ifndef TINYML_RUNTIME_H
#define TINYML_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

#define INPUT_DIM   128
#define HIDDEN_DIM  16
#define OUTPUT_DIM  2

int tinyml_infer(const uint8_t input[INPUT_DIM], int32_t *classification, uint32_t *confidence_q16);

#endif /* TINYML_RUNTIME_H */
