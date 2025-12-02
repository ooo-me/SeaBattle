# version.cmake
# Dynamic version generation for YY.DDD.PATCH format

# Get current date components
string(TIMESTAMP CURRENT_YEAR "%y")     # YY: two-digit year
string(TIMESTAMP CURRENT_DAY "%j")      # DDD: day of year (001-366)

# Determine PATCH value based on build context
if(DEFINED ENV{CI})
    # CI/CD environment
    if(DEFINED ENV{SEABATTLE_PATCH})
        # Use provided PATCH from environment
        set(VERSION_PATCH "$ENV{SEABATTLE_PATCH}")
    else
        # Default to 1 for nightly builds
        set(VERSION_PATCH "1")
    endif()
else
    # Local build - use "999" to signal non-production build
    set(VERSION_PATCH "999")
endif()

# Construct version string
set(PROJECT_VERSION "${CURRENT_YEAR}.${CURRENT_DAY}.${VERSION_PATCH}")

# Set CMake version variables for CPack
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${CURRENT_YEAR}")
set(CPACK_PACKAGE_VERSION_MINOR "${CURRENT_DAY}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")

# Display version information
message(STATUS "SeaBattle Version: ${PROJECT_VERSION}")
message(STATUS "  Year: ${CURRENT_YEAR}")
message(STATUS "  Day of Year: ${CURRENT_DAY}")
message(STATUS "  Patch: ${VERSION_PATCH}")
if(NOT DEFINED ENV{CI})
    message(WARNING "Building locally - version marked as unstable (PATCH=999)")
endif()
