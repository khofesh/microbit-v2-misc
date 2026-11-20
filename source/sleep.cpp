#include "sleep.h"
#include "motion.h"
#include "MicroBit.h"

#include "LSM303Accelerometer.h"

extern MicroBit uBit;

#define LSM303_INT1_SRC_IA 0x40

static int savedCtrl2 = -1;
static int savedCtrl3 = -1;
static int savedInt1Cfg = -1;
static int savedInt1Ths = -1;
static int savedInt1Duration = -1;
static uint8_t wakeSrc;

static int writeRegister(uint8_t reg, uint8_t value)
{
    return uBit._i2c.writeRegister(LSM303_A_DEFAULT_ADDR, reg, value);
}

static int readRegister(uint8_t reg)
{
    uint8_t value = 0;

    if (uBit._i2c.readRegister(LSM303_A_DEFAULT_ADDR, reg, &value, 1) != 0)
        return -1;

    return value;
}

static void restoreRegister(int saved, uint8_t reg)
{
    if (saved >= 0)
        writeRegister(reg, (uint8_t)saved);
}

void sleep_enter(uint8_t sensitivity)
{
    const Thresholds &thresholds = motion_thresholds(sensitivity);

    int threshold = (thresholds.moveMg + 15) / 16;
    if (threshold < 1)
        threshold = 1;
    if (threshold > 127)
        threshold = 127;

    savedCtrl2 = readRegister(LSM303_CTRL_REG2_A);
    savedCtrl3 = readRegister(LSM303_CTRL_REG3_A);
    savedInt1Cfg = readRegister(LSM303_INT1_CFG_A);
    savedInt1Ths = readRegister(LSM303_INT1_THS_A);
    savedInt1Duration = readRegister(LSM303_INT1_DURATION_A);

    // High-pass filter on the interrupt path removes gravity; OR of X/Y/Z high events.
    writeRegister(LSM303_CTRL_REG2_A, 0x01);
    writeRegister(LSM303_INT1_THS_A, (uint8_t)threshold);
    writeRegister(LSM303_INT1_DURATION_A, 1);
    writeRegister(LSM303_INT1_CFG_A, 0x2a);
    writeRegister(LSM303_CTRL_REG3_A, 0x40);

    // Clear any pending interrupt.
    readRegister(LSM303_INT1_SRC_A);

    uBit.buttonA.wakeOnActive(1);
    uBit.buttonB.wakeOnActive(1);

    // Stop CODAL's accelerometer driver from disabling the sensor while asleep.
    uBit.accelerometer.removeComponent();

#if CONFIG_ENABLED(DEVICE_BLE) && CONFIG_ENABLED(MICROBIT_BLE_ENABLED)
    uBit.bleManager.stopAdvertising();
#endif

    uBit.power.deepSleep();
}

SleepWakeCause sleep_exit()
{
    int src = readRegister(LSM303_INT1_SRC_A);
    wakeSrc = src >= 0 ? (uint8_t)src : 0;

    bool buttonA = uBit.buttonA.isPressed();
    bool buttonB = uBit.buttonB.isPressed();

    restoreRegister(savedCtrl2, LSM303_CTRL_REG2_A);
    restoreRegister(savedInt1Cfg, LSM303_INT1_CFG_A);
    restoreRegister(savedInt1Ths, LSM303_INT1_THS_A);
    restoreRegister(savedInt1Duration, LSM303_INT1_DURATION_A);
    restoreRegister(savedCtrl3, LSM303_CTRL_REG3_A);

    readRegister(LSM303_INT1_SRC_A);

    uBit.buttonA.wakeOnActive(0);
    uBit.buttonB.wakeOnActive(0);

    uBit.accelerometer.addComponent();

    if (buttonA)
        return SLEEP_WAKE_BUTTON_A;

    if (buttonB)
        return SLEEP_WAKE_BUTTON_B;

    if (wakeSrc & LSM303_INT1_SRC_IA)
        return SLEEP_WAKE_MOTION;

    return SLEEP_WAKE_UNKNOWN;
}
