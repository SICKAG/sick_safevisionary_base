[![License](https://img.shields.io/badge/License-Apache_2.0-yellow.svg)](https://opensource.org/licenses/Apache-2.0)
![build badge](https://github.com/SICKAG/sick_safevisionary_base/actions/workflows/industrial_ci_noetic_action.yml/badge.svg)
![build badge](https://github.com/SICKAG/sick_safevisionary_base/actions/workflows/industrial_ci_humble_action.yml/badge.svg)
![build badge](https://github.com/SICKAG/sick_safevisionary_base/actions/workflows/industrial_ci_iron_action.yml/badge.svg)
![build badge](https://github.com/SICKAG/sick_safevisionary_base/actions/workflows/industrial_ci_rolling_action.yml/badge.svg)

# Sick Safevisionary Base
This is the C++ driver library for SICK safeVisionary 3D cameras.
It implements the core functionality of the sensor communication for usage in different frameworks.

## Sensor configuration
Each camera needs an initial configuration once to get started. Here's a [brief explanation](./resources/doc/safety_designer.md) how to do that.

## ROS1/2 usage
There are two lean drivers that cover all supported ROS versions.
You'll find them here:
- [sick_safevisionary_ros1](https://github.com/SICKAG/sick_safevisionary_ros1)
- [sick_safevisionary_ros2](https://github.com/SICKAG/sick_safevisionary_ros2)

## Standalone build and usage
You can also use this library in your non-ROS C++ application.
In a suitable directory, get this package with
```bash
git clone https://github.com/SICKAG/sick_safevisionary_base.git
```

Navigate into the freshly cloned package and call
```bash
mkdir build && cd "$_"
cmake ..
make
```
to build the library with plain *CMake*.
You can then include this library in the *CMakeLists.txt* of your application with the usual functionality:
```cmake
cmake_minimum_required(VERSION 3.10)
project(your_application)

# Find the base library as a dependency
find_package(sick_safevisionary_base REQUIRED)

# Link your application against the library
target_link_libraries(your_application
  sick_safevisionary_base::sick_safevisionary_base
)

```
