cmake_minimum_required(VERSION 3.10)

project(window_manager)

file(GLOB_RECURSE CPP_SRC_FILES ./*.cpp)

file(GLOB_RECURSE CPP_HEADER_FILES ./*.hpp)

add_library(window_manager ${CPP_SRC_FILES} ${CPP_HEADER_FILES})

find_package(X11 REQUIRED)
if(!X11_XTest_FOUND)
    message(FATAL_ERROR "X11 extensions are required, but not found!")
endif()
if(!X11_Xfixes_LIB)
    message(FATAL_ERROR "X11 fixes extension is required, but not found!")
endif()
if (!X11_Xext_LIB)
    message(FATAL_ERROR "X11 Xext extension required but not found!")
endif()

target_link_libraries(window_manager
${X11_Xext_LIB}
${X11_Xcomposite_LIB}
${X11_Xrender_LIB}
${X11_LIBRARIES}
${X11_Xfixes_LIB}
${X11_XTest_LIB}
${X11_Xinerama_LIB}
)
