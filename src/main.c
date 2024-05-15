#include <nvapi.h>
#include <nvml.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  int mret;
  int aret;
  NvPhysicalGpuHandle hPhysicalGpu[NVAPI_MAX_PHYSICAL_GPUS];
  NvU32 physicalGpuCount = 0;

  if ((mret = nvmlInit()) != NVML_SUCCESS) {
    printf("nvmlInit() failed = %s", nvmlErrorString(result));
    return 1;
  }

  if ((aret = NvAPI_Initialize()) != NVAPI_OK) {
    printf("NvAPI_Initialize() failed = 0x%x", ret);
    return 1;
  }

  if ((aret = NvAPI_EnumPhysicalGPUs(hPhysicalGpu, &physicalGpuCount)) != NVAPI_OK) {
    printf("NvAPI_EnumPhysicalGPUs() failed = 0x%x", ret);
    return 1;
  }

  while (1) {
    for (int i = 0; i < physicalGpuCount; i++) {
      if ((ret = NvAPI_GPU_GetDynamicPstatesInfoEx(hPhysicalGpu[i], &pDynamicPstatesInfoEx)) != NVAPI_OK) {
        printf("NvAPI_GPU_GetDynamicPstatesInfoEx() failed = 0x%x", ret);
        return 1;
      }

      pDynamicPstatesInfo->utilization[NVAPI_GPU_UTILIZATION_DOMAIN_GPU]
    }

    usleep(1000);
  }

  if ((aret = NvAPI_Unload()) != NVAPI_OK) {
    printf("NvAPI_Unload() failed = 0x%x", ret);
    return 1;
  }

  return 0;
}
