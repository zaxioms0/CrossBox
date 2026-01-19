#include "board.h"
#include "globals.h"
#include "util.h"
#include <Adafruit_Thermal.h>
#include <Arduino.h>
#include <optional>
#include <time.h>
#include <vector>

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

bool readNumBit(int num, int i, int j) {
    int idx = 16 * j + i;
    int b = idx / 8;
    char offset = 1 << (7 - (idx % 8));
    return (nums[num][b] & offset) ? 1 : 0;
}

bool validateAscii(String &s) {
    for (unsigned char c : s) {
        if (c > 127)
            return false;
    }
    return true;
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

void writeOutline(int square_dim, int width, bool close) {
    for (int x = 0; x < width; x++) {
        for (int i = 0; i < square_dim; i++) {
            // top
            writeScratchBitLogical(x * square_dim + i);
            // writeScratchBitLogical(x * square_dim + board_px + i);
            //  left
            writeScratchBitLogical(x * square_dim + (board_px * i));
            // writeScratchBitLogical(x * square_dim + (board_px * i) + 1);
            //  bottom
            if (close)
                writeScratchBitLogical(x * square_dim + (board_px * (square_dim - 1)) +
                                       i);
            // writeScratchBitLogical(x * square_dim + (board_px * (square_dim - 2)) +
            // i);
        }
    }
    // right
    for (int i = 0; i < square_dim; i++) {
        writeScratchBitLogical(i * board_px + (board_px - 1));
        // writeScratchBitLogical(i * board_px + (board_px - 2));
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
                if (!readNumBit(data, i, j)) {
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
            if (!readNumBit(d1, i, j)) {
                int idx = col * square_dim + (i + offset) + ((j + offset) * board_px);
                writeScratchBitLogical(idx);
            }
            if (!readNumBit(d2, i, j)) {
                int idx2 =
                    col * square_dim + (i + 8 + offset) + ((j + offset) * board_px);
                writeScratchBitLogical(idx2);
            }
        }
    }
}

void writeBoardRow(int row, Grid grid_data, bool bottom) {
    int square_dim = board_px / grid_data.width;
    memset(scratch, 0, SCRATCH_SIZE);
    writeOutline(square_dim, grid_data.width, bottom);
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
        writeBoardRow(i, grid_data, i == grid_data.height - 1);
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
    printer.println(data.header);

    printer.doubleHeightOff();
    printer.doubleWidthOff();
    if (data.title)
        printer.println(data.title.value());
    printer.boldOff();
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
            printer.print(", ");
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
    unsigned int max_across = 0;
    for (auto clue : data.across_clues) {
        max_across = max(max_across, clue.num);
    }
    for (auto clue : data.across_clues) {
        char msg[512];
        if (max_across >= 10)
            sprintf(msg, "%2d) %s", clue.num, clue.data.c_str());
        else
            sprintf(msg, "%d) %s", clue.num, clue.data.c_str());
        printAlign(msg, 3 + (int)(max_across >= 10));
    }
    printer.println();
    printer.boldOn();
    printer.doubleHeightOn();
    printer.doubleWidthOn();
    printer.println("Down:");
    printer.boldOff();
    printer.doubleHeightOff();
    printer.doubleWidthOff();
    unsigned int max_down = 0;
    for (auto clue : data.down_clues) {
        max_down = max(max_down, clue.num);
    }
    for (auto clue : data.down_clues) {
        char msg[512];
        if (max_down >= 10)
            sprintf(msg, "%2d) %s", clue.num, clue.data.c_str());
        else
            sprintf(msg, "%d) %s", clue.num, clue.data.c_str());
        printAlign(msg, 3 + (int)(max_down >= 10));
    }
}

void printCrossword(Grid data) {
    printer.wake();
    printer.reset();
    if (ALI_EXPRESS)
        printer.setHeatConfig(11, 200, 60);
    printHeader(data);
    printer.println();
    printGrid(data);
    printClues(data);
    printer.println();
    printer.println();
    printer.println();
    printer.reset();
}
