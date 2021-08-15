#include "ChillerSettings.h"
#include <EEPROM.h>
#include <FastCRC.h>

ChillerSettings::ChillerSettings() {
    setDefaults();
}

bool ChillerSettings::isCrcValid()
{
    return _isCrcValid;
}

uint8_t ChillerSettings::getTempWarning()
{
    return settings.tempWarning;
}

uint8_t ChillerSettings::getTempAlarm()
{
    return settings.tempAlarm;
}

uint8_t ChillerSettings::getFlowWarning()
{
    return settings.flowWarning;
}

uint8_t ChillerSettings::getFlowAlarm()
{
    return settings.flowAlarm;
}

bool ChillerSettings::isTempUse()
{
    return settings.isTempUse;
}

bool ChillerSettings::isFlowUse()
{
    return settings.isFlowUse;
}

bool ChillerSettings::isSoundEnabled()
{
    return settings.isSoundEnabled;
}

uint8_t ChillerSettings::getStartTimeout()
{
    return settings.startTimeout;
}

void ChillerSettings::setTempWarning(uint8_t value)
{
    settings.tempWarning = value;
}

void ChillerSettings::setTempAlarm(uint8_t value)
{
    settings.tempAlarm = value;
}

void ChillerSettings::setFlowWarning(uint8_t value)
{
    settings.flowWarning = value;
}

void ChillerSettings::setFlowAlarm(uint8_t value)
{
    settings.flowAlarm = value;
}

void ChillerSettings::setTempUse(bool value)
{
    settings.isTempUse = value;
}

void ChillerSettings::setFlowUse(bool value)
{
    settings.isFlowUse = value;
}

void ChillerSettings::setSoundEnabled(bool value)
{
    settings.isSoundEnabled = value;
}

void ChillerSettings::setStartTimeout(uint8_t value)
{
    settings.startTimeout = value;
}

void ChillerSettings::read()
{
    uint16_t eepromCrc, calcCrc;
    FastCRC16 CRC16;

    eepromCrc = EEPROM.read(SETTINGS_ADDR_CRC_H) << 8;
    eepromCrc |= 0x00FF & EEPROM.read(SETTINGS_ADDR_CRC_L);

    settings.tempWarning = EEPROM.read(SETTINGS_ADDR_TEMP_WARNING);
    settings.tempAlarm = EEPROM.read(SETTINGS_ADDR_TEMP_ALARM);
    settings.flowWarning = EEPROM.read(SETTINGS_ADDR_FLOW_WARNING);
    settings.flowAlarm = EEPROM.read(SETTINGS_ADDR_FLOW_ALARM);
    settings.isTempUse = EEPROM.read(SETTINGS_ADDR_TEMP_USE);
    settings.isFlowUse = EEPROM.read(SETTINGS_ADDR_FLOW_USE);
    settings.isSoundEnabled = EEPROM.read(SETTINGS_ADDR_SOUND_ENABLED);
    settings.startTimeout = EEPROM.read(SETTINGS_ADDR_START_TIMEOUT);

    calcCrc = CRC16.ccitt((uint8_t *)&settings, sizeof(settings));
    _isCrcValid = eepromCrc == calcCrc;
}

void ChillerSettings::write()
{
    uint16_t calcCrc;
    FastCRC16 CRC16;

    calcCrc = CRC16.ccitt((uint8_t *)&settings, sizeof(settings));

    EEPROM.update(SETTINGS_ADDR_TEMP_WARNING, settings.tempWarning);
    EEPROM.update(SETTINGS_ADDR_TEMP_ALARM, settings.tempAlarm);
    EEPROM.update(SETTINGS_ADDR_FLOW_WARNING, settings.flowWarning);
    EEPROM.update(SETTINGS_ADDR_FLOW_ALARM, settings.flowAlarm);
    EEPROM.update(SETTINGS_ADDR_TEMP_USE, settings.isTempUse);
    EEPROM.update(SETTINGS_ADDR_FLOW_USE, settings.isFlowUse);
    EEPROM.update(SETTINGS_ADDR_SOUND_ENABLED, settings.isSoundEnabled);
    EEPROM.update(SETTINGS_ADDR_START_TIMEOUT, settings.startTimeout);
    EEPROM.update(SETTINGS_ADDR_CRC_H, (uint8_t)(calcCrc >> 8));
    EEPROM.update(SETTINGS_ADDR_CRC_L, (uint8_t)calcCrc);
    _isCrcValid = true;
}

void ChillerSettings::setDefaults()
{
    settings.tempWarning = 20;
    settings.tempAlarm = 25;
    settings.flowWarning = 100;
    settings.flowAlarm = 90;
    settings.isTempUse = false;
    settings.isFlowUse = false;
    settings.isSoundEnabled = true;
    settings.startTimeout = 5;
    _isCrcValid = true;
}