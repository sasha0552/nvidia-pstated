# nvidia-pstated

A daemon that automatically manages the performance states of NVIDIA GPUs.

## Installation

### Prerequirements

#### Linux

Make sure you have the proprietary NVIDIA driver and the packages providing `libnvidia-api.so.1` and `libnvidia-ml.so.1` installed.

- ArchLinux: `nvidia-utils`
- Debian: `libnvidia-api1` or `libnvidia-tesla-api1` (depending on the GPU and driver installed)

On Debian derivatives, you can use `apt search libnvidia-api.so.1` and `apt search libnvidia-ml.so.1` to find the package you need.

#### Windows

Make sure the NVIDIA driver is installed.

### Installation

Download the latest version of the executable for your OS from [releases](https://github.com/sasha0552/nvidia-pstated/releases).

## Building from source code

### Prerequirements

* CMake
* CUDA toolkit
* Wget (only on Linux)

### Linux

```sh
# Download NVAPI headers
./nvapi.sh

# Configure
cmake -B build

# Build
cmake --build build
```

### Windows

```sh
# Download NVAPI headers
.\nvapi.ps1

# Configure
cmake -B build

# Build
cmake --build build
```
