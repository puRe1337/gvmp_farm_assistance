cmake_minimum_required(VERSION 3.3)

# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake Targets")

project(gvmp_farm_assistance C CXX)
set(CMAKE_CXX_STANDARD 20)

# Output directory settings
set(output_dir ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${output_dir})

if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()

#set(CPPAN_BUILD_SHARED_LIBS ON)
add_subdirectory(.cppan)
add_subdirectory(fmt EXCLUDE_FROM_ALL)
#include_directories(include)
set(OpenCV_DIR "D:/User/Downloads/opencv-4.1.0/opencv-4.1.0/build/install")
set(OpenCV_STATIC FALSE)
FIND_PACKAGE(OpenCV REQUIRED core highgui imgcodecs imgproc)


add_executable(gvmp_farm_assistance src/main.cpp include/utils.hpp include/timer.hpp gvmp_farm_assistance.rc)
target_include_directories(gvmp_farm_assistance PUBLIC include)
target_link_libraries(gvmp_farm_assistance
    pvt.cppan.demo.google.tesseract.libtesseract
    pvt.cppan.demo.danbloomberg.leptonica
    fmt-header-only
    ${OpenCV_LIBS}
)
message(${OpenCV_LIBS})

MACRO (COPY_DLL trgt libname)
        ADD_CUSTOM_COMMAND (
        TARGET ${trgt}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy "${OpenCV_DIR}/x64/vc16/bin/${libname}.dll" $<TARGET_FILE_DIR:${trgt}>
    )
ENDMACRO (COPY_DLL)
COPY_DLL(gvmp_farm_assistance opencv_core410)
COPY_DLL(gvmp_farm_assistance opencv_highgui410)
COPY_DLL(gvmp_farm_assistance opencv_imgcodecs410)
COPY_DLL(gvmp_farm_assistance opencv_imgproc410)