#ifndef GLOBALS__H
#define GLOBALS__H

#include <stdint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <GyverButton.h>
#include "chiller.h"
#include "ChillerSettings.h"

bool isBackLightOn;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress waterThermometerAddr;

Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_CLK, LCD_DIN, LCD_DC, LCD_CE, LCD_RST);

// текущая температурв
uint16_t temp;

// настройки приложения
ChillerSettings settings;

// флаг аварийной остановки (чтобы можно было понять из за чего был остановлен станок)
bool isChillerHalt;

// declare reset fuction at address 0
void (*resetSystem)(void) = 0;

// Переменные потока датчика воды
volatile uint16_t pulse_frequency;
uint8_t litersPerHour, litersPerMinute;

GButton button(PIN_BUTTON);
GButton buttonUp;
GButton buttonDown;
GButton buttonLeft;
GButton buttonRight;

uint8_t mode = MODE_MAIN_SCREEN;

enum events
{
  eventNone,
  eventTempWarning,
  eventTempAlarm,
  eventFlowWarning,
  eventFlowAlarm
};

uint8_t event = eventNone;

bool isRedraw = true;

bool isTempWarning = false;
bool isTempAlarm = false;
bool isFlowWarning = false;
bool isFlowAlarm = false;

#endif
