#include <nvml.h>

#include "nvml.h"
#include "utils.h"

/***** ***** ***** ***** ***** TYPES ***** ***** ***** ***** *****/

typedef nvmlReturn_t (*nvmlDeviceGetHandleByIndex_v2_t)(unsigned int, nvmlDevice_t);
typedef nvmlReturn_t (*nvmlDeviceGetName_t)(nvmlDevice_t, char *, unsigned int);
typedef nvmlReturn_t (*nvmlDeviceGetTemperature_t)(nvmlDevice_t, nvmlTemperatureSensors_t, unsigned int);
typedef nvmlReturn_t (*nvmlDeviceGetUtilizationRates_t)(nvmlDevice_t, nvmlUtilization_t);
typedef       char * (*nvmlErrorString_t)(nvmlReturn_t);
typedef nvmlReturn_t (*nvmlInit_v2_t)(void);
typedef nvmlReturn_t (*nvmlShutdown_t)(void);

/***** ***** ***** ***** ***** VARIABLES ***** ***** ***** ***** *****/

static void * lib;

static nvmlDeviceGetHandleByIndex_v2_t _nvmlDeviceGetHandleByIndex_v2;
static nvmlDeviceGetName_t             _nvmlDeviceGetName;
static nvmlDeviceGetTemperature_t      _nvmlDeviceGetTemperature;
static nvmlDeviceGetUtilizationRates_t _nvmlDeviceGetUtilizationRates;
static nvmlErrorString_t               _nvmlErrorString;
static nvmlInit_v2_t                   _nvmlInit_v2;
static nvmlShutdown_t                  _nvmlShutdown;

/***** ***** ***** ***** ***** MACROS ***** ***** ***** ***** *****/

#define NVML_POINTER(pointer) do {   \
  if (pointer == NULL) {             \
    return NVML_ERROR_UNINITIALIZED; \
  }                                  \
} while(0)

/***** ***** ***** ***** ***** IMPLEMENTATION ***** ***** ***** ***** *****/

nvmlReturn_t nvmlDeviceGetHandleByIndex_v2(unsigned int index, nvmlDevice_t * device) {
  // Ensure the function pointer is valid
  NVML_POINTER(_nvmlDeviceGetHandleByIndex_v2);

  // Invoke the function using the provided parameters
  return _nvmlDeviceGetHandleByIndex_v2(index, *device);
}

nvmlReturn_t nvmlDeviceGetName(nvmlDevice_t device, char * name, unsigned int length) {
  // Ensure the function pointer is valid
  NVML_POINTER(_nvmlDeviceGetName);

  // Invoke the function using the provided parameters
  return _nvmlDeviceGetName(device, name, length);
}

nvmlReturn_t nvmlDeviceGetTemperature(nvmlDevice_t device, nvmlTemperatureSensors_t sensorType, unsigned int * temp) {
  // Ensure the function pointer is valid
  NVML_POINTER(_nvmlDeviceGetTemperature);

  // Invoke the function using the provided parameters
  return _nvmlDeviceGetTemperature(device, sensorType, *temp);
}

nvmlReturn_t nvmlDeviceGetUtilizationRates(nvmlDevice_t device, nvmlUtilization_t * utilization) {
  // Ensure the function pointer is valid
  NVML_POINTER(_nvmlDeviceGetUtilizationRates);

  // Invoke the function using the provided parameters
  return _nvmlDeviceGetUtilizationRates(device, *utilization);
}

const char * nvmlErrorString(nvmlReturn_t result) {
  // Ensure the function pointer is valid
  if (_nvmlErrorString == NULL) {
    return "<nvmlErrorString() call failed>";
  }

  // Invoke the function using the provided parameters
  return _nvmlErrorString(result);
}

nvmlReturn_t nvmlInit_v2(void) {
  // Check the platform and load the appropriate NVML library
  #ifdef _WIN32
    if (!lib) {
      lib = library_open("nvml64.dll");
    }

    if (!lib) {
      lib = library_open("nvml.dll");
    }
  #elif __linux__
    if (!lib) {
      lib = library_open("libnvidia-ml.so.1");
    }

    if (!lib) {
      lib = library_open("libnvidia-ml.so");
    }
  #endif

  // If the library handle is still not initialized, loading the library failed
  if (!lib) {
    // Print an error message indicating failure to load the NVML library
    fprintf(stderr, "Unable to load NVML library\n");

    // Return an error status indicating that the library was not found
    return NVML_ERROR_LIBRARY_NOT_FOUND;
  }

  // Retrieve the addresses of specific NVML functions
  _nvmlDeviceGetHandleByIndex_v2 = (nvmlDeviceGetHandleByIndex_v2_t) library_proc(lib, "nvmlDeviceGetHandleByIndex_v2");
  _nvmlDeviceGetName = (nvmlDeviceGetName_t) library_proc(lib, "nvmlDeviceGetName");
  _nvmlDeviceGetTemperature = (nvmlDeviceGetTemperature_t) library_proc(lib, "nvmlDeviceGetTemperature");
  _nvmlDeviceGetUtilizationRates = (nvmlDeviceGetUtilizationRates_t) library_proc(lib, "nvmlDeviceGetUtilizationRates");
  _nvmlErrorString = (nvmlErrorString_t) library_proc(lib, "nvmlErrorString");
  _nvmlInit_v2 = (nvmlInit_v2_t) library_proc(lib, "nvmlInit_v2");
  _nvmlShutdown = (nvmlShutdown_t) library_proc(lib, "nvmlShutdown");

  // Ensure the function pointer is valid
  NVML_POINTER(_nvmlInit_v2);

  // Invoke the function using the provided parameters
  return _nvmlInit_v2();
}

nvmlReturn_t nvmlShutdown(void) {
  // Ensure the function pointer is valid
  NVML_POINTER(_nvmlShutdown);

  // Invoke the function using the provided parameters
  nvmlReturn_t ret = _nvmlShutdown();

  // If the function call was successful, proceed with cleanup
  if (ret == NVML_SUCCESS) {
    // If the library handle is initialized
    if (lib) {
      // Nullify all the function pointers to prevent further use
      _nvmlDeviceGetHandleByIndex_v2 = NULL;
      _nvmlDeviceGetName = NULL;
      _nvmlDeviceGetTemperature = NULL;
      _nvmlDeviceGetUtilizationRates = NULL;
      _nvmlErrorString = NULL;
      _nvmlInit_v2 = NULL;
      _nvmlShutdown = NULL;

      // Free the loaded library
      library_close(lib);
    }
  }

  // Return the status of the function call
  return ret;
}
