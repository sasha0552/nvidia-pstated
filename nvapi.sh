#!/bin/sh
set -e

# Constants
readonly NVAPI_HEADERS="nvapi"
readonly NVAPI_HEADERS_ARCHIVE="nvapi.tar"
readonly NVAPI_HEADERS_URL="https://download.nvidia.com/XFree86/nvapi-open-source-sdk/R555-OpenSource.tar"

# Download NVAPI headers
if [ ! -f "$NVAPI_HEADERS_ARCHIVE" ]; then
  wget --output-document "$NVAPI_HEADERS_ARCHIVE" "$NVAPI_HEADERS_URL"
fi

# Extract NVAPI headers
if [ ! -d "$NVAPI_HEADERS" ]; then
  # Create directory
  mkdir "$NVAPI_HEADERS"

  # Extract headers
  tar --directory "$NVAPI_HEADERS" --extract --file "$NVAPI_HEADERS_ARCHIVE" --strip-components 2
fi
