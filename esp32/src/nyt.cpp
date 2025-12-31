#include "board.h"
#include "globals.h"
#include "util.h"
#include "wifi_setup.h"
#include <Adafruit_Thermal.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <optional>
#include <time.h>
#include <vector>

std::optional<Grid> getNYTGridData() {
    char url[128];
    char date[32];
    WiFiClientSecure client;
    HTTPClient http;
    time_t puzz_epoch = time(NULL);
    if (!getDateString(date, false)) {
        return {};
    }
    sprintf(url, "https://www.nytimes.com/svc/crosswords/v6/puzzle/mini/%s.json", date);
    Serial.println(url);

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    client.setInsecure();
    http.begin(client, url);
    char nyts_cookie[512];
    sprintf(nyts_cookie, "NYT-S=%s", nyts);
    Serial.println(nyts_cookie);
    http.addHeader("Cookie", nyts_cookie);
    delay(500);
    int httpResponseCode = http.GET();
    if (httpResponseCode == 403) {
        char msg[64];
        sprintf(msg, "Failed to get NYT data. HTTP response: %d\n", httpResponseCode);
        Serial.println(msg);
        strcpy(msg, "HTTP got 403. Check your NYT-S token.");
        printDebug(msg);
        return {};
    } else if (httpResponseCode != 200) {
        char msg[64];
        sprintf(msg, "Failed to get NYT data. HTTP response: %d\n", httpResponseCode);
        Serial.println(msg);
        return {};
    }
    NetworkClient *s;
    s = http.getStreamPtr();

    JsonDocument doc;
    char start_target[16] = "\"cells\":";
    char end_target[16] = "SVG";

    Serial.println("Getting JSON");
    readStreamUntil(s, start_target, 8, NULL, 0, false);
    strcpy(scratch, "{\"cells\":");
    int buf_len = readStreamUntil(s, end_target, 3, scratch + 9, SCRATCH_SIZE, false);
    scratch[9 + buf_len - 5] = '}';
    scratch[9 + buf_len - 4] = 0;
    Serial.println("Parsing JSON");

    DeserializationError error = deserializeJson(doc, scratch);
    if (error) {
        Serial.println("JSON parsing clues failed: ");
        Serial.println(error.c_str());
        return {};
    }

    Grid grid;
    grid.height = doc["dimensions"]["height"].as<int>();
    grid.width = doc["dimensions"]["width"].as<int>();
    grid.puzz_epoch = puzz_epoch;
    grid.header = "NYT MINI";
    grid.title = {};
    grid.square_data = {};
    grid.authors = {};
    grid.across_clues = {};
    grid.down_clues = {};
    for (int j = 0; j < grid.height; j++) {
        for (int i = 0; i < grid.width; i++) {
            int idx = i + grid.width * j;
            if (doc["cells"][idx] && doc["cells"][idx].as<JsonObject>().size() == 0) {
                Square d;
                d.row = j;
                d.col = i;
                d.data = 0;
                grid.square_data.push_back(d);
            } else if (doc["cells"][idx]["label"]) {
                int label = atoi(doc["cells"][idx]["label"].as<String>().c_str());
                Square d;
                d.row = j;
                d.col = i;
                d.data = label;
                grid.square_data.push_back(d);
            }
        }
    }
    for (auto clue : doc["clues"].as<JsonArray>()) {
        String direction = clue["direction"].as<String>();
        int label = atoi(clue["label"].as<String>().c_str());
        String text = clue["text"][0]["plain"].as<String>();
        Clue c;
        c.num = label;
        c.data = text;
        if (strcmp(direction.c_str(), "Across") == 0) {
            grid.across_clues.push_back(c);
        } else {
            grid.down_clues.push_back(c);
        }
    }
    Serial.println("Parsed first bit");

    doc.clear();
    strcpy(start_target, "\"constructors\":");
    strcpy(end_target, "\"copyright\"");
    Serial.println("Getting second JSON");
    readStreamUntil(s, start_target, 15, NULL, 0);
    strcpy(scratch, "{\"constructors\":");
    int buflen = readStreamUntil(s, end_target, 11, scratch + 16, SCRATCH_SIZE);
    scratch[16 + buflen - 12] = '}';
    scratch[16 + buflen - 11] = 0;
    error = deserializeJson(doc, scratch);
    if (error) {
        Serial.println("JSON parsing constructors failed: ");
        Serial.println(error.c_str());
        return {};
    }
    for (auto author : doc["constructors"].as<JsonArray>()) {
        String a = author.as<String>();
        grid.authors.push_back(a);
    }

    return grid;
}

void getAndPrintNYTCrossword() {
    std::optional<Grid> data_opt = std::nullopt;
    for (int i = 0; i < 3; i++) {
        data_opt = getNYTGridData();
        if (data_opt)
            break;
        else
            Serial.printf("Print failed: %d\n", i);
    }

    if (!data_opt) {
        char msg[128] = "Failed to get crossword after 3 attempts sorry :( You should "
                        "take a look at Serial for debugging info";
        printDebug(msg);
    } else {
        digitalWrite(ONBOARD_LED, HIGH);
        Grid data = data_opt.value();
        printCrossword(data);
        digitalWrite(ONBOARD_LED, LOW);
    }
}
