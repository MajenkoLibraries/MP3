#include <MP3.h>

int MP3::readBytesFromStream() {
    memmove(_readBuf, _readPtr, _bytesLeft);
    uint32_t nRead = _source->readBytes(_readBuf + _bytesLeft, READBUF_SIZE - _bytesLeft);
    memset(_readBuf + _bytesLeft + nRead, 0, READBUF_SIZE - _bytesLeft - nRead);
    _bytesLeft += nRead;
    _readPtr = _readBuf;
    return nRead;
}

MP3::MP3(Stream &str) {
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

int MP3::decodeFrameInfo(uint8_t *ptr) {
    return MP3GetNextFrameInfo(&_mp3DecInfo, &_frameInfo, ptr);
}

int MP3::decodeStream(uint8_t **ptr, int *len, int16_t *outBuf) {
    return MP3Decode(&_mp3DecInfo, ptr, len, outBuf, 0);

}

int MP3::begin() {
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

int MP3::decodeNewChunk() {
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

int MP3::read() {
    if ((_numSamples == 0) || (_outPos == _numSamples)) {
        if (decodeNewChunk() < 0) {
            return -70000;
        }
    }
    return _outBuf[_outPos++];
}
