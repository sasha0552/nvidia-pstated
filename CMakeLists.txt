# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.17)

# Define the project name and programming language
project(nvidia-pstated C)

# Find the CUDAToolkit package
find_package(CUDAToolkit REQUIRED COMPONENTS nvml)

# Define the executable target
add_executable(nvidia-pstated
  src/main.c
  src/nvapi.c
)

# Include directories for the target
target_include_directories(nvidia-pstated SYSTEM PRIVATE
  nvapi
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
