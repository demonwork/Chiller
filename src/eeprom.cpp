#include <stdint.h>
#include <EEPROM.h>
#include <FastCRC.h>
#include "eeprom.h"

extern uint8_t tempWarning;
extern uint8_t tempAlarm;
extern uint8_t flowWarning;
extern uint8_t flowAlarm;

bool readSettings()
{
  uint16_t eepromCrc, calcCrc;
  uint8_t settings = 0;
  uint8_t tmpBuff[4];

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
