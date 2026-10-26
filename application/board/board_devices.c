#include "board_devices.h"
#include "mpu6500driver.h"
#include "imu_temperature.h"
#include "tim.h"
#include "spi.h"
#include "ins_task.h"

static MPU6500Instance *board_imu;
static bool started,ready;
static bool SampleIMU(void *context,IMUSample_s *sample) {return MPU6500Read(context,sample);}
static void HeatIMU(void *context,float duty) {(void)context;IMU_Temperature_Update(duty);}

bool BoardDevicesInit(void)
{
    if(started) return ready;
    started=true;
    const PWM_Init_Config_s heater={.htim=&htim3,.channel=TIM_CHANNEL_2,
        .period=.0001f,.dutyratio=.5f};
    if(!IMU_Temperature_Init(&heater)) return false;
    const MPU6500_Init_Config_s imu={.spi_config={.spi_handle=&hspi5,
        .GPIOx=GPIOF,.cs_pin=GPIO_PIN_6,.cs_mode=SPI_CS_GPIO,.spi_work_mode=SPI_BLOCK_MODE},
        .timeout_us=500};
    board_imu=MPU6500Register(&imu);
    ready=board_imu!=NULL;
    if(ready) {
        const INSServiceConfig_s service={.sample=SampleIMU,.heat=HeatIMU,.context=board_imu};
        ready=INSServiceConfigure(&service);
    }
    if(!ready) IMU_Temperature_Update(0);
    return ready;
}
