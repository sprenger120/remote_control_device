#pragma once

/* Interfaces canopen MSG... calls with terminalIO logging, see canfestival/applicfg.h */

void canopen_log(char *format, ...);
void canopen_logWarn(char *format, ...);
void canopen_logErr(char *format, ...);