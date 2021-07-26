#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <SPI.h>
#include "images.h"
#include <DallasTemperature.h>
#include <Wire.h>
#include <GyverButton.h>
#include <EEPROM.h>
#include <FastCRC.h>

/*
 * TODO:
 * - Удобный интерфейс для настройки
 * - Выделить пин для реле, через которое будет "нажиматься" конопка аварийной остановки
 * - Экран маленький, вывод текущих значений датчиков по очереди
 * - Экран маленький, вывод опасных/критических значений по очереди
 * - Показывать десятые доли температуры
 * - Для опасных событий звук попроще чем сирена
 * - В аварийных ситуациях при мигании, вместо пустого экрана, показывать например крест
 * - В опасных ситуациях при мигании, вместо пустого экрана, показывать например !
 */

/*
 * FIXME:
 * - Так как "нормальный" звук можно получить только через delay(), моргание экраном реализовано не прозрачно.
 * Один цикл сирены это 3 секунды, в "разрыве" 1,5 секунды сейчас гасится экран.
 * Нужно реализовать таймер, в котором будет "гасится" экран.
 */

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
#define pinVRY A0    // Джойстик Y
#define pinVRX A1    // Джойстик X
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
GButton buttonUp;
GButton buttonDown;
GButton buttonLeft;
GButton buttonRight;

#define MODE_MAIN_SCREEN 0
#define MODE_SET_TEMP_WARNING 1
#define MODE_SET_TEMP_ALARM 2
#define MODE_SET_FLOW_WARNING 3
#define MODE_SET_FLOW_ALARM 4
uint8_t mode = MODE_MAIN_SCREEN;

#define SETTINGS_ADDR_CRC_H 0
#define SETTINGS_ADDR_CRC_L 1
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

bool isRedraw = true;

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
void displayMainScreen()
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

  isRedraw = false;
}

void soundSiren()
{
  static bool up = true;
  static uint16_t i = 700;
  if (up)
  {
    if (i < 800)
    {
      tone(pizoPin, i);
      delay(15);
      i++;
    }

    if (i == 800)
    {
      up = false;
    }
  }
  else
  {
    if (i > 700)
    {
      tone(pizoPin, i);
      delay(15);
      i--;
    }

    if (i == 700)
    {
      up = true;
    }
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

  isRedraw = false;
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
    drawAlerInfo("CHECK TEMP!", "STOP WORK NOW!", round(temp), 0);
    break;

  case eventTempAlarm:
    drawAlerInfo("HIGH TEMP!", "WORK STOPPED!", round(temp), 0);
    break;

  case eventFlowWarning:
    drawAlerInfo("CHECK FLOW!", "STOP WORK NOW!", litersPerHour, 1);
    break;

  case eventFlowAlarm:
    drawAlerInfo("LOW FLOW!", "WORK STOPPED!", litersPerHour, 1);
    break;
  }
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

  isRedraw = false;
}

void readJoystickValue(uint8_t *value)
{
  // Обработка событий джойстика
  if (buttonRight.isClick())
  {
    // Если стик вправо то ++
    (*value)++;
    isRedraw = true;
  }

  if (buttonRight.isStep())
  {
    // Если стик вправо то ++
    (*value)++;
    isRedraw = true;
  }

  if (buttonLeft.isClick())
  {
    // Если стик влево то --
    (*value)--;
    isRedraw = true;
  }

  if (buttonLeft.isStep())
  {
    // Если стик влево то --
    (*value)--;
    isRedraw = true;
  }
}

void readJoystickMainScreen()
{
  if (buttonUp.isClick())
  {
    // зажигает подсветку.
    digitalWrite(10, LOW);
  }

  if (buttonDown.isClick())
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
  int tmpTemp, tmpFlow;
  currentTime = millis();
  measuredPeriod = currentTime - currentTimePrev;
  if (measuredPeriod >= MEASURE_PERIOD_LENGTH)
  {
    currentTimePrev = currentTime;
    litersPerMinute = (pulse_frequency / (measuredPeriod / MEASURE_PERIOD_LENGTH)) / 7.5f;
    pulse_frequency = 0;
    tmpFlow = litersPerMinute * 60;

#ifdef DEBUG
    tmpFlow = 113;
#endif

    if (tmpFlow != litersPerHour)
    {
      litersPerHour = tmpFlow;
      isRedraw = true;
    }

    // дёргаем датчик температуры и забираем данные.
    sensors.requestTemperatures();
    tmpTemp = sensors.getTempC(waterThermometerAddr);

#ifdef DEBUG
    tmpTemp = 19;
#endif

    if (tmpTemp != temp)
    {
      temp = tmpTemp;
      isRedraw = true;
    }
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

  eepromCrc = EEPROM.read(SETTINGS_ADDR_CRC_H) << 8;
  eepromCrc |= 0x00FF & EEPROM.read(SETTINGS_ADDR_CRC_L);

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
  EEPROM.update(SETTINGS_ADDR_CRC_H, (uint8_t)(calcCrc >> 8));
  EEPROM.update(SETTINGS_ADDR_CRC_L, (uint8_t)calcCrc);
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

  buttonLeft.setTickMode(MANUAL);
  buttonLeft.setStepTimeout(200);

  buttonRight.setTickMode(MANUAL);
  buttonRight.setStepTimeout(200);

  buttonUp.setTickMode(MANUAL);
  buttonUp.setStepTimeout(200);

  buttonDown.setTickMode(MANUAL);
  buttonDown.setStepTimeout(200);

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

void readAnalogButton()
{
  int analogX = analogRead(pinVRX);
  int analogY = analogRead(pinVRY);

  buttonUp.tick(analogY > 1000);
  buttonDown.tick(analogY < 400);
  buttonRight.tick(analogX > 1000);
  buttonLeft.tick(analogX < 400);
}

void loop()
{
  bool isClick = button.isClick();

  readAnalogButton();

  if (mode == MODE_MAIN_SCREEN && isClick)
  {
    mode = MODE_SET_TEMP_WARNING;
    isClick = false;
    isRedraw = true;
    noTone(pizoPin);
  }

  if (mode == MODE_SET_TEMP_WARNING && isClick)
  {
    mode = MODE_SET_TEMP_ALARM;
    isClick = false;
    isRedraw = true;
    writeSettings();
  }

  if (mode == MODE_SET_TEMP_ALARM && isClick)
  {
    mode = MODE_SET_FLOW_WARNING;
    isClick = false;
    isRedraw = true;
    writeSettings();
  }

  if (mode == MODE_SET_FLOW_WARNING && isClick)
  {
    mode = MODE_SET_FLOW_ALARM;
    isClick = false;
    isRedraw = true;
    writeSettings();
  }

  if (mode == MODE_SET_FLOW_ALARM && isClick)
  {
    mode = MODE_MAIN_SCREEN;
    isClick = false;
    isRedraw = true;
    writeSettings();
  }

  // опрашиваем датчики
  getMeasures();

  // если значения датчиков выше заданных, устанавливаем соответствующее событие
  setEvent();

  if (event != eventNone)
  {
    soundSiren();
  }

  // отрисовываем экран в зависимости от режима в котором находится устройство
  switch (mode)
  {
  case MODE_MAIN_SCREEN:
    if (event == eventNone)
    {
      if (isRedraw)
      {
        displayMainScreen();
      }

      readJoystickMainScreen();
    }
    else
    {
      if (isRedraw)
      {
        displayAlarm();
      }
    }
    break;
  case MODE_SET_TEMP_WARNING:
    readJoystickValue(&tempWarning);
    if (isRedraw)
    {
      displaySetValue("Set temp [TW]", tempWarning);
    }
    break;
  case MODE_SET_TEMP_ALARM:
    readJoystickValue(&tempAlarm);
    if (isRedraw)
    {
      displaySetValue("Set temp [TA]", tempAlarm);
    }
    break;
  case MODE_SET_FLOW_WARNING:
    readJoystickValue(&flowWarning);
    if (isRedraw)
    {
      displaySetValue("Set flow [FW]", flowWarning);
    }
    break;
  case MODE_SET_FLOW_ALARM:
    readJoystickValue(&flowAlarm);
    if (isRedraw)
    {
      displaySetValue("Set flow [FA]", flowAlarm);
    }
    break;
  }
}
