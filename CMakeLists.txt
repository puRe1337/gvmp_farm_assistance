cmake_minimum_required(VERSION 3.3)

# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake Targets")

project(gvmp_farm_assistance C CXX)

# Output directory settings
set(output_dir ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${output_dir})

if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /std:c++latest")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

#set(CPPAN_BUILD_SHARED_LIBS ON)
add_subdirectory(.cppan)
add_subdirectory(fmt-5.3.0 EXCLUDE_FROM_ALL)
#include_directories(include)

add_executable(gvmp_farm_assistance src/main.cpp include/utils.hpp include/timer.hpp gvmp_farm_assistance.rc)
target_include_directories(gvmp_farm_assistance PUBLIC include)
target_link_libraries(gvmp_farm_assistance
    pvt.cppan.demo.google.tesseract.libtesseract
    pvt.cppan.demo.danbloomberg.leptonica
    fmt-header-only
)