#include <nvapi.h>
#include <nvml.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32
  #include <windows.h>
#elif __linux__
  #include <unistd.h>
#endif

#include "nvapi.h"
#include "nvml.h"
#include "utils.h"

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

// Utilization threshold (in percentage)
#define UTILIZATION_THRESHOLD 0

/***** ***** ***** ***** ***** STRUCTURES ***** ***** ***** ***** *****/

// Structure to hold the state of each GPU
typedef struct {
  // Counter for iterations when in a specific state
  unsigned int iterations;

  // Current performance state of the GPU
  unsigned int pstateId;

  // GPU management state
  bool managed;
} gpuState;

/***** ***** ***** ***** ***** VARIABLES ***** ***** ***** ***** *****/

// Flag indicating whether the program should continue running
static volatile sig_atomic_t shouldRun = true;

// Flag indicating whether an error has occurred
static bool errorOccurred = false;

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

static void handle_exit(int signal) {
  // Check if the received signal is SIGINT or SIGTERM
  if (signal == SIGINT || signal == SIGTERM) {
    // Set the global flag to false to indicate the program should stop running
    shouldRun = false;
  }
}

static bool enter_pstate(unsigned int i, unsigned int pstateId) {
  // Get the current state of the GPU
  gpuState * state = &gpuStates[i];

  // If GPU are unmanaged
  if (!state->managed) {
    // Return true to indicate success
    return true;
  }

  // Set the GPU to the desired performance state
  NVAPI_CALL(NvAPI_GPU_SetForcePstate(nvapiDevices[i], pstateId, 0), failure);

  // Reset the iteration counter
  state->iterations = 0;

  // Update the GPU state with the new performance state
  state->pstateId = pstateId;

  // Print the current GPU state
  printf("GPU %u entered performance state %u\n", i, state->pstateId);

  // Return true to indicate success
  return true;

  failure:
  // Return false to indicate failure
  return false;
}

static int run(int argc, char * argv[]) {
  /***** OPTIONS *****/
  unsigned long ids[NVAPI_MAX_PHYSICAL_GPUS] = { 0 };
  size_t idsCount = 0;
  unsigned long iterationsBeforeSwitch = ITERATIONS_BEFORE_SWITCH;
  unsigned long performanceStateHigh = PERFORMANCE_STATE_HIGH;
  unsigned long performanceStateLow = PERFORMANCE_STATE_LOW;
  unsigned long sleepInterval = SLEEP_INTERVAL;
  unsigned long temperatureThreshold = TEMPERATURE_THRESHOLD;
  unsigned long utilizationThreshold = UTILIZATION_THRESHOLD;

  /***** OPTION PARSING *****/
  {
    // Iterate through command-line arguments
    for (unsigned int i = 1; i < argc; i++) {
      // Check if the option is "-i" or "--ids" and if there is a next argument
      if ((IS_OPTION("-i") || IS_OPTION("--ids")) && HAS_NEXT_ARG) {
        // Parse the integer array option and store it in ids
        ASSERT_TRUE(parse_ulong_array(argv[++i], ",", NVAPI_MAX_PHYSICAL_GPUS, ids, &idsCount), usage);
      }

      // Check if the option is "-h" or "--help"
      if ((IS_OPTION("-h") || IS_OPTION("--help"))) {
        // Print usage instructions
        goto usage;
      }

      // Check if the option is "-ibs" or "--iterations-before-switch" and if there is a next argument
      if ((IS_OPTION("-ibs") || IS_OPTION("--iterations-before-switch")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in iterationsBeforeSwitch
        ASSERT_TRUE(parse_ulong(argv[++i], &iterationsBeforeSwitch), usage);
      }

      // Check if the option is "-psh" or "--performance-state-high" and if there is a next argument
      if ((IS_OPTION("-psh") || IS_OPTION("--performance-state-high")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in performanceStateHigh
        ASSERT_TRUE(parse_ulong(argv[++i], &performanceStateHigh), usage);
      }

      // Check if the option is "-psl" or "--performance-state-low" and if there is a next argument
      if ((IS_OPTION("-psl") || IS_OPTION("--performance-state-low")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in performanceStateLow
        ASSERT_TRUE(parse_ulong(argv[++i], &performanceStateLow), usage);
      }

      // Check if the option is "-s" or "--service"
      if ((IS_OPTION("-s") || IS_OPTION("--service"))) {
        // Skip option
        continue;
      }

      // Check if the option is "-si" or "--sleep-interval" and if there is a next argument
      if ((IS_OPTION("-si") || IS_OPTION("--sleep-interval")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in sleepInterval
        ASSERT_TRUE(parse_ulong(argv[++i], &sleepInterval), usage);
      }

      // Check if the option is "-tt" or "--temperature-threshold" and if there is a next argument
      if ((IS_OPTION("-tt") || IS_OPTION("--temperature-threshold")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in temperatureThreshold
        ASSERT_TRUE(parse_ulong(argv[++i], &temperatureThreshold), usage);
      }

      // Check if the option is "-ut" or "--utilization-threshold" and if there is a next argument
      if ((IS_OPTION("-ut") || IS_OPTION("--utilization-threshold")) && HAS_NEXT_ARG) {
        // Parse the integer option and store it in utilizationThreshold
        ASSERT_TRUE(parse_ulong(argv[++i], &utilizationThreshold), usage);
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
      printf("  -i, --ids <value><,value...>              Set the GPU(s) to control (default: all)\n");
      printf("  -ibs, --iterations-before-switch <value>  Set the number of iterations to wait before switching states (default: %u)\n", ITERATIONS_BEFORE_SWITCH);
      printf("  -psh, --performance-state-high <value>    Set the high performance state for the GPU (default: %u)\n", PERFORMANCE_STATE_HIGH);
      printf("  -psl, --performance-state-low <value>     Set the low performance state for the GPU (default: %u)\n", PERFORMANCE_STATE_LOW);

      #ifdef _WIN32
        printf("  -s, --service                             Run as a Windows service\n");
      #endif

      printf("  -si, --sleep-interval <value>             Set the sleep interval in milliseconds between utilization checks (default: %u)\n", SLEEP_INTERVAL);
      printf("  -tt, --temperature-threshold <value>      Set the temperature threshold in degrees C (default: %u)\n", TEMPERATURE_THRESHOLD);
      printf("  -ut, --utilization-threshold <value>      Set the utilization threshold in percentage (default: %u)\n", UTILIZATION_THRESHOLD);

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

  /***** SORT NVAPI HANDLES */
  {
    // Array to hold NVML device identifiers
    NvU32 nvmlIdentifiers[NVAPI_MAX_PHYSICAL_GPUS];

    // Array to hold NVAPI device identifiers
    NvU32 nvapiIdentifiers[NVAPI_MAX_PHYSICAL_GPUS];

    // Step 1: Loop through each device to retrieve and store NVML and NVAPI identifiers
    for (unsigned int i = 0; i < deviceCount; i++) {
      // Initialize struct to hold PCI info
      nvmlPciInfo_t nvmlPciInfo;

      // Get PCI info
      NVML_CALL(nvmlDeviceGetPciInfo(nvmlDevices[i], &nvmlPciInfo), errored);

      // Store bus id in nvmlIdentifiers array
      nvmlIdentifiers[i] = nvmlPciInfo.bus;

      // Variable to hold bus id
      NvU32 nvapiBusId;

      // Get bus id
      NVAPI_CALL(NvAPI_GPU_GetBusId(nvapiDevices[i], &nvapiBusId), errored);

      // Store in nvapiIdentifiers array
      nvapiIdentifiers[i] = nvapiBusId;
    }

    // Array to store NVAPI device handles in sorted order
    NvPhysicalGpuHandle sortedNvapiDevices[NVAPI_MAX_PHYSICAL_GPUS];

    // Step 2: Match and order NVAPI devices based on serial numbers
    for (unsigned int i = 0; i < deviceCount; i++) {
      for (unsigned int j = 0; j < deviceCount; j++) {
        // Compare NVML and NVAPI identifiers
        if (nvmlIdentifiers[i] == nvapiIdentifiers[j]) {
          // Store matched device handle in sorted array
          sortedNvapiDevices[i] = nvapiDevices[j];

          // Exit the inner loop
          break;
        }
      }
    }

    // Step 3: Copy sorted handles back to original array
    memcpy(nvapiDevices, sortedNvapiDevices, sizeof(sortedNvapiDevices));
  }

  /***** INIT *****/
  {
    // Print ids
    {
      // Print the initial text
      printf("ids = ");

      // Loop through each element in the array
      for (size_t i = 0; i < idsCount; i++) {
        // Print the current element with %lu for unsigned long
        printf("%lu", ids[i]);

        // If this is not the last element
        if (i + 1 < idsCount) {
          // Print a comma
          printf(",");
        }
      }

      // If array is empty
      if (idsCount == 0) {
        // Print "N/A"
        printf("N/A");
      }

      // Print the count of elements in the array and newline character
      printf(" (%zu)\n", idsCount);
    }

    // Print remaining variables
    printf("iterationsBeforeSwitch = %lu\n", iterationsBeforeSwitch);
    printf("performanceStateHigh = %lu\n", performanceStateHigh);
    printf("performanceStateLow = %lu\n", performanceStateLow);
    printf("sleepInterval = %lu\n", sleepInterval);
    printf("temperatureThreshold = %lu\n", temperatureThreshold);
    printf("utilizationThreshold = %lu\n", utilizationThreshold);

    // Check if there are specific GPU ids to process
    if (idsCount != 0) {
      // Iterate over each provided id
      for (size_t i = 0; i < idsCount; i++) {
        // Get the current id
        unsigned long id = ids[i];

        // Validate the id
        if (id < 0 || id > deviceCount) {
          // Print error message for invalid id
          printf("Invalid GPU id: %zu\n", i);

          // Skip to the next id
          continue;
        }

        // Get the current state of the GPU
        gpuState * state = &gpuStates[id];

        // Mark the GPU as managed
        state->managed = true;
      }
    } else {
      // Iterate through each GPU
      for (unsigned int i = 0; i < deviceCount; i++) {
        // Get the current state of the GPU
        gpuState * state = &gpuStates[i];

        // Mark the GPU as managed
        state->managed = true;
      }
    }

    // Initialize the counter for managed GPUs
    unsigned int managedGPUs = 0;

    // Iterate through each GPU
    for (unsigned int i = 0; i < deviceCount; i++) {
      // Get the current state of the GPU
      gpuState * state = &gpuStates[i];

      // If GPU is managed
      if (state->managed) {
        // Buffer to store the GPU name
        char gpuName[256];

        // Retrieve the GPU name
        NVML_CALL(nvmlDeviceGetName(nvmlDevices[i], gpuName, sizeof(gpuName)), errored);

        // Print the managed GPU details
        printf("%u. %s (GPU id = %u)\n", managedGPUs, gpuName, i);

        // Increment the managed GPU counter
        managedGPUs++;
      }
    }

    // If no GPUs are managed, report an error
    if (managedGPUs == 0) {
      // Print error message
      printf("Can't find GPUs to manage!\n");

      // Jump to error handling section
      goto errored;
    }

    // Print the number of GPUs being managed
    printf("Managing %u GPUs...\n", managedGPUs);

    // Iterate through each GPU
    for (unsigned int i = 0; i < deviceCount; i++) {
      // Switch to low performance state
      if (!enter_pstate(i, performanceStateLow)) {
        goto errored;
      }
    }
  }

  /***** MAIN LOOP *****/
  {
    // Infinite loop to continuously monitor GPU temperature and utilization
    while (shouldRun) {
      // Loop through all devices
      for (unsigned int i = 0; i < deviceCount; i++) {
        // Get the current state of the GPU
        gpuState * state = &gpuStates[i];

        // Retrieve the current temperature of the GPU
        NVML_CALL(nvmlDeviceGetTemperature(nvmlDevices[i], NVML_TEMPERATURE_GPU, &temperature), errored);

        // Check if the GPU temperature exceeds the defined threshold
        if (temperature > temperatureThreshold) {
          // If the GPU is not already in low performance state
          if (state->pstateId != performanceStateLow) {
            // Switch to low performance state
            if (!enter_pstate(i, performanceStateLow)) {
              goto errored;
            }
          }

          // Skip further checks for this iteration
          continue;
        }

        // Retrieve the current utilization rates of the GPU
        NVML_CALL(nvmlDeviceGetUtilizationRates(nvmlDevices[i], &utilization), errored);

        // Check if the GPU utilization is above the defined threshold
        if (utilization.gpu > utilizationThreshold) {
          // If the GPU is not already in high performance state
          if (state->pstateId != performanceStateHigh) {
            // Switch to high performance state
            if (!enter_pstate(i, performanceStateHigh)) {
              goto errored;
            }
          } else {
            // Reset the iteration counter
            state->iterations = 0;
          }
        } else {
          // If the GPU is not already in low performance state
          if (state->pstateId != performanceStateLow) {
            // If the number of iterations exceeds the threshold
            if (state->iterations > iterationsBeforeSwitch) {
              // Switch to low performance state
              if (!enter_pstate(i, performanceStateLow)) {
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
        Sleep(sleepInterval);
      #elif __linux__
        usleep(sleepInterval * 1000);
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
    errorOccurred = true;
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
    return errorOccurred;
  }
}

#ifdef _WIN32
  // Service name
  #define SERVICE_NAME "nvidia-pstated"

  // Service status handle
  static SERVICE_STATUS_HANDLE serviceStatusHandle;

  // Service status structure
  static SERVICE_STATUS serviceStatus;

  static void WINAPI ServiceCtrlHandler(DWORD ctrlCode) {
    switch (ctrlCode) {
      case SERVICE_CONTROL_SHUTDOWN:
      case SERVICE_CONTROL_STOP: {
        // Set the service status to stop pending
        serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;

        // Set the global flag to false to indicate the program should stop running
        shouldRun = false;
      }
    }

    // Update the service status
    SetServiceStatus(serviceStatusHandle, &serviceStatus);
  }

  static void WINAPI ServiceMain(DWORD argc, LPTSTR * argv) {
    // Initialize the service status handle
    serviceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

    // Set the service type
    serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    // Set the service current state
    serviceStatus.dwCurrentState = SERVICE_RUNNING;

    // Set the service's accepted controls
    serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;

    // Update the service status
    SetServiceStatus(serviceStatusHandle, &serviceStatus);

    // Run the daemon
    int ret = run(__argc, __argv);

    // If the daemon returns an error, set the exit code
    if (ret != 0) {
      // Set the service generic exit code
      serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;

      // Set the service specific exit code
      serviceStatus.dwServiceSpecificExitCode = ret;
    }

    // Set the service status to stopped
    serviceStatus.dwCurrentState = SERVICE_STOPPED;

    // Update the service status
    SetServiceStatus(serviceStatusHandle, &serviceStatus);
  }
#endif

int main(int argc, char * argv[]) {
  // If on Windows
  #ifdef _WIN32
    // Iterate through command-line arguments
    for (unsigned int i = 1; i < argc; i++) {
      // Check if the option is "-s" or "--service"
      if ((IS_OPTION("-s") || IS_OPTION("--service"))) {
        // Create a service table entry
        SERVICE_TABLE_ENTRY serviceTableEntry[] = { { SERVICE_NAME, ServiceMain }, { NULL, NULL } };

        // Start the service control dispatcher
        StartServiceCtrlDispatcher(serviceTableEntry);

        // Return 0 to indicate success
        return 0;
      }
    }
  #endif

  // Run the daemon
  return run(argc, argv);
}
