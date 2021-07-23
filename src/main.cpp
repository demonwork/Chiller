#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <SPI.h>
#include "images.h"
#include <DallasTemperature.h>
#include <Wire.h>
#include <GyverButton.h>
#include <EEPROM.h>
#include <FastCRC.h>

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

#define SETTINGS_ADDR_CRC 0
#define SETTINGS_ADDR_TEMP_WARNING 2
#define SETTINGS_ADDR_TEMP_ALARM 3
#define SETTINGS_ADDR_FLOW_WARNING 4
#define SETTINGS_ADDR_FLOW_ALARM 5

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
int displayMainScreen()
{
  digitalWrite(10, LOW);

  display.clearDisplay();
  display.drawBitmap(0, 0, heatImg, 24, 20, 1);
  display.setTextSize(3);
  display.setCursor(26, 0);
  display.println(round(temp));
  display.setTextSize(1);
  display.setCursor(63, 0);
  display.println("o");
  display.setTextSize(2);
  display.setCursor(67, 7);
  display.println("C");
  //
  // display.setTextSize(1);
  display.drawBitmap(0, 22, flowImg, 24, 15, 1);
  display.setTextSize(2);
  display.setCursor(26, 22);
  display.println(litersPerHour, DEC);
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

  return 0;
}

void soundSiren()
{
  for (int i = 700; i < 800; i++)
  {
    tone(pizoPin, i);
    delay(15);
  }

  digitalWrite(10, HIGH);
  for (int i = 800; i > 700; i--)
  {
    tone(pizoPin, i);
    delay(15);
  }
}

void drawAlerInfo(const char *title, const char *footer, uint8_t value, uint8_t mode)
{
  display.clearDisplay();

  display.drawBitmap(0, 12, exclaimImg, 24, 20, 1);
  display.setTextSize(1);
  display.setCursor(10, 0);
  display.print(title);
  display.setTextSize(2);
  display.setCursor(28, 15);
  display.print(value);

  switch (mode)
  {
  case 0:
    display.setTextSize(1);
    display.print("o");
    display.setTextSize(2);
    display.print("C");
    break;

  case 1:
    display.setTextSize(1);
    display.print("lpm");
    break;

  default:
    break;
  }

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print(footer);

  display.display();
}

/**
 * Функция вывода сигнализации на экран
 */
void displayAlarm()
{
  // зажигает подсветку.
  digitalWrite(10, LOW);

  switch (event)
  {
  case eventTempWarning:
    // soundSiren();
    drawAlerInfo("CHECK TEMP!", "STOP WORK NOW!", round(temp), 0);
    break;

  case eventTempAlarm:
    // soundSiren();
    drawAlerInfo("HIGH TEMP!", "WORK STOPPED!", round(temp), 0);
    break;

  case eventFlowWarning:
    // soundSiren();
    drawAlerInfo("CHECK FLOW!", "STOP WORK NOW!", litersPerHour, 1);
    break;

  case eventFlowAlarm:
    // soundSiren();
    drawAlerInfo("LOW FLOW!", "WORK STOPPED!", litersPerHour, 1);
    break;
  }

  soundSiren();
}

void displaySetValue(const char *title, uint8_t value)
{
  display.clearDisplay();

  // зажигает подсветку.
  digitalWrite(10, LOW);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(title);
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
  display.print("Click to save");

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
 * Основная функция, запрашивает поток и температуру
 */
void getMeasures()
{
  currentTime = millis();
  measuredPeriod = currentTime - currentTimePrev;
  if (measuredPeriod >= MEASURE_PERIOD_LENGTH)
  {
    currentTimePrev = currentTime;
    litersPerMinute = (pulse_frequency / (measuredPeriod / MEASURE_PERIOD_LENGTH)) / 7.5f;
    litersPerHour = litersPerMinute * 60;
    pulse_frequency = 0;

    // дёргаем датчик температуры и забираем данные.
    sensors.requestTemperatures();
    temp = sensors.getTempC(waterThermometerAddr);

#ifdef DEBUG
    temp = 19;
    litersPerHour = 113;
#endif
  }
}

void setEvent()
{
  uint8_t integerTemp = round(temp);
  if (integerTemp >= tempWarning && integerTemp < tempAlarm)
  {
    event = eventTempWarning;
  }
  else if (integerTemp >= tempAlarm)
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
    noTone(pizoPin);
  }
}

bool readSettings()
{
  uint16_t eepromCrc, calcCrc;
  uint8_t settings = 0;
  uint8_t tmpBuff[4];

  eepromCrc = EEPROM.read(SETTINGS_ADDR_CRC) << 8;
  Serial.println(eepromCrc);
  eepromCrc |= 0x00FF & EEPROM.read(SETTINGS_ADDR_CRC + 1);
  Serial.println(eepromCrc);

  settings = EEPROM.read(SETTINGS_ADDR_TEMP_WARNING);
  // tempWarning = settings == 0 ? 20 : settings;
  tmpBuff[0] = tempWarning = settings;

  settings = EEPROM.read(SETTINGS_ADDR_TEMP_ALARM);
  // tempAlarm = settings == 0 ? 30 : settings;
  tmpBuff[1] = tempAlarm = settings;

  settings = EEPROM.read(SETTINGS_ADDR_FLOW_WARNING);
  // flowWarning = settings == 0 ? 100 : settings;
  tmpBuff[2] = flowWarning = settings;

  settings = EEPROM.read(SETTINGS_ADDR_FLOW_ALARM);
  // flowAlarm = settings == 0 ? 90 : settings;
  tmpBuff[3] = flowAlarm = settings;

  FastCRC16 CRC16;
  calcCrc = CRC16.ccitt(tmpBuff, sizeof(tmpBuff));
  Serial.println(calcCrc);
  if (eepromCrc != calcCrc)
  {
    return false;
  }
  else
  {
    return true;
  }
}

void writeSettings()
{
  uint16_t calcCrc;
  uint8_t tmpBuff[4];

  tmpBuff[0] = tempWarning;
  tmpBuff[1] = tempAlarm;
  tmpBuff[2] = flowWarning;
  tmpBuff[3] = flowAlarm;
  EEPROM.update(SETTINGS_ADDR_TEMP_WARNING, tempWarning);
  EEPROM.update(SETTINGS_ADDR_TEMP_ALARM, tempAlarm);
  EEPROM.update(SETTINGS_ADDR_FLOW_WARNING, flowWarning);
  EEPROM.update(SETTINGS_ADDR_FLOW_ALARM, flowAlarm);

  FastCRC16 CRC16;
  calcCrc = CRC16.ccitt(tmpBuff, sizeof(tmpBuff));
  Serial.println(calcCrc);
  EEPROM.update(SETTINGS_ADDR_CRC, (uint8_t)(calcCrc >> 8));
  EEPROM.update(SETTINGS_ADDR_CRC + 1, (uint8_t)calcCrc);
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

  if (!readSettings())
  {

    tempWarning = 20;
    tempAlarm = 25;
    flowWarning = 100;
    flowAlarm = 90;
    writeSettings();
  }

  // Пищим при запуске.
  tone(pizoPin, 400);
  delay(300);
  tone(pizoPin, 800);
  delay(300);
  tone(pizoPin, 1500);
  delay(300);
  noTone(pizoPin);
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
    // EEPROM.update(SETTINGS_ADDR_TEMP_WARNING, tempWarning);
    writeSettings();
  }

  if (mode == MODE_SET_TEMP_ALARM && isClick)
  {
    mode = MODE_SET_FLOW_WARNING;
    isClick = false;
    // EEPROM.update(SETTINGS_ADDR_TEMP_ALARM, tempAlarm);
    writeSettings();
  }

  if (mode == MODE_SET_FLOW_WARNING && isClick)
  {
    mode = MODE_SET_FLOW_ALARM;
    isClick = false;
    // EEPROM.update(SETTINGS_ADDR_FLOW_WARNING, flowWarning);
    writeSettings();
  }

  if (mode == MODE_SET_FLOW_ALARM && isClick)
  {
    mode = MODE_MAIN_SCREEN;
    isClick = false;
    // EEPROM.update(SETTINGS_ADDR_FLOW_ALARM, flowAlarm);
    writeSettings();
  }

  // опрашиваем датчики
  getMeasures();

  // если значения датчиков выше заданных, устанавливаем соответствующее событие
  setEvent();

  // отрисовываем экран в зависимости от режима в котором находится устройство
  switch (mode)
  {
  case MODE_MAIN_SCREEN:
    if (event == eventNone)
    {
      displayMainScreen();
      readJoystickMainScreen();
    }
    else
    {
      displayAlarm();
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
