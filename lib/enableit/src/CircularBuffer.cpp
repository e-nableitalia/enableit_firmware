#include <Arduino.h>
#include <mutex>
#include <Console.h>
#include <CircularBuffer.h>

CircularBuffer::CircularBuffer() {
	 m_datalen = m_datain = m_dataout = 0;
     m_streambuffer = NULL;
     m_buffer = nullptr;
	//m_head = m_tail =
    //m_empty = true;
    //portMUX_INITIALIZE(&m_mux);
}

void CircularBuffer::size(int s, int len, int frame) {
    m_datalen = len;
    m_size = s * m_datalen;
    //m_head = 0;
	//m_tail = 0;
    
    if (m_buffer) {
        free(m_buffer);
    }
    if (m_streambuffer) {
        vStreamBufferDelete(m_streambuffer);
    }
    if (m_size > 0) {
        DBG("Allocated buffer, size[%d]", m_size);
        m_buffer = (uint8_t *) malloc(m_size + 1);
        for (int i = 0; i < m_size; i++)
            m_buffer[i] = 0;
        
        m_streambuffer = xStreamBufferCreateStatic(m_size, frame, m_buffer, &m_bufferdata);
    } else {
        m_buffer = nullptr;
        m_streambuffer = NULL;
    }
}

void CircularBuffer::produce(uint8_t *data, int size) {
    
    if (m_streambuffer) {
        size_t pushed_data = xStreamBufferSendFromISR(m_streambuffer, data, size, NULL);
        m_datain+=pushed_data;
    }
}

int CircularBuffer::consume(uint8_t *data, int size, int timeout) {

    if (m_streambuffer) {
        int read_data = xStreamBufferReceive(m_streambuffer, data, size, pdMS_TO_TICKS(timeout));
        m_dataout += read_data;
        return read_data;
    }

    return 0;
}

int CircularBuffer::avail() {
    return xStreamBufferBytesAvailable( m_streambuffer );
}

void CircularBuffer::dump() {
    DBG("Buffer: avail(%d), datain(%d), dataout(%d)",
         xStreamBufferBytesAvailable( m_streambuffer ),
         m_datain,
         m_dataout
         );

    // telemetry.debug(_dump);
}