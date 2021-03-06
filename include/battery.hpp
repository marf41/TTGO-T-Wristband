#include <Arduino.h>
#include <esp_adc_cal.h>
#include "hal.hpp"

#define BATTERY_MIN_V 3.3
#define BATTERY_MAX_V 4.2

void setupADC();
void setupBattery();
float getVoltage();
float getBusVoltage();
uint8_t calcPercentage(float volts);
void updateBatteryChargeStatus();
bool isCharging();
