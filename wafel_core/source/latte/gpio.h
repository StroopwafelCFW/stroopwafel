#ifndef __GPIO_H__
#define __GPIO_H__

#include "types.h"

enum {
    GP_POWER        = 0,
    GP_SHUTDOWN     = 1,
    GP_FAN          = 2,
    GP_DCDC         = 3,
    GP_DISPIN       = 4,
    GP_SLOTLED      = 5,
    GP_EJECTBTN     = 6,
    GP_SLOTIN       = 7,
    GP_SENSORBAR    = 8,
    GP_DOEJECT      = 9,
    GP_EEP_CS       = 10,
    GP_EEP_CLK      = 11,
    GP_EEP_MOSI     = 12,
    GP_EEP_MISO     = 13,
    GP_AVE_SCL      = 14,
    GP_AVE_SDA      = 15,
    GP_DEBUG0       = 16,
    GP_DEBUG1       = 17,
    GP_DEBUG2       = 18,
    GP_DEBUG3       = 19,
    GP_DEBUG4       = 20,
    GP_DEBUG5       = 21,
    GP_DEBUG6       = 22,
    GP_DEBUG7       = 23,
    GP_AV1_I2C_CLK  = 24,
    GP_AV1_I2C_DAT  = 25,
};

enum {
    GP2_FANSPEED        = 0,
    GP2_SMC_I2C_CLK     = 1,
    GP2_SMC_I2C_DAT     = 2,
    GP2_DCDC2           = 3,
    GP2_AVINTERRUPT     = 4,
    GP2_CCRIO12         = 5,
    GP2_AVRESET         = 6,
};

#define GPIO_DIR_IN  (0)
#define GPIO_DIR_OUT (1)

#define GP_DEBUG_SHIFT 16
#define GP_DEBUG_MASK 0xFF0000

#define GP_DEBUG_SERIAL_MASK 0xBF0000 // bit1 is input

#define GP2_SMC (BIT(GP2_SMC_I2C_CLK)|BIT(GP2_SMC_I2C_DAT))

#define SERIAL_DELAY (1)

extern u8 test_read_serial;

void gpio_enable(u16 gpio_id, u8 val);
void gpio_set_dir(u16 gpio_id, u8 dir);

void gpio_dcdc_pwrcnt2_set(u8 val);
void gpio_dcdc_pwrcnt_set(u8 val);
void gpio_fan_set(u8 val);
void gpio_smc_i2c_init();
void gpio_ave_i2c_init();
void gpio_basic_set(u16 gpio_id, u8 val);
void gpio2_basic_set(u16 gpio_id, u8 val);
void gpio_debug_send(u8 val);
void gpio_debug_serial_send(u8 val);
u8 gpio_debug_serial_read();

#endif

