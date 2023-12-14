#ifndef PACKET_PROC_H
#define PACKET_PROC_H

#include <Wire.h>
#include "typedefs.h"

namespace PacketProc
{
    template <typename T> int I2C_writeAnything (const T& value)
      {
        const byte * p = (const byte*) &value;
        unsigned int i;
        for (i = 0; i < sizeof value; i++)
              Wire.write(*p++);
        return i;
      }  // end of I2C_writeAnything

    template <typename T> int I2C_readAnything(T& value)
      {
        byte * p = (byte*) &value;
        unsigned int i;
        for (i = 0; i < sizeof value; i++)
              *p++ = Wire.read();
        return i;
      }  // end of I2C_readAnything
}

#endif