#include "globals.h"
#include "util.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

void WifiSetup() {
    wm.setConfigPortalBlocking(false);
    wm.setBreakAfterConfig(true);
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
    if (print_time_param.getValueLength() > 0) {
        int h = atoi(print_time_param.getValue());
        prefs.putInt("print_time", h);
        print_hr = h;
    }
    Serial.printf("got NYT-S: %s\n", nyts_param.getValue());
    if (strlen(nyts_param.getValue()) > 5) {
        prefs.putString("nyts", nyts_param.getValue());
        strcpy(nyts, nyts_param.getValue());
    }
    if (strlen(crosstype_param.getValue()) > 0) {
        String crosstype_enter = crosstype_param.getValue();
        Serial.println(crosstype_enter);
        crosstype_enter.trim();
        crosstype_enter.toLowerCase();
        if (crosstype_enter == "puzzmo" || crosstype_enter == "pzm") {
            primary_cross = Puzzmo;
            prefs.putInt("cross_type", Puzzmo);
        } else {
            primary_cross = NYT;
            prefs.putInt("cross_type", NYT);
        }
    }
    prefs.putString("ssid", wm.getWiFiSSID().c_str());
    prefs.putString("wifi_pass", wm.getWiFiPass().c_str());
    prefs.end();
    Serial.println("Done with Wifi");
    digitalWrite(BUTT_LED, LOW);
}
