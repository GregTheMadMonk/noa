include(CheckLanguage)
check_language(CUDA)
if (CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    if (NOT DEFINED CMAKE_CUDA_STANDARD)
        set(CMAKE_CUDA_STANDARD 17)
        set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    endif ()
    cuda_select_nvcc_arch_flags(DETECT_CUDA_ARCHITECTURES)
    list(APPEND CUDA_NVCC_FLAGS ${DETECT_CUDA_ARCHITECTURES})
endif ()