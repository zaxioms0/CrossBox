#include <Adafruit_Thermal.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <nums.h>
#include <optional>
#include <time.h>
#include "wifi_setup.h"
#include "globals.h"
#include "util.h"
#include <vector>

struct SquareData {
    unsigned char row;
    unsigned char col;
    unsigned char data;
};

struct ClueData {
    unsigned char num;
    String data;
};

struct GridData {
    unsigned char height;
    unsigned char width;
    time_t puzz_epoch;
    std::vector<String> authors;
    std::vector<SquareData> square_data;
    std::vector<ClueData> across_clues;
    std::vector<ClueData> down_clues;
};

void printGridDataSerial(GridData d) {
    Serial.printf("Height: %d\n", d.height);
    Serial.printf("Width: %d\n", d.width);
    Serial.print("Author(s): ");
    for (size_t i = 0; i < d.authors.size(); i++) {
        Serial.print(d.authors[i].c_str());
        if (i < d.authors.size() - 1)
            Serial.print(", ");
    }
    Serial.println();
    Serial.print("Data: ");
    for (auto p : d.square_data) {
        Serial.printf("<(%d, %d) : %d> ", p.row, p.col, p.data);
    }
    Serial.println();
    Serial.println("Across:");
    for (auto p : d.across_clues) {
        Serial.printf("%d: %s\n", p.num, p.data.c_str());
    }
    Serial.println();
    Serial.println("Down:");
    for (auto p : d.down_clues) {
        Serial.printf("%d: %s\n", p.num, p.data.c_str());
    }
}

std::optional<GridData> getGridData() {
    char url[128];
    char date[32];
    WiFiClientSecure client;
    HTTPClient http;
    time_t puzz_epoch = time(NULL);
    getDateString(date, false);
    sprintf(url, "https://www.nytimes.com/svc/crosswords/v6/puzzle/mini/%s.json", date);
    Serial.println(url);

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    client.setInsecure();
    http.begin(client, url);
    delay(500);
    int httpResponseCode = http.GET();
    NetworkClient *s;
    if (httpResponseCode == 403) {
        http.end();
        // try again with tomorrow's date
        getDateStringEpoch(date, time(NULL) + 84600, false);
        puzz_epoch = time(NULL) + 84600;

        sprintf(url, "https://www.nytimes.com/svc/crosswords/v6/puzzle/mini/%s.json",
                date);
        Serial.println("Trying again with tomorrow's date");
        Serial.println(url);
        client.clear();
        http.begin(client, url);
        delay(500);
        httpResponseCode = http.GET();
    }

    if (httpResponseCode != 200) {
        char msg[64];
        sprintf(msg, "Failed to get NYT data. HTTP response: %d\n", httpResponseCode);
        return {};
    } else {
        s = http.getStreamPtr();
    }
    JsonDocument doc;
    char start_target[16] = "\"cells\":";
    char end_target[16] = "SVG";

    readStreamUntil(s, start_target, 8, NULL, 0, false);
    strcpy(scratch, "{\"cells\":");
    int buf_len = readStreamUntil(s, end_target, 3, scratch + 9, SCRATCH_SIZE, false);
    scratch[9 + buf_len - 5] = '}';
    scratch[9 + buf_len - 4] = 0;
    DeserializationError error = deserializeJson(doc, scratch);
    if (error) {
        Serial.println("JSON parsing failed: ");
        Serial.println(error.c_str());

        return {};
    }

    GridData data;
    data.height = doc["dimensions"]["height"].as<int>();
    data.width = doc["dimensions"]["width"].as<int>();
    data.puzz_epoch = puzz_epoch;
    data.square_data = {};
    data.authors = {};
    data.across_clues = {};
    data.down_clues = {};
    for (int j = 0; j < data.height; j++) {
        for (int i = 0; i < data.width; i++) {
            int idx = i + data.width * j;
            if (doc["cells"][idx] && doc["cells"][idx].as<JsonObject>().size() == 0) {
                SquareData d;
                d.row = j;
                d.col = i;
                d.data = 0;
                data.square_data.push_back(d);
            } else if (doc["cells"][idx]["label"]) {
                int label = atoi(doc["cells"][idx]["label"].as<String>().c_str());
                SquareData d;
                d.row = j;
                d.col = i;
                d.data = label;
                data.square_data.push_back(d);
            }
        }
    }
    for (auto clue : doc["clues"].as<JsonArray>()) {
        String direction = clue["direction"].as<String>();
        int label = atoi(clue["label"].as<String>().c_str());
        String text = clue["text"][0]["plain"].as<String>();
        ClueData c;
        c.num = label;
        c.data = text;
        if (strcmp(direction.c_str(), "Across") == 0) {
            data.across_clues.push_back(c);
        } else {
            data.down_clues.push_back(c);
        }
    }
    doc.clear();
    strcpy(start_target, "\"constructors\":");
    strcpy(end_target, "\"copyright\"");
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
        data.authors.push_back(a);
    }

    printGridDataSerial(data);
    // digitalWrite(ONBOARD_LED, HIGH);
    return data;
}

void writeScratchBit(int idx) {
    int b = idx / 8;
    int offset = 7 - (idx % 8);
    scratch[b] |= ((char)1) << offset;
}

// write to "logical" position in bitmap
// e.g. assuming an n x n bitmap is truly
// n x n, not worrying about row allignment
// or centering
void writeScratchBitLogical(int i) {
    // centering
    int idx = i + (MAX_BOARD - board_px) * (i / board_px); // byte align in bitmap
    writeScratchBit(idx);
}

void writeOutline(int square_dim, int width) {
    for (int x = 0; x < width; x++) {
        for (int i = 0; i < square_dim; i++) {
            // top
            writeScratchBitLogical(x * square_dim + i);
            writeScratchBitLogical(x * square_dim + board_px + i);
            // left
            writeScratchBitLogical(x * square_dim + (board_px * i));
            writeScratchBitLogical(x * square_dim + (board_px * i) + 1);
            // bottom
            writeScratchBitLogical(x * square_dim + (board_px * (square_dim - 2)) + i);
            writeScratchBitLogical(x * square_dim + (board_px * (square_dim - 1)) + i);
        }
    }
    // right
    for (int i = 0; i < square_dim; i++) {
        writeScratchBitLogical(i * board_px + (board_px - 1));
        writeScratchBitLogical(i * board_px + (board_px - 2));
    }
}

void writeSquare(int col, int square_dim, int data) {
    // fill in
    if (data == 0) {
        for (int i = 0; i < square_dim; i++) {
            for (int j = 0; j < square_dim; j++) {
                int idx = (col * square_dim) + i + (board_px * j);
                writeScratchBitLogical(idx);
            }
        }
        return;
    }
    int offset = 3;
    // one digit
    if (data <= 9) {
        for (int j = 0; j < 16; j++) {
            for (int i = 0; i < 16; i++) {
                if (!read_num_bit(data, i, j)) {
                    int idx =
                        col * square_dim + (i + offset) + ((j + offset) * board_px);
                    writeScratchBitLogical(idx);
                }
            }
        }
        return;
    }
    // two digit
    int d1 = data / 10;
    int d2 = data % 10;
    for (int j = 0; j < 16; j++) {
        for (int i = 0; i < 16; i++) {
            if (!read_num_bit(d1, i, j)) {
                int idx = col * square_dim + (i + offset) + ((j + offset) * board_px);
                writeScratchBitLogical(idx);
            }
            if (!read_num_bit(d2, i, j)) {
                int idx2 =
                    col * square_dim + (i + 8 + offset) + ((j + offset) * board_px);
                writeScratchBitLogical(idx2);
            }
        }
    }
}

void writeBoardRow(int row, GridData grid_data) {
    int square_dim = board_px / grid_data.width;
    memset(scratch, 0, SCRATCH_SIZE);
    writeOutline(square_dim, grid_data.width);
    for (auto p : grid_data.square_data) {
        if (p.row == row) {
            writeSquare(p.col, square_dim, p.data);
        }
    }
}

void printGrid(GridData grid_data, bool dump_bytes = false) {
    board_px = MAX_BOARD;
    while (board_px % grid_data.width != 0) {
        board_px -= 1;
    }
    if (dump_bytes) {
        Serial.printf("x dim: %d\n", board_px);
        Serial.printf("y dim: %d\n", (board_px / grid_data.width) * grid_data.height);
    }
    for (int i = 0; i < grid_data.height; i++) {
        writeBoardRow(i, grid_data);
        if (dump_bytes) {
            for (int j = 0; j < (board_px * board_px / grid_data.width / 8); j++) {
                Serial.printf("0x%02x ", scratch[j]);
            }
        }
        printer.printBitmap(board_px, board_px / grid_data.width,
                            (unsigned char *)scratch, true);
    }
}

void printHeader(GridData data) {
    printer.justify('C');
    printer.boldOn();
    printer.doubleHeightOn();
    printer.doubleWidthOn();
    printer.println(F("NYT MINI"));
    printer.boldOff();
    printer.doubleHeightOff();
    printer.doubleWidthOff();
    char date[32];
    getDateStringEpoch(date, data.puzz_epoch, true);
    printer.println(date);
    if (data.authors.size() == 1) {
        printer.print("Author: ");
    } else {
        printer.print("Authors: ");
    }
    for (size_t i = 0; i < data.authors.size(); i++) {
        printer.print(data.authors[i].c_str());
        if (i < data.authors.size() - 1)
            Serial.print(", ");
    }
}

void printClues(GridData data) {
    printer.justify('L');
    printer.boldOn();
    printer.doubleHeightOn();
    printer.doubleWidthOn();
    printer.println("Across:");
    printer.boldOff();
    printer.doubleHeightOff();
    printer.doubleWidthOff();
    for (auto clue : data.across_clues) {
        printer.printf("%d) %s\n", clue.num, clue.data.c_str());
    }
    printer.println();
    printer.boldOn();
    printer.doubleHeightOn();
    printer.doubleWidthOn();
    printer.println("Down:");
    printer.boldOff();
    printer.doubleHeightOff();
    printer.doubleWidthOff();
    for (auto clue : data.down_clues) {
        printer.printf("%d) %s\n", clue.num, clue.data.c_str());
    }
}
void printCrossword(GridData data) {
    printHeader(data);
    printer.println();
    printGrid(data);
    printClues(data);
    printer.println();
    printer.println();
    printer.println();
}

void getAndPrintCrossword() {
    std::optional<GridData> data_opt = std::nullopt;
    for (int i = 0; i < 3; i++) {
        data_opt = getGridData();
        if (data_opt)
            break;
    }

    if (!data_opt) {
        char msg[] = "Failed to get crossword after 3 attempts sorry :(";
        printDebug(msg);
    } else {
        GridData data = data_opt.value();
        printCrossword(data);
    }
}
