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
#include <sstream>
#include <time.h>
#include <vector>

void makeGameplayId(char *buf, int buf_len) {
    const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    for (int i = 0; i < buf_len - 1; i++) {
        buf[i] = charset[random(sizeof(charset) - 1)];
    }
    buf[buf_len] = 0;
}

bool inBoundsGrid(std::vector<String> &board, int r, int c) {
    return r >= 0 && c >= 0 && r < board.size() && c < board[0].length();
}

std::optional<Grid> getPuzzmoGridData() {
    char url[64] = "https://www.puzzmo.com/_api/prod/graphql?PlayGameScreenQuery";
    char date[32];
    WiFiClientSecure client;
    HTTPClient http;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return {};
    }
    time_t puzz_epoch = time(NULL);
    if (timeinfo.tm_hour == 0) {
        // get yesterday's puzzle
        puzz_epoch -= 86400;
    }
    getDateStringEpoch(date, puzz_epoch, false);

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    client.setInsecure();
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    char gameplay_id[32];
    makeGameplayId(gameplay_id, 32);
    http.addHeader("Puzzmo-Gameplay-Id", gameplay_id);
    delay(500);

    char query[] =
        "query PlayGameScreenQuery("
        "  $finderKey: String!"
        "  $gameContext: StartGameContext!"
        ") {"
        "  startOrFindGameplay(finderKey: $finderKey, context: $gameContext) {"
        "    __typename"
        "    ... on ErrorableResponse {"
        "      message"
        "      failed"
        "      success"
        "    }"
        "    ... on HasGamePlayed {"
        "      gamePlayed {"
        "        puzzle {"
        "          name"
        "          dailyTitle"
        "          puzzle"
        "          authors {"
        "            publishingName"
        "            name"
        "          }"
        "        }"
        "      }"
        "    }"
        "  }"
        "}";

    char finder_key[64];
    sprintf(finder_key, "today:/%s/crossword/mini", date);

    char payload[1024];
    sprintf(payload,
            "{"
            "\"operationName\":\"PlayGameScreenQuery\","
            "\"query\":\"%s\","
            "\"variables\":{"
            "  \"finderKey\":\"%s\","
            "  \"gameContext\":{"
            "    \"partnerSlug\":null,"
            "    \"pingOwnerForMultiplayer\":true"
            "  }"
            "}"
            "}",
            query, finder_key);

    int httpResponseCode = http.POST((uint8_t *)payload, strlen(payload));
    if (httpResponseCode != 200) {
        char msg[64];
        sprintf(msg, "Failed to get Puzzmo data. HTTP response: %d\n",
                httpResponseCode);
        Serial.println(msg);
        Serial.println(payload);
        return {};
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    if (error) {
        Serial.println("JSON failed");
        Serial.println(error.c_str());
        return {};
    }

    if (doc["data"]["startOrFindGameplay"]["__typename"] == "ErrorableResponse") {
        Serial.println(doc["data"]["startOrFindGameplay"]["message"].as<String>());
        return {};
    }
    JsonObject puzzle_data = doc["data"]["startOrFindGameplay"]["gamePlayed"]["puzzle"];
    serializeJsonPretty(puzzle_data, Serial);
    String puz_xd = puzzle_data["puzzle"].as<String>();
    Serial.println(puz_xd);
    int start_grid = puz_xd.indexOf("## Grid\n\n") + 9;
    int end_grid = puz_xd.indexOf("\n\n## Clues") + 1;
    String grid_xd = puz_xd.substring(start_grid, end_grid);
    std::vector<String> text_grid = {};
    String buffer = "";
    for (int i = 0; i < grid_xd.length(); i++) {
        if (grid_xd[i] == '\n') {
            text_grid.push_back(buffer);
            buffer = "";
        } else {
            buffer += grid_xd[i];
        }
    }

    Grid grid;
    grid.header = "Puzzmo Mini";
    grid.title = puzzle_data["name"].as<String>();
    grid.puzz_epoch = puzz_epoch;
    grid.width = text_grid[0].length();
    grid.height = text_grid.size();
    grid.authors = {};
    grid.down_clues = {};
    grid.across_clues = {};
    grid.square_data = {};

    for (auto author : puzzle_data["authors"].as<JsonArray>()) {
        grid.authors.push_back(author["name"].as<String>());
    }

    int clue_count = 0;
    for (int r = 0; r < grid.height; r++) {
        for (int c = 0; c < grid.width; c++) {
            Serial.printf("%d, %d\n", r, c);
            if (text_grid[r][c] == '.') {
                Square s;
                s.col = c;
                s.row = r;
                s.data = 0;
                grid.square_data.push_back(s);
            } else {
                bool across_clue = false;
                if ((c == 0 || text_grid[r][c - 1] == '.') &&
                    inBoundsGrid(text_grid, r, c + 1) && text_grid[r][c + 1] != '.') {
                    // across clue
                    across_clue = true;
                    clue_count += 1;
                    Serial.printf("Got across %d\n", clue_count);
                    Square s;
                    s.col = c;
                    s.row = r;
                    s.data = clue_count;
                    grid.square_data.push_back(s);
                }
                if (r == 0 || text_grid[r - 1][c] == '.' &&
                                  inBoundsGrid(text_grid, r + 1, c) &&
                                  text_grid[r + 1][c] != '.') {
                    // down clue
                    if (!across_clue) {
                        clue_count += 1;
                        Square s;
                        s.col = c;
                        s.row = r;
                        s.data = clue_count;
                        grid.square_data.push_back(s);
                    }
                    Serial.printf("Got down %d\n", clue_count);
                }
            }
        }
    }

    String clues_xd = puz_xd.substring(puz_xd.indexOf("## Clues\n\n") + 10);
    String clue_buf = "";
    int idx = 0;
    while (idx >= 0 && idx < clues_xd.length() - 1) {
        int old_idx = idx;
        idx = clues_xd.indexOf('\n', idx+1);
        String clue_line = clues_xd.substring(old_idx, idx);
        clue_line.trim();
        if (clue_line.length() == 0)
            continue;
        int period_idx = clue_line.indexOf('.');
        int tilde_idx = clue_line.indexOf('~');
        int clue_num = atoi(clue_line.substring(1, period_idx + 1).c_str());
        Clue clue;
        clue.num = clue_num;
        clue.data = clue_line.substring(period_idx + 2, tilde_idx);
        if (clue_line[0] == 'A') {
            grid.across_clues.push_back(clue);
        } else {
            grid.down_clues.push_back(clue);
        }
    }
    printGridDataSerial(grid);
    Serial.println(esp_get_free_heap_size());
    return grid;
}

void getAndPrintPuzzmoCrossword() {
    std::optional<Grid> data_opt = std::nullopt;
    for (int i = 0; i < 3; i++) {
        data_opt = getPuzzmoGridData();
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