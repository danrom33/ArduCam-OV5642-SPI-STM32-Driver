# Adapted from Michael Noyce

# Configures the STM32CubeL4 HAL and FATFS libraries.
# It is expected the https://github.com/STMicroelectronics/STM32CubeL4 is cloned to ${CMAKE_SOURCE_DIR}/Libraries/STM32CubeL4.

# Generates static libraries:
# - SHARC::HAL
# - SHARC::FATFS
set(STM32CUBEL4_HAL_INCLUDE_DIRECTORIES
	${CMAKE_SOURCE_DIR}/Inc
    # ${CMAKE_SOURCE_DIR}/Core/Inc/HAL_Init
	${CMAKE_SOURCE_DIR}/Libraries/STM32CubeL4/Drivers/STM32L4xx_HAL_Driver/Inc
	# ${CMAKE_SOURCE_DIR}/Libraries/STM32CubeL4/Drivers/STM32L4xx_HAL_Driver/Inc/Legacy
	${CMAKE_SOURCE_DIR}/Libraries/STM32CubeL4/Drivers/CMSIS/Device/ST/STM32L4xx/Include
	${CMAKE_SOURCE_DIR}/Libraries/STM32CubeL4/Drivers/CMSIS/Include
	# ${CMAKE_SOURCE_DIR}/Libraries/STM32CubeL4/Drivers/CMSIS/DSP/Include
)

file(GLOB STM32CUBEL4_HAL_SOURCES
	${CMAKE_SOURCE_DIR}/Libraries/STM32CubeL4/Drivers/STM32L4xx_HAL_Driver/Src/*_hal*.c
	${CMAKE_SOURCE_DIR}/Libraries/STM32CubeL4/Drivers/STM32L4xx_HAL_Driver/Src/*_ll*.c
)

# Workaround - Broken template files should not be compiled.
#Many template files in src files to show what code should look like...don't want to compile these
list(FILTER STM32CUBEL4_HAL_SOURCES EXCLUDE REGEX ".*_template.c")

# HAL Library
add_library(STM32CUBEL4_HAL STATIC
	${STM32CUBEL4_HAL_SOURCES}
)

#Only for DSP Library?
# add_library(ARM_MATH_LIB STATIC IMPORTED)
# set_target_properties(ARM_MATH_LIB PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/Modules/STM32CubeL4/Drivers/CMSIS/DSP/Lib/GCC/libarm_cortexM4lf_math.a)

# target_link_libraries(STM32CUBEL4_HAL PUBLIC ARM_MATH_LIB)

set(STM32CUBEL4_HAL_COMPILE_DEFINITIONS
	USE_HAL_DRIVER
	STM32L476xx
	# ARM_MATH_CM4
)

target_compile_definitions(STM32CUBEL4_HAL PUBLIC
	${STM32CUBEL4_HAL_COMPILE_DEFINITIONS}
)

target_include_directories(STM32CUBEL4_HAL SYSTEM
	PUBLIC ${STM32CUBEL4_HAL_INCLUDE_DIRECTORIES}
)

add_library(MAIN::HAL ALIAS STM32CUBEL4_HAL)