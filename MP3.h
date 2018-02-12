#ifndef _MP3_H
#define _MP3_H

#include <Arduino.h>

#include "utility/mp3dec.h"
#include "utility/coder.h"

#define READBUF_SIZE 1940

class MP3 : public Stream {
    private:
        FrameHeader _frameHeader;
        SideInfo _sideInfo;
        ScaleFactorInfo _scaleFactorInfo;
        HuffmanInfo _huffmanInfo;
        DequantInfo _dequantInfo;
        IMDCTInfo _IMDCTInfo;
        SubbandInfo _subbandInfo;

        MP3DecInfo _mp3DecInfo;

        int16_t _outBuf[2 * 1152];
        uint32_t _outPos;
        uint32_t _numSamples;
        uint8_t _readBuf[READBUF_SIZE];
        uint8_t *_readPtr;
        int _bytesLeft;

        Stream *_source;

        int readBytesFromStream();

    public:
        MP3FrameInfo _frameInfo;

        MP3(Stream &str);

        int decodeFrameInfo(uint8_t *ptr);
        int decodeStream(uint8_t **ptr, int *len, int16_t *outBuf);
        int begin();
        int decodeNewChunk();
        size_t write(uint8_t) { return 0; }
        int available() { return _numSamples - _outPos; }
        int read();
        int peek() {
            return 0; // Later...!
        }
        void flush() { };
};


#endif
