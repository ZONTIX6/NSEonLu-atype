cmake_minimum_required(VERSION 3.16)
# 指定编译平台/架构与语言标准, 推荐指定Ninja为构建工具,可以加快编译速度(相比make)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
# 指定工具链
set(CMAKE_C_COMPILER_FORCED TRUE) # skip compiler test
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size) 
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

# 指定工程名称和语言类型
project(basic_framework C ASM) 

# 选择构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
endif()

# board specific settings, arch/fpu/instruction
set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16)
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/STM32F427IIHx_FLASH.ld") # 指定链接脚本
set(DSP_NAME "libCMSISDSP.a") # 指定DSP库名称
link_directories(${CMAKE_SOURCE_DIR}/Middlewares/ST/ARM/DSP/Lib)

# Generic compiler settings for optimization and basic link lib
add_compile_options(-pipe ${MCU_FLAGS} -Wall -Werror -fmessage-length=0 # basic options
                    -ffunction-sections -fdata-sections -fno-common -fstack-usage # optimize options
                    )
add_link_options(-pipe ${MCU_FLAGS} -T${LINKER_SCRIPT} -Wl,--no-warn-rwx-segments # close RWX warning
                 -Wl,--gc-sections -flto -specs=nano.specs -specs=nosys.specs # optimize options
                 -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map -Wl,--cref -Wl,--print-memory-usage # map options
                 ) # if your executable is too large , try option '-s' to strip symbols

# add_compile_definitions() works for compile stage
# while add_definitions() works for both compile and link stage
add_definitions(
    -DUSE_HAL_DRIVER 
    -DSTM32F427xx
    -DARM_MATH_CM4 
    ) # need -D<macro> to define macro

set(ATYPE_PROFILE "atype-vehicle" CACHE STRING "A-board profile")
if(NOT ATYPE_PROFILE MATCHES "^atype-(vehicle|bench|direct)$")
    message(FATAL_ERROR "Invalid ATYPE_PROFILE")
endif()
if(DEFINED UART1_USE)
    message(FATAL_ERROR "UART1_USE is retired: USART1 is DBUS; use REFEREE_ENABLE and LOG_OUTPUT")
endif()
if(ATYPE_PROFILE STREQUAL "atype-bench")
    if(NOT DEFINED ATYPE_BENCH_MODE)
        set(ATYPE_BENCH_MODE 1)
    endif()
    if(NOT DEFINED CHASSIS_POWER_BENCH_TEST)
        set(CHASSIS_POWER_BENCH_TEST 1)
    endif()
endif()
if(NOT ATYPE_PROFILE STREQUAL "atype-vehicle" AND NOT DEFINED CHASSIS_POWER_SOURCE_MODE)
    set(CHASSIS_POWER_SOURCE_MODE 0)
endif()
file(STRINGS "${CMAKE_SOURCE_DIR}/config/99_build/atype-options.txt" ATYPE_OPTIONS)
if(NOT DEFINED CHASSIS_CONTROL_MODE)
    set(CHASSIS_CONTROL_MODE 0)
endif()
if(NOT CHASSIS_CONTROL_MODE MATCHES "^[01]$")
    message(FATAL_ERROR "CHASSIS_CONTROL_MODE must be 0 or 1")
endif()
if(NOT DEFINED CHASSIS_REMOTE_IMU_FUSION_ENABLE)
    set(CHASSIS_REMOTE_IMU_FUSION_ENABLE 0)
endif()
if(NOT DEFINED CHASSIS_WHEEL_REDUNDANCY_MODE)
    set(CHASSIS_WHEEL_REDUNDANCY_MODE 0)
endif()
if(NOT DEFINED CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE)
    set(CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE 0)
endif()
if(NOT DEFINED CHASSIS_TRACTION_MODE)
    set(CHASSIS_TRACTION_MODE 0)
endif()
if(NOT CHASSIS_TRACTION_MODE MATCHES "^[0123]$")
    message(FATAL_ERROR "CHASSIS_TRACTION_MODE must be 0, 1, 2 or 3")
endif()
if(NOT CHASSIS_TRACTION_MODE STREQUAL "0" AND
   (NOT ONE_BOARD STREQUAL "1" OR NOT CHASSIS_CONTROL_MODE STREQUAL "0" OR
    NOT CHASSIS_VELOCITY_FUSION_ENABLE STREQUAL "1" OR
    NOT CHASSIS_REMOTE_IMU_FUSION_ENABLE STREQUAL "1" OR
    NOT CHASSIS_VELOCITY_FUSION_CALIBRATED STREQUAL "0" OR
    NOT CHASSIS_IMU_CAN_RX_ENABLE STREQUAL "1" OR IMU_CAPTURE_ONLY STREQUAL "1"))
    message(FATAL_ERROR "Traction requires single-board speed control and the remote chassis IMU observer")
endif()
foreach(FUSION_OPTION IN ITEMS CHASSIS_VELOCITY_FUSION_ENABLE CHASSIS_VELOCITY_FUSION_CALIBRATED CHASSIS_REMOTE_IMU_FUSION_ENABLE CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE)
    if(DEFINED ${FUSION_OPTION} AND NOT "${${FUSION_OPTION}}" MATCHES "^[01]$")
        message(FATAL_ERROR "${FUSION_OPTION} must be 0 or 1")
    endif()
endforeach()
if(NOT CHASSIS_WHEEL_REDUNDANCY_MODE MATCHES "^[012]$")
    message(FATAL_ERROR "CHASSIS_WHEEL_REDUNDANCY_MODE must be 0, 1 or 2")
endif()
if(NOT CHASSIS_WHEEL_REDUNDANCY_MODE STREQUAL "0" AND
   (NOT ONE_BOARD STREQUAL "1" OR NOT CHASSIS_CONTROL_MODE STREQUAL "0" OR
    NOT CHASSIS_VELOCITY_FUSION_ENABLE STREQUAL "1" OR
    NOT CHASSIS_REMOTE_IMU_FUSION_ENABLE STREQUAL "1" OR
    NOT CHASSIS_IMU_CAN_RX_ENABLE STREQUAL "1"))
    message(FATAL_ERROR "Wheel redundancy requires the single-board remote chassis IMU observer")
endif()
if(CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE STREQUAL "1" AND
   (NOT ONE_BOARD STREQUAL "1" OR NOT CHASSIS_CONTROL_MODE STREQUAL "0" OR
    NOT CHASSIS_VELOCITY_FUSION_ENABLE STREQUAL "1" OR
    NOT CHASSIS_VELOCITY_FUSION_CALIBRATED STREQUAL "0" OR
    NOT CHASSIS_REMOTE_IMU_FUSION_ENABLE STREQUAL "1" OR
    NOT CHASSIS_IMU_CAN_RX_ENABLE STREQUAL "1" OR IMU_CAPTURE_ONLY STREQUAL "1"))
    message(FATAL_ERROR "Velocity-control trial requires the single-board remote-IMU speed observer")
endif()
foreach(IMU_OPTION IN ITEMS IMU_CAPTURE_ONLY IMU_CAPTURE_CAN_TX_ENABLE CHASSIS_IMU_CAN_RX_ENABLE)
    if(DEFINED ${IMU_OPTION} AND NOT "${${IMU_OPTION}}" MATCHES "^[01]$")
        message(FATAL_ERROR "${IMU_OPTION} must be 0 or 1")
    endif()
endforeach()
if(DEFINED VOFA_TELEMETRY_MODE AND NOT VOFA_TELEMETRY_MODE MATCHES "^[012]$")
    message(FATAL_ERROR "VOFA_TELEMETRY_MODE must be 0, 1 or 2")
endif()
if(DEFINED VOFA_OBSERVATION_PROFILE AND NOT VOFA_OBSERVATION_PROFILE MATCHES "^[0123]$")
    message(FATAL_ERROR "VOFA_OBSERVATION_PROFILE must be 0, 1, 2 or 3")
endif()
foreach(OPTION IN LISTS ATYPE_OPTIONS)
    if(DEFINED ${OPTION})
        add_compile_definitions(${OPTION}=${${OPTION}})
    endif()
endforeach()

# add inc
if(NOT DEFINED CHASSIS_FORCE_CALIBRATION_ENABLE)
    set(CHASSIS_FORCE_CALIBRATION_ENABLE 0)
endif()
if(NOT CHASSIS_FORCE_CALIBRATION_ENABLE MATCHES "^[01]$")
    message(FATAL_ERROR "CHASSIS_FORCE_CALIBRATION_ENABLE must be 0 or 1")
endif()
# 递归包含头文件的函数
function(include_sub_directories_recursively root_dir)
    if (IS_DIRECTORY ${root_dir})               # 当前路径是一个目录吗，是的话就加入到包含目录
        message("include dir: " ${root_dir})
        include_directories(${root_dir})
    endif()

    file(GLOB ALL_SUB RELATIVE ${root_dir} ${root_dir}/*) # 获得当前目录下的所有文件，让如ALL_SUB列表中
    foreach(sub ${ALL_SUB})
        if (IS_DIRECTORY ${root_dir}/${sub})
            include_sub_directories_recursively(${root_dir}/${sub}) # 对子目录递归调用，包含
        endif()
    endforeach()
endfunction()
include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/Drivers)
include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/Middlewares)
include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/bsp)
include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/modules)
include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/application)
include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/Inc)
include_sub_directories_recursively(${CMAKE_SOURCE_DIR}/config)
include_directories("${CMAKE_BINARY_DIR}")

# Match Make's CMSIS-DSP headers; recursive traversal also discovers legacy Inc/.
include_directories(BEFORE
    "${CMAKE_SOURCE_DIR}/Middlewares/ST/ARM/DSP/Include/dsp"
    "${CMAKE_SOURCE_DIR}/Middlewares/ST/ARM/DSP/Include")

# add source, only surfix .c
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "Drivers/*.c"
    "Src/*.c"
    "Middlewares/*.c"
    "bsp/*.c"
    "modules/*.c"
    "application/*.c"
    "config/*.c"
)

list(FILTER SOURCES EXCLUDE REGEX "/modules/imu/(ins_task|BMI088driver|BMI088Middleware)\\.c$")
if(NOT CHASSIS_FORCE_CALIBRATION_ENABLE STREQUAL "1")
    list(FILTER SOURCES EXCLUDE REGEX "/modules/chassis_force_calibration/")
    list(FILTER SOURCES EXCLUDE REGEX "/modules/motor/DJImotor/dji_force_calibration\\.c$")
    list(FILTER SOURCES EXCLUDE REGEX "/application/(board/board_force_calibration|runtime/force_calibration_app)\\.c$")
endif()
if(CHASSIS_CONTROL_MODE STREQUAL "1")
    list(FILTER SOURCES EXCLUDE REGEX "/config/[^/]+/chassis_(power|spin)_config\\.c$")
    list(FILTER SOURCES EXCLUDE REGEX "/application/chassis/(chassis|chassis_init|chassis_tuning)\\.c$")
    list(FILTER SOURCES EXCLUDE REGEX "/modules/algorithm/chassis_spin\\.c$")
    list(FILTER SOURCES EXCLUDE REGEX "/modules/motor/DJImotor/dji_power_adapter\\.c$")
else()
    list(FILTER SOURCES EXCLUDE REGEX "/(chassis_force_[^/]*|dji_force_adapter|chassis_power_force)\\.c$")
    list(FILTER SOURCES EXCLUDE REGEX "/application/board/board_chassis_force\\.c$")
endif()
foreach(NUMERIC_SOURCE IN LISTS SOURCES)
    if(NUMERIC_SOURCE MATCHES "/application/gimbal/")
        set_property(SOURCE "${NUMERIC_SOURCE}" APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math)
    endif()
    if(NUMERIC_SOURCE MATCHES "/config/")
        set_property(SOURCE "${NUMERIC_SOURCE}" APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math)
    endif()
    if(NUMERIC_SOURCE MATCHES "/modules/algorithm/(chassis_velocity_(fusion|center)|chassis_gimbal_imu|chassis_remote_fusion|chassis_wheel_redundancy|chassis_traction)\\.c$"
        OR (IMU_CAPTURE_ONLY STREQUAL "1" AND NUMERIC_SOURCE MATCHES "/modules/imu/ins_service\\.c$")
        OR (CHASSIS_VELOCITY_FUSION_ENABLE STREQUAL "1" AND NUMERIC_SOURCE MATCHES "/modules/(imu/ins_service|algorithm/chassis_velocity)\\.c$"))
        set_property(SOURCE "${NUMERIC_SOURCE}" APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math)
    endif()
    if(NUMERIC_SOURCE MATCHES "/application/runtime/shadow_jscope\\.c$")
        set_property(SOURCE "${NUMERIC_SOURCE}" APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math)
    endif()
    if(NUMERIC_SOURCE MATCHES "/(application/runtime/((robot|vofa)_telemetry|vofa_observation|vofa_traction|chassis_imu_can)|modules/imu/chassis_imu_can_protocol|modules/telemetry/usb_telemetry_(format|imu)|modules/super_cap/super_cap_protocol)\\.c$")
        # Keep finite checks on sensor/CAN data in optimized telemetry builds.
        set_property(SOURCE "${NUMERIC_SOURCE}" APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math)
    endif()
    if(NUMERIC_SOURCE MATCHES "/(modules/telemetry/chassis_speed_capture|application/runtime/chassis_speed_observation)\\.c$")
        set_property(SOURCE "${NUMERIC_SOURCE}" APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math)
    endif()
    if(NUMERIC_SOURCE MATCHES "/modules/predictive_power/")
        set_property(SOURCE "${NUMERIC_SOURCE}" APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math)
    endif()
    if(NUMERIC_SOURCE MATCHES "/(modules/algorithm/(chassis_power_|power_model|chassis_spin)|modules/chassis_force_calibration/|modules/chassis_force/|modules/chassis_power/|modules/motor/DJImotor/|modules/telemetry/(chassis_force_telemetry|force_calibration_protocol)|application/runtime/force_calibration_app|application/chassis/|application/board/(board_chassis_force|board_force_calibration)|application/cmd/robot_cmd)")
        set_property(SOURCE "${NUMERIC_SOURCE}" APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math)
    endif()
endforeach()

# 汇编文件路径
# ENABLE_LANGUAGE(ASM)
set(ASM_SOURCES
    startup_stm32f427xx.s
    Middlewares/Third_Party/SEGGER/RTT/SEGGER_RTT_ASM_ARMv7M.s
    )
set_source_files_properties(${ASM_SOURCES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

# Build types
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g -gdwarf-2)
    add_definitions(-DESC_DEBUG) # ESC Debug
endif ()

# build binary and hex file
add_executable(${PROJECT_NAME}.elf  ${SOURCES} ${ASM_SOURCES} ${LINKER_SCRIPT})
target_link_libraries(${PROJECT_NAME}.elf ${DSP_NAME} m c nosys)
if(ATYPE_LINK_MODULES)
    file(STRINGS "${CMAKE_SOURCE_DIR}/config/99_build/atype-link-symbols.txt" ATYPE_LINK_SYMBOLS)
    foreach(SYMBOL IN LISTS ATYPE_LINK_SYMBOLS)
        target_link_options(${PROJECT_NAME}.elf PRIVATE "-Wl,--undefined=${SYMBOL}")
    endforeach()
endif()

add_custom_command(
    TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Building hex & bin file..."
    COMMENT "EXCUTABLE SIZE:"
    COMMAND ${SIZE} ${PROJECT_NAME}.elf
  )
