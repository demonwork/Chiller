#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <SPI.h>
#include "images.h"
#include <DallasTemperature.h>
#include <Wire.h>
#include <GyverButton.h>

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

float temp;        // переменная температуры
int setTemp = 30;  // значение предустановленной критичной температуры
int setFlow = 100; // значение предустановленного критическго потока
                   // Переменные потока датчика воды
volatile uint16_t pulse_frequency;
uint8_t litersPerHour, litersPerMinute;
uint64_t currentTime;
uint16_t measuredPeriod, currentTimePrev;
uint8_t waterFlowInterruptNumber = 0;

GButton button(pinButton);
#define MODE_MAIN_SCREEN 0
#define MODE_SET_TEMP 2
#define MODE_SET_FLOW 3
uint8_t mode = MODE_MAIN_SCREEN;

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
int displayShow(int f, float t)
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
  display.print("[T-");
  display.print(setTemp);
  display.print("]");
  display.print("[F-");
  display.print(setFlow);
  display.print("]");

  display.display();
  display.clearDisplay();

  return 0;
}

/**
 * Функция вывода сигнализации на экран
 */
int displayAlarm(int errorcode, int f, int t)
{
  switch (errorcode)
  {

  case 1:
    tone(pizoPin, 500, 500);
    // зажигает подсветку.
    digitalWrite(10, LOW);
    display.clearDisplay();
    display.drawBitmap(0, 15, exclaimImg, 24, 20, 1);
    display.setTextSize(1);
    display.setCursor(10, 0);
    display.println("HIGH WATER!");
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

    if (millis() - timeLoopAlarm >= 3000)
    {
      // гасим экран.
      digitalWrite(10, HIGH);
      timeLoopAlarm = millis();
    }

    break;

  case 2:
    break;
  }

  return 0;
}

/**
 * Данная функция по работе с меню установки температуры
 */
int menuSet()
{
  switch (mode)
  {
  case MODE_SET_TEMP:
    // зажигает подсветку.
    digitalWrite(10, LOW);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("> Set temp [T]");
    display.setTextSize(2);
    display.setCursor(5, 15);
    display.print("<-");
    display.print(setTemp, DEC);
    display.print("+>");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("Hold to next");
    display.display();

    break;
  case MODE_SET_FLOW:
    // зажигает подсветку.
    digitalWrite(10, LOW);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("> Set flow [F]");
    display.setTextSize(2);
    display.setCursor(0, 15);
    display.print("<-");
    display.print(setFlow, DEC);
    display.print("+>");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("Hold to save");
    display.display();

    break;
  default:
    break;
  }

  // Обработка событий джойстика
  if (analogRead(pinVRY) > 1000)
  {
    // Если стик вправо то ++
    switch (mode)
    {
    case MODE_SET_TEMP:
      setTemp++;
      delay(200);
      break;

    case MODE_SET_FLOW:
      setFlow++;
      delay(200);
      break;

    default:
      break;
    }
  }

  if (analogRead(pinVRY) < 400)
  {
    // Если стик влево то --
    switch (mode)
    {
    case MODE_SET_TEMP:
      setTemp--;
      delay(200);
      break;

    case MODE_SET_FLOW:
      setFlow--;
      delay(200);
      break;

    default:
      break;
    }
  }

  return 0;
}

/**
 * Основная функция, запрашивает поток и температуру, а затем отсылает на дисплей.
 */
int rootSys()
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

    // Проверка условий температуры и потока воды
    if (temp >= setTemp)
    {
      // ставим номерок кода и передаём в функцию Аларма, там само пусть разбирается что писать.
      int error = 1;
      // выводим ошибку.
      displayAlarm(error, litersPerHour, temp);
    }

    if (temp < setTemp)
    {
      // Отправляем данные на экран с текущими показателями.
      displayShow(litersPerHour, temp);
    }
  }

  return 0;
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
    mode = MODE_SET_TEMP;
    isClick = false;
  }

  if (mode == MODE_SET_TEMP && isClick)
  {
    mode = MODE_SET_FLOW;
    isClick = false;
    display.clearDisplay();
  }

  if (mode == MODE_SET_FLOW && isClick)
  {
    mode = MODE_MAIN_SCREEN;
    isClick = false;
    display.clearDisplay();
  }

  switch (mode)
  {
  case MODE_MAIN_SCREEN:
    rootSys();
    break;
  case MODE_SET_TEMP:
  case MODE_SET_FLOW:
    menuSet();
    break;
  }

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
