CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
SOURCE := test_vofa_telemetry.c ../../application/runtime/vofa_telemetry.c
HEADERS := stubs/usart.h ../../application/runtime/vofa_telemetry.h ../../Inc/board_config.h ../../config/90_board/board_config.h ../../config/_vofa/vofa_config.h ../../Inc/imu_capture_config.h ../../config/32_capture/imu_capture_config.h ../../modules/telemetry/usb_telemetry_format.h ../../application/chassis/chassis_velocity_observer.h
HEADERS += ../../modules/motor/DJImotor/dji_motor_diagnostics.h
INCLUDES := -DVOFA_OBSERVATION_PROFILE=0 -Istubs -I../../Inc -I../../application/runtime -I../../modules/telemetry -I../../modules/algorithm
MEASUREMENT_SOURCE := test_vofa_measurement.c ../../application/runtime/vofa_telemetry.c ../../modules/telemetry/usb_telemetry_format.c
MEASUREMENT_INCLUDES := -I../../modules/chassis_power -I../../modules/power_meter -I../../modules/referee -I../../modules/super_cap -I../../application/chassis

.PHONY: test
test: test_observation_power.exe test_observation_kalman.exe test_observation_remote.exe

OBSERVATION_SOURCE := test_observation.c ../../application/runtime/vofa_observation.c
OBSERVATION_HEADERS := stubs/FreeRTOS.h stubs/task.h ../../application/runtime/vofa_schema.h ../../application/runtime/vofa_observation.h ../../application/runtime/vofa_kalman_capture.h ../../modules/algorithm/chassis_remote_fusion.h ../../modules/chassis_power/chassis_power_types.h ../../modules/super_cap/super_cap_types.h ../../modules/referee/referee_power.h
test_observation_power.exe: $(OBSERVATION_SOURCE) $(OBSERVATION_HEADERS) $(HEADERS)
	$(CC) $(CFLAGS) -fno-fast-math $(filter-out -DVOFA_OBSERVATION_PROFILE=0,$(INCLUDES)) $(MEASUREMENT_INCLUDES) -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=2 -DVOFA_OBSERVATION_PROFILE=1 $(OBSERVATION_SOURCE) -o $@

test_observation_kalman.exe: $(OBSERVATION_SOURCE) $(OBSERVATION_HEADERS) $(HEADERS)
	$(CC) $(CFLAGS) -fno-fast-math $(filter-out -DVOFA_OBSERVATION_PROFILE=0,$(INCLUDES)) $(MEASUREMENT_INCLUDES) -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=2 -DVOFA_OBSERVATION_PROFILE=2 $(OBSERVATION_SOURCE) -o $@

test_observation_remote.exe: $(OBSERVATION_SOURCE) $(OBSERVATION_HEADERS) $(HEADERS)
	$(CC) $(CFLAGS) -fno-fast-math $(filter-out -DVOFA_OBSERVATION_PROFILE=0,$(INCLUDES)) $(MEASUREMENT_INCLUDES) -I../../modules/imu -DONE_BOARD -DCHASSIS_VELOCITY_FUSION_ENABLE=1 -DCHASSIS_IMU_CAN_RX_ENABLE=1 -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=2 -DVOFA_OBSERVATION_PROFILE=2 $(OBSERVATION_SOURCE) -o $@

CAPTURE_SOURCE := test_kalman_capture.c ../../application/runtime/vofa_observation.c
test_kalman_capture.exe: $(CAPTURE_SOURCE) $(OBSERVATION_HEADERS) $(HEADERS)
	$(CC) $(CFLAGS) -fno-fast-math $(filter-out -DVOFA_OBSERVATION_PROFILE=0,$(INCLUDES)) $(MEASUREMENT_INCLUDES) -I../../modules/imu -DONE_BOARD -DCHASSIS_VELOCITY_FUSION_ENABLE=1 -DCHASSIS_VELOCITY_FUSION_CALIBRATED=0 -DCHASSIS_IMU_CAN_RX_ENABLE=1 -DCHASSIS_REMOTE_IMU_FUSION_ENABLE=1 -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=2 -DVOFA_OBSERVATION_PROFILE=2 $(CAPTURE_SOURCE) -lm -o $@

test: test_kalman_capture.exe

test: test_vofa_telemetry.exe test_vofa_measurement.exe test_imu_capture.exe test_vofa_remote.exe test_vofa_remote_fusion.exe test_vofa_remote_verified.exe test_measurement_justfloat.exe test_imu_justfloat.exe test_remote_justfloat.exe test_remote_fusion_justfloat.exe test_remote_verified_justfloat.exe test_measurement_transport.exe
	./test_vofa_telemetry.exe
	./test_vofa_measurement.exe
	./test_imu_capture.exe
	./test_vofa_remote.exe
	./test_vofa_remote_fusion.exe
	./test_vofa_remote_verified.exe
	./test_measurement_justfloat.exe
	./test_imu_justfloat.exe
	./test_remote_justfloat.exe
	./test_remote_fusion_justfloat.exe
	./test_remote_verified_justfloat.exe
	./test_measurement_transport.exe
	./test_observation_power.exe
	./test_observation_kalman.exe
	./test_observation_remote.exe
	./test_kalman_capture.exe startup
	./test_kalman_capture.exe retry
	./test_kalman_capture.exe overflow
	./test_kalman_capture.exe jitter
	./test_kalman_capture.exe heartbeat
	./test_kalman_capture.exe wrap
	./test_kalman_capture.exe mode

test_vofa_telemetry.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=0 $(SOURCE) -o $@

test_vofa_measurement.exe: $(MEASUREMENT_SOURCE) $(HEADERS) ../../modules/chassis_power/chassis_power_types.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=1 $(MEASUREMENT_SOURCE) -o $@

IMU_SOURCE := test_imu_capture.c ../../application/runtime/robot_telemetry.c ../../application/runtime/vofa_telemetry.c ../../modules/telemetry/usb_telemetry_format.c
test_imu_capture.exe: $(IMU_SOURCE) $(HEADERS) ../../modules/imu/ins_task.h ../../Inc/imu_capture_config.h ../../config/32_capture/imu_capture_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -I../../modules/imu -DIMU_CAPTURE_ONLY=1 -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=1 -DUSB_TELEMETRY_ENABLE=0 $(IMU_SOURCE) -o $@

REMOTE_SOURCE := test_vofa_remote.c ../../application/runtime/vofa_telemetry.c ../../modules/telemetry/usb_telemetry_format.c
REMOTE_HEADERS := $(HEADERS) ../../application/runtime/chassis_imu_can.h ../../modules/imu/chassis_imu_sample.h
REMOTE_FLAGS := -DONE_BOARD -DCHASSIS_CONTROL_MODE=0 -DCHASSIS_VELOCITY_FUSION_ENABLE=1 -DCHASSIS_VELOCITY_FUSION_CALIBRATED=0 -DCHASSIS_IMU_CAN_RX_ENABLE=1 -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=1
test_vofa_remote.exe: $(REMOTE_SOURCE) $(REMOTE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -I../../modules/imu $(REMOTE_FLAGS) -DCHASSIS_REMOTE_IMU_FUSION_ENABLE=0 $(REMOTE_SOURCE) -o $@

test_vofa_remote_fusion.exe: $(REMOTE_SOURCE) $(REMOTE_HEADERS) ../../config/30_velocity_observation/chassis_remote_imu_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -I../../modules/imu $(REMOTE_FLAGS) -DCHASSIS_REMOTE_IMU_FUSION_ENABLE=1 $(REMOTE_SOURCE) -o $@

test_vofa_remote_verified.exe: $(REMOTE_SOURCE) $(REMOTE_HEADERS) ../../config/30_velocity_observation/chassis_remote_imu_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -I../../modules/imu $(REMOTE_FLAGS) -DCHASSIS_REMOTE_IMU_FUSION_ENABLE=1 -DCHASSIS_REMOTE_IMU_CONFIG_VERIFIED=1 $(REMOTE_SOURCE) -o $@

test_measurement_justfloat.exe: $(MEASUREMENT_SOURCE) $(HEADERS) justfloat_decode.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=2 $(MEASUREMENT_SOURCE) -o $@

test_imu_justfloat.exe: $(IMU_SOURCE) $(HEADERS) justfloat_decode.h ../../modules/imu/ins_task.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -I../../modules/imu -DIMU_CAPTURE_ONLY=1 -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=2 -DUSB_TELEMETRY_ENABLE=0 $(IMU_SOURCE) -o $@

JUSTFLOAT_REMOTE_FLAGS := $(filter-out -DVOFA_TELEMETRY_MODE=1,$(REMOTE_FLAGS)) -DVOFA_TELEMETRY_MODE=2
test_remote_justfloat.exe: $(REMOTE_SOURCE) $(REMOTE_HEADERS) justfloat_decode.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -I../../modules/imu $(JUSTFLOAT_REMOTE_FLAGS) -DCHASSIS_REMOTE_IMU_FUSION_ENABLE=0 $(REMOTE_SOURCE) -o $@

test_remote_fusion_justfloat.exe: $(REMOTE_SOURCE) $(REMOTE_HEADERS) justfloat_decode.h ../../config/30_velocity_observation/chassis_remote_imu_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -I../../modules/imu $(JUSTFLOAT_REMOTE_FLAGS) -DCHASSIS_REMOTE_IMU_FUSION_ENABLE=1 $(REMOTE_SOURCE) -o $@

test_remote_verified_justfloat.exe: $(REMOTE_SOURCE) $(REMOTE_HEADERS) justfloat_decode.h ../../config/30_velocity_observation/chassis_remote_imu_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -I../../modules/imu $(JUSTFLOAT_REMOTE_FLAGS) -DCHASSIS_REMOTE_IMU_FUSION_ENABLE=1 -DCHASSIS_REMOTE_IMU_CONFIG_VERIFIED=1 $(REMOTE_SOURCE) -o $@

TRANSPORT_SOURCE := test_measurement_transport.c ../../application/runtime/vofa_telemetry.c ../../modules/telemetry/usb_telemetry_format.c
test_measurement_transport.exe: $(TRANSPORT_SOURCE) $(HEADERS) justfloat_decode.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=2 $(TRANSPORT_SOURCE) -o $@
