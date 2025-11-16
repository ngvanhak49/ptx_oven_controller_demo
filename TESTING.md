# Testing Guide

## Overview

This project includes two test suites:
1. **Original tests** - Simple custom test framework
2. **Google Test** - Modern C++ testing framework with rich assertions

## Running Google Test (Recommended)

### Prerequisites
- CMake 3.14 or higher
- C++14 compatible compiler (g++, clang++, MSVC)

### Build and Run

```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Run tests
cd build
ctest --output-on-failure

# Or run the executable directly for detailed output
./oven_control_test
```

### Windows (PowerShell)

```powershell
# Configure
cmake -B build -S .

# Build
cmake --build build --config Release

# Run tests
cd build
ctest -C Release --output-on-failure
```

## Running Original Tests

```bash
# Linux/Mac
g++ -std=c++17 \
  -I. -Itests/stubs \
  tests/mocks/mock_api.cpp \
  tests/mocks/mock_logging.cpp \
  ptx_oven_config.cpp \
  ptx_sensor_filter.cpp \
  ptx_actuator.cpp \
  ptx_oven_control.cpp \
  tests/test_oven_control.cpp \
  -o tests/run_tests

./tests/run_tests
```

```powershell
# Windows (PowerShell)
g++ -std=c++17 `
  -I. -Itests/stubs `
  tests/mocks/mock_api.cpp `
  tests/mocks/mock_logging.cpp `
  ptx_oven_config.cpp `
  ptx_sensor_filter.cpp `
  ptx_actuator.cpp `
  ptx_oven_control.cpp `
  tests/test_oven_control.cpp `
  -o tests/run_tests.exe

.\tests\run_tests.exe
```

## Test Coverage

Both test suites cover:
- ✅ Door open safety shutdown
- ✅ Ignition timing (5 seconds)
- ✅ Hysteresis control (180°C ± 2°C)
- ✅ Sensor fault detection (timed, >1s)
- ✅ Auto-resume after fault cleared (3s valid window)
- ✅ Ignition retry mechanism (3 attempts)
- ✅ Gas purge after failed ignition (2.5s)
- ✅ Safety lockout after max failures
- ✅ Manual lockout reset

## Benefits of Google Test

- Rich assertion macros (EXPECT_*, ASSERT_*)
- Test fixtures for setup/teardown
- Parameterized tests support
- Death tests for crash testing
- Better error reporting
- IDE integration (Visual Studio, CLion, VS Code)
- Industry standard framework

## CI/CD

Both test suites run automatically on GitHub Actions:
- `ci-gtest.yml` - Runs Google Test suite
- `ci.yml` - Runs original test suite (for backwards compatibility)
