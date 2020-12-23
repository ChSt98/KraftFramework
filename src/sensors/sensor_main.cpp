#include "sensors_main.h"


namespace Sensors {

    volatile uint32_t testData;

    volatile double big = 1.34893859384934;

    namespace {

        IntervalControl intervalControl;

    }

}


void Sensors::sensorThread(void* arg) {

    Serial.println("Sensor thread Start!");


    intervalControl.syncInternal();
    intervalControl.setRate(IMU_RATE_HZ);
    //intervalControl.setLimit(false);

    testData = 0;


    while(true) {

        if (intervalControl.isTimeToRun()) { //Run through code if time to run

            testData++;

            #ifdef DO_BIG_CALC_ON_THREADS

                for (long double i = 0; i < 100; i++) {
                    big = sin(big);
                    //big = exp10(big);
                }

                big = 1.34893859384934;

            #endif

        }

        #ifdef YIELD_THREADS_WHEN_DONE
            threads.yield();
        #endif

    }


}


