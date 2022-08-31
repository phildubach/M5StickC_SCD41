#include <Arduino.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>
#include <stdio.h>

#include <SensirionI2CScd4x.h>

#include "Monospaced_plain_22.h"

static char errorMessage[256];
SensirionI2CScd4x scd4x;
TwoWire i2c_ext = TwoWire(0);

void setup() {
    auto cfg = M5.config();

    cfg.serial_baudrate = 115200;
    cfg.clear_display = true;

    M5.begin(cfg);
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);

    M5.Display.setTextSize(1);
    M5.Display.setFont(&Monospaced_plain_22);
    M5.Display.startWrite();
    M5.Display.print("Measuring");
    M5.Display.endWrite();

    M5.Ex_I2C.release();

    i2c_ext.begin(GPIO_NUM_32, GPIO_NUM_33, (uint32_t)400000);
    scd4x.begin(i2c_ext);
    uint16_t error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, sizeof(errorMessage));
        Serial.println(errorMessage);
    }
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, sizeof(errorMessage));
        Serial.println(errorMessage);
    }
}

void loop() {
    uint16_t co2;
    float temperature;
    float humidity;
    delay(100);
    uint16_t error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        // we expect frequent errors, because a measurement will only be
        // available every 5s; could poll more slowly
#ifdef VERBOSE_DEBUG        
        Serial.print("readMeasurement(): ");
        errorToString(error, errorMessage, sizeof(errorMessage));
        Serial.println(errorMessage);
#endif
    } else {
		static char buf[32];
        Serial.print("Co2:");
        Serial.print(co2);
        Serial.print("\t");
        Serial.print("Temperature:");
        Serial.print(temperature);
        Serial.print("\t");
        Serial.print("Humidity:");
        Serial.println(humidity);
        M5.Display.clear();
        M5.Display.setCursor(0,0);
        M5.Display.startWrite();
		snprintf(buf, sizeof(buf), "%6d ppm", co2);
        M5.Display.println(buf);
		snprintf(buf, sizeof(buf), "%6.1f C", temperature);
        M5.Display.println(buf);
		snprintf(buf, sizeof(buf), "%6.1f %%", humidity);
        M5.Display.println(buf);
        M5.Display.endWrite();
    }
    M5.update();
}
