# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.16)

# Include the ExternalProject module
include(ExternalProject)

# Define the project name and programming language
project(nvidia-pstated C)

# Download and set up the NvAPI
ExternalProject_Add(nvapi
  URL      https://download.nvidia.com/XFree86/nvapi-open-source-sdk/R555-OpenSource.tar
  URL_HASH SHA256=71339c274a6a633f19b6bd358c7f3045063c6bc106b7dc488aaa7360a6d2b9d7

  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/R555-OpenSource <INSTALL_DIR>/include
)

# Download and set up the NVML
ExternalProject_Add(nvml
  URL      https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-nvml-dev-12-6_12.6.68-1_amd64.deb
  URL_HASH SHA256=fda6d4fdf26e20db4ca4950489033f4c6747c7473db3f9dc0529d56f2cc237de

  CONFIGURE_COMMAND ""
  BUILD_COMMAND ${CMAKE_COMMAND} -E tar xf <SOURCE_DIR>/data.tar.xz
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <BINARY_DIR>/usr/local/cuda-12.6/targets/x86_64-linux/include <INSTALL_DIR>/include
)

# Define the executable target
add_executable(nvidia-pstated
  src/main.c
  src/nvapi.c
  src/nvml.c
  src/utils.c
)

# Set dependencies for the executable
add_dependencies(nvidia-pstated
  nvapi
  nvml
)

# Include directories for the target
target_include_directories(nvidia-pstated SYSTEM PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/nvapi-prefix/include
  ${CMAKE_CURRENT_BINARY_DIR}/nvml-prefix/include
)

# Conditional linking for Linux platform
if(UNIX AND NOT APPLE)
  target_link_libraries(nvidia-pstated PRIVATE -static dl)
endif()
