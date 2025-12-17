#include <Arduino.h>
#include <WiFi.h>
#include "wifi_setup.h"
#include "globals.h"
#include "util.h"
#include "board.h"

void setup() {
    pinMode(ONBOARD_LED, OUTPUT);
    pinMode(BUTT_LED, OUTPUT);
    pinMode(BUTT_LED_OUT, OUTPUT);
    digitalWrite(BUTT_LED_OUT, LOW);

    pinMode(BUTT, INPUT_PULLUP);
    digitalWrite(ONBOARD_LED, LOW);

    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, RX, TX);
    delay(10);

    wm.addParameter(&print_time_param);
    wm.setEnableConfigPortal(false);
    if (!wm.autoConnect()) {
        wm.setEnableConfigPortal(true);
        WifiSetup();
    }
    wm.setEnableConfigPortal(true);
    digitalWrite(BUTT_LED, HIGH);

    Serial.println('\n');
    Serial.println("Connection established!");
    configTime(0, 0, "time.google.com", "pool.ntp.org");
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
    tzset();
    prefs.begin("config", false);
    char pt[10];
    prefs.getString("print_time", pt, 10);
    prefs.end();
    Serial.printf("Setup to print at: %s\n", pt);
    print_hr = atoi(pt);
    printer.begin();
}

void loop() {
    struct tm timeinfo;
    if (WiFi.isConnected() && getLocalTime(&timeinfo)) {
        int hour = timeinfo.tm_hour;
        int minute = timeinfo.tm_min;
        if (hour == print_hr && !print_today) {
            print_today = true;
            getAndPrintCrossword();
        } else if (hour == 0 && minute == 1) {
            print_today = false;
        }
    }

    int cur_button = digitalRead(BUTT);

    // press
    if (cur_button == LOW && !pressed) {
        press_time = millis();
        pressed = true;
    }

    // unpress
    if (pressed && cur_button == HIGH) {
        delay(20);
        if (digitalRead(BUTT) != HIGH)
            return;

        if (!WiFi.isConnected()) {
            char msg[128];
            sprintf(msg,
                    "Tried to print, but was not connected to WiFi network: %s and "
                    "password: %s "
                    "Attempting to reconnect...",
                    WiFi.SSID(), WiFi.psk());
            printDebug(msg);
            WiFi.disconnect();
            WiFi.begin();
        } else {
            getAndPrintCrossword();
        }
        pressed = false;
    }

    // hold
    if (pressed && millis() - press_time > 5000) {
        WifiSetup();
        digitalWrite(BUTT_LED, HIGH);
        pressed = false;
    }
}