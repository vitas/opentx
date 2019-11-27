/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

enum BluetoothStates {
#if defined(PCBX9E)
  BLUETOOTH_INIT,
  BLUETOOTH_WAIT_TTM,
  BLUETOOTH_WAIT_BAUDRATE_CHANGE,
#endif
  BLUETOOTH_STATE_OFF,
  BLUETOOTH_STATE_FACTORY_BAUDRATE_INIT,
  BLUETOOTH_STATE_BAUDRATE_SENT,
  BLUETOOTH_STATE_BAUDRATE_INIT,
  BLUETOOTH_STATE_NAME_SENT,
  BLUETOOTH_STATE_POWER_SENT,
  BLUETOOTH_STATE_ROLE_SENT,
  BLUETOOTH_STATE_IDLE,
  BLUETOOTH_STATE_DISCOVER_REQUESTED,
  BLUETOOTH_STATE_DISCOVER_SENT,
  BLUETOOTH_STATE_DISCOVER_START,
  BLUETOOTH_STATE_DISCOVER_END,
  BLUETOOTH_STATE_BIND_REQUESTED,
  BLUETOOTH_STATE_CONNECT_SENT,
  BLUETOOTH_STATE_CONNECTED,
  BLUETOOTH_STATE_UPLOAD,
  BLUETOOTH_STATE_DISCONNECTED,
  BLUETOOTH_STATE_CLEAR_REQUESTED,
  BLUETOOTH_STATE_FLASH_FIRMWARE
};

constexpr uint8_t LEN_BLUETOOTH_ADDR =         16;
constexpr uint8_t MAX_BLUETOOTH_DISTANT_ADDR = 6;
constexpr uint8_t BLUETOOTH_BUFFER_SIZE =      128;

class Bluetooth
{
  enum FrameType {
    FRAME_TYPE_UPLOAD = 0x05,
    FRAME_TYPE_UPLOAD_ACK = 0x80 + FRAME_TYPE_UPLOAD,
    FRAME_TYPE_DOWNLOAD = 0x06,
    FRAME_TYPE_DOWNLOAD_ACK = 0x80 + FRAME_TYPE_DOWNLOAD,
    FRAME_TYPE_CHANNELS = 0x07,
    FRAME_TYPE_TELEMETRY = 0x08,
  };

  public:
    void writeString(const char * str);
    char * readline(bool error_reset = true);
    void write(const uint8_t * data, uint8_t length);

    void sendTelemetryFrame(const uint8_t * packet);
    void wakeup();
    const char * flashFirmware(const char * filename);

    volatile uint8_t state;
    char localAddr[LEN_BLUETOOTH_ADDR+1];
    char distantAddr[LEN_BLUETOOTH_ADDR+1];

  protected:
    void initFrame(uint8_t frameType);
    void pushByte(uint8_t byte);
    void sendFrame();

    uint8_t read(uint8_t * data, uint8_t size, uint32_t timeout=1000/*ms*/);
    // void appendTrainerByte(uint8_t data);
    // void processTrainerFrame(const uint8_t * buffer);
    // void processTrainerByte(uint8_t data);
    // void sendTrainer();
    // void receiveTrainer();
    bool readFrame();
    bool processFrameByte(uint8_t byte);
    bool checkFrame();
    void processFrame();
    void processChannelsFrame();
    void processTelemetryFrame();
    void processUploadFrame();
    void appendFrameByte(uint8_t byte);

    uint8_t bootloaderChecksum(uint8_t command, const uint8_t * data, uint8_t size);
    void bootloaderSendCommand(uint8_t command, const void *data = nullptr, uint8_t size = 0);
    void bootloaderSendCommandResponse(uint8_t response);
    const char * bootloaderWaitCommandResponse(uint32_t timeout=1000/*ms*/);
    const char * bootloaderWaitResponseData(uint8_t *data, uint8_t size);
    const char * bootloaderSetAutoBaud();
    const char * bootloaderReadStatus(uint8_t &status);
    const char * bootloaderCheckStatus();
    const char * bootloaderSendData(const uint8_t * data, uint8_t size);
    const char * bootloaderEraseFlash(uint32_t start, uint32_t size);
    const char * bootloaderStartWriteFlash(uint32_t start, uint32_t size);
    const char * bootloaderWriteFlash(const uint8_t * data, uint32_t size);
    const char * doFlashFirmware(const char * filename);

    uint8_t buffer[BLUETOOTH_BUFFER_SIZE];
    uint8_t bufferIndex = 0;
    tmr10ms_t wakeupTime = 0;
    uint8_t crc;
    uint8_t dataState = STATE_DATA_START;

    FIL file;
    uint32_t lastPartIndex;
};

extern Bluetooth bluetooth;
