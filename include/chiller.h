#ifndef CHILLER__H
#define CHILLER__H

// Дисплей Nokia 5110
//  LCD Nokia 5110 ARDUINO
//  1 RST 3
//  2 CE 4
//  3 DC 5
//  4 DIN 6
//  5 CLK 7
//  6 VCC 3.3V
//  7 LIGHT GND
//  8 GND GND
// ----------  Определение пинов устройств и переменных
//  На 9 пине датчик температуры.

#define LCD_RST 3
#define LCD_CE 4
#define LCD_DC 5
#define LCD_DIN 6
#define LCD_CLK 7
#define PIN_BACKLIGHT 10

// пин кнопки
#define PIN_BUTTON 11
// Джойстик Y
#define PIN_VRY A0
// Джойстик X
#define PIN_VRX A1
// пин зумера
#define PIN_PIZO 8
// пин датчика воды
#define PIN_WATER_FLOW 2
// пин датчика температуры DS
#define ONE_WIRE_BUS 9
#define TEMPERATURE_PRECISION 9
// пин на котором будет установлена 1 при аварии
#define PIN_EMERGENCY_STOP 12

// период между измерениями температуры и потока в милисекундах
#define MEASURE_PERIOD_LENGTH 1000

#define WATER_FLOW_INTERRUPT_NUMBER 0

// состояния приложения
#define MODE_MAIN_SCREEN 0
#define MODE_SET_TEMP_WARNING 1
#define MODE_SET_TEMP_ALARM 2
#define MODE_SET_FLOW_WARNING 3
#define MODE_SET_FLOW_ALARM 4
#define MODE_SET_TEMP_USE 5
#define MODE_SET_FLOW_USE 6
#define MODE_SET_SOUND_ENABLED 7
#define MODE_SET_START_TIMEOUT 8 

#endif
