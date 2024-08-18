#include <nvapi.h>
#include <nvml.h>

/***** ***** ***** ***** ***** CONSTANTS ***** ***** ***** ***** *****/

// Number of iterations to wait before switching states
#define ITERATIONS_BEFORE_SWITCH 30

// High performance state for the GPU
#define PERFORMANCE_STATE_HIGH 16

// Low performance state for the GPU
#define PERFORMANCE_STATE_LOW 8

// Sleep interval (in milliseconds) between utilization checks
#define SLEEP_INTERVAL 100

// Temperature threshold (in degrees C)
#define TEMPERATURE_THRESHOLD 80

/***** ***** ***** ***** ***** MACROS ***** ***** ***** ***** *****/

// Macro to check if the current argument matches the given string
#define IS_OPTION(arg) (strcmp(argv[i], (arg)) == 0)

// Macro to check if there is a next argument
#define HAS_NEXT_ARG (i + 1 < argc)

// Macro to check if a condition is true and jump to a label if it is not
#define IS_TRUE(call, label) do {       \
  /* Evaluate the condition */          \
  int result = (call);                  \
                                        \
  /* Check if the condition is false */ \
  if (!result) {                        \
    /* Jump to the specified label */   \
    goto label;                         \
  }                                     \
} while(0);

// Macro to check if a condition is false and jump to a label if it is not
#define IS_FALSE(call, label) do {     \
  /* Evaluate the condition */         \
  int result = (call);                 \
                                       \
  /* Check if the condition is true */ \
  if (result) {                        \
    /* Jump to the specified label */  \
    goto label;                        \
  }                                    \
} while(0);

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
} while(0)

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
} while(0)

/***** ***** ***** ***** ***** STRUCTURES ***** ***** ***** ***** *****/

// Structure to hold the state of each GPU
typedef struct {
  // Counter for iterations when in a specific state
  unsigned int iterations;

  // Current performance state of the GPU
  unsigned int pstateId;
} gpuState;
