#include "globals.h"
#include "nyt.h"
#include "puzzmo.h"
#include "util.h"
#include "wifi_setup.h"
#include <Arduino.h>
#include <WiFi.h>

void setup() {
    pinMode(ONBOARD_LED, OUTPUT);
    pinMode(BUTT_LED, OUTPUT);
    pinMode(BUTT_LED_OUT, OUTPUT);
    pinMode(BUTT, INPUT_PULLUP);

    digitalWrite(BUTT_LED_OUT, LOW);
    digitalWrite(ONBOARD_LED, LOW);

    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, RX, TX);
    printer.begin();

    delay(10);

    wm.addParameter(&nyts_param);
    wm.addParameter(&print_time_param);
    wm.addParameter(&crosstype_param);
    wm.setEnableConfigPortal(false);
    if (!wm.autoConnect()) {
        wm.setEnableConfigPortal(true);
        WifiSetup();
    }
    wm.setEnableConfigPortal(true);
    Serial.println('\n');
    Serial.println("Connection established!");
    configTime(0, 0, "time.google.com", "pool.ntp.org");
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
    tzset();

    prefs.begin("config", false);
    print_hr = prefs.getInt("print_time", -1);
    primary_cross = (CrossType)prefs.getInt("cross_type", NYT);
    strcpy(nyts, prefs.getString("nyts", "").c_str());
    strcpy(ssid, prefs.getString("ssid", "").c_str());
    strcpy(wifi_pass, prefs.getString("wifi_pass", "").c_str());
    Serial.printf("Setup to print at %d\n", print_hr);
    prefs.end();

    WiFi.setSleep(false);
    if (!ensureInternet()) {
        char msg[256];
        sprintf(msg,
                "Was not connected to WiFi network: %s and "
                "password: %s at end of startup sequence. Restarting.",
                ssid, wifi_pass);
        printDebug(msg);
        ESP.restart();
    }
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("SETUP FAILED");
        char msg[] = "Could not retrieve time at end of startup sequence despite being "
                     "connected to WiFi. Restarting.";
        printDebug(msg);
        ESP.restart();
    }
    Serial.println("Setup Complete");
    xTaskCreate(threadBlink, "blink", 1024, (void *)3, 1, NULL);
}

void printPrimaryCrossword() {
    switch (primary_cross) {
    case NYT: {
        if (strlen(nyts) <= 5) { // "" or "NYT-S"
            char msg[] = "NYT-S Cookie not set";
            printDebug(msg);
            break;
        }
        getAndPrintNYTCrossword();
        break;
    }
    case Puzzmo: {
        getAndPrintPuzzmoCrossword();
        break;
    }
    default: {
        char msg[] = "Primary crossword set improperly.";
        printDebug(msg);
    }
    }
}

void loop() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        int hour = timeinfo.tm_hour;
        int minute = timeinfo.tm_min;
        if (hour == print_hr && !print_today) {
            if (!ensureInternet()) {
                char msg[128];
                sprintf(msg,
                        "Tried to print at %d:%02d, but failed to connect to the "
                        "internet.\n",
                        hour, minute);
                printDebug(msg);
            } else {
                TaskHandle_t handle;
                xTaskCreate(threadBlink, "blink", 1024, (void *)-1, 1, &handle);
                printPrimaryCrossword();
                vTaskDelete(handle);
                digitalWrite(BUTT_LED, LOW);
            }
            print_today = true;
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

        if (!ensureInternet() || !getLocalTime(&timeinfo)) {
            char msg[256];
            sprintf(msg,
                    "Tried to print, but was not connected to WiFi network: %s and "
                    "password: %s or could not get time. Please try again.",
                    ssid, wifi_pass);
            printDebug(msg);
        } else {
            Serial.println("Printing Start");
            TaskHandle_t handle;
            xTaskCreate(threadBlink, "blink", 1024, (void *)-1, 1, &handle);
            printPrimaryCrossword();
            vTaskDelete(handle);
            digitalWrite(BUTT_LED, LOW);
        }
        pressed = false;
        Serial.println(esp_get_free_heap_size());
    }

    // hold
    if (pressed && millis() - press_time > 5000) {
        WifiSetup();
        pressed = false;
    }
}