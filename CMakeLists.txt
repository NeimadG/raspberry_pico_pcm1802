
cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

include(pico_extras_import_optional.cmake)

project(rpico_pcm1802 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

message ("PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
set(MONITOR_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

include(rpico_pcm1802_auto_set_url.cmake)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )
        
        
add_executable(rpico_pcm1802
        rpico_pcm1802.cpp
		pcm1802_pio/pio_pcm1802.cpp
        )

pico_generate_pio_header(rpico_pcm1802 ${CMAKE_CURRENT_LIST_DIR}/pcm1802_pio/pio_pcm1802.pio)

pico_enable_stdio_usb(rpico_pcm1802 1)
pico_enable_stdio_uart(rpico_pcm1802 1)

target_link_libraries(rpico_pcm1802
                      pico_stdlib
                      hardware_adc
                      hardware_pwm
                      hardware_spi
                      hardware_dma
                      pico_float
                      pico_multicore
                      hardware_pio
                      )

# create map/bin/hex file etc.
pico_add_extra_outputs(rpico_pcm1802)

# add url via pico_set_program_url
rpico_pcm1802_auto_set_url(rpico_pcm1802)
