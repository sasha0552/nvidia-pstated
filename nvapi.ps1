# Constants
$NVAPI_HEADERS = "nvapi"
$NVAPI_HEADERS_ARCHIVE = "nvapi.tar"
$NVAPI_HEADERS_URL = "https://download.nvidia.com/XFree86/nvapi-open-source-sdk/R555-OpenSource.tar"

# Download NVAPI headers
if (-not (Test-Path $NVAPI_HEADERS_ARCHIVE)) {
  Invoke-WebRequest -OutFile $NVAPI_HEADERS_ARCHIVE -Uri $NVAPI_HEADERS_URL
}

# Extract NVAPI headers
if (-not (Test-Path $NVAPI_HEADERS)) {
  # Create directory
  New-Item -ItemType Directory -Path $NVAPI_HEADERS

  # Extract headers
  tar --directory $NVAPI_HEADERS --extract --file $NVAPI_HEADERS_ARCHIVE --strip-components 2
}
