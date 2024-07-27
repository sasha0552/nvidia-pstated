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

// Variable to store GPU utilization information
static nvmlUtilization_t utilization;

// Variable to store GPU states
static gpuState gpuStates[NVAPI_MAX_PHYSICAL_GPUS];

/***** ***** ***** ***** ***** FUNCTIONS ***** ***** ***** ***** *****/

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

int main() {
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
    // Print the number of GPUs being managed
    printf("Managing %d GPUs...\n", deviceCount);

    // Iterate through each GPU
    for (unsigned int i = 0; i < deviceCount; i++) {
      // Switch to low performance state
      if (!enter_pstate(i, PERFORMANCE_STATE_LOW)) {
        goto errored;
      }
    }
  }

  /***** MAIN LOOP *****/
  {
    // Infinite loop to continuously monitor GPU utilization
    while (should_run) {
      // Loop through all devices to get their utilization rates
      for (unsigned int i = 0; i < deviceCount; i++) {
        // Get the current state of the GPU
        gpuState * state = &gpuStates[i];

        // Retrieve the current utilization rates of the GPU
        NVML_CALL(nvmlDeviceGetUtilizationRates(nvmlDevices[i], &utilization), errored);

        // Check if the GPU utilization is not zero
        if (utilization.gpu != 0) {
          // If the GPU is not already in high performance state
          if (state->pstateId != PERFORMANCE_STATE_HIGH) {
            // Switch to high performance state
            if (!enter_pstate(i, PERFORMANCE_STATE_HIGH)) {
              goto errored;
            }
          } else {
            // Reset the iteration counter
            state->iterations = 0;
          }
        } else {
          // If the GPU is not already in low performance state
          if (state->pstateId != PERFORMANCE_STATE_LOW) {
            // If the number of iterations exceeds the threshold
            if (state->iterations > ITERATIONS_BEFORE_SWITCH) {
              // Switch to low performance state
              if (!enter_pstate(i, PERFORMANCE_STATE_LOW)) {
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
        Sleep(SLEEP_INTERVAL);
      #elif __linux__
        usleep(SLEEP_INTERVAL * 1000);
      #endif
    }
  }

  /***** NORMAL EXIT *****/
  {
    // Notify about the exit
    printf("Exiting...\n");

    // Jump to cleanup section
    goto cleanup;
  }

  errored:
  /***** APPLICATION ERROR HAPPENED *****/
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
