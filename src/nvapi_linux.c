#ifdef __linux__ 
#include <nvapi.h>

/*
#include <dlfcn.h>
#include <stdio.h>

int load_nvapi() {
  void * handle = dlopen("libnvidia-api.so.1", RTLD_LAZY);

  if (!handle) {
    fprintf(stderr, "Failed to load library: %s\n", dlerror());
    return 1;
  }
}
*/

NvAPI_Status NvAPI_EnumPhysicalGPUs(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount) {
  return NVAPI_OK;
}

NvAPI_Status NvAPI_GPU_GetDynamicPstatesInfoEx(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_DYNAMIC_PSTATES_INFO_EX *pDynamicPstatesInfoEx) {
  return NVAPI_OK;
}

NvAPI_Status NvAPI_Initialize() {
  return NVAPI_OK;
}

NvAPI_Status NvAPI_Unload() {
  return NVAPI_OK;
}
#endif
