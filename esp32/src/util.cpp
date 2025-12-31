#include "globals.h"
#include <Arduino.h>
#include <WiFiClient.h>
#include <stdio.h>
#include <time.h>

bool getDateString(char *buff, bool pp) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return false;
    }
    if (pp)
        sprintf(buff, "%d/%d/%d", 1 + timeinfo.tm_mon, timeinfo.tm_mday,
                1900 + timeinfo.tm_year);
    else
        sprintf(buff, "%d-%02d-%02d", 1900 + timeinfo.tm_year, 1 + timeinfo.tm_mon,
                timeinfo.tm_mday);
    return true;
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
           (buffer == NULL || (idx < (buffer_len - 1)))) {
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

int readStringUntil(String s, const char *match, int match_len, char *buffer,
                    int buffer_len, bool dump_chars = false) {
    int i = 0;
    int idx = 0;
    while ((idx < s.length()) && i < match_len &&
           (buffer == NULL || (idx < (buffer_len - 1)))) {
        char c = s[idx];
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
    }
    if (buffer != NULL)
        buffer[idx] = 0;
    return idx;
}

void printAlign(char *msg, int indent = 0) {
    char buf[TEXT_WIDTH + 1] = ""; // TEXT_WIDTH + NULL-Terminator
    int buf_idx = 0;
    Serial.println(msg);
    // Serial.println(strlen(msg));
    char *p = msg;
    const int MAX_WORD_LEN = 256;
    char next_word[MAX_WORD_LEN] = "";

    while (*p != '\0') {
        int word_len = 0;
        memset(next_word, 0, MAX_WORD_LEN);
        bool space_token = isspace(*p);

        while (*p != '\0' && word_len + 1 < MAX_WORD_LEN) {
            if (space_token) {
                if (isspace(*p)) {
                    next_word[word_len] = *p;
                    word_len += 1;
                    p++;
                } else {
                    break;
                }
            } else {
                if (!isspace(*p)) {
                    next_word[word_len] = *p;
                    word_len += 1;
                    p++;
                } else {
                    break;
                }
            }
        }
        next_word[word_len] = 0;
        if (buf_idx + word_len <= TEXT_WIDTH) {
            memcpy(buf + buf_idx, next_word, word_len);
            buf_idx += word_len;
        } else {
            int indent_amt = indent;
            if (space_token && word_len > 1) {
                indent_amt = min(indent + word_len, TEXT_WIDTH);
            }
            if (space_token) {
                Serial.println(buf);
                printer.println(buf);
                buf_idx = 0;
                memset(buf, 0, TEXT_WIDTH + 1);
                for (int i = 0; i < indent_amt; i++) {
                    buf[buf_idx] = ' ';
                    buf_idx += 1;
                }
            } else if (indent_amt + word_len <= TEXT_WIDTH) {
                Serial.println(buf);
                printer.println(buf);
                buf_idx = 0;
                memset(buf, 0, TEXT_WIDTH + 1);
                for (int i = 0; i < indent_amt; i++) {
                    buf[buf_idx] = ' ';
                    buf_idx += 1;
                }
                memcpy(buf + buf_idx, next_word, word_len);
                buf_idx += word_len;
            } else {
                int word_idx = 0;
                while (word_idx < word_len) {
                    buf[buf_idx] = next_word[word_idx];
                    buf_idx += 1;
                    word_idx += 1;
                    if (buf_idx == 31) {
                        buf[31] = '-';
                        Serial.println(buf);
                        printer.println(buf);
                        memset(buf, 0, TEXT_WIDTH + 1);
                        buf_idx = 0;
                        for (int i = 0; i < indent_amt; i++) {
                            buf[buf_idx] = ' ';
                            buf_idx += 1;
                        }
                    }
                }
            }
        }
    }
    if (buf_idx > 0) {
        Serial.println(buf);
        printer.println(buf);
    }
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

bool ensureInternet(unsigned long timeout_ms = 15000) {
    if (WiFi.isConnected())
        return true;
    if (strlen(ssid) == 0)
        return false;

    WiFi.setSleep(false);
    WiFi.begin(ssid, wifi_pass);

    unsigned long start = millis();
    while (!WiFi.isConnected()) {
        if (millis() - start > timeout_ms) {
            WiFi.disconnect();
            return false;
        }
        delay(100);
    }
    return true;
}
