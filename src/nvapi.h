#pragma once

#include <stdio.h>
#include <string.h>

#include <nvapi.h>

/***** ***** ***** ***** ***** MACROS ***** ***** ***** ***** *****/

// Macro to simplify NVAPI function calls and handle errors
#define NVAPI_CALL(call, label) do {                                 \
  /* Evaluate the NVAPI function call and store the result */        \
  NvAPI_Status result = (call);                                      \
                                                                     \
  /* Check if the result indicates an error */                       \
  if (result != NVAPI_OK) {                                          \
    /* Prepare a buffer to hold the error message */                 \
    NvAPI_ShortString error;                                         \
                                                                     \
    /* Retrieve the error message associated with the result code */ \
    if (NvAPI_GetErrorMessage(result, error) != NVAPI_OK) {          \
      strcpy(error, "<NvAPI_GetErrorMessage() call failed>");        \
    }                                                                \
                                                                     \
    /* Print the error message to standard error */                  \
    fprintf(stderr, "%s: %s\n", #call, error);                       \
                                                                     \
    /* Jump to the specified label */                                \
    goto label;                                                      \
  }                                                                  \
} while (0)

/***** ***** ***** ***** ***** FUNCTIONS ***** ***** ***** ***** *****/

NvAPI_Status NvAPI_GPU_SetForcePstate(NvPhysicalGpuHandle hPhysicalGpu, NvU32 pstateId, NvU32 fallbackState);
