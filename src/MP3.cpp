#include <MP3.h>

int MP3::readBytesFromStream() {
    // Step one - pull down the remaining part of the buffer to the start of memory
    if (_bytesLeft > 0) {
        memmove(_readBuf, _readPtr, _bytesLeft);
    }
    uint32_t nr;
    int rv;

    // Step two - fill the latter part of the buffer (now empty) with more data.
    if ((rv = _file->fsread(_readBuf + _bytesLeft, READBUF_SIZE - _bytesLeft, &nr, (READBUF_SIZE - _bytesLeft) / 512 + 1)) != FR_OK) {
        return -1;
    }
//    memset(_readBuf + _bytesLeft + nr, 0, READBUF_SIZE - _bytesLeft - nr);
    _bytesLeft += nr;
    _readPtr = _readBuf;
    return nr;
}

MP3::MP3(DFILE *file) {
    _file = file; 
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

int MP3::decodeFrameInfo(uint8_t *ptr) {
    return MP3GetNextFrameInfo(&_mp3DecInfo, &_frameInfo, ptr);
}

int MP3::decodeStream(uint8_t **ptr, int *len, int16_t *outBuf) {
    return MP3Decode(&_mp3DecInfo, ptr, len, outBuf, 0);
}

void MP3::initStream() {
    _file->fslseek(0);
    int offset = -1;
    _bytesLeft = 0;
    bool found = false; 
    while (!found) {
        while (offset < 0) {
            uint32_t nr;

            nr = readBytesFromStream();
            if (nr < 0) {
                return;
            }

            offset = MP3FindSyncWord(_readPtr, nr); 

            if (offset < 0) { // Discard the buffer.
                _bytesLeft = 0;
            } else {
                _readPtr = _readBuf + offset;
                _bytesLeft -= offset;
            }
        }
        offset = -1;

        // We have a sync word. Let's top up the buffer.
        readBytesFromStream();
        // This should have shifted the sync word to the start of the buffer.
        int err = decodeFrameInfo(_readPtr);
        if (err >= 0) {
            found = true;
        }
    }
}

int MP3::decodeNewChunk() {
    int err = -1;
    do {
        err = decodeStream(&_readPtr, &_bytesLeft, _outBuf);
        switch (err) {
            case 0:
            case -2: // Read more.
                if (readBytesFromStream() <= 0) {
                    return -2;
                }
                break;
            default:
                return err;
        }
    } while (err != 0);
    _numSamples = _mp3DecInfo.nGrans * _mp3DecInfo.nGranSamps * _mp3DecInfo.nChans;
    _outPos = 0;
    return err;
}

// Stream et al

int MP3::read() {
    if ((_numSamples == 0) || (_outPos == _numSamples)) {
        if (decodeNewChunk() < 0) {
            return -70000;
        }
    }
    return _outBuf[_outPos++];
}

uint32_t MP3::getNextSampleBlock(int16_t *samples, uint32_t len) {
    for (int i = 0; i < len; i++) {
        int v = read();
        if (v == -70000) {
            return i;
        }
        samples[i] = v;
    }
    return len;
}

uint32_t MP3::getFrameLength() {
    return _frameInfo.outputSamps;
}

uint32_t MP3::getChannels() {
    return _frameInfo.nChans;
}
