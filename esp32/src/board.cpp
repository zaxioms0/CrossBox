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

struct Square {
    unsigned int row;
    unsigned int col;
    unsigned int data; // 0 means filled in
};

struct Clue {
    unsigned int num;
    String data;
};

struct Grid {
    unsigned int height;
    unsigned int width;
    time_t puzz_epoch;
    // not "constructors" because that means something else
    std::vector<String> authors;
    std::vector<Square> square_data;
    std::vector<Clue> across_clues;
    std::vector<Clue> down_clues;
};

const unsigned char nums[10][32] = {
    {0xff, 0xff, 0xfc, 0x7f, 0xf9, 0x3f, 0xf3, 0x9f, 0xf7, 0x9f, 0xf7,
     0xdf, 0xf7, 0xdf, 0xf7, 0xdf, 0xf7, 0xdf, 0xf7, 0x9f, 0xf3, 0x9f,
     0xf9, 0x3f, 0xfc, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xff, 0xff, 0xff, 0x7f, 0xfe, 0x7f, 0xfc, 0x7f, 0xfb, 0x7f, 0xff,
     0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f,
     0xff, 0x7f, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xff, 0xff, 0xfc, 0x7f, 0xf3, 0x3f, 0xf7, 0x9f, 0xf7, 0xdf, 0xff,
     0x9f, 0xff, 0xbf, 0xff, 0x3f, 0xfe, 0x7f, 0xf9, 0xff, 0xfb, 0xff,
     0xf3, 0xff, 0xf0, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xff, 0xff, 0xfc, 0x7f, 0xf3, 0x3f, 0xf7, 0x9f, 0xff, 0x9f, 0xff,
     0x3f, 0xfe, 0x3f, 0xff, 0x9f, 0xff, 0x9f, 0xff, 0xdf, 0xf7, 0x9f,
     0xf3, 0x3f, 0xf8, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xff, 0xff, 0xff, 0x3f, 0xfe, 0x3f, 0xfe, 0x3f, 0xfd, 0x3f, 0xf9,
     0x3f, 0xfb, 0x3f, 0xf3, 0x3f, 0xe7, 0x3f, 0xe0, 0x1f, 0xff, 0x3f,
     0xff, 0x3f, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xff, 0xff, 0xf8, 0x1f, 0xfb, 0xff, 0xfb, 0xff, 0xf3, 0xff, 0xf0,
     0x3f, 0xf3, 0x1f, 0xff, 0x9f, 0xff, 0xdf, 0xff, 0xdf, 0xf7, 0x9f,
     0xf3, 0x3f, 0xf8, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xff, 0xff, 0xfc, 0x7f, 0xf9, 0xbf, 0xf3, 0x9f, 0xf7, 0xff, 0xf4,
     0x3f, 0xf1, 0x3f, 0xf3, 0x9f, 0xf7, 0xdf, 0xf7, 0xdf, 0xf3, 0x9f,
     0xf9, 0x3f, 0xfc, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xff, 0xff, 0xf0, 0x1f, 0xff, 0x9f, 0xff, 0xbf, 0xff, 0x3f, 0xfe,
     0x7f, 0xfe, 0x7f, 0xfe, 0xff, 0xfc, 0xff, 0xfd, 0xff, 0xfd, 0xff,
     0xfd, 0xff, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xff, 0xff, 0xfc, 0x7f, 0xf9, 0x3f, 0xf3, 0x9f, 0xf3, 0x9f, 0xfb,
     0xbf, 0xf8, 0x3f, 0xf3, 0x3f, 0xf7, 0x9f, 0xf7, 0xdf, 0xf7, 0x9f,
     0xf3, 0x1f, 0xf8, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xff, 0xff, 0xf8, 0x7f, 0xf3, 0x3f, 0xf7, 0x9f, 0xf7, 0xdf, 0xf7,
     0x9f, 0xf3, 0x9f, 0xf8, 0x5f, 0xff, 0xdf, 0xff, 0x9f, 0xf7, 0x9f,
     0xf3, 0x3f, 0xf8, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
};

bool read_num_bit(int num, int i, int j) {
    int idx = 16 * j + i;
    int b = idx / 8;
    char offset = 1 << (7 - (idx % 8));
    return (nums[num][b] & offset) ? 1 : 0;
}

void printGridDataSerial(Grid d) {
    Serial.printf("Height: %d\n", d.height);
    Serial.printf("Width: %d\n", d.width);
    Serial.print("Constructors(s): ");
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

std::optional<Grid> getGridData() {
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
    NetworkClient *s;

    if (httpResponseCode != 200) {
        char msg[64];
        sprintf(msg, "Failed to get NYT data. HTTP response: %d\n", httpResponseCode);
        Serial.println(msg);
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
        Serial.println("JSON parsing clues failed: ");
        Serial.println(error.c_str());
        return {};
    }

    Grid data;
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
                Square d;
                d.row = j;
                d.col = i;
                d.data = 0;
                data.square_data.push_back(d);
            } else if (doc["cells"][idx]["label"]) {
                int label = atoi(doc["cells"][idx]["label"].as<String>().c_str());
                Square d;
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
        Clue c;
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
            writeScratchBitLogical(x * square_dim + (board_px * (square_dim - 1)) + i);
            writeScratchBitLogical(x * square_dim + (board_px * (square_dim - 2)) + i);
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

void writeBoardRow(int row, Grid grid_data) {
    int square_dim = board_px / grid_data.width;
    memset(scratch, 0, SCRATCH_SIZE);
    writeOutline(square_dim, grid_data.width);
    for (auto p : grid_data.square_data) {
        if (p.row == row) {
            writeSquare(p.col, square_dim, p.data);
        }
    }
}

void printGrid(Grid grid_data, bool dump_bytes = false) {
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

void printHeader(Grid data) {
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
        printer.print("Constructor: ");
    } else {
        printer.print("Constructors: ");
    }
    for (size_t i = 0; i < data.authors.size(); i++) {
        printer.print(data.authors[i].c_str());
        if (i < data.authors.size() - 1)
            Serial.print(", ");
    }
}

void printClues(Grid data) {
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
void printCrossword(Grid data) {
    printer.wake();
    printer.reset();
    printHeader(data);
    printer.println();
    printGrid(data);
    printClues(data);
    printer.println();
    printer.println();
    printer.println();
    printer.reset();
}

void getAndPrintCrossword() {
    std::optional<Grid> data_opt = std::nullopt;
    for (int i = 0; i < 3; i++) {
        data_opt = getGridData();
        if (data_opt)
            break;
        else
            Serial.printf("Print failed: %i\n");
    }

    if (!data_opt) {
        char msg[] = "Failed to get crossword after 3 attempts sorry :( You should "
                     "take a look at Serial for debugging info";
        printDebug(msg);
    } else {
        Grid data = data_opt.value();
        printCrossword(data);
    }
}
