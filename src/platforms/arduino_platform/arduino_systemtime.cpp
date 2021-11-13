#include "KraftKontrol/platforms/arduino_platform/arduino_systemtime.h"


int64_t NOW() {

    static int32_t g_lastMicroseconds = 0;
    static int64_t g_currentTime = 0;

    int32_t time = micros();

    if (time != g_lastMicroseconds) {
        g_currentTime += int64_t(time - g_lastMicroseconds)*MICROSECONDS;
        g_lastMicroseconds = time;
    }

    return g_currentTime;

}

