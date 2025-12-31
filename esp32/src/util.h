#include <WiFiClient.h>
#include <time.h>
bool getDateString(char *buff, bool pp);
void getDateStringEpoch(char *buff, time_t epoch, bool pp);
int readStreamUntil(WiFiClient *stream, const char *match, int match_len, char *buffer,
                    int buffer_len, bool dump_chars = false);
int readStringUntil(String s, const char *match, int match_len, char *buffer,
                    int buffer_len, bool dump_chars = false);
void printDebug(char *debug_msg);
void printAlign(char *msg, int indent = 0);
void threadBlink(void *count);
bool ensureInternet(unsigned long timeout_ms = 15000);