#include <limits.h>
#include <nvapi.h>
#include <nvml.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
#elif __linux__
  #include <unistd.h>
#endif

#include "nvapi.h"
#include "utils.h"

/***** ***** ***** ***** ***** VARIABLES ***** ***** ***** ***** *****/

// Flag indicating whether the program should continue running
static volatile sig_atomic_t should_run = true;

// Flag indicating whether an error has occurred
static bool error_occurred = false;

// Flags to check initialization status of NVML and NVAPI libraries
static bool nvapiInitialized = false;
static bool nvmlInitialized = false;

// Variables to store device handles for all GPUs
static NvPhysicalGpuHandle nvapiDevices[NVAPI_MAX_PHYSICAL_GPUS];
static nvmlDevice_t nvmlDevices[NVAPI_MAX_PHYSICAL_GPUS];

// Variable to store the number of GPU devices
static unsigned int deviceCount;

// Variable to store GPU temperature
static unsigned int temperature;

// Variable to store GPU utilization information
static nvmlUtilization_t utilization;

// Variable to store GPU states
static gpuState gpuStates[NVAPI_MAX_PHYSICAL_GPUS];

/***** ***** ***** ***** ***** FUNCTIONS ***** ***** ***** ***** *****/

static bool parse_uint(const char *arg, unsigned int *value) {
  // Check if either the input argument or the output value pointer is NULL
  if (arg == NULL || value == NULL) {
    return false;
  }

  // Declare a pointer to track where strtoll stops parsing
  char *endptr;

  // Convert the string to a long integer using base 10
  long long int result = strtoll(arg, &endptr, 10);

  // Check if the entire string was not consumed or if no digits were found
  if (*endptr != '\0' || endptr == arg) {
    return false;
  }

  // Check if the result is within the range of an unsigned int
  if (result < 0 || result > UINT_MAX) {
    return false;
  }

  // Assign the result to the output value
  *value = (unsigned int) result;

  // Conversion successful and the input was valid
  return true;
}

static void handle_exit(int signal) {
  // Check if the received signal is SIGINT or SIGTERM
  if (signal == SIGINT || signal == SIGTERM) {
    // Set the global flag to false to indicate the program should stop running
    should_run = false;
  }
}

static bool enter_pstate(unsigned int i, unsigned int pstateId) {
  // Get the current state of the GPU
  gpuState * state = &gpuStates[i];

  // Set the GPU to the desired performance state
  NVAPI_CALL(NvAPI_GPU_SetForcePstate(nvapiDevices[i], pstateId, 2), failure);

  // Reset the iteration counter
  state->iterations = 0;

  // Update the GPU state with the new performance state
  state->pstateId = pstateId;

  // Print the current GPU state
  printf("GPU %d entered performance state %d\n", i, state->pstateId);

  // Return true to indicate success
  return true;

  failure:
  // Return false to indicate failure
  return false;
}

int main(int argc, char *argv[]) {
  /***** OPTIONS *****/
  unsigned int iterations_before_switch = ITERATIONS_BEFORE_SWITCH;
  unsigned int performance_state_high = PERFORMANCE_STATE_HIGH;
  unsigned int performance_state_low = PERFORMANCE_STATE_LOW;
  unsigned int sleep_interval = SLEEP_INTERVAL;
  unsigned int temperature_threshold = TEMPERATURE_THRESHOLD;

  /***** OPTION PARSING *****/
  {
    // Iterate through command-line arguments
    for (unsigned int i = 1; i < argc; i++) {
      // Check if the option is "-ibs" or "--iterations-before-switch" and if there is a next argument
      if ((IS_OPTION("-ibs") || IS_OPTION("--iterations-before-switch")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in iterations_before_switch
        if (!parse_uint(argv[++i], &iterations_before_switch)) {
          goto usage;
        }
      }

      // Check if the option is "-psh" or "--performance-state-high" and if there is a next argument
      if ((IS_OPTION("-psh") || IS_OPTION("--performance-state-high")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in performance_state_high
        if (!parse_uint(argv[++i], &performance_state_high)) {
          goto usage;
        }
      }

      // Check if the option is "-psl" or "--performance-state-low" and if there is a next argument
      if ((IS_OPTION("-psl") || IS_OPTION("--performance-state-low")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in performance_state_low
        if (!parse_uint(argv[++i], &performance_state_low)) {
          goto usage;
        }
      }

      // Check if the option is "-si" or "--sleep-interval" and if there is a next argument
      if ((IS_OPTION("-si") || IS_OPTION("--sleep-interval")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in sleep_interval
        if (!parse_uint(argv[++i], &sleep_interval)) {
          goto usage;
        }
      }

      // Check if the option is "-tt" or "--temperature-threshold" and if there is a next argument
      if ((IS_OPTION("-tt") || IS_OPTION("--temperature-threshold")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in sleep_interval
        if (!parse_uint(argv[++i], &temperature_threshold)) {
          goto usage;
        }
      }
    }

    // Display usage instructions to the user
    if (false) {
      // Display usage instructions to the user
      usage:

      // Print the usage instructions
      printf("Usage: %s [options]\n", argv[0]);
      printf("\n");
      printf("Options:\n");
      printf("  -ibs, --iterations-before-switch <value>  Set the number of iterations to wait before switching states (default: %d)\n", ITERATIONS_BEFORE_SWITCH);
      printf("  -psh, --performance-state-high <value>    Set the high performance state for the GPU (default: %d)\n", PERFORMANCE_STATE_HIGH);
      printf("  -psl, --performance-state-low <value>     Set the low performance state for the GPU (default: %d)\n", PERFORMANCE_STATE_LOW);
      printf("  -si, --sleep-interval <value>             Set the sleep interval in milliseconds between utilization checks (default: %d)\n", SLEEP_INTERVAL);
      printf("  -tt, --temperature-threshold <value>      Set the temperature threshold in degrees C (default: %d)\n", TEMPERATURE_THRESHOLD);

      // Jump to the error handling code
      goto errored;
    }
  }

  /***** SIGNALS *****/
  {
    // Set up signal handling
    signal(SIGINT, handle_exit);
    signal(SIGTERM, handle_exit);
  }

  /***** NVAPI INIT *****/
  {
    // Initialize NVAPI library
    NVAPI_CALL(NvAPI_Initialize(), errored);

    // Mark NVAPI as initialized
    nvapiInitialized = true;
  }

  /***** NVML INIT *****/
  {
    // Initialize NVML library
    NVML_CALL(nvmlInit(), errored);

    // Mark NVML as initialized
    nvmlInitialized = true;
  }

  /***** NVAPI HANDLES *****/
  {
    // Get NVAPI device handles for all GPUs
    NVAPI_CALL(NvAPI_EnumPhysicalGPUs(nvapiDevices, &deviceCount), errored);
  }

  /***** NVML HANDLES *****/
  {
    // Get NVML device handles for all GPUs
    for (unsigned int i = 0; i < deviceCount; i++) {
      NVML_CALL(nvmlDeviceGetHandleByIndex(i, &nvmlDevices[i]), errored);
    }
  }

  /***** INIT *****/
  {
    // Print variables
    printf("iterations_before_switch = %d\n", iterations_before_switch);
    printf("performance_state_high = %d\n", performance_state_high);
    printf("performance_state_low = %d\n", performance_state_low);
    printf("sleep_interval = %d\n", sleep_interval);
    printf("temperature_threshold = %d\n", temperature_threshold);

    // Print the number of GPUs being managed
    printf("Managing %d GPUs...\n", deviceCount);

    // Iterate through each GPU
    for (unsigned int i = 0; i < deviceCount; i++) {
      // Switch to low performance state
      if (!enter_pstate(i, performance_state_low)) {
        goto errored;
      }
    }
  }

  /***** MAIN LOOP *****/
  {
    // Infinite loop to continuously monitor GPU temperature and utilization
    while (should_run) {
      // Loop through all devices
      for (unsigned int i = 0; i < deviceCount; i++) {
        // Get the current state of the GPU
        gpuState * state = &gpuStates[i];

        // Retrieve the current temperature of the GPU
        NVML_CALL(nvmlDeviceGetTemperature(nvmlDevices[i], NVML_TEMPERATURE_GPU, &temperature), errored);

        // Check if the GPU temperature exceeds the defined threshold
        if (temperature > temperature_threshold) {
          // If the GPU is not already in low performance state
          if (state->pstateId != performance_state_low) {
            // Switch to low performance state
            if (!enter_pstate(i, performance_state_low)) {
              goto errored;
            }
          }

          // Skip further checks for this iteration
          continue;
        }

        // Retrieve the current utilization rates of the GPU
        NVML_CALL(nvmlDeviceGetUtilizationRates(nvmlDevices[i], &utilization), errored);

        // Check if the GPU utilization is not zero
        if (utilization.gpu != 0) {
          // If the GPU is not already in high performance state
          if (state->pstateId != performance_state_high) {
            // Switch to high performance state
            if (!enter_pstate(i, performance_state_high)) {
              goto errored;
            }
          } else {
            // Reset the iteration counter
            state->iterations = 0;
          }
        } else {
          // If the GPU is not already in low performance state
          if (state->pstateId != performance_state_low) {
            // If the number of iterations exceeds the threshold
            if (state->iterations > iterations_before_switch) {
              // Switch to low performance state
              if (!enter_pstate(i, performance_state_low)) {
                goto errored;
              }
            }

            // Increment the iteration counter
            state->iterations++;
          }
        }
      }

      // Sleep for a defined interval before the next check
      #ifdef _WIN32
        Sleep(sleep_interval);
      #elif __linux__
        usleep(sleep_interval * 1000);
      #endif
    }
  }

  /***** NORMAL EXIT *****/
  {
    // Iterate through each GPU
    for (unsigned int i = 0; i < deviceCount; i++) {
      // Switch to automatic management of performance state
      if (!enter_pstate(i, 16)) {
        goto errored;
      }
    }

    // Notify about the exit
    printf("Exiting...\n");

    // Jump to cleanup section
    goto cleanup;
  }

  errored:
  /***** APPLICATION ERROR OCCURRED *****/
  {
    error_occurred = true;
  }

  cleanup:
  /***** NVAPI DEINIT *****/
  {
    // Unload NVAPI library if it was initialized
    if (nvapiInitialized) {
      // Set NVAPI initialization flag to false
      nvapiInitialized = false;

      // Unload NVAPI library
      NVAPI_CALL(NvAPI_Unload(), errored);
    }
  }

  /***** NVML DEINIT *****/
  {
    // Shutdown NVML library if it was initialized
    if (nvmlInitialized) {
      // Set NVML initialization flag to false
      nvmlInitialized = false;

      // Shutdown NVML library
      NVML_CALL(nvmlShutdown(), errored);
    }
  }

  /***** RETURN *****/
  {
    return error_occurred;
  }
}
