#pragma once

#include <stdio.h>

#include <nvml.h>

/***** ***** ***** ***** ***** MACROS ***** ***** ***** ***** *****/

// Macro to simplify NVML function calls and handle errors
#define NVML_CALL(call, label) do {                              \
  /* Evaluate the NVML function call and store the result */     \
  nvmlReturn_t result = (call);                                  \
                                                                 \
  /* Check if the result indicates an error */                   \
  if (result != NVML_SUCCESS) {                                  \
    /* Print the error message to standard error */              \
    fprintf(stderr, "%s: %s\n", #call, nvmlErrorString(result)); \
                                                                 \
    /* Jump to the specified label */                            \
    goto label;                                                  \
  }                                                              \
} while (0)
