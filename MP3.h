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

        int readBytesFromStream() {
            memmove(_readBuf, _readPtr, _bytesLeft);
            uint32_t nRead = _source->readBytes(_readBuf + _bytesLeft, READBUF_SIZE - _bytesLeft);
            memset(_readBuf + _bytesLeft + nRead, 0, READBUF_SIZE - _bytesLeft - nRead);
            _bytesLeft += nRead;
            _readPtr = _readBuf;
            return nRead;
        }


    public:
        MP3FrameInfo _frameInfo;

        MP3(Stream &str) {
            _source = &str;
            _mp3DecInfo.FrameHeaderPS =     (void *)&_frameHeader;
            _mp3DecInfo.SideInfoPS =        (void *)&_sideInfo;
            _mp3DecInfo.ScaleFactorInfoPS = (void *)&_scaleFactorInfo;
            _mp3DecInfo.HuffmanInfoPS =     (void *)&_huffmanInfo;
            _mp3DecInfo.DequantInfoPS =     (void *)&_dequantInfo;
            _mp3DecInfo.IMDCTInfoPS =       (void *)&_IMDCTInfo;
            _mp3DecInfo.SubbandInfoPS =     (void *)&_subbandInfo;

            memset(&_frameHeader, 0, sizeof(FrameHeader));
            memset(&_sideInfo, 0, sizeof(SideInfo));
            memset(&_scaleFactorInfo, 0, sizeof(ScaleFactorInfo));
            memset(&_huffmanInfo, 0, sizeof(HuffmanInfo));
            memset(&_dequantInfo, 0, sizeof(DequantInfo));
            memset(&_IMDCTInfo, 0, sizeof(IMDCTInfo));
            memset(&_subbandInfo, 0, sizeof(SubbandInfo));
            _outPos = 0;
            _numSamples = 0;
            _bytesLeft = 0;
            _readPtr = _readBuf;
        }

        int decodeFrameInfo(uint8_t *ptr) {
            return MP3GetNextFrameInfo(&_mp3DecInfo, &_frameInfo, ptr);
        }

        int decodeStream(uint8_t **ptr, int *len, int16_t *outBuf) {
            return MP3Decode(&_mp3DecInfo, ptr, len, outBuf, 0);

        }

        int begin() {
            int offset = -1;
            while (offset < 0) {
                _bytesLeft = _source->readBytes(_readBuf, READBUF_SIZE);
                if (_bytesLeft == 0) {
                    return -1;
                }
                _readPtr = _readBuf;
                offset = MP3FindSyncWord(_readPtr, _bytesLeft); 
            }
            _readPtr += offset;
            _bytesLeft -= offset;
            int err = decodeFrameInfo(_readPtr);
            Serial.printf("Channels: %d\r\n", _frameInfo.nChans);
            Serial.printf("Sample Rate: %d\r\n", _frameInfo.samprate);

            readBytesFromStream();
        }

        int decodeNewChunk() {
            int err = -1;
            do {
                err = decodeStream(&_readPtr, &_bytesLeft, _outBuf);
                switch (err) {
                    case 0:
                    case -2: // Read more.
                        if (readBytesFromStream() <= 0) {
Serial.println("EOF");
                            return -2;
                        }
                        break;
                    default:
Serial.print("Error: ");
Serial.println(err);
                        return err;
                }
            } while (err != 0);
            _numSamples = 1152;
            _outPos = 0;
            return err;
        }

        // Stream et al
        
        size_t write(uint8_t) { return 0; }
        int available() { return _numSamples - _outPos; }
        int read() {
            if ((_numSamples == 0) || (_outPos == _numSamples)) {
                if (decodeNewChunk() < 0) {
                    return -70000;
                }
            }
            return _outBuf[_outPos++];
        }
        int peek() {
            return 0; // Later...!
        }
        void flush() { };

        

};


#endif
