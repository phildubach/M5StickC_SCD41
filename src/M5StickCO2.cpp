#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>

#include <stdio.h>

#include <SensirionI2CScd4x.h>
#include <PubSubClient.h>
#include "json.h"

#include "Monospaced_plain_22.h"

#if defined(ARDUINO_M5Stack_CoreInk)
#define SKIP_MEASUREMENTS 0
#else
#define SKIP_MEASUREMENTS 0
#endif

#if defined(WIFI_SSID) && defined(WIFI_PASS) and \
    defined(MQTT_HOST) && defined(MQTT_TOPIC)
#define USE_MQTT
#endif

static char errorMessage[256];
SensirionI2CScd4x scd4x;
TwoWire i2c_ext = TwoWire(0);
#ifdef USE_MQTT
static char safety_buffer[100];
static char client_id[32];
static WiFiClient espClient;
static PubSubClient mqtt(espClient);
static char json_string[128];
#endif

M5Canvas canvas(&M5.Display);
uint16_t origin_x, origin_y;

void setup() {
    auto cfg = M5.config();

    cfg.serial_baudrate = 115200;
    cfg.clear_display = true;

    M5.begin(cfg);
#if defined(ARDUINO_M5Stick_C)
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);
#endif
    if (M5.Display.isEPD()) {
        M5.Display.setEpdMode(epd_mode_t::epd_fastest);
    }
    M5.Display.invertDisplay(true);
    M5.Display.clear(TFT_BLACK);

    canvas.setColorDepth(1);
    canvas.createSprite(160, 80);
    canvas.setTextSize(1);
    canvas.setFont(&Monospaced_plain_22);
    origin_x = (M5.Display.width() - canvas.width())/2;
    origin_y = (M5.Display.height() - canvas.height())/2;

    canvas.clear();
    canvas.setCursor(0,0);
    canvas.println("Measuring");

    canvas.pushSprite(origin_x, origin_y);

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

#ifdef USE_MQTT
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int count = 0;
    while ((WiFi.status() != WL_CONNECTED) && ++count <= 10) {
        canvas.print(".");
        canvas.pushSprite(origin_x, origin_y);
        delay(1000);
    }
    // attempt MQTT connection regardless of WiFi state, so params are set
    snprintf(client_id, sizeof(client_id), "ESP32-%012X", ESP.getEfuseMac());
    mqtt.setServer(MQTT_HOST, 1883);
    mqtt.connect(client_id);
#endif //defined(WIFI_SSID) && defined(WIFI_PASS)
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
        static uint8_t numRefresh = SKIP_MEASUREMENTS;
        if (++numRefresh > SKIP_MEASUREMENTS) {
            numRefresh = 0;
            canvas.clear();
            canvas.setCursor(0,0);
            snprintf(buf, sizeof(buf), "%6d ppm", co2);
            canvas.println(buf);
            snprintf(buf, sizeof(buf), "%6.1f C", temperature);
            canvas.println(buf);
            snprintf(buf, sizeof(buf), "%6.1f %%", humidity);
            canvas.println(buf);
            canvas.pushSprite(origin_x, origin_y);
#ifdef USE_MQTT
            JsonBuffer json;
            json_init_buffer(&json, json_string, sizeof(json_string));
            json_start_object(&json);
            json_named_cstring_append(&json, "id", client_id);
            json_named_uint32_append(&json, "co2", co2);
            json_named_fixedpoint_append(&json, "temp", (uint32_t)(temperature*10),  1);
            json_named_fixedpoint_append(&json, "hum", (uint32_t)(humidity*10), 1);
            json_end_object(&json);
            json_terminate(&json);
            mqtt.publish(MQTT_TOPIC, json_string);
#endif
        }
    }
    M5.update();
#ifdef USE_MQTT
    mqtt.loop();
#endif
}
