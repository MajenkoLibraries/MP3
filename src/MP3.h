#ifndef _MP3_H
#define _MP3_H

#include <Arduino.h>
#include <I2S.h>
#include <DFATFS.h>

#include "utility/mp3dec.h"
#include "utility/coder.h"

#define READBUF_SIZE 1940

class MP3 : public AudioSource {
    private:

        DFILE *_file;

        FrameHeader _frameHeader;
        SideInfo _sideInfo;
        ScaleFactorInfo _scaleFactorInfo;
        HuffmanInfo _huffmanInfo;
        DequantInfo _dequantInfo;
        IMDCTInfo _IMDCTInfo;
        SubbandInfo _subbandInfo;

        MP3DecInfo _mp3DecInfo;

        int read();

        int16_t _outBuf[2 * 1152];
        uint32_t _outPos;
        uint32_t _numSamples;
        uint8_t _readBuf[READBUF_SIZE];
        uint8_t *_readPtr;
        int _bytesLeft;

        int readBytesFromStream();
        int decodeFrameInfo(uint8_t *ptr);
        int decodeStream(uint8_t **ptr, int *len, int16_t *outBuf);
        int decodeNewChunk();

    public:
        MP3FrameInfo _frameInfo;

        MP3(DFILE *file);
        MP3(DFILE &file) : MP3(&file) {}

        void initStream();

        uint32_t getNextSampleBlock(int16_t *samples, uint32_t len);
        uint32_t getFrameLength();
        uint32_t getChannels();
        uint32_t seekToFrame(uint32_t f) { return 0; }
};


#endif
