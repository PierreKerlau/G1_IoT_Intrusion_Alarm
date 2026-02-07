#ifndef SECURITY_CODE_H
#define SECURITY_CODE_H

void setupSecurity();
bool runSecurityLogic();
void startAlarmState();
void setLedColorHSB(float hue, float saturation, float brightness);
bool isWaitingForRelease();

#endif // SECURITY_CODE_H