#include "globals.h"
#include <Adafruit_Thermal.h>
#include <Arduino.h>
#include <Preferences.h>
#include <WiFiManager.h>

const int MAX_BOARD = 384;
const int SCRATCH_SIZE = 8192; // 8kb
const int TEXT_WIDTH = 32;
int board_px = MAX_BOARD;
bool print_today = false;
char scratch[SCRATCH_SIZE];
unsigned long press_time = 0;
unsigned int ctr = 0;
bool pressed = false;
int print_hr = -1;
char nyts[256] = "";
char ssid[64] = "";
char wifi_pass[64] = "";

Adafruit_Thermal printer(&Serial1);

WiFiManager wm;
WiFiManagerParameter
    print_time_param("printtime",
                     "What time would you like to print automatically? Please enter a "
                     "number from 0 - 23 for the hour in EST. Or leave blank for no "
                     "automatic printing.",
                     "", 10);
WiFiManagerParameter nyts_param("nyts", "Please Provide your NYT-S Cookie", "", 256);

Preferences prefs;