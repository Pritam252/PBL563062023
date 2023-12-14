#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#define SetMemory(destination,data) memset((&destination), data, sizeof(destination))
#define ZeroMemory(destination) memset((&destination), 0, sizeof(destination))

#include "Arduino.h"

typedef uint8_t BOOL;

typedef uint8_t byte_t;
typedef int16_t s16;
typedef int32_t s32;

typedef float f32;
typedef double f64;

typedef uint8_t result_flag_t;

typedef struct {
  f32 dust;
  f32 temp;
  f32 moist;
  s32 motion; //s32 is used to avoid byte padding problems.
} MCUPacket_t;

typedef BOOL MotorState;

#endif