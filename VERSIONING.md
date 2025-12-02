# SeaBattle Versioning System

## Version Format: YY.DDD.PATCH

SeaBattle uses a date-based versioning system with the following format:

- **YY**: Two-digit year (e.g., 25 for 2025)
- **DDD**: Day of year, zero-padded (001-366)
- **PATCH**: Build number for the day

### Examples

- `25.001.1` - First nightly build of January 1st, 2025
- `25.336.1` - First nightly build of December 2nd, 2025
- `25.336.2` - Second build of December 2nd, 2025
- `25.336.999` - Local development build (unstable)

## Version Generation

Versions are automatically generated during the build process by `cmake/version.cmake`.

### Nightly Builds (CI/CD)

Nightly builds run automatically at **02:00 UTC** via GitHub Actions:
- The PATCH number is set to **1** for the first automated nightly build
- Version is determined at build time based on the current date

### Manual/Release Builds (CI/CD)

For subsequent builds on the same day (testing, integration, release):
- Set the `SEABATTLE_PATCH` environment variable to increment the patch number
- Example: `SEABATTLE_PATCH=2` for the second build of the day

### Local Development Builds

When building locally (outside of CI/CD):
- The PATCH number is set to **999** to indicate an unstable, non-production build
- A warning message is displayed: "Building locally - version marked as unstable (PATCH=999)"
- This signals that the build is for development purposes only and should not be distributed

## Building

### Nightly Build (CI/CD)

The nightly build workflow uses the `x64-nightly` preset:

```bash
cmake --workflow --preset x64-nightly
```

### Local Development Build

To build locally with the unstable version marker:

```bash
cmake --workflow --preset x64-nightly
```

The version will automatically show PATCH=999 when CI environment is not detected.

### Custom Version Build (CI/CD)

To create a custom build with a specific PATCH number in CI:

```bash
export CI=true
export SEABATTLE_PATCH=2
cmake --workflow --preset x64-nightly
```

## Workflow Configuration

The nightly build workflow is defined in `.github/workflows/nightly-build.yml`:

- Runs daily at 02:00 UTC
- Uses Windows 2022 runner
- Automatically sets `SEABATTLE_PATCH=1`
- Uploads artifacts with version-tagged names

## Technical Details

The versioning system is implemented in `cmake/version.cmake` and is automatically included in the main `CMakeLists.txt` before the project is defined. This ensures the version is available throughout the entire CMake configuration process.

Version information is also used by CPack for generating installer packages with proper version metadata.
