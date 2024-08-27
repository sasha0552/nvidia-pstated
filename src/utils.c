#include "utils.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

bool parse_ulong(const char *arg, unsigned long *value) {
  // Check if the input or output argument is invalid
  if (arg == NULL || value == NULL) {
    return false;
  }

  // Declare a pointer to track where strtoul stops parsing
  char *endptr;


  // Reset errno to ensure we catch errors from strtoul correctly
  errno = 0;

  // Convert the string to a unsigned long using base 10
  unsigned long result = strtoul(arg, &endptr, 10);

  // Check if the entire string was not consumed or if no digits were found
  if (*endptr != '\0' || endptr == arg) {
    return false;
  }

  // Check if an overflow occurred during conversion
  if (result == ULONG_MAX && errno == ERANGE) {
    // Reset errno to indicate the error has been handled
    errno = 0;

    // Return false as the conversion was not successful
    return false;
  }

  // Assign the result to the output value
  *value = result;

  // Conversion successful and the input was valid
  return true;
}
