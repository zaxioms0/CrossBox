#include <Arduino.h>
#include <optional>
#include <time.h>
#include <vector>

#pragma once
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
    String header;
    std::optional<String> title;
    // not "constructors" because that means something else
    std::vector<String> authors;
    std::vector<Square> square_data;
    std::vector<Clue> across_clues;
    std::vector<Clue> down_clues;
};

void printCrossword(Grid data);
void printGridDataSerial(Grid d);