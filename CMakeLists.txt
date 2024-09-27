# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

SET(PROJECT_NAME pico2-atari800)
set(FLASH_SIZE 4096)
SET(BUILD_NAME "${PROJECT_NAME}-${FLASH_SIZE}K")

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

project(pico2-atari800 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(drivers/nespad)
add_subdirectory(drivers/ps2)
add_subdirectory(drivers/fatfs)
add_subdirectory(drivers/psram)
add_subdirectory(drivers/sdcard)
add_subdirectory(drivers/vga-nextgen)
add_subdirectory(drivers/graphics)

#target_compile_definitions(${PROJECT_NAME} PRIVATE VGA)

# INCLUDE FILES THAT SHOULD BE COMPILED:
file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c"
#        "drivers/usb/*.c"
)

message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")

add_executable(${PROJECT_NAME} ${SRC})

# pull in common dependencies
target_link_libraries(pico2-atari800 PRIVATE
    ps2
    sdcard
    graphics
    psram
    fatfs
    nespad
    vga-nextgen

    pico_multicore
    pico_runtime
    pico_stdlib

    hardware_dma
    hardware_pio
    hardware_i2c
    hardware_interp
    hardware_timer
    hardware_clocks
    hardware_pwm
    hardware_flash

    stdc++
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb
)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(pico2-atari800 pico_cyw43_arch_none)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}")

pico_enable_stdio_uart(${PROJECT_NAME} 0)
##pico_enable_stdio_usb(${PROJECT_NAME} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

# add url via pico_set_program_url
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage)
target_compile_definitions(${PROJECT_NAME} PRIVATE FLASH_SIZE=${FLASH_SIZE})

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
target_compile_options(${PROJECT_NAME} PUBLIC
        -Wno-error=suggest-attribute=format
        -Wno-error=cast-qual
        -Wno-error=unused-parameter
        -Wno-error=conversion
        -Wno-error=format=
        -Wno-error=sign-compare
        -Wno-error=missing-field-initializers
        -Wno-error=switch
        -Wno-error=implicit-fallthrough=
        -Wno-error=stringop-truncation
        -Wno-error=restrict
        -w
)
endif ()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        PACKAGE_VERSION="3.2.0"

        PICO_FLASH_SIZE_BYTES=16777216

        # VGA 8 pins starts from pin:
        VGA_BASE_PIN=6

        # HDMI 8 pins starts from pin:
        HDMI_BASE_PIN=6

        # TFT
        TFT_CS_PIN=6
        TFT_RST_PIN=8
        TFT_LED_PIN=9
        TFT_DC_PIN=10
        TFT_DATA_PIN=12
        TFT_CLK_PIN=13

        # SDCARD
        SDCARD_PIN_SPI0_SCK=2
        SDCARD_PIN_SPI0_MOSI=3
        SDCARD_PIN_SPI0_MISO=4
        SDCARD_PIN_SPI0_CS=5

        # PS2 keyboard
        KBD_CLOCK_PIN=0
        KBD_DATA_PIN=1

        # NES Gamepad
        NES_GPIO_CLK=14
        NES_GPIO_DATA=16
        NES_GPIO_LAT=15
        
        USE_WII
        WII_SDA_PIN=14
        WII_SCL_PIN=15

        PSRAM
        # PSRAM_MUTEX=1
        PSRAM_SPINLOCK=1
        PSRAM_ASYNC=1

        PSRAM_PIN_CS=18
        PSRAM_PIN_SCK=19
        PSRAM_PIN_MOSI=20
        PSRAM_PIN_MISO=21

        LOAD_WAV_PIO=22
        PWM_PIN0=26
        PWM_PIN1=27
        BEEPER_PIN=28
        STEREO_SOUND=1
        CONSOLE_SOUND=1
        SERIO_SOUND=1
        VOICEBOX=1
        SOUND_THIN_API=1

        #MNGR_DEBUG=1
        #DEBUG2=1

        HAVE_OPENDIR=1
)

