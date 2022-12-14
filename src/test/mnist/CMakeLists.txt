cmake_minimum_required(VERSION 3.0)
project(cudaserver LANGUAGES C CXX CUDA)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xfatbin -compress-all")
# set(CMAKE_CUDA_ARCHITECTURES "75")

file(GLOB cpp_srcs
    "src/datasets/*.cpp"
    "src/layers/*.cpp"
    "src/loggers/*.cpp"
    "src/loss/*.cpp"
    "src/models/*.cpp"
    "src/optimizers/*.cpp"
    "src/tensor/*.cpp"
    "src/utils.cpp"
)

file(GLOB cu_srcs
    # "src/kernels.cu"
    "src/tensor/*.cu"
    "src/layers/*.cu"
    "src/loss/*.cu"
    "src/models/*.cu"
    "src/*.cu"
)

include_directories(./)
include_directories(/usr/local/cuda-11.4/include)

find_library(CUDART cudart /usr/local/cuda-11.4/targets/x86_64-linux/lib/)

add_library(mnist_kernel SHARED ${cpp_srcs} ${cu_srcs})

add_executable(mnist src/main.cpp)

target_link_libraries(mnist ${CUDART})
target_link_libraries(mnist dl)
target_link_libraries(mnist mnist_kernel)
target_link_libraries(mnist_kernel ${CUDART})

# add_executable(mnist_server ${cu_srcs})

# target_link_libraries(mnist_server ${CUDART})
# target_link_libraries(mnist_server dl)
# target_link_libraries(mnist_server cudaserver)

# add_executable(mnist_client ${cu_srcs} ${cpp_srcs})
# target_link_libraries(mnist_client cudart_rpc)

# add_executable(cuclient ${srcs})
