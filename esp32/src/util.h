#include <WiFiClient.h>
#include <time.h>
void getDateString(char *buff, bool pp);
void getDateStringEpoch(char *buff, time_t epoch, bool pp);
int readStreamUntil(WiFiClient *stream, const char *match, int match_len, char *buffer,
                    int buffer_len, bool dump_chars = false);
void printDebug(char *debug_msg);
void threadBlink(void *count);