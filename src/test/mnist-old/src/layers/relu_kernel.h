

#pragma once

#include "cuda_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void kReLu_kernel(dim3 d1, dim3 d2, float *A, int aX, int aY, float* B);

#ifdef __cplusplus
}
#endif /* __cplusplus */