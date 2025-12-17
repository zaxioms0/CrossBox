#include <Arduino.h>
#include <WiFi.h>
#include "WiFiManager.h"
#include "globals.h"

void WifiSetup() {
    wm.setConfigPortalBlocking(false);
    wm.startConfigPortal("CrossBox Setup");

    unsigned long flash_time = millis();
    bool led_state = true;
    digitalWrite(BUTT_LED, led_state);
    while (wm.getConfigPortalActive()) {
        wm.process();
        if (millis() - flash_time > 1000) {
            led_state = !led_state;
            digitalWrite(BUTT_LED, led_state);
            flash_time = millis();
        }
    }
    prefs.begin("config", false);
    Serial.printf("got print time: %s\n", print_time_param.getValue());
    prefs.putString("print_time", print_time_param.getValue());
    char pt[10];
    prefs.getString("print_time", pt, 10);
    Serial.printf("Checking if there: %s\n", pt);
    print_hr = atoi(pt);
    prefs.end();
    WiFi.begin();
    Serial.println("Done with Wifi");
    digitalWrite(BUTT_LED, LOW);
}
