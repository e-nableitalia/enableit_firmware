//
// CircularBuffer: Circular buffer for real time data buffering
//
// Author: A.Navatta

#ifndef CIRCULAR_BUFFER_H

#define CIRCULAR_BUFFER_H

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <BufferProducer.h>

class CircularBuffer : public BufferProducer {
public:
    CircularBuffer();
    
    void produce(uint8_t value);
    uint8_t consume();
    void produce(uint8_t *buffer, int size);
    virtual int consume(uint8_t *buffer, int size, bool fill = true);
        
    void dump();

    int avail() { return _avail; }
    void size(int s,int datalen);
    inline int size() { return sz; };
    inline int data_size() { return datalen; };

private:

    int head;
    int tail;
    bool overflow;
    int sz;
    int _avail;
    int datalen;
    uint8_t *buffer;
};

#endif