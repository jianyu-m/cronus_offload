
#pragma once

#include <cublas_v2.h>

#ifndef __cplusplus
#define true 1
#define false 0
#define bool int
#endif

static inline bool isDevPtr(const void* dev_ptr) {
    if ((unsigned long)dev_ptr < 0xffffffff) {
        return true;
    }
    return false;
}

static inline int cublasHostOrDeviceGemmTypeSize(const void* par, size_t base_size, const void* dev_ptr, cublasComputeType_t compute_type, cudaDataType cType) {
    switch (compute_type) {
        case CUBLAS_COMPUTE_16F:
        case CUBLAS_COMPUTE_16F_PEDANTIC: (cType == CUDA_R_16F); return CUDA_R_16F;
        case CUBLAS_COMPUTE_32I:
        case CUBLAS_COMPUTE_32I_PEDANTIC: (cType == CUDA_R_32I); return CUDA_R_32I;
        case CUBLAS_COMPUTE_32F:
        case CUBLAS_COMPUTE_32F_PEDANTIC: 
            if (cType == CUDA_C_32F || cType == CUDA_C_32F) return CUDA_C_32F;
            return CUDA_R_32F; /* assert */
        case CUBLAS_COMPUTE_32F_FAST_16F:
        case CUBLAS_COMPUTE_32F_FAST_16BF:
        case CUBLAS_COMPUTE_32F_FAST_TF32:
            if (cType == CUDA_R_32F) return CUDA_R_32F;
            return CUDA_C_32F;
        case CUBLAS_COMPUTE_64F:
        case CUBLAS_COMPUTE_64F_PEDANTIC:
            if (cType == CUDA_R_64F) return CUDA_R_64F;
            return CUDA_C_64F;
        default: return 0;
    }
}

static inline int cublasHostOrDeviceSize(const void* par, size_t base_size, const void* dev_ptr) {
    return (isDevPtr(dev_ptr)) ? 0 : base_size;
}

static inline int cublasHostOrDeviceTypeSize(const void* par, size_t base_size, const void* dev_ptr, cudaDataType resultType) {
	switch (resultType) {
         case CUDA_R_16F:
        case CUDA_C_16F:
        case CUDA_R_16BF:
        case CUDA_C_16BF:   return 2;
        case CUDA_R_32F:
        case CUDA_C_32F:    return 4;
        case CUDA_R_64F:
        case CUDA_C_64F:    return 8;
        case CUDA_R_4I :
        case CUDA_C_4I :
        case CUDA_R_4U :
        case CUDA_C_4U :
        case CUDA_R_8I :
        case CUDA_C_8I :
        case CUDA_R_8U :
        case CUDA_C_8U :    return 1;
        case CUDA_R_16I:
        case CUDA_C_16I:
        case CUDA_R_16U:
        case CUDA_C_16U:    return 2;
        case CUDA_R_32I:
        case CUDA_C_32I:
        case CUDA_R_32U:
        case CUDA_C_32U:    return 4;
        case CUDA_R_64I:
        case CUDA_C_64I:
        case CUDA_R_64U:
        case CUDA_C_64U:    return 8;
        default:
            return 0;
    }
}