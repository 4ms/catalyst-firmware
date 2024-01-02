cmake_minimum_required(VERSION 3.15.3)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi-gcc.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(f401 LANGUAGES C CXX ASM)

message("Configuring f401")

set(root ${CMAKE_SOURCE_DIR})

set(target f401)
set(TARGET_LINK_SCRIPT ${CMAKE_SOURCE_DIR}/linker/STM32F401CEUx_FLASH.ld)
set(ARCH_DEFINES STM32F401xE STM32F4 ARM_MATH_CM4)
set(ARCH_FLAGS -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb)

set(TARGET_INCLUDES
    ${root}/src/f401-drivers/
    ${root}/lib/mdrivlib/target/stm32f401
    ${root}/lib/mdrivlib/target/stm32f4xx
    ${root}/lib/CMSIS/Device/ST/STM32F4xx/Include
    ${root}/lib/STM32F4xx_HAL_Driver/Inc
)

set(TARGET_SOURCES
)

set(WAV_ENCODE_PYTHON_CMD
    # cmake-format: off
    # python3 ${CMAKE_SOURCE_DIR}/src/bootloader/stm_audio_bootloader/qpsk/encoder.py -s 48000 -b 12000 -c 6000 -p 256 -t stm32f4 -a 3 -g 131072 $<TARGET_FILE_DIR:${target}.elf>/${target}.bin
    # cmake-format: on
)

set_hal_sources(HAL_SOURCES stm32f4)
create_target(${target} stm32f4xx)
