//
// CircularBuffer: Circular buffer for real time data buffering
//
// Author: A.Navatta

#ifndef CIRCULAR_BUFFER_H

#define CIRCULAR_BUFFER_H

#include <freertos/stream_buffer.h>

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <BufferProducer.h>

class CircularBuffer : public BufferProducer {
public:
    CircularBuffer();
    
//    void produce(uint8_t value);
//    uint8_t consume();
    void produce(uint8_t *buffer, int size);
    virtual int consume(uint8_t *buffer, int size, int timeout = 0);
        
    void dump();

    virtual int avail(); // { return ((m_head == m_tail) && !m_empty) ? m_size : (m_head - m_tail + m_size) % m_size; }
    void size(int s,int datalen, int frame);
    inline int size() { return m_size; };
    inline int data_size() { return m_datalen; };

private:
	int		m_size;		// Buffer size
    int     m_datain;
    int     m_dataout;
    int     m_datalen;
    uint8_t *m_buffer;
    StreamBufferHandle_t m_streambuffer;
    StaticStreamBuffer_t m_bufferdata;
//	int		m_tail;		// Tail pointer
//	int		m_head;		// Head pointer

//	bool	m_empty;
//    portMUX_TYPE m_mux;
};

#endif