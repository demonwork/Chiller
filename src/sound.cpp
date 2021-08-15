#include <stdint.h>
#include <Arduino.h>
#include "sound.h"
#include "chiller.h"
#include "ChillerSettings.h"

extern ChillerSettings settings;

/**
 * Звук предупреждения о опасной ситуации
 */
void soundBeep()
{
  if (!settings.isSoundEnabled()) {
    noTone(PIN_PIZO);
    return;
  }

  static bool up = true;
  static uint16_t i = 700;
  if (up)
  {
    if (i < 800)
    {
      tone(PIN_PIZO, 400);
      delay(15);
      i++;
    }

    if (i == 800)
    {
      up = false;
      noTone(PIN_PIZO);
    }
  }
  else
  {
    if (i > 700)
    {
      // tone(PIN_PIZO, i);
      delay(15);
      i--;
    }

    if (i == 700)
    {
      up = true;
    }
  }
}

/**
 * Звук предупреждения о аварии
 */
void soundSiren()
{
  if (!settings.isSoundEnabled()) {
    noTone(PIN_PIZO);
    return;
  }

  static bool up = true;
  static uint16_t i = 700;
  if (up)
  {
    if (i < 800)
    {
      tone(PIN_PIZO, i);
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
      tone(PIN_PIZO, i);
      delay(15);
      i--;
    }

    if (i == 700)
    {
      up = true;
    }
  }
}

