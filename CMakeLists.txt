# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.16)

# Include the FetchContent module
include(FetchContent)

# Define the project name and programming language
project(nvidia-pstated C)

# Find the CUDAToolkit package
find_package(CUDAToolkit REQUIRED COMPONENTS nvml)

# Declare the nvapi package
FetchContent_Declare(
  nvapi

  URL      https://download.nvidia.com/XFree86/nvapi-open-source-sdk/R555-OpenSource.tar
  URL_HASH SHA256=71339c274a6a633f19b6bd358c7f3045063c6bc106b7dc488aaa7360a6d2b9d7
)

# Download and make the nvapi content available for use
FetchContent_MakeAvailable(nvapi)

# Define the executable target
add_executable(nvidia-pstated
  src/main.c
  src/nvapi.c
  src/utils.c
)

# Include directories for the target
target_include_directories(nvidia-pstated SYSTEM PRIVATE
  ${nvapi_SOURCE_DIR}/R555-OpenSource
)

# Link libraries
target_link_libraries(nvidia-pstated PRIVATE
  CUDA::nvml
)

# Conditional linking for Linux platform
if(UNIX AND NOT APPLE)
  target_link_libraries(nvidia-pstated PRIVATE
    dl
  )
endif()
