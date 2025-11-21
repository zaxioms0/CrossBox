#include <Adafruit_Thermal.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <creds.h>
#include <nums.h>
#include <optional>
#include <time.h>
#include <vector>

// RX, TX
// SoftwareSerial printer_serial(14, 16);
#define ONBOARD_LED 2

Adafruit_Thermal printer(&Serial1);
int board_px = 384;
const int scratch_size = 8192; // 8kb
bool print_today = false;

char scratch[scratch_size];

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
        sprintf(buff, "%d-%d-%d", 1900 + timeinfo.tm_year, 1 + timeinfo.tm_mon,
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

std::optional<GridData> getGridData() {
    char url[128];
    char date[64];
    board_px = 384;
    WiFiClientSecure client;
    HTTPClient http;
    getDateString(date, false);
    sprintf(url, "https://www.nytimes.com/svc/crosswords/v6/puzzle/mini/%s.json", date);
    Serial.println(url);

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    // http.addHeader("User-Agent", "Mozilla/5.0");
    //  client.setCACert(root_ca);
    client.setInsecure();
    http.begin(client, url);
    delay(500);
    int httpResponseCode = http.GET();
    NetworkClient *s;
    if (httpResponseCode == 200) {
        s = http.getStreamPtr();
    }
    if (httpResponseCode == 403) {
        WiFiClientSecure client;
        HTTPClient http;
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        client.setCACert(root_ca);
        http.addHeader("User-Agent", "Mozilla/5.0");
        //  try again with tomorrow's date
        configTime((24 - 4) * 60 * 60, 0, "time.google.com");
        getDateString(date, false);

        sprintf(url,
                "https://www.nytimes.com/svc/crosswords/v6/puzzle/mini/2025-11-%s.json",
                date);
        Serial.println("Trying again with tomorrow's date");
        Serial.println(url);
        client.clear();
        http.begin(client, url);
        delay(500);
        httpResponseCode = http.GET();
        s = http.getStreamPtr();
    }

    if (httpResponseCode != 200) {
        Serial.printf("Failed to get NYT data. HTTP response: %d\n", httpResponseCode);
        return {};
    }

    JsonDocument doc;
    char start_target[16] = "\"cells\":";
    char end_target[16] = "SVG";

    readStreamUntil(s, start_target, 8, NULL, 0, false);
    strcpy(scratch, "{\"cells\":");
    int buf_len = readStreamUntil(s, end_target, 3, scratch + 9, scratch_size, false);
    scratch[9 + buf_len - 5] = '}';
    scratch[9 + buf_len - 4] = 0;
    DeserializationError error = deserializeJson(doc, scratch);
    if (error) {
        Serial.println("JSON parsing failed: ");
        Serial.println(error.c_str());
        return {};
    }

    // serializeJsonPretty(doc, Serial);
    GridData data;
    data.height = doc["dimensions"]["height"].as<int>();
    data.width = doc["dimensions"]["width"].as<int>();
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
    int buflen = readStreamUntil(s, end_target, 11, scratch + 16, scratch_size);
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
    digitalWrite(ONBOARD_LED, HIGH);

    return data;
}

void writeScratchBit(int i) {
    int b = i / 8;
    int offset = 7 - (i % 8);
    scratch[b] |= ((char)1) << offset;
}

void writeOutline(int square_dim, int width) {
    for (int x = 0; x < width; x++) {
        for (int i = 0; i < square_dim; i++) {
            // top
            writeScratchBit(x * square_dim + i);
            writeScratchBit(x * square_dim + board_px + i);
            // left
            writeScratchBit(x * square_dim + (board_px * i));
            writeScratchBit(x * (square_dim) + (board_px * i) + 1);
            // bottom
            writeScratchBit(x * square_dim + (board_px * (square_dim - 2)) + i);
            writeScratchBit(x * square_dim + (board_px * (square_dim - 1)) + i);
        }
    }
    // right
    for (int i = 0; i < square_dim; i++) {
        writeScratchBit(i * board_px + (board_px - 1));
        writeScratchBit(i * board_px + (board_px - 2));
    }
}

void writeSquare(int col, int square_dim, int data) {
    // fill in
    if (data == 0) {
        for (int i = 0; i < square_dim; i++) {
            for (int j = 0; j < square_dim; j++) {
                int idx = (col * square_dim) + i + (board_px * j);
                writeScratchBit(idx);
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
                    writeScratchBit(idx);
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
                writeScratchBit(idx);
            }
            if (!read_num_bit(d2, i, j)) {
                int idx2 =
                    col * square_dim + (i + 8 + offset) + ((j + offset) * board_px);
                writeScratchBit(idx2);
            }
        }
    }
}

void writeBoardRow(int row, GridData grid_data) {
    int square_dim = board_px / grid_data.width;
    memset(scratch, 0, scratch_size);
    writeOutline(square_dim, grid_data.width);
    for (auto p : grid_data.square_data) {
        if (p.row == row) {
            writeSquare(p.col, square_dim, p.data);
        }
    }
}

void printGrid(GridData grid_data, bool dump_bytes = false) {
    int tmp = board_px;
    for (int i = 0; i < tmp / 8; i++) {
        if (board_px % grid_data.width == 0) {
            break;
        }
        board_px -= 8;
    }
    if (dump_bytes) {
        Serial.printf("x dim: %d\n", board_px);
        Serial.printf("y dim: %d\n", board_px / grid_data.width);
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
    char date[64];
    getDateString(date, true);
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
    printer.println();
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
    printer.doubleHeightOn();
    printer.doubleWidthOn();
    printer.println("Down:");
    printer.boldOff();
    printer.doubleHeightOff();
    printer.doubleWidthOff();
    for (auto clue : data.down_clues) {
        printer.printf("%d) %s\n", clue.num, clue.data.c_str());
    }
    printer.println();
    printer.println();
    printer.println();
}

void setup() {
    pinMode(ONBOARD_LED, OUTPUT);
    digitalWrite(ONBOARD_LED, LOW);
    Serial.begin(115200);
    Serial1.begin(9600);

    delay(10);
    Serial.println('\n');
    WiFiManager wm;

    WiFiManagerParameter print_time(
        "mynum",
        "What time would you like to print automatically? Please enter a "
        "number from 0 - 23 for the hour in EST. Or leave as -1 for no "
        "automatic printing.",
        "-1", 10);
    wm.addParameter(&print_time);

    if (!wm.autoConnect("Crossbox Setup")) {
        Serial.println("Failed to connect via WiFiManager");
        ESP.restart();
    }
    // WiFi.begin(ssid, password);
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println(" ...");

    int i = 0;
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        delay(1000);
        Serial.print(++i);
        Serial.print(' ');
    }

    Serial.println('\n');
    Serial.println("Connection established!");
    configTime(-4 * 60 * 60, 0, "time.google.com", "pool.ntp.org");

    // printer.begin();
    // printHeader(data);
    // printGrid(data);
    // printClues(data);
}

void loop() {
    // struct tm timeinfo;
    // if (!getLocalTime(&timeinfo))
    //     return;
    // int hour = timeinfo.tm_hour;
    // int minute = timeinfo.tm_min;

    // if (hour == 6 && !print_today) {
    //     print_today = true;
    // } else if (hour == 0 && minute == 1) {
    //     print_today = false;
    // }

    // delay(50);
    std::optional<GridData> data_opt = getGridData();
    while (!data_opt) {
        data_opt = getGridData();
    }
    GridData data = data_opt.value();

    Serial.println(esp_get_free_heap_size());
}