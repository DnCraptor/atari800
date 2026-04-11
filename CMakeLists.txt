# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")
cmake_minimum_required(VERSION 3.28)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

set(PACKAGE_VERSION "3.3.1")
set(CPU_MHZ 378)

#set(m1p2_launcher ON)
#set(M2 ON)
set(HDMI ON)

SET(PROJECT_NAME p2-atari800)
if (M2)
    SET(PROJECT_NAME "m2${PROJECT_NAME}")
else()
    SET(PROJECT_NAME "m1${PROJECT_NAME}")
endif()
set(FLASH_SIZE 4096)
if (HDMI)
    SET(BUILD_NAME "${PROJECT_NAME}-HDMI-PWM-${CPU_MHZ}-${PACKAGE_VERSION}")
else ()
    SET(BUILD_NAME "${PROJECT_NAME}-VGA-PWM-${CPU_MHZ}-${PACKAGE_VERSION}")
endif ()

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

project(${PROJECT_NAME} C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(drivers/nespad)
add_subdirectory(drivers/ps2)
add_subdirectory(drivers/fatfs)
add_subdirectory(drivers/psram)
add_subdirectory(drivers/sdcard)
add_subdirectory(drivers/graphics)
if (HDMI)
    add_subdirectory(drivers/hdmi)
else ()
    add_subdirectory(drivers/vga-nextgen)
endif ()

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
target_link_libraries(${PROJECT_NAME} PRIVATE
    ps2
    sdcard
    graphics
    psram
    fatfs
    nespad

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

if (HDMI)
    target_link_libraries(${PROJECT_NAME} PRIVATE hdmi)
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE vga-nextgen)
endif ()

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb
)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(${PROJECT_NAME} pico_cyw43_arch_none)
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

if (HDMI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HDMI=1)
else ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE VGA=1)
endif ()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        PACKAGE_VERSION="${PACKAGE_VERSION}"
        CPU_MHZ=${CPU_MHZ}
        PICO_FLASH_SIZE_BYTES=16777216

        STEREO_SOUND=1
        CONSOLE_SOUND=1
        SERIO_SOUND=1
        VOICEBOX=1
        SOUND_THIN_API=1
        SOUND=1
        
        HAVR_FF_WRAP_H=1

        #MNGR_DEBUG=1
        #DEBUG2=1

        HAVE_OPENDIR=1
)

if (M2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        MURM2=1
        # VGA 8 pins starts from pin:
        VGA_BASE_PIN=12
        # HDMI 8 pins starts from pin:
        HDMI_BASE_PIN=12

        # TFT
        TFT_CS_PIN=12
        TFT_RST_PIN=14
        TFT_LED_PIN=15
        TFT_DC_PIN=16
        TFT_DATA_PIN=18
        TFT_CLK_PIN=19

        # SDCARD
        SDCARD_PIN_SPI0_CS=5
        SDCARD_PIN_SPI0_SCK=6
        SDCARD_PIN_SPI0_MOSI=7
        SDCARD_PIN_SPI0_MISO=4

        # PS2 keyboard
        KBD_CLOCK_PIN=2
        KBD_DATA_PIN=3

        # NES Gamepad
        NES_GPIO_CLK=20
        NES_GPIO_LAT=21
        NES_GPIO_DATA=26
        
        USE_WII
        WII_SDA_PIN=20
        WII_SCL_PIN=21

       # PSRAM
        # PSRAM_MUTEX=1
        PSRAM_SPINLOCK=1
        PSRAM_ASYNC=1

        PSRAM_PIN_CS=18
        PSRAM_PIN_SCK=19
        PSRAM_PIN_MOSI=20
        PSRAM_PIN_MISO=21

        LOAD_WAV_PIO=22
        BEEPER_PIN=9 #
        PWM_PIN0=10 #
        PWM_PIN1=11
    )
else ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE
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

       # PSRAM
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
    )
endif()

IF(m1p2_launcher)
    IF(M2)
        SET(BUILD_NAME "${BUILD_NAME}.m2p2")
    ELSEIF(PICO_PC)
        SET(BUILD_NAME "${BUILD_NAME}.PCp2")
    ELSEIF(PICO_DV)
        SET(BUILD_NAME "${BUILD_NAME}.DVp2")
    ELSEIF(ZERO2)
        SET(BUILD_NAME "${BUILD_NAME}.z0p2")
    ELSE()
        SET(BUILD_NAME "${BUILD_NAME}.m1p2")
    ENDIF()
ENDIF()

message(STATUS "BUILD_NAME: ${BUILD_NAME}")

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}")

if (m1p2_launcher)
    pico_set_linker_script(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/memmap.ld")
endif ()

if (m1p2_launcher)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename
        "${OUTPUT_DIR}/${BUILD_NAME}.uf2"
        "${OUTPUT_DIR}/${BUILD_NAME}"
    )
endif ()
