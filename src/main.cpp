#include <stdint.h>
#include "images.h"
#include "eeprom.h"
#include "chiller.h"
#include "sound.h"
#include "globals.h"
#include "display.h"
#include "time.h"

/*
 * TODO:
 * - Удобный интерфейс для настройки
 * - Управление помпой. Т.е. включение выключение при достижении определённой температуры.
 */

/**
 *  функция получения данных с датчика потока, работает по прерыванию
 */
void waterFlowInterruptHandler()
{
  pulse_frequency++;
}

/**
 * Функция вывода сигнализации на экран
 */
void displayAlarm()
{
  static bool switchWarning = false;
  static uint64_t prevSwitchTime = 0;

  if (isTempWarning && isFlowWarning)
  {
    if (isTimeoutLeft(1000, &prevSwitchTime))
    {
      switchWarning = !switchWarning;
    }

    if (switchWarning)
    {
      drawAlertInfo("CHECK TEMP!", "STOP WORK NOW!", temp / 10, 0);
    }
    else
    {
      drawAlertInfo("CHECK FLOW!", "STOP WORK NOW!", litersPerHour, 1);
    }
  }
  else
  {

    switch (event)
    {
    case eventTempWarning:
      drawAlertInfo("CHECK TEMP!", "STOP WORK NOW!", temp / 10, 0);
      break;

    case eventTempAlarm:
      drawAlertInfo("HIGH TEMP!", "WORK STOPPED!", temp / 10, 0);
      break;

    case eventFlowWarning:
      drawAlertInfo("CHECK FLOW!", "STOP WORK NOW!", litersPerHour, 1);
      break;

    case eventFlowAlarm:
      drawAlertInfo("LOW FLOW!", "WORK STOPPED!", litersPerHour, 1);
      break;
    }
  }
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

void readJoystickUseValue(bool *value)
{
  // Обработка событий джойстика
  if (buttonRight.isClick() || buttonLeft.isClick())
  {
    // в любую сторону меняем знаяение на противоположное
    (*value) = !(*value);
    isRedraw = true;
  }
}

void readJoystickMainScreen()
{
  if (buttonUp.isClick())
  {
    // зажигает подсветку
    digitalWrite(PIN_BACKLIGHT, LOW);
    isBackLightOn = true;
  }

  if (buttonDown.isClick())
  {
    // гасит подсветку
    digitalWrite(PIN_BACKLIGHT, HIGH);
    isBackLightOn = false;
  }
}

/**
 * Основная функция, запрашивает поток и температуру
 */
void getMeasures()
{
  uint16_t tmpTemp, tmpFlow;
  uint16_t measuredPeriod;
  uint64_t currentTime = millis();
  static uint64_t prevTime = 0;
  float realTemp;
  measuredPeriod = currentTime - prevTime;
  if (measuredPeriod >= MEASURE_PERIOD_LENGTH)
  {
    prevTime = currentTime;
    litersPerMinute = (pulse_frequency / (measuredPeriod / MEASURE_PERIOD_LENGTH)) / 7.5f;
    pulse_frequency = 0;
    tmpFlow = litersPerMinute * 60;

#ifdef DEBUG
    // tmpFlow = 113;
#endif

    if (tmpFlow != litersPerHour)
    {
      litersPerHour = tmpFlow;
      isRedraw = true;
    }

    // дёргаем датчик температуры и забираем данные.
    if (sensors.getDS18Count() > 0)
    {
      sensors.requestTemperatures();
      realTemp = sensors.getTempC(waterThermometerAddr);
      tmpTemp = realTemp * 10;
    }
    else
    {
      tmpTemp = 0;
    }

#ifdef DEBUG
    // tmpTemp = 19;
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
  if (isTempUse && sensors.getDS18Count() > 0)
  {
    isTempWarning = temp >= tempWarning * 10 && temp < tempAlarm * 10;
    isTempAlarm = temp >= tempAlarm * 10;
  }
  else
  {
    isTempWarning = false;
    isTempAlarm = false;
  }

  if (isFlowUse)
  {
    isFlowWarning = litersPerHour > flowAlarm && litersPerHour <= flowWarning;
    isFlowAlarm = litersPerHour <= flowAlarm;
  }
  else
  {
    isFlowWarning = false;
    isFlowAlarm = false;
  }

  if (isTempAlarm)
  {
    event = eventTempAlarm;
    digitalWrite(PIN_EMERGENCY_STOP, HIGH);
  }
  else if (isFlowAlarm)
  {
    event = eventFlowAlarm;
    digitalWrite(PIN_EMERGENCY_STOP, HIGH);
  }
  else if (isTempWarning)
  {
    event = eventTempWarning;
  }
  else if (isFlowWarning)
  {
    event = eventFlowWarning;
  }
  else
  {
    event = eventNone;
    noTone(PIN_PIZO);
    digitalWrite(PIN_EMERGENCY_STOP, LOW);
  }
}

bool setupTempSensor()
{
  sensors.begin();
  if (sensors.getDS18Count() > 0)
  {
    sensors.getAddress(waterThermometerAddr, 0);
    sensors.setResolution(waterThermometerAddr, TEMPERATURE_PRECISION);
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * Инициализация приложения
 */
void setup()
{
  Serial.begin(9600);
  Serial.println("Chiller start.");

  digitalWrite(PIN_EMERGENCY_STOP, LOW);
  pinMode(PIN_EMERGENCY_STOP, OUTPUT);

  if (!readSettings())
  {

    tempWarning = 20;
    tempAlarm = 25;
    flowWarning = 100;
    flowAlarm = 90;
    isTempUse = false;
    isFlowUse = false;
    isSoundEnabled = true;
    startTimeout = 5;
    writeSettings();
  }

  // инициализируем либу для работы с датчиками температуры
  setupTempSensor();

  pinMode(PIN_WATER_FLOW, INPUT);
  // прерывание на пине к которому подключен датчик воды, дёргает когда приходят данные
  attachInterrupt(WATER_FLOW_INTERRUPT_NUMBER, waterFlowInterruptHandler, FALLING);

  // подсветка экрана
  pinMode(PIN_BACKLIGHT, OUTPUT);
  digitalWrite(PIN_BACKLIGHT, LOW);
  isBackLightOn = true;

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

  button.setTickMode(AUTO);

  buttonLeft.setTickMode(MANUAL);
  buttonLeft.setStepTimeout(200);

  buttonRight.setTickMode(MANUAL);
  buttonRight.setStepTimeout(200);

  buttonUp.setTickMode(MANUAL);
  buttonUp.setStepTimeout(200);

  buttonDown.setTickMode(MANUAL);
  buttonDown.setStepTimeout(200);

  // Пищим при запуске.
  tone(PIN_PIZO, 400);
  delay(300);
  tone(PIN_PIZO, 800);
  delay(300);
  tone(PIN_PIZO, 1500);
  delay(300);
  noTone(PIN_PIZO);

  // задержка перед стартом
  startTimeOut(startTimeout);
}

void readAnalogButton()
{
  int analogX = analogRead(PIN_VRX);
  int analogY = analogRead(PIN_VRY);

  buttonUp.tick(analogY < 400);
  buttonDown.tick(analogY > 800);
  buttonLeft.tick(analogX < 400);
  buttonRight.tick(analogX > 800);
}

/**
 * Главный цикл
 */
void loop()
{
  bool isClick = button.isClick();
  static uint64_t switchPrevTime = 0;
  static uint64_t flashPrevTime = 0;
  static bool switchScreen = false;

  readAnalogButton();

  if (mode == MODE_MAIN_SCREEN && isClick)
  {
    mode = MODE_SET_TEMP_USE;
    isClick = false;
    isRedraw = true;
    noTone(PIN_PIZO);
  }

  if (mode == MODE_SET_TEMP_USE && isClick)
  {
    mode = MODE_SET_TEMP_WARNING;
    isClick = false;
    isRedraw = true;
    writeSettings();

    if (isTempUse)
    {
      // настройка сохранена, инициализируем датчик, если не удалось, в текущем сеансе "отключаем" его
      setupTempSensor();
    }
    else
    {
      // если датчик не используем, лимиты нет смысла настраивать
      mode = MODE_SET_FLOW_USE;
    }
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
    mode = MODE_SET_FLOW_USE;
    isClick = false;
    isRedraw = true;
    writeSettings();
  }

  if (mode == MODE_SET_FLOW_USE && isClick)
  {
    mode = MODE_SET_FLOW_WARNING;
    isClick = false;
    isRedraw = true;
    writeSettings();

    if (!isFlowUse)
    {
      // если датчик не используем, лимиты нет смысла настраивать
      mode = MODE_SET_SOUND_ENABLED;
    }
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
    mode = MODE_SET_SOUND_ENABLED;
    isClick = false;
    isRedraw = true;
    writeSettings();
  }

  if (mode == MODE_SET_SOUND_ENABLED && isClick)
  {
    mode = MODE_SET_START_TIMEOUT;
    isClick = false;
    isRedraw = true;
    writeSettings();
  }

  if (mode == MODE_SET_START_TIMEOUT && isClick)
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

  // пищим и мигаем если необходимо
  switch (event)
  {
  case eventTempWarning:
  case eventFlowWarning:
    soundBeep();
    if (isTimeoutLeft(2000, &flashPrevTime))
    {
      flashBacklight();
    }

    break;
  case eventTempAlarm:
  case eventFlowAlarm:
    soundSiren();
    if (isTimeoutLeft(1000, &flashPrevTime))
    {
      flashBacklight();
    }

    break;
  default:
    noTone(PIN_PIZO);
    restoreBackLight();
    break;
  }

  // отрисовываем экран в зависимости от режима в котором находится устройство
  switch (mode)
  {
  case MODE_MAIN_SCREEN:
    if (isTimeoutLeft(4000, &switchPrevTime))
    {
      switchScreen = !switchScreen;
      isRedraw = true;
    }

    if (isRedraw)
    {
      if (event == eventNone)
      {
        // главный экран с текущими значениями датчиков
        if (switchScreen)
        {
          displayMainScreenTemp();
        }
        else
        {
          displayMainScreenFlow();
        }
      }
      else
      {
        // экран уведомления, когда параметры датчиков вышли за заданные границы
        displayAlarm();
      }
    }

    readJoystickMainScreen();
    break;
  case MODE_SET_TEMP_USE:
    readJoystickUseValue(&isTempUse);
    if (isRedraw)
    {
      displaySetUseValue("Use T sensor?", isTempUse);
    }
    break;
  case MODE_SET_TEMP_WARNING:
    readJoystickValue(&tempWarning);
    if (isRedraw)
    {
      displaySetValue("Temp W-limit", tempWarning);
    }
    break;
  case MODE_SET_TEMP_ALARM:
    readJoystickValue(&tempAlarm);
    if (isRedraw)
    {
      displaySetValue("Temp A-limit", tempAlarm);
    }
    break;
  case MODE_SET_FLOW_USE:
    readJoystickUseValue(&isFlowUse);
    if (isRedraw)
    {
      displaySetUseValue("Use F sensor?", isFlowUse);
    }
    break;
  case MODE_SET_FLOW_WARNING:
    readJoystickValue(&flowWarning);
    if (isRedraw)
    {
      displaySetValue("Flow W-limit", flowWarning);
    }
    break;
  case MODE_SET_FLOW_ALARM:
    readJoystickValue(&flowAlarm);
    if (isRedraw)
    {
      displaySetValue("Flow A-limit", flowAlarm);
    }
    break;
  case MODE_SET_SOUND_ENABLED:
    readJoystickUseValue(&isSoundEnabled);
    if (isRedraw)
    {
      displaySetUseValue("Enable sound?", isSoundEnabled);
    }
    break;
  case MODE_SET_START_TIMEOUT:
    readJoystickValue(&startTimeout);
    if (isRedraw)
    {
      displaySetValue("Start timeout", startTimeout);
    }
    break;
  }
}
