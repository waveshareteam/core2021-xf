# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.18)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(04_lr2021_tx_cw C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_compile_options(
    -Wall
    -Wno-format
    -Wno-unused-function
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../../libraries/RadioLib" "${CMAKE_CURRENT_BINARY_DIR}/RadioLib")

add_executable(${PROJECT_NAME}
    main.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../libraries/RadioLib/src/hal/RPiPico/PicoHal.cpp"
)
target_compile_definitions(${PROJECT_NAME} PUBLIC RADIOLIB_BUILD_RPI_PICO)

# Pull in common dependencies
target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_spi hardware_gpio hardware_timer pico_multicore hardware_pwm RadioLib)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})
