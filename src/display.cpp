#include <Arduino.h>
#include <Adafruit_PCD8544.h>
#include "chiller.h"
#include "display.h"
#include "images.h"
#include "ChillerSettings.h"

extern bool isRedraw;
extern uint16_t temp;
extern ChillerSettings settings;
extern bool isBackLightOn;
extern Adafruit_PCD8544 display;
extern uint8_t litersPerHour, litersPerMinute;

void restoreBackLight()
{
  if (isBackLightOn)
  {
    digitalWrite(PIN_BACKLIGHT, LOW);
  }
  else
  {
    digitalWrite(PIN_BACKLIGHT, HIGH);
  }
}

/**
 * Показываем текущую температуру
 */
void displayMainScreenTemp()
{
  restoreBackLight();

  display.clearDisplay();
  display.drawBitmap(0, 3, heatImg, 16, 18, 1);
  display.setTextSize(3);
  display.setTextSize(3);
  display.setCursor(26, 0);
  display.print(temp / 10);
  display.setTextSize(2);
  display.setCursor(59, 6);
  display.print(".");
  display.print(temp % 10);
  display.setTextSize(1);
  display.setCursor(0, 30);

  display.print("W-limit: ");
  display.println(settings.getTempWarning());
  display.print("A-limit: ");
  display.println(settings.getTempAlarm());

  display.display();

  isRedraw = false;
}

/**
 * Показываем текущий поток охлаждающий жидкости
 */
void displayMainScreenFlow()
{
  restoreBackLight();

  display.clearDisplay();
  display.drawBitmap(0, 3, flowImg, 24, 15, 1);
  display.setTextSize(3);
  display.setCursor(26, 0);
  display.println(litersPerHour, DEC);
  display.setTextSize(1);
  display.setCursor(0, 30);

  display.print("W-limit: ");
  display.println(settings.getFlowWarning());
  display.print("A-limit: ");
  display.println(settings.getFlowAlarm());

  display.display();

  isRedraw = false;
}

/**
 * Показываем информацию о опасной ситуации или аварии
 */
void drawAlertInfo(const char *title, const char *footer, uint8_t value, uint8_t mode)
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
 * Показывает экран с настройкой значения
 */
void displaySetValue(const char *title, uint8_t value)
{
  // зажигает подсветку
  digitalWrite(PIN_BACKLIGHT, LOW);

  display.clearDisplay();
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

/**
 * Показывает экран с настройкой значения вкл/выкл
 */
void displaySetUseValue(const char *title, bool value)
{
  // зажигает подсветку
  digitalWrite(PIN_BACKLIGHT, LOW);

  display.clearDisplay();
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(title);
  display.setTextSize(2);
  display.setCursor(5, 15);
  display.print(value == true ? "On" : "Off" );
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("Click to save");

  display.display();

  isRedraw = false;
}

/**
 * Показывает таймер задержки перед началом измерений
 */
void startTimeOut(uint8_t seconds)
{
  int mss = millis();
  for (uint8_t i = 0; i < seconds; i++)
  {
    display.clearDisplay();
    display.setTextSize(3);
    display.setCursor(40, 14);
    display.println(seconds - i);
    display.display();
    while ((millis() - mss) < 1000)
    {
    }
    mss = millis();
  }
}

void flashBacklight()
{
    static bool flashState = true;

    if (flashState)
    {
        digitalWrite(PIN_BACKLIGHT, LOW);
    }
    else
    {
        digitalWrite(PIN_BACKLIGHT, HIGH);
    }

    flashState = !flashState;
}