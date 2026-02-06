#ifndef SECURITY_CODE_H
#define SECURITY_CODE_H

void setupSecurity();
bool runSecurityLogic();
void resetAlarmState();
void setLedColorHSB(float hue, float saturation, float brightness);

#endif