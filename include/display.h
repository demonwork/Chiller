#ifndef DISPLAY__H
#define CHILLER__H

#include <stdint.h>

void flashBacklight();
void restoreBackLight();
void displayMainScreenTemp();
void displayMainScreenFlow();
void drawAlertInfo(const char *title, const char *footer, uint8_t value, uint8_t mode);
void displaySetValue(const char *title, uint8_t value);
void startTimeOut(uint8_t seconds);
void displaySetUseValue(const char *title, bool value);

#endif
