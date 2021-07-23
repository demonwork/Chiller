#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <SPI.h>
#include "images.h"
#include <DallasTemperature.h>
#include <Wire.h>
#include <GyverButton.h>
#include <EEPROM.h>

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

#define pinButton 11 // пин кнопки
#define pinVRX A0    // Джойстик Х
#define pinVRY A1    // Джойстик Y
#define pizoPin 8    // пин зумера

// период между измерениями температуры и потока в милисекундах
#define MEASURE_PERIOD_LENGTH 1000

// пин датчика температуры DS
#define ONE_WIRE_BUS 9
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress waterThermometerAddr;
int TEMPERATURE_PRECISION = 9;

Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_CLK, LCD_DIN, LCD_DC, LCD_CE, LCD_RST);

unsigned long timeLoopAlarm; // Время для таймера моргающего экраном аларма
const int waterFlowPin = 2;  // пин датчика воды

// текущая температурв
float temp;
// значение опасно высокой температуры
uint8_t tempWarning;
// значение аварийно высокой температуры
uint8_t tempAlarm;
// значение опасно низкого потока
uint8_t flowWarning;
// значение аварийно низкого потока
uint8_t flowAlarm;

// Переменные потока датчика воды
volatile uint16_t pulse_frequency;
uint8_t litersPerHour, litersPerMinute;
uint64_t currentTime;
uint16_t measuredPeriod, currentTimePrev;
uint8_t waterFlowInterruptNumber = 0;

GButton button(pinButton);
#define MODE_MAIN_SCREEN 0
#define MODE_SET_TEMP_WARNING 1
#define MODE_SET_TEMP_ALARM 2
#define MODE_SET_FLOW_WARNING 3
#define MODE_SET_FLOW_ALARM 4
uint8_t mode = MODE_MAIN_SCREEN;

#define SETTINGS_ADDR_TEMP_WARNING 0
#define SETTINGS_ADDR_TEMP_ALARM 1
#define SETTINGS_ADDR_FLOW_WARNING 2
#define SETTINGS_ADDR_FLOW_ALARM 3

enum events
{
  eventNone,
  eventTempWarning,
  eventTempAlarm,
  eventFlowWarning,
  eventFlowAlarm
};

uint8_t event = eventNone;

/**
 *  функция полученя данных с датчика потока, работает по прерыванию
 */
void waterFlowInterruptHandler()
{
  pulse_frequency++;
}

/**
 * Функция вывода на экран
 */
int displayMainScreen(int f, float t)
{

  display.drawBitmap(0, 0, heatImg, 24, 20, 1);
  display.setTextSize(3);
  display.setCursor(26, 0);
  display.println(t, 0);
  display.setTextSize(1);
  display.setCursor(63, 0);
  display.println("o");
  display.setTextSize(2);
  display.setCursor(67, 7);
  display.println("C");
  //
  // display.setTextSize(1);
  display.drawBitmap(0, 22, flowImg, 24, 15, f); // удобно f ставить, если = 0 то иконки нет.
  display.setTextSize(2);
  display.setCursor(26, 22);
  display.println(f, DEC);
  //
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("[T=");
  display.print(tempAlarm);
  display.print("]");
  display.print("[F=");
  display.print(flowAlarm);
  display.print("]");

  display.display();
  display.clearDisplay();

  return 0;
}

/**
 * Функция вывода сигнализации на экран
 */
void displayAlarm(uint8_t event, int f, int t)
{
  display.clearDisplay();
  // зажигает подсветку.
  digitalWrite(10, LOW);

  switch (event)
  {
  case eventTempWarning:
    tone(pizoPin, 500, 500);
    display.drawBitmap(0, 15, exclaimImg, 24, 20, 1);
    display.setTextSize(1);
    display.setCursor(10, 0);
    display.println("CHECK TEMP!");
    display.setTextSize(3);
    display.setCursor(28, 15);
    display.println(t);
    display.setTextSize(1);
    display.setCursor(65, 12);
    display.println("o");
    display.setTextSize(2);
    display.setCursor(68, 20);
    display.println("C");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("STOP WORK NOW!");
    display.display();
    break;

  case eventTempAlarm:
    tone(pizoPin, 500, 500);
    // зажигает подсветку.
    digitalWrite(10, LOW);
    display.clearDisplay();
    display.drawBitmap(0, 15, exclaimImg, 24, 20, 1);
    display.setTextSize(1);
    display.setCursor(10, 0);
    display.println("HIGH TEMP!");
    display.setTextSize(3);
    display.setCursor(28, 15);
    display.println(t);
    display.setTextSize(1);
    display.setCursor(65, 12);
    display.println("o");
    display.setTextSize(2);
    display.setCursor(68, 20);
    display.println("C");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("STOP WORK NOW!");
    display.display();
    break;

  case eventFlowWarning:
    tone(pizoPin, 500, 500);
    // зажигает подсветку.
    digitalWrite(10, LOW);
    display.clearDisplay();
    display.drawBitmap(0, 15, exclaimImg, 24, 20, 1);
    display.setTextSize(1);
    display.setCursor(10, 0);
    display.println("CHECK FLOW!");
    display.setTextSize(3);
    display.setCursor(28, 15);
    display.println(t);
    display.setTextSize(1);
    display.setCursor(65, 12);
    display.println("o");
    display.setTextSize(2);
    display.setCursor(68, 20);
    display.println("C");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("STOP WORK NOW!");
    display.display();
    break;

  case eventFlowAlarm:
    tone(pizoPin, 500, 500);
    // зажигает подсветку.
    digitalWrite(10, LOW);
    display.clearDisplay();
    display.drawBitmap(0, 15, exclaimImg, 24, 20, 1);
    display.setTextSize(1);
    display.setCursor(10, 0);
    display.println("LOW FLOW!");
    display.setTextSize(3);
    display.setCursor(28, 15);
    display.println(t);
    display.setTextSize(1);
    display.setCursor(65, 12);
    display.println("o");
    display.setTextSize(2);
    display.setCursor(68, 20);
    display.println("C");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("STOP WORK NOW!");
    display.display();
    break;
  }

  // if (millis() - timeLoopAlarm >= 3000)
  // {
  //   // гасим экран.
  //   digitalWrite(10, HIGH);
  //   timeLoopAlarm = millis();
  // }
}

void displaySetValue(const char *title, uint8_t value)
{
  // зажигает подсветку.
  digitalWrite(10, LOW);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(title);
  display.setTextSize(2);
  display.setCursor(5, 15);
  display.print("- ");
  display.print(value, DEC);
  if (value < 100)
  {
    display.print(" ");
  }

  display.print("+");
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println("Click to save");
  display.display();
}

void readJoystickValue(uint8_t *value)
{
  // Обработка событий джойстика
  if (analogRead(pinVRY) > 1000)
  {
    // Если стик вправо то ++
    (*value)++;
    delay(200);
  }

  if (analogRead(pinVRY) < 400)
  {
    // Если стик влево то --
    (*value)--;
    delay(200);
  }
}

void readJoystickMainScreen()
{
  if (analogRead(pinVRX) > 1000)
  {
    // зажигает подсветку.
    digitalWrite(10, LOW);
  }

  if (analogRead(pinVRX) < 400)
  {
    // зажигает подсветку.
    digitalWrite(10, HIGH);
  }
}

/**
 * Основная функция, запрашивает поток и температуру, а затем отсылает на дисплей.
 */
void rootSys()
{
  currentTime = millis();
  measuredPeriod = currentTime - currentTimePrev;
  if (measuredPeriod >= MEASURE_PERIOD_LENGTH)
  {
    currentTimePrev = currentTime;
    litersPerMinute = (pulse_frequency / (measuredPeriod / MEASURE_PERIOD_LENGTH)) / 7.5f;
    litersPerHour = litersPerMinute * 60;
#ifdef DEBUG
    litersPerHour = 113;
#endif
    pulse_frequency = 0;

    // дёргаем датчик температуры и забираем данные.
    sensors.requestTemperatures();
    temp = sensors.getTempC(waterThermometerAddr);

    if (temp >= tempWarning && temp < tempAlarm)
    {
      event = eventTempWarning;
    }
    else if (temp >= tempAlarm)
    {
      event = eventTempAlarm;
    }
    else if (litersPerHour > flowAlarm && litersPerHour <= flowWarning)
    {
      event = eventFlowWarning;
    }
    else if (litersPerHour <= flowAlarm)
    {
      event = eventFlowAlarm;
    }
    else
    {
      event = eventNone;
    }
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Chiller start.");

  // инициализируем либу для работы с датчиками температуры
  sensors.begin();
  sensors.getAddress(waterThermometerAddr, 0);
  sensors.setResolution(waterThermometerAddr, TEMPERATURE_PRECISION);

  pinMode(waterFlowPin, INPUT);
  // прерывание на пине к которому подключен датчик воды, дёргает когда приходят данные
  attachInterrupt(waterFlowInterruptNumber, waterFlowInterruptHandler, FALLING);

  // Проверяем данные и выводим без задержки сравнивая время.
  currentTime = millis();
  measuredPeriod = currentTimePrev = currentTime;

  // LCD дисплей, инициализация и заставка
  display.begin();
  display.setContrast(20);
  display.clearDisplay();
  display.drawBitmap(2, 0, logoImg, 80, 48, 1);
  // показываем заставку
  display.display();
  delay(1000);
  // очищаем экран и буфер
  display.clearDisplay();
  // установка цвета текста
  display.setTextColor(BLACK);
  // подсветка экрана
  pinMode(10, OUTPUT);
  // digitalWrite(10, LOW);

  button.setTickMode(AUTO);

  uint8_t settings = 0;
  settings = EEPROM.read(SETTINGS_ADDR_TEMP_WARNING);
  tempWarning = settings == 0 ? 20 : settings;

  settings = EEPROM.read(SETTINGS_ADDR_TEMP_ALARM);
  tempAlarm = settings == 0 ? 30 : settings;

  settings = EEPROM.read(SETTINGS_ADDR_FLOW_WARNING);
  flowWarning = settings == 0 ? 90 : settings;

  settings = EEPROM.read(SETTINGS_ADDR_FLOW_ALARM);
  flowAlarm = settings == 0 ? 100 : settings;

  // Пищим при запуске.
  tone(pizoPin, 400, 300);
  delay(300);
  tone(pizoPin, 800, 300);
  delay(300);
  tone(pizoPin, 1500, 300);
}

void loop()
{
  bool isClick = button.isClick();
  if (mode == MODE_MAIN_SCREEN && isClick)
  {
    mode = MODE_SET_TEMP_WARNING;
    isClick = false;
  }

  if (mode == MODE_SET_TEMP_WARNING && isClick)
  {
    mode = MODE_SET_TEMP_ALARM;
    isClick = false;
    EEPROM.update(SETTINGS_ADDR_TEMP_WARNING, tempWarning);
    display.clearDisplay();
  }

  if (mode == MODE_SET_TEMP_ALARM && isClick)
  {
    mode = MODE_SET_FLOW_WARNING;
    isClick = false;
    EEPROM.update(SETTINGS_ADDR_TEMP_ALARM, tempAlarm);
    display.clearDisplay();
  }

  if (mode == MODE_SET_FLOW_WARNING && isClick)
  {
    mode = MODE_SET_FLOW_ALARM;
    isClick = false;
    EEPROM.update(SETTINGS_ADDR_FLOW_WARNING, flowWarning);
    display.clearDisplay();
  }

  if (mode == MODE_SET_FLOW_ALARM && isClick)
  {
    mode = MODE_MAIN_SCREEN;
    isClick = false;
    EEPROM.update(SETTINGS_ADDR_FLOW_ALARM, flowAlarm);
    display.clearDisplay();
  }

  rootSys();

  switch (mode)
  {
  case MODE_MAIN_SCREEN:
    readJoystickMainScreen();
    if (event == eventNone)
    {
      displayMainScreen(litersPerHour, temp);
    }
    else
    {
      displayAlarm(event, litersPerHour, temp);
    }
    break;
  case MODE_SET_TEMP_WARNING:
    readJoystickValue(&tempWarning);
    displaySetValue("Set temp [TW]", tempWarning);
    break;
  case MODE_SET_TEMP_ALARM:
    readJoystickValue(&tempAlarm);
    displaySetValue("Set temp [TA]", tempAlarm);
    break;
  case MODE_SET_FLOW_WARNING:
    readJoystickValue(&flowWarning);
    displaySetValue("Set flow [FW]", flowWarning);
    break;
  case MODE_SET_FLOW_ALARM:
    readJoystickValue(&flowAlarm);
    displaySetValue("Set flow [FA]", flowAlarm);
    break;
  }
}
