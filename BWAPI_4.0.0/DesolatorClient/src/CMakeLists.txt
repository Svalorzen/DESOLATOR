cmake_minimum_required (VERSION 2.6)

find_package(Boost 1.53 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(desolator AI.cpp BWAPIHelpers.cpp DesolatorModuleFrame.cpp DesolatorModuleStart.cpp DesolatorModuleText.cpp main.cpp MDPState.cpp UnitState.cpp)
