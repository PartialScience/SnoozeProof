cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(BedAlarm)
pico_sdk_init()

add_executable(Main)

#Generate header files from PIO ASM
pico_generate_pio_header(Main ${CMAKE_CURRENT_LIST_DIR}/Buzzer.pio)

target_sources(Main PRIVATE 
    main.c
    Buzzer.c
    SPI.c
    HC05.c
    PressureSensor.c
)

target_link_libraries(Main 
    pico_stdlib
    hardware_sync
    hardware_adc
    hardware_pio
    hardware_rtc
)

pico_add_extra_outputs(Main)
