#include <nvapi.h>
#include <stddef.h>
#include <stdio.h>

#include "nvapi.h"
#include "utils.h"

/***** ***** ***** ***** ***** TYPES ***** ***** ***** ***** *****/

typedef void * (*nvapi_QueryInterface_t)(int);

typedef NvAPI_Status (*NvAPI_EnumPhysicalGPUs_t)(NvPhysicalGpuHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *);
typedef NvAPI_Status (*NvAPI_GPU_SetForcePstate_t)(NvPhysicalGpuHandle, NvU32, NvU32);
typedef NvAPI_Status (*NvAPI_GetErrorMessage_t)(NvAPI_Status, NvAPI_ShortString);
typedef NvAPI_Status (*NvAPI_Initialize_t)();
typedef NvAPI_Status (*NvAPI_Unload_t)();

/***** ***** ***** ***** ***** VARIABLES ***** ***** ***** ***** *****/

static void * lib;

static NvAPI_EnumPhysicalGPUs_t   _NvAPI_EnumPhysicalGPUs;
static NvAPI_GPU_SetForcePstate_t _NvAPI_GPU_SetForcePstate;
static NvAPI_GetErrorMessage_t    _NvAPI_GetErrorMessage;
static NvAPI_Initialize_t         _NvAPI_Initialize;
static NvAPI_Unload_t             _NvAPI_Unload;

/***** ***** ***** ***** ***** MACROS ***** ***** ***** ***** *****/

#define NVAPI_POINTER(pointer) do {   \
  if (pointer == NULL) {              \
    return NVAPI_API_NOT_INITIALIZED; \
  }                                   \
} while(0)

/***** ***** ***** ***** ***** IMPLEMENTATION ***** ***** ***** ***** *****/

NvAPI_Status NvAPI_EnumPhysicalGPUs(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount) {
  // Ensure the function pointer is valid
  NVAPI_POINTER(_NvAPI_EnumPhysicalGPUs);

  // Invoke the function using the provided parameters
  return _NvAPI_EnumPhysicalGPUs(nvGPUHandle, pGpuCount);
}

NvAPI_Status NvAPI_GPU_SetForcePstate(NvPhysicalGpuHandle hPhysicalGpu, NvU32 pstateId, NvU32 fallbackState) {
  // Ensure the function pointer is valid
  NVAPI_POINTER(_NvAPI_GPU_SetForcePstate);

  // Invoke the function using the provided parameters
  return _NvAPI_GPU_SetForcePstate(hPhysicalGpu, pstateId, fallbackState);
}

NvAPI_Status NvAPI_GetErrorMessage(NvAPI_Status nr, NvAPI_ShortString szDesc) {
  // Ensure the function pointer is valid
  NVAPI_POINTER(_NvAPI_GetErrorMessage);

  // Invoke the function using the provided parameters
  return _NvAPI_GetErrorMessage(nr, szDesc);
}

NvAPI_Status NvAPI_Initialize() {
  // Load the appropriate NvAPI library
  #ifdef _WIN32
    if (!lib) {
      lib = library_open("nvapi64.dll");
    }

    if (!lib) {
      lib = library_open("nvapi.dll");
    }
  #elif __linux__
    if (!lib) {
      lib = library_open("libnvidia-api.so.1");
    }

    if (!lib) {
      lib = library_open("libnvidia-api.so");
    }
  #endif

  // If the library handle is still not initialized, loading the library failed
  if (!lib) {
    // Print an error message indicating failure to load the NvAPI library
    fprintf(stderr, "Unable to load NvAPI library\n");

    // Return an error status indicating that the library was not found
    return NVAPI_LIBRARY_NOT_FOUND;
  }

  // Declare a function pointer for nvapi_QueryInterface
  nvapi_QueryInterface_t nvapi_QueryInterface;

  // Get the address of the nvapi_QueryInterface function from the loaded library
  nvapi_QueryInterface = (nvapi_QueryInterface_t) library_proc(lib, "nvapi_QueryInterface");

  // If the function pointer is still null, gathering the address failed
  if (!nvapi_QueryInterface) {
    // Print an error message indicating failure to gather the function address
    fprintf(stderr, "Unable to retrieve nvapi_QueryInterface function\n");

    // Return an error status indicating that the library was not found
    return NVAPI_LIBRARY_NOT_FOUND;
  }

  // Retrieve the addresses of specific NvAPI functions using nvapi_QueryInterface
  _NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t) nvapi_QueryInterface(0xe5ac921f);
  _NvAPI_GPU_SetForcePstate = (NvAPI_GPU_SetForcePstate_t) nvapi_QueryInterface(0x025bfb10);
  _NvAPI_GetErrorMessage = (NvAPI_GetErrorMessage_t) nvapi_QueryInterface(0x6c2d048c);
  _NvAPI_Initialize = (NvAPI_Initialize_t) nvapi_QueryInterface(0x0150e828);
  _NvAPI_Unload = (NvAPI_Unload_t) nvapi_QueryInterface(0xd22bdd7e);

  // Ensure the function pointer is valid
  NVAPI_POINTER(_NvAPI_Initialize);

  // Invoke the function using the provided parameters
  return _NvAPI_Initialize();
}

NvAPI_Status NvAPI_Unload() {
  // Ensure the function pointer is valid
  NVAPI_POINTER(_NvAPI_Unload);

  // Invoke the function using the provided parameters
  NvAPI_Status ret = _NvAPI_Unload();

  // If the function call was successful, proceed with cleanup
  if (ret == NVAPI_OK) {
    // If the library handle is initialized
    if (lib) {
      // Nullify all the function pointers to prevent further use
      _NvAPI_EnumPhysicalGPUs = NULL;
      _NvAPI_GPU_SetForcePstate = NULL;
      _NvAPI_GetErrorMessage = NULL;
      _NvAPI_Initialize = NULL;
      _NvAPI_Unload = NULL;

      // Free the loaded library
      library_close(lib);
    }
  }

  // Return the status of the function call
  return ret;
}
