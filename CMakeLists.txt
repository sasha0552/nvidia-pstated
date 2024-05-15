cmake_minimum_required(VERSION 3.15)

project(nvidia-pstated C)

add_definitions(-D_POSIX_C_SOURCE)
set(CMAKE_C_STANDARD 99)

add_executable(nvidia-pstated
  src/main.c
  src/nvapi_linux.c
)

if(MSVC)
    target_compile_definitions(nvidia-pstated PRIVATE _CRT_SECURE_NO_WARNINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
endif()

target_include_directories(nvidia-pstated SYSTEM PRIVATE
  nvapi
)

if(LINUX)
  target_link_libraries(nvidia-pstated PRIVATE
    dl
  )
endif()
