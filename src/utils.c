#include "utils.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

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

bool parse_ulong_array(const char *arg, const char *delimiter, const size_t max_count, unsigned long *values, size_t *count) {
  // Check if the input or output argument is invalid
  if (arg == NULL || values == NULL || count == NULL) {
    return false;
  }

  // Duplicate the input string
  char *string = strdup(arg);

  // Check if string duplication failed
  if (string == NULL) {
    return false;
  }

  // Get the first token
  char *token = strtok(string, delimiter);

  // Initialize index to keep track of the number of parsed values
  size_t index = 0;

  // Iterate over the tokens
  while (token != NULL) {
    // Check if the parsed values exceed the maximum allowed count
    if (index >= max_count) {
      // Free the duplicated string
      SAFE_FREE(string);

      // Return false due to exceeding max count
      return false;
    }

    // Parse the current token into an unsigned long
    if (!parse_ulong(token, &values[index])) {
      // Free the duplicated string
      SAFE_FREE(string);

      // Return false if parsing fails
      return false;
    }

    // Increment the index after successful parsing
    index++;

    // Get the next token
    token = strtok(NULL, delimiter);
  }

  // Store the number of successfully parsed values
  *count = index;

  // Free the duplicated string
  SAFE_FREE(string);

  // Return true if parsing were successful
  return true;
}
