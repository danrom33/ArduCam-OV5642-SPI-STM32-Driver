cmake_minimum_required(VERSION 3.10)
project(STM32)


# Collect all .c source files in Core/Sr
file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c")

# Add the executable using SRC files
add_library(SPI_CAM 
    ${SRC_FILES}
)

# Include headers
target_include_directories(SPI_CAM 
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
)

# target_link_libraries(MAIN PUBLIC STM32CUBEL4_HAL)