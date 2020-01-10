cmake_minimum_required(VERSION 3.3)

# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake Targets")

project(gvmp_farm_assistance C CXX)
set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_WIN32_EXECUTABLE TRUE)

# Output directory settings
set(output_dir ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${output_dir})

if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()

set(DEPENDENCIES
    org.sw.demo.google.tesseract.libtesseract-master
)

find_package(SW REQUIRED)
sw_add_package(${DEPENDENCIES})
sw_execute()

add_subdirectory(fmt EXCLUDE_FROM_ALL)
#include_directories(include)
set(OpenCV_DIR "D:/User/Downloads/opencv-4.1.0/opencv-4.1.0/build/install")
set(OpenCV_STATIC FALSE)
FIND_PACKAGE(OpenCV REQUIRED core highgui imgcodecs imgproc)

#set(CEF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/cef")
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")
#find_package(CEF REQUIRED)
#add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
#PRINT_CEF_CONFIG()

add_executable(gvmp_farm_assistance src/main.cpp include/utils.hpp include/timer.hpp include/globals.hpp include/resource.h gvmp_farm_assistance.rc)
target_include_directories(gvmp_farm_assistance PUBLIC include)
target_link_libraries(gvmp_farm_assistance
    ${DEPENDENCIES}
    fmt-header-only
    ${OpenCV_LIBS}
)
message("OpenCV Libs: ${OpenCV_LIBS}")

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