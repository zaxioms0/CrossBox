#include "WiFiManager.h"
#include <WiFi.h>
#include <Arduino.h>
#include <Adafruit_Thermal.h>
#include <Preferences.h>

#pragma once
#define RX 25          // YELLOW
#define TX 26          // WHITE
#define BUTT 19        // FRONT WIRE
#define BUTT_LED_OUT 5 // RIGHT WIRE
#define BUTT_LED 18    // LEFT WIRE
#define ONBOARD_LED 2


extern WiFiManager wm;
extern WiFiManagerParameter print_time_param;
extern Preferences prefs;
extern Adafruit_Thermal printer;

extern const int MAX_BOARD;
extern const int SCRATCH_SIZE;
extern int board_px;
extern bool print_today;
extern int print_hr;
extern char scratch[];
extern unsigned long press_time;
extern unsigned int ctr;
extern bool pressed;
