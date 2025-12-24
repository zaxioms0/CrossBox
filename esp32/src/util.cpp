#include "globals.h"
#include <Arduino.h>
#include <WiFiClient.h>
#include <stdio.h>
#include <time.h>

void getDateString(char *buff, bool pp) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        exit(1);
    }
    if (pp)
        sprintf(buff, "%d/%d/%d", 1 + timeinfo.tm_mon, timeinfo.tm_mday,
                1900 + timeinfo.tm_year);
    else
        sprintf(buff, "%d-%02d-%02d", 1900 + timeinfo.tm_year, 1 + timeinfo.tm_mon,
                timeinfo.tm_mday);
}

void getDateStringEpoch(char *buff, time_t epoch, bool pp) {
    struct tm timeinfo;
    localtime_r(&epoch, &timeinfo);
    if (pp)
        sprintf(buff, "%d/%d/%d", 1 + timeinfo.tm_mon, timeinfo.tm_mday,
                1900 + timeinfo.tm_year);
    else
        sprintf(buff, "%d-%02d-%02d", 1900 + timeinfo.tm_year, 1 + timeinfo.tm_mon,
                timeinfo.tm_mday);
}

// returns index of null terminator in buff
int readStreamUntil(WiFiClient *stream, const char *match, int match_len, char *buffer,
                    int buffer_len, bool dump_chars = false) {
    int i = 0;
    int idx = 0;
    while ((stream->connected() || stream->available()) && i < match_len &&
           (!buffer || (idx < buffer_len))) {
        if (stream->available()) {
            char c = stream->read();
            if (dump_chars)
                Serial.print(c);
            if (c == match[i]) {
                i += 1;
            } else {
                i = 0;
            }
            if (buffer != NULL) {
                buffer[idx] = c;
            }
            idx += 1;
        } else {
            yield();
        }
    }
    if (buffer != NULL)
        buffer[idx] = 0;
    return idx;
}

void printDebug(char *debug_msg) {
    printer.wake();
    printer.reset();
    printer.println(debug_msg);
    printer.println();
    printer.println();
    printer.reset();
}

void threadBlink(void *count) {
    int cnt = (int)count;
    int i = 0;
    while (cnt == -1 || i < cnt) {
        digitalWrite(BUTT_LED, HIGH);
        vTaskDelay(200);
        digitalWrite(BUTT_LED, LOW);
        vTaskDelay(200);
        i += 1;
    }
    vTaskDelete(NULL);
}