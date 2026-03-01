// @MX:NOTE: [MOCK] I2C Wire library mock for native testing
// PlatformIO native environment에서 I2C 통신 모방

#ifndef ARTHUR_WIRE_MOCK_H
#define ARTHUR_WIRE_MOCK_H

#include <cstdint>
#include <cstdio>

#ifdef ARTHUR_NATIVE_TEST

// I2C 버스 상태
#define WIRE_SUCCESS 0
#define WIRE_ERROR 1
#define WIRE_TIMEOUT 2

// TwoWire 클래스 모의
class TwoWire {
public:
    TwoWire() {
        _initialized = false;
        _transmissionInProgress = false;
        _slaveAddress = 0;
    }

    void begin() {
        _initialized = true;
        _transmissionInProgress = false;
    }

    void begin(uint8_t address) {
        // Slave 모드 (테스트용)
        _initialized = true;
        _slaveAddress = address;
    }

    void begin(int address) {
        begin((uint8_t)address);
    }

    void end() {
        _initialized = false;
        _transmissionInProgress = false;
    }

    // 마스터 모드 전송 시작
    void beginTransmission(uint8_t address) {
        if (!_initialized) return;
        _transmissionInProgress = true;
        _slaveAddress = address;
        _txBufferIndex = 0;
    }

    void beginTransmission(int address) {
        beginTransmission((uint8_t)address);
    }

    // 데이터를 전송 버퍼에 추가
    size_t write(uint8_t data) {
        if (!_transmissionInProgress) return 0;
        if (_txBufferIndex >= TX_BUFFER_SIZE) return 0;

        _txBuffer[_txBufferIndex++] = data;
        return 1;
    }

    size_t write(const uint8_t* data, size_t quantity) {
        if (!_transmissionInProgress) return 0;

        size_t written = 0;
        for (size_t i = 0; i < quantity && _txBufferIndex < TX_BUFFER_SIZE; i++) {
            _txBuffer[_txBufferIndex++] = data[i];
            written++;
        }
        return written;
    }

    // 전송 종료 및 실제 전송 실행
    uint8_t endTransmission(bool sendStop = true) {
        if (!_transmissionInProgress) return WIRE_ERROR;
        _transmissionInProgress = false;
        // 테스트에서는 항상 성공 반환
        return WIRE_SUCCESS;
    }

    uint8_t endTransmission(void) {
        return endTransmission(true);
    }

    // 마스터 모드에서 데이터 요청
    uint8_t requestFrom(uint8_t address, size_t quantity, bool sendStop = true) {
        if (!_initialized) return 0;
        // 테스트에서는 요청한 수만큼 반환 가능하다고 가정
        _rxBufferIndex = 0;
        _rxBufferSize = (quantity > RX_BUFFER_SIZE) ? RX_BUFFER_SIZE : quantity;
        return _rxBufferSize;
    }

    uint8_t requestFrom(uint8_t address, size_t quantity) {
        return requestFrom(address, quantity, true);
    }

    uint8_t requestFrom(int address, int quantity) {
        return requestFrom((uint8_t)address, (size_t)quantity);
    }

    uint8_t requestFrom(int address, int quantity, int sendStop) {
        return requestFrom((uint8_t)address, (size_t)quantity, sendStop != 0);
    }

    // 수신 버퍼에서 데이터 읽기
    int available() {
        if (_transmissionInProgress) return 0;
        return _rxBufferSize - _rxBufferIndex;
    }

    int read() {
        if (_rxBufferIndex >= _rxBufferSize) return -1;
        return _rxBuffer[_rxBufferIndex++];
    }

    int peek() {
        if (_rxBufferIndex >= _rxBufferSize) return -1;
        return _rxBuffer[_rxBufferIndex];
    }

    void flush() {
        _rxBufferIndex = 0;
        _rxBufferSize = 0;
    }

    // 테스트 헬퍼: 수신 버퍼 설정
    void mock_set_rx_buffer(const uint8_t* data, size_t length) {
        _rxBufferSize = (length > RX_BUFFER_SIZE) ? RX_BUFFER_SIZE : length;
        for (size_t i = 0; i < _rxBufferSize; i++) {
            _rxBuffer[i] = data[i];
        }
        _rxBufferIndex = 0;
    }

    // 테스트 헬퍼: 전송 버퍼 내용 확인
    const uint8_t* mock_get_tx_buffer() const {
        return _txBuffer;
    }

    size_t mock_get_tx_buffer_length() const {
        return _txBufferIndex;
    }

    void mock_clear_tx_buffer() {
        _txBufferIndex = 0;
    }

private:
    static const size_t TX_BUFFER_SIZE = 32;
    static const size_t RX_BUFFER_SIZE = 32;

    bool _initialized;
    bool _transmissionInProgress;
    uint8_t _slaveAddress;

    uint8_t _txBuffer[TX_BUFFER_SIZE];
    size_t _txBufferIndex;

    uint8_t _rxBuffer[RX_BUFFER_SIZE];
    size_t _rxBufferIndex;
    size_t _rxBufferSize;
};

// 전역 Wire 인스턴스
extern TwoWire Wire;

#endif // ARTHUR_NATIVE_TEST
#endif // ARTHUR_WIRE_MOCK_H
