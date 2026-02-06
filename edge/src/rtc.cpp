#include "Wire.h"
#include "iarduino_RTC.h"

iarduino_RTC rtc(RTC_DS1307); // Module DS1307 I2C

void setupRTC() {
    rtc.begin();

    // The time only needs to be set once, or after a power loss
    // rtc.settime(
    //     40, // seconds
    //     17, // minutes
    //     11, // hours
    //     5, // day
    //     2, // month
    //     26, // year
    //     4   // weekday (0-6, 0=Sunday)
    // );
}

String getTimeString() {
    return String(rtc.gettime("d-m-Y, H:i:s, D"));
}
