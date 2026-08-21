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

// Macro to simplify NVML function calls that query the size of an array.
// Such calls report the number of required elements through their count
// argument, so a buffer that is too small is an expected result, not an error.
#define NVML_CALL_QUERY_SIZE(call, label) do {                   \
  /* Evaluate the NVML function call and store the result */     \
  nvmlReturn_t result = (call);                                  \
                                                                 \
  /* Check if the result indicates an error */                   \
  if (result != NVML_SUCCESS && result != NVML_ERROR_INSUFFICIENT_SIZE) { \
    /* Print the error message to standard error */              \
    fprintf(stderr, "%s: %s\n", #call, nvmlErrorString(result)); \
                                                                 \
    /* Jump to the specified label */                            \
    goto label;                                                  \
  }                                                              \
} while (0)
