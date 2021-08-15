#include "ChillerSettings.h"
#include <EEPROM.h>
#include <FastCRC.h>

bool ChillerSettings::isCrcValid()
{
    return _isCrcValid;
}

uint8_t ChillerSettings::getTempWarning()
{
    return tempWarning;
}

uint8_t ChillerSettings::getTempAlarm()
{
    return tempAlarm;
}

uint8_t ChillerSettings::getFlowWarning()
{
    return flowWarning;
}

uint8_t ChillerSettings::getFlowAlarm()
{
    return flowAlarm;
}

bool ChillerSettings::isTempUse()
{
    return tempUse;
}

bool ChillerSettings::isFlowUse()
{
    return flowUse;
}

bool ChillerSettings::isSoundEnabled()
{
    return soundEnabled;
}

uint8_t ChillerSettings::getStartTimeout()
{
    return startTimeout;
}

void ChillerSettings::setTempWarning(uint8_t value)
{
    tempWarning = value;
}

void ChillerSettings::setTempAlarm(uint8_t value)
{
    tempAlarm = value;
}

void ChillerSettings::setFlowWarning(uint8_t value)
{
    flowWarning = value;
}

void ChillerSettings::setFlowAlarm(uint8_t value)
{
    flowAlarm = value;
}

void ChillerSettings::setTempUse(bool value)
{
    tempUse = value;
}

void ChillerSettings::setFlowUse(bool value)
{
    flowUse = value;
}

void ChillerSettings::setSoundEnabled(bool value)
{
    soundEnabled = value;
}

void ChillerSettings::setStartTimeout(uint8_t value)
{
    startTimeout = value;
}

void ChillerSettings::read()
{
    uint16_t eepromCrc, calcCrc;
    uint8_t tmpBuff[8];

    eepromCrc = EEPROM.read(SETTINGS_ADDR_CRC_H) << 8;
    eepromCrc |= 0x00FF & EEPROM.read(SETTINGS_ADDR_CRC_L);

    tmpBuff[0] = tempWarning = EEPROM.read(SETTINGS_ADDR_TEMP_WARNING);
    tmpBuff[1] = tempAlarm = EEPROM.read(SETTINGS_ADDR_TEMP_ALARM);
    tmpBuff[2] = flowWarning = EEPROM.read(SETTINGS_ADDR_FLOW_WARNING);
    tmpBuff[3] = flowAlarm = EEPROM.read(SETTINGS_ADDR_FLOW_ALARM);
    tmpBuff[4] = tempUse = EEPROM.read(SETTINGS_ADDR_TEMP_USE);
    tmpBuff[5] = flowUse = EEPROM.read(SETTINGS_ADDR_FLOW_USE);
    tmpBuff[6] = soundEnabled = EEPROM.read(SETTINGS_ADDR_SOUND_ENABLED);
    tmpBuff[7] = startTimeout = EEPROM.read(SETTINGS_ADDR_START_TIMEOUT);

    FastCRC16 CRC16;
    calcCrc = CRC16.ccitt(tmpBuff, sizeof(tmpBuff));
    _isCrcValid = eepromCrc == calcCrc;
}

void ChillerSettings::write()
{
  uint16_t calcCrc;
  uint8_t tmpBuff[8];
  FastCRC16 CRC16;

  tmpBuff[0] = tempWarning;
  tmpBuff[1] = tempAlarm;
  tmpBuff[2] = flowWarning;
  tmpBuff[3] = flowAlarm;
  tmpBuff[4] = tempUse;
  tmpBuff[5] = flowUse;
  tmpBuff[6] = soundEnabled;
  tmpBuff[7] = startTimeout;
  calcCrc = CRC16.ccitt(tmpBuff, sizeof(tmpBuff));

  EEPROM.update(SETTINGS_ADDR_TEMP_WARNING, tempWarning);
  EEPROM.update(SETTINGS_ADDR_TEMP_ALARM, tempAlarm);
  EEPROM.update(SETTINGS_ADDR_FLOW_WARNING, flowWarning);
  EEPROM.update(SETTINGS_ADDR_FLOW_ALARM, flowAlarm);
  EEPROM.update(SETTINGS_ADDR_TEMP_USE, tempUse);
  EEPROM.update(SETTINGS_ADDR_FLOW_USE, flowUse);
  EEPROM.update(SETTINGS_ADDR_SOUND_ENABLED, soundEnabled);
  EEPROM.update(SETTINGS_ADDR_START_TIMEOUT, startTimeout);
  EEPROM.update(SETTINGS_ADDR_CRC_H, (uint8_t)(calcCrc >> 8));
  EEPROM.update(SETTINGS_ADDR_CRC_L, (uint8_t)calcCrc);
}

void ChillerSettings::setDefaults() {
  tempWarning = 20;
  tempAlarm = 25;
  flowWarning = 100;
  flowAlarm = 90;
  tempUse = false;
  flowUse = false;
  soundEnabled = true;
  startTimeout = 5;
}