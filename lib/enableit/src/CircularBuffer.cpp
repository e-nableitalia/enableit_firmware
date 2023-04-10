#include <Arduino.h>
#include <mutex>
#include <debug.h>
#include <CircularBuffer.h>

std::mutex _mutex;

CircularBuffer::CircularBuffer() {
    sz = 0;
    datalen = 0;
}

void CircularBuffer::size(int s, int len) {
    datalen = len;
    sz = s * datalen;
    head = tail = _avail = 0;
    overflow = false;
    
    if (buffer)
        free(buffer);
    if (sz > 0) {
        DBG("Allocated buffer, size[%d]", sz);
        buffer = (uint8_t *) malloc(sz);
        for (int i = 0; i < sz; i++)
            buffer[i] = 0;
    } else buffer = nullptr;
}

void CircularBuffer::produce(uint8_t value) {
    
    if (!buffer) return;

    { // sync
        std::lock_guard<std::mutex> lck(_mutex);

        buffer[head] = value;

        head++;
        head = head % sz;
        
        _avail++;

        if (_avail > sz) {

            // telemetry.debug("overflow");

            overflow = true;
            // advance tail
            tail++;
            tail = tail % sz;
            _avail = sz;
        }
    }
}

void CircularBuffer::produce(uint8_t *b, int size) {
    
    if (!buffer) return;

    { // sync
        std::lock_guard<std::mutex> lck(_mutex);

        for (int i = 0; i < size; i++) {
            buffer[head++] = *b++;

            head = head % sz;
        
            _avail++;

            if (_avail > sz) {
                overflow = true;
                // advance tail
                tail++;
                tail = tail % sz;
                _avail = sz;
            }
        }
    }
}

uint8_t CircularBuffer::consume() {
    
    if (!_avail) 
        return -1;
        
    uint8_t value;

    { // sync
        std::lock_guard<std::mutex> lck(_mutex);
        // String _dump = String::format("Buffer::consume value(%d), tail(%d)",value,tail);
        // telemetry.debug(_dump);
        value = buffer[tail];
        
        tail++;

        _avail--;
        
        tail = tail % sz;
    }

    return value;
}

int CircularBuffer::consume(uint8_t *b, int s, bool fill) {
    DBG("Avail(%d)",_avail);

    if (_avail < s)
        s = _avail;
    
    int pos = 0;
    
    { // sync
        std::lock_guard<std::mutex> lck(_mutex);
        for (int i=0; i < s; i++) {
            // unsigned short value = buffer[tail];
            // String _dump = String::format("Buffer::consume value(%d), tail(%d), index(%d)",value,tail,i);
            // telemetry.debug(_dump);
            b[pos++] = buffer[tail++];

            tail = tail % sz;
        }
        _avail -= s;
    }

    return s;
}

void CircularBuffer::dump() {
    // String _dump = String::format("Buffer: head(%d), tail(%d), avail(%d), overflow(%s)",
    //     head,
    //     tail,
    //     _avail,
    //     (overflow ? "true" : "false"));

    // telemetry.debug(_dump);
}