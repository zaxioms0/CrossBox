#include <Arduino.h>
#include <Adafruit_Thermal.h>
#include <globals.h>

const int MAX_BOARD = 384;
const int SCRATCH_SIZE = 8192; // 8kb
int board_px = MAX_BOARD;
bool print_today = false;
char scratch[SCRATCH_SIZE];
unsigned long press_time = 0;
unsigned int ctr = 0;
bool pressed = false;
int print_hr = -1;

Adafruit_Thermal printer(&Serial1);

WiFiManager wm;
WiFiManagerParameter
    print_time_param("mynum",
               "What time would you like to print automatically? Please enter a "
               "number from 0 - 23 for the hour in EST. Or leave as -1 for no "
               "automatic printing.",
               "-1", 10);
               
Preferences prefs;