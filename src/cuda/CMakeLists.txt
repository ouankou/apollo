
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED QUIET COMPONENTS CUPTI)
    find_package(CUPTI REQUIRED)
    if(CUDAToolkit_FOUND)
        if(CUPTI_FOUND)
            message(STATUS "---- CUDA:  Found!")
            message(STATUS "---- CUDA:    CUDAToolkit_VERSION      = ${CUDAToolkit_VERSION}")
            message(STATUS "---- CUDA:    CUDAToolkit_INCLUDE_DIRS = ${CUDAToolkit_INCLUDE_DIRS}")
            message(STATUS "---- CUDA:    CUDAToolkit_LIBRARY_DIR  = ${CUDAToolkit_LIBRARY_DIR}")
            message(STATUS "---- CUDA:    CUPTI_INCLUDE_DIRS = ${CUPTI_INCLUDE_DIRS}")
            message(STATUS "---- CUDA:    CUPTI_LIBRARIES    = ${CUPTI_LIBRARIES}")
            message(STATUS "---- CUDA:    CUPTI_DEFINITIONS  = ${CUPTI_DEFINITIONS}")
        else()
            message(STATUS "---- CUDA:  ** ERROR ** CUDA compiler found, but NO CUPTI support detected.")
        endif(CUPTI_FOUND)
    endif(CUDAToolkit_FOUND)
else()
    message(STATUS "---- CUDA:  ** ERROR ** No CUDA support detected.")
endif(CMAKE_CUDA_COMPILER)

message(STATUS "---- apollo_cuda:  Adding CUDA async measurement library...")


include_directories(       "${APOLLO_DIR}/include")
include_directories(       "${CUDAToolkit_INCLUDE_DIRS}")
include_directories(       "${CUPTI_INCLUDE_DIRS}")

set_source_files_properties(CudaAsync.cpp PROPERTIES LANGUAGE CUDA)
add_library(                apollo_cuda CudaAsync.cpp)
set_target_properties(      apollo_cuda PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(      apollo_cuda PUBLIC CUDA::cupti)
target_link_libraries(      apollo_cuda PUBLIC apollo ${CUPTI_LIBRARIES})
install(TARGETS
    EXPORT apollo_cuda
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)

