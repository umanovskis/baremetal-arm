cmake_minimum_required (VERSION 2.8)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CROSSCOMPILING TRUE)

set(UBOOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../common_uboot")
set(MKIMAGE "${UBOOT_PATH}/tools/mkimage")

project (bare-metal-arm C ASM)

set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_ASM_COMPILER "arm-none-eabi-as")
set(CMAKE_OBJCOPY "arm-none-eabi-objcopy")

file(GLOB LINKSCRIPT "src/linkscript.ld")
set(ASMFILES src/startup.s)
set(SRCLIST src/cstart.c src/uart_pl011.c src/gic.c src/irq.c src/ptimer.c src/systime.c src/sched.c src/tasks.c src/syscall.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostartfiles -mthumb -mcpu=cortex-a9 -g -Og -Wall -DCPU_A9")
set(CMAKE_EXE_LINKER_FLAGS "-T ${LINKSCRIPT} -lgcc -lm")

add_custom_target(u-boot 
            COMMAND make vexpress_ca9x4_config ARCH=arm CROSS_COMPILE=arm-none-eabi- 
            COMMAND make all ARCH=arm CROSS_COMPILE=arm-none-eabi- 
            WORKING_DIRECTORY ${UBOOT_PATH})

add_executable(bare-metal ${SRCLIST} ${ASMFILES})
set_target_properties(bare-metal PROPERTIES OUTPUT_NAME "bare-metal.elf")
add_dependencies(bare-metal u-boot)

add_custom_command(TARGET bare-metal POST_BUILD COMMAND ${CMAKE_OBJCOPY}
    -O binary bare-metal.elf bare-metal.bin COMMENT "Converting ELF to binary")

add_custom_command(TARGET bare-metal POST_BUILD COMMAND ${MKIMAGE}
    -A arm -C none -T kernel -a 0x60000000 -e 0x60000000 -d bare-metal.bin bare-arm.uimg
    COMMENT "Building U-Boot image")

add_custom_command(TARGET bare-metal POST_BUILD COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/create-sd.sh 
    sdcard.img bare-arm.uimg
    COMMENT "Creating SD card image")

add_custom_target(run)
add_custom_command(TARGET run POST_BUILD COMMAND 
                 qemu-system-arm -M vexpress-a9 -m 512M -no-reboot -nographic 
                 -monitor telnet:127.0.0.1:1234,server,nowait -kernel ${UBOOT_PATH}/u-boot -sd sdcard.img -serial mon:stdio
                 COMMENT "Running QEMU...")

string(CONCAT GDBSCRIPT "target remote localhost:2159\n"
                        "file bare-metal.elf")
file(WRITE ${CMAKE_BINARY_DIR}/gdbscript ${GDBSCRIPT})


add_custom_target(drun)
add_custom_command(TARGET drun PRE_BUILD COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                    "To connect the debugger, run arm-none-eabi-gdb -x gdbscript")
add_custom_command(TARGET drun PRE_BUILD COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                    "To start execution, type continue in gdb")

add_custom_command(TARGET drun POST_BUILD COMMAND
                 qemu-system-arm -S -M vexpress-a9 -m 512M -no-reboot -nographic -gdb tcp::2159
                 -monitor telnet:127.0.0.1:1234,server,nowait -kernel ${UBOOT_PATH}/u-boot -sd sdcard.img -serial mon:stdio
                 COMMENT "Running QEMU with debug server...")
