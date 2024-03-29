
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <cstring>
#include <cstdlib>

#include <cuda_runtime_api.h>

#include "cuda_runtime_t.h"
#include "cuda_runtime_header.h"
#include <unordered_map>

#include "cuda_offset.h"
#include "kernel_demangle.h"

std::unordered_map<std::string, const char*> *cudaserver_kernel_functions;

typedef void (*cudaRegisterFunction_t)(
        void   **fatCubinHandle,
  const char    *hostFun,
        char    *deviceFun,
  const char    *deviceName,
        int      thread_limit,
        uint3   *tid,
        uint3   *bid,
        dim3    *bDim,
        dim3    *gDim,
        int     *wSize
);

 cudaRegisterFunction_t ___cudaRegisterFunction;

void cudaRegisterFuncPtrNames(const char* hostFun, char *deviceFun) {
	if (!cudaserver_kernel_functions) {
        cudaserver_kernel_functions = new std::unordered_map<std::string, const char*> ();
        ___cudaRegisterFunction = (cudaRegisterFunction_t) dlsym(RTLD_NEXT, "__cudaRegisterFunction");
    }
    (*cudaserver_kernel_functions)[std::string(deviceFun)] = hostFun;
}

extern "C" {
extern void CUDARTAPI __cudaRegisterFunction(
        void   **fatCubinHandle,
  const char    *hostFun,
        char    *deviceFun,
  const char    *deviceName,
        int      thread_limit,
        uint3   *tid,
        uint3   *bid,
        dim3    *bDim,
        dim3    *gDim,
        int     *wSize
) {
    cudaRegisterFuncPtrNames(hostFun, deviceFun);
    ___cudaRegisterFunction(fatCubinHandle, hostFun, deviceFun, deviceName, thread_limit, tid, bid, bDim, gDim, wSize);
}
};

static const char* resolve_kernel_function(char* funcname) {
    void* func_ptr = NULL;
    void* handle = dlopen(NULL, RTLD_LAZY);
    func_ptr = dlsym(NULL, funcname);
    dlclose(handle);
    if (func_ptr) {
        return (const char*) func_ptr;
    }

    auto found_item = cudaserver_kernel_functions->find(std::string(funcname));
    if (found_item == cudaserver_kernel_functions->end()) {
        return NULL;
    }
    return found_item->second;
}

cudaError_t cudaLaunchKernelByName(char* funcname, dim3 gridDim, dim3 blockDim, 
    void* args_buf, int total_size, uint32_t* parameters, int par_len, size_t sharedMem, cudaStream_t stream) {
    const char* func_ptr = resolve_kernel_function(funcname);

    if (!func_ptr) {
        cudaserver_log_err("cannot find kernel %s\n", funcname);
        return cudaErrorLaunchFailure;
    }

    auto par_types = kernel_name_parameter(funcname);
    cudaserver_log_info("par types: %s", par_types);

    int parameter_len = (int)(par_len / sizeof(uint32_t));
    void** args = (void**) malloc(parameter_len * sizeof(void*));
    int total_offset = 0;
    for (int i = 0;i < parameter_len;i++) {
        args[i] = (void*)((char*)args_buf + total_offset);
        total_offset += parameters[i];
        if (par_types[i] == '*') {
            auto val_ptr = (unsigned long*)(args[i]);
            auto orig_val = *val_ptr;
            devOffsetToPtr((void**)args[i]);
            cudaserver_log_info("transform %d (%lx -> %lx)", i, orig_val, *val_ptr);
        } else if (par_types[i] == 'L') {
            // lambda function
            cudaserver_log_info("transform lambda %d", i);
            auto lambda_pars = (unsigned long*)(args[i]);
            for (int j = 0;j < parameters[i] / sizeof(void*);j++) {
                if (isDevOffset(lambda_pars[j])) {
                    devOffsetToPtr(&lambda_pars[j]);
                }
            }
        } else {
            cudaserver_log_info("not transform %d %d", i, parameters[i]);
        }
    }

    auto pre_status = cudaDeviceSynchronize();
    if (pre_status != cudaSuccess) {
        cudaserver_log_err("error in previous kernel");
    }

    auto status = cudaLaunchKernel(func_ptr, gridDim, blockDim, args, sharedMem, stream);
    if (status != cudaSuccess) return status;
    return cudaDeviceSynchronize();
}