#ifndef IR_LIB_H
#define IR_LIB_H

#include <Arduino.h>

extern volatile unsigned long bitDecoding;
extern volatile unsigned long previousISRTime;

extern volatile byte bitTimeCount;
extern int intalizeIR(byte pin);

extern bool decoded;
extern volatile bool decodeReady;

extern void Decode();

extern byte command;
extern byte address;

#endif
