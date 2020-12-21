cmake_minimum_required(VERSION 3.10)
project(imageio)

set(TARGET imageio)
set(PUBLIC_HDR_DIR include)

# ==================================================================================================
# Sources and headers
# ==================================================================================================
set(PUBLIC_HDRS
        include/imageio/ImageDecoder.h
        include/imageio/ImageDiffer.h
        include/imageio/ImageEncoder.h
)

set(SRCS
        src/ImageDecoder.cpp
        src/ImageDiffer.cpp
        src/ImageEncoder.cpp
)

# HACK: this is more about arm64 than BlueGL
if (INCLUDE_BLUEGL)
    list(APPEND PUBLIC_HDRS include/imageio/BlockCompression.h)
    list(APPEND SRCS src/BlockCompression.cpp)
endif()

# ==================================================================================================
# Include and target definitions
# ==================================================================================================
include_directories(${PUBLIC_HDR_DIR})

add_library(${TARGET} STATIC ${PUBLIC_HDRS} ${SRCS})

target_include_directories(${TARGET} PUBLIC ${PUBLIC_HDR_DIR})

target_link_libraries(${TARGET} PUBLIC image math png tinyexr utils z stb)

if (WIN32)
    target_link_libraries(${TARGET} PRIVATE wsock32)
endif()

# ==================================================================================================
# Transitive macro definitions
# ==================================================================================================
if (INCLUDE_BLUEGL) # HACK: this is more about arm64 than BlueGL
    target_compile_definitions(${TARGET} PUBLIC IMAGEIO_SUPPORTS_BLOCK_COMPRESSION)
    target_link_libraries(${TARGET} PUBLIC astcenc EtcLib)
endif()

# ==================================================================================================
# Compiler flags
# ==================================================================================================
if (NOT MSVC)
    target_compile_options(${TARGET} PRIVATE -Wno-deprecated-register)
endif()

if (MSVC)
    target_compile_options(${TARGET} PRIVATE $<$<CONFIG:Release>:/fp:fast>)
else()
    target_compile_options(${TARGET} PRIVATE $<$<CONFIG:Release>:-ffast-math>)
endif()
