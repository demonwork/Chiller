#ifndef ChillerSettings_h
#define ChillerSettings_h

#include <stdint.h>

// #pragma pack(push,1)
// typedef struct Settings
// {
//     uint16_t crc;
//     uint8_t tempWarning;
//     uint8_t tempAlarm;
//     uint8_t flowWarning;
//     uint8_t flowAlarm;
//     bool tempUse;
//     bool flowUse;
//     bool soundUse;
//     uint8_t startTimeOut;
// };
// #pragma pack(pop)

#define SETTINGS_ADDR_CRC_H 0
#define SETTINGS_ADDR_CRC_L 1
#define SETTINGS_ADDR_TEMP_WARNING 2
#define SETTINGS_ADDR_TEMP_ALARM 3
#define SETTINGS_ADDR_FLOW_WARNING 4
#define SETTINGS_ADDR_FLOW_ALARM 5
#define SETTINGS_ADDR_TEMP_USE 6
#define SETTINGS_ADDR_FLOW_USE 7
#define SETTINGS_ADDR_SOUND_ENABLED 8
#define SETTINGS_ADDR_START_TIMEOUT 9

class ChillerSettings
{
public:
    bool isCrcValid();
    uint8_t getTempWarning();
    uint8_t getTempAlarm();
    uint8_t getFlowWarning();
    uint8_t getFlowAlarm();
    bool isTempUse();
    bool isFlowUse();
    bool isSoundEnabled();
    uint8_t getStartTimeout();

    void setTempWarning(uint8_t value);
    void setTempAlarm(uint8_t value);
    void setFlowWarning(uint8_t value);
    void setFlowAlarm(uint8_t value);
    void setTempUse(bool value);
    void setFlowUse(bool value);
    void setSoundEnabled(bool value);
    void setStartTimeout(uint8_t value);

    void read();
    void write();
    void setDefaults();

private:
    // контрольная сумма
    bool _isCrcValid;
    // значение опасно высокой температуры
    uint8_t tempWarning;
    // значение аварийно высокой температуры
    uint8_t tempAlarm;
    // значение опасно низкого потока
    uint8_t flowWarning;
    // значение аварийно низкого потока
    uint8_t flowAlarm;
    // используем датчик температуры?
    bool tempUse;
    // используем датчик потока?
    bool flowUse;
    // включить звук?
    bool soundEnabled;
    // время задержки работы при старте в секундах
    uint8_t startTimeout;
};

#endif