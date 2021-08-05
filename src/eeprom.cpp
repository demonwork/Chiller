#include <stdint.h>
#include <EEPROM.h>
#include <FastCRC.h>
#include "eeprom.h"

extern uint8_t tempWarning;
extern uint8_t tempAlarm;
extern uint8_t flowWarning;
extern uint8_t flowAlarm;
extern bool isTempUse;
extern bool isFlowUse;
extern bool isSoundEnabled;

bool readSettings()
{
  uint16_t eepromCrc, calcCrc;
  uint8_t settings = 0;
  uint8_t tmpBuff[7];

  eepromCrc = EEPROM.read(SETTINGS_ADDR_CRC_H) << 8;
  eepromCrc |= 0x00FF & EEPROM.read(SETTINGS_ADDR_CRC_L);

  settings = EEPROM.read(SETTINGS_ADDR_TEMP_WARNING);
  tmpBuff[0] = tempWarning = settings;

  settings = EEPROM.read(SETTINGS_ADDR_TEMP_ALARM);
  tmpBuff[1] = tempAlarm = settings;

  settings = EEPROM.read(SETTINGS_ADDR_FLOW_WARNING);
  tmpBuff[2] = flowWarning = settings;

  settings = EEPROM.read(SETTINGS_ADDR_FLOW_ALARM);
  tmpBuff[3] = flowAlarm = settings;

  settings = EEPROM.read(SETTINGS_ADDR_TEMP_USE);
  tmpBuff[4] = isTempUse = settings;

  settings = EEPROM.read(SETTINGS_ADDR_FLOW_USE);
  tmpBuff[5] = isFlowUse = settings;

  settings = EEPROM.read(SETTINGS_ADDR_SOUND_ENABLED);
  tmpBuff[6] = isSoundEnabled = settings;

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
  uint8_t tmpBuff[7];

  tmpBuff[0] = tempWarning;
  tmpBuff[1] = tempAlarm;
  tmpBuff[2] = flowWarning;
  tmpBuff[3] = flowAlarm;
  tmpBuff[4] = isTempUse;
  tmpBuff[5] = isFlowUse;
  tmpBuff[6] = isSoundEnabled;
  EEPROM.update(SETTINGS_ADDR_TEMP_WARNING, tempWarning);
  EEPROM.update(SETTINGS_ADDR_TEMP_ALARM, tempAlarm);
  EEPROM.update(SETTINGS_ADDR_FLOW_WARNING, flowWarning);
  EEPROM.update(SETTINGS_ADDR_FLOW_ALARM, flowAlarm);
  EEPROM.update(SETTINGS_ADDR_TEMP_USE, isTempUse);
  EEPROM.update(SETTINGS_ADDR_FLOW_USE, isFlowUse);
  EEPROM.update(SETTINGS_ADDR_SOUND_ENABLED, isSoundEnabled);

  FastCRC16 CRC16;
  calcCrc = CRC16.ccitt(tmpBuff, sizeof(tmpBuff));
  EEPROM.update(SETTINGS_ADDR_CRC_H, (uint8_t)(calcCrc >> 8));
  EEPROM.update(SETTINGS_ADDR_CRC_L, (uint8_t)calcCrc);
}
