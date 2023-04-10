//
// BufferProducer: BufferProducer interface
//
// Author: A.Navatta

#ifndef BUFFER_PRODUCER_H

#define BUFFER_PRODUCER_H

#include <Arduino.h>

class BufferProducer {
    public:
        BufferProducer() { };
        virtual int consume(uint8_t *buffer, int size, bool fill = true) = 0;
};

#endif