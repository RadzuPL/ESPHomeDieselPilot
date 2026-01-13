#pragma once
#include "esphome.h"
#include <SPI.h>

// Piny CC1101 jak w oryginalnym projekcie
#define PIN_SCK   18
#define PIN_MISO  19
#define PIN_MOSI  23
#define PIN_SS    5
#define PIN_GDO2  4

// Komendy
#define CMD_WAKEUP 0x23
#define CMD_MODE   0x24
#define CMD_POWER  0x2B
#define CMD_UP     0x3C
#define CMD_DOWN   0x3E

// Stany pieca
#define STATE_OFF           0
#define STATE_STARTUP       1
#define STATE_WARMING       2
#define STATE_WARMING_WAIT  3
#define STATE_PRE_RUN       4
#define STATE_RUNNING       5
#define STATE_SHUTDOWN      6
#define STATE_SHUTTING_DOWN 7
#define STATE_COOLING       8

using namespace esphome;

class DieselPilotComponent : public PollingComponent {
 public:
  // Ułatwienie do wywołań z lambdy
  static DieselPilotComponent *instance;

  // ====== PUBLICZNE ENCE (podają Ci je z YAML przez settery) ======
  // Tutaj podpinamy sensory/text_sensory z YAML
  sensor::Sensor *voltage_sensor   = nullptr;
  sensor::Sensor *ambient_sensor   = nullptr;
  sensor::Sensor *case_sensor      = nullptr;
  sensor::Sensor *setpoint_sensor  = nullptr;
  sensor::Sensor *pump_sensor      = nullptr;
  sensor::Sensor *rssi_sensor      = nullptr;
  text_sensor::TextSensor *state_text = nullptr;
  text_sensor::TextSensor *mode_text  = nullptr;

  // Adres nagrzewnicy – na razie na sztywno; możesz go zmienić w kodzie
  // albo później dodać mechanizm parowania.
  uint32_t heater_address = 0x00000000;
  bool heater_paired = false;

  // Konstruktor – pooling co 3 sekundy
  DieselPilotComponent() : PollingComponent(3000) {
    instance = this;
  }

  // ====== API dla przycisków z YAML ======
  void send_power() { send_command(CMD_POWER); }
  void send_up()    { send_command(CMD_UP);    }
  void send_down()  { send_command(CMD_DOWN);  }
  void send_mode()  { send_command(CMD_MODE);  }

  // Jak chcesz ręcznie ustawić adres z YAML/lambdy, możesz zrobić:
  void set_heater_address(uint32_t addr) {
    heater_address = addr;
    heater_paired = (addr != 0);
  }

  // ====== setup() – inicjalizacja CC1101 ======
  void setup() override {
    // GPIO
    pinMode(PIN_SCK, OUTPUT);
    pinMode(PIN_MOSI, OUTPUT);
    pinMode(PIN_MISO, INPUT);
    pinMode(PIN_SS, OUTPUT);
    pinMode(PIN_GDO2, INPUT);

    // SPI
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SS);

    cc1101_init();

    // Jeśli chcesz na sztywno podać adres z kodu:
    // heater_address = 0xCA00445B;
    // heater_paired = true;
  }

  // ====== update() – co 3 s odczyt statusu ======
  void update() override {
    if (!heater_paired || heater_address == 0)
      return;

    update_heater_status();

    // publikacja do encji ESPHome
    if (voltage_sensor != nullptr)
      voltage_sensor->publish_state(heater_status_.voltage / 10.0f);
    if (ambient_sensor != nullptr)
      ambient_sensor->publish_state(heater_status_.ambient_temp);
    if (case_sensor != nullptr)
      case_sensor->publish_state(heater_status_.case_temp);
    if (setpoint_sensor != nullptr)
      setpoint_sensor->publish_state(heater_status_.setpoint);
    if (pump_sensor != nullptr)
      pump_sensor->publish_state(heater_status_.pump_freq / 10.0f);
    if (rssi_sensor != nullptr)
      rssi_sensor->publish_state(heater_status_.rssi);

    if (state_text != nullptr)
      state_text->publish_state(get_state_name(heater_status_.state));
    if (mode_text != nullptr)
      mode_text->publish_state(heater_status_.auto_mode ? "AUTO" : "MANUAL");
  }

 protected:
  // ====== struktura statusu – uproszczona wersja z .ino ======
  struct HeaterStatus {
    uint8_t  state        = 0;
    uint8_t  power        = 0;
    uint16_t voltage      = 0;
    int8_t   ambient_temp = 0;
    uint8_t  case_temp    = 0;
    int8_t   setpoint     = 0;
    uint8_t  pump_freq    = 0;
    bool     auto_mode    = false;
    int16_t  rssi         = 0;
    unsigned long last_update = 0;
  } heater_status_;

  uint8_t packet_seq_ = 0;

  // ====== CC1101 low-level ======
  void cc1101_write_reg(uint8_t addr, uint8_t val) {
    digitalWrite(PIN_SS, LOW);
    while (digitalRead(PIN_MISO));
    SPI.transfer(addr);
    SPI.transfer(val);
    digitalWrite(PIN_SS, HIGH);
  }

  void cc1101_write_burst(uint8_t addr, uint8_t len, uint8_t *bytes) {
    digitalWrite(PIN_SS, LOW);
    while (digitalRead(PIN_MISO));
    SPI.transfer(addr);
    for (int i = 0; i < len; i++)
      SPI.transfer(bytes[i]);
    digitalWrite(PIN_SS, HIGH);
  }

  void cc1101_strobe(uint8_t addr) {
    digitalWrite(PIN_SS, LOW);
    while (digitalRead(PIN_MISO));
    SPI.transfer(addr);
    digitalWrite(PIN_SS, HIGH);
  }

  uint8_t cc1101_read_reg(uint8_t addr) {
    digitalWrite(PIN_SS, LOW);
    while (digitalRead(PIN_MISO));
    SPI.transfer(addr);
    uint8_t val = SPI.transfer(0xFF);
    digitalWrite(PIN_SS, HIGH);
    return val;
  }

  uint16_t crc16_modbus(uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
      crc ^= (uint8_t) buf[pos];
      for (int i = 8; i != 0; i--) {
        if ((crc & 0x0001) != 0) {
          crc >>= 1;
          crc ^= 0xA001;
        } else {
          crc >>= 1;
        }
      }
    }
    return crc;
  }

  void cc1101_init() {
    cc1101_strobe(0x30);  // SRES
    delay(100);

    cc1101_write_reg(0x00, 0x07); // IOCFG2
    cc1101_write_reg(0x02, 0x06); // IOCFG0
    cc1101_write_reg(0x03, 0x47); // FIFOTHR
    cc1101_write_reg(0x07, 0x04); // PKTCTRL1
    cc1101_write_reg(0x08, 0x05); // PKTCTRL0
    cc1101_write_reg(0x0A, 0x00); // CHANNR
    cc1101_write_reg(0x0B, 0x06); // FSCTRL1
    cc1101_write_reg(0x0C, 0x00); // FSCTRL0

    // 433.937 MHz
    cc1101_write_reg(0x0D, 0x10); // FREQ2
    cc1101_write_reg(0x0E, 0xB0); // FREQ1
    cc1101_write_reg(0x0F, 0x9C); // FREQ0

    cc1101_write_reg(0x10, 0xF8); // MDMCFG4
    cc1101_write_reg(0x11, 0x93); // MDMCFG3
    cc1101_write_reg(0x12, 0x13); // MDMCFG2
    cc1101_write_reg(0x13, 0x22); // MDMCFG1
    cc1101_write_reg(0x14, 0xF8); // MDMCFG0
    cc1101_write_reg(0x15, 0x26); // DEVIATN
    cc1101_write_reg(0x17, 0x30); // MCSM1
    cc1101_write_reg(0x18, 0x18); // MCSM0
    cc1101_write_reg(0x19, 0x16); // FOCCFG
    cc1101_write_reg(0x1A, 0x6C); // BSCFG
    cc1101_write_reg(0x1B, 0x03); // AGCTRL2
    cc1101_write_reg(0x1C, 0x40); // AGCTRL1
    cc1101_write_reg(0x1D, 0x91); // AGCTRL0
    cc1101_write_reg(0x20, 0xFB); // WORCTRL
    cc1101_write_reg(0x21, 0x56); // FREND1
    cc1101_write_reg(0x22, 0x17); // FREND0
    cc1101_write_reg(0x23, 0xE9); // FSCAL3
    cc1101_write_reg(0x24, 0x2A); // FSCAL2
    cc1101_write_reg(0x25, 0x00); // FSCAL1
    cc1101_write_reg(0x26, 0x1F); // FSCAL0
    cc1101_write_reg(0x2C, 0x81); // TEST2
    cc1101_write_reg(0x2D, 0x35); // TEST1
    cc1101_write_reg(0x2E, 0x09); // TEST0
    cc1101_write_reg(0x09, 0x00); // ADDR
    cc1101_write_reg(0x04, 0x7E); // SYNC1
    cc1101_write_reg(0x05, 0x3C); // SYNC0

    uint8_t paTable[8] = {0x00, 0x12, 0x0E, 0x34, 0x60, 0xC5, 0xC1, 0xC0};
    cc1101_write_burst(0x7E, 8, paTable);

    cc1101_strobe(0x31); // SFSTXON
    cc1101_strobe(0x36); // SIDLE
    cc1101_strobe(0x3B); // SFTX
    cc1101_strobe(0x36); // SIDLE
    cc1101_strobe(0x3A); // SFRX
    delay(136);
  }

  void tx_flush() {
    cc1101_strobe(0x36); // SIDLE
    cc1101_strobe(0x3B); // SFTX
    delay(16);
  }

  void tx_burst(uint8_t len, uint8_t *bytes) {
    tx_flush();
    cc1101_write_burst(0x7F, len, bytes);
    cc1101_strobe(0x35); // STX
  }

  void rx_flush() {
    cc1101_strobe(0x36); // SIDLE
    cc1101_read_reg(0xBF); // dummy read
    cc1101_strobe(0x3A); // SFRX
    delay(16);
  }

  void rx_enable() {
    cc1101_strobe(0x34); // SRX
  }

  bool receive_packet(uint8_t *bytes, uint16_t timeout) {
    unsigned long t = millis();
    uint8_t rxLen;

    rx_flush();
    rx_enable();

    while (true) {
      yield();
      if (millis() - t > timeout)
        return false;

      while (!digitalRead(PIN_GDO2)) {
        yield();
        if (millis() - t > timeout)
          return false;
      }

      delay(5);
      rxLen = cc1101_read_reg(0xFB);
      if (rxLen >= 23 && rxLen <= 26)
        break;

      rx_flush();
      rx_enable();
    }

    for (int i = 0; i < rxLen; i++)
      bytes[i] = cc1101_read_reg(0xBF);

    rx_flush();

    uint16_t crc   = crc16_modbus(bytes, 21);
    uint16_t rxCrc = (bytes[21] << 8) | bytes[22];
    return (crc == rxCrc);
  }

  void send_command(uint8_t cmd) {
    if (!heater_paired || heater_address == 0)
      return;

    uint8_t buf[10];
    buf[0] = 9;
    buf[1] = cmd;
    buf[2] = (heater_address >> 24) & 0xFF;
    buf[3] = (heater_address >> 16) & 0xFF;
    buf[4] = (heater_address >> 8) & 0xFF;
    buf[5] = heater_address & 0xFF;
    buf[6] = packet_seq_++;
    buf[9] = 0;

    uint16_t crc = crc16_modbus(buf, 7);
    buf[7] = (crc >> 8) & 0xFF;
    buf[8] = crc & 0xFF;

    // oryginalny kod wysyła ramkę 10 razy z czekaniem na zakończenie TX
    for (int i = 0; i < 10; i++) {
      tx_burst(10, buf);
      unsigned long t = millis();
      while (cc1101_read_reg(0xF5) != 0x01) {
        delay(1);
        if (millis() - t > 100)
          return;
      }
    }
  }

  void update_heater_status() {
    send_command(CMD_WAKEUP);

    uint8_t buf[32];
    if (!receive_packet(buf, 2000))
      return;

    uint32_t addr = ((uint32_t)buf[2] << 24) |
                    ((uint32_t)buf[3] << 16) |
                    ((uint32_t)buf[4] << 8)  |
                    buf[5];

    if (addr != heater_address)
      return;

    heater_status_.state        = buf[6];
    heater_status_.power        = buf[7];
    heater_status_.voltage      = buf[9];
    heater_status_.ambient_temp = (int8_t)buf[10];
    heater_status_.case_temp    = buf[12];
    heater_status_.setpoint     = (int8_t)buf[13];
    heater_status_.auto_mode    = (buf[14] == 0x32);
    heater_status_.pump_freq    = buf[15];
    heater_status_.rssi         = ((buf[23] - (buf[23] >= 128 ? 256 : 0)) / 2) - 74;
    heater_status_.last_update  = millis();
  }

  const char *get_state_name(uint8_t state) {
    switch (state) {
      case STATE_OFF:           return "OFF";
      case STATE_STARTUP:       return "STARTUP";
      case STATE_WARMING:       return "WARMING";
      case STATE_WARMING_WAIT:  return "WARM WAIT";
      case STATE_PRE_RUN:       return "PRE-RUN";
      case STATE_RUNNING:       return "RUNNING";
      case STATE_SHUTDOWN:      return "SHUTDOWN";
      case STATE_SHUTTING_DOWN: return "SHUTTING";
      case STATE_COOLING:       return "COOLING";
      default:                  return "UNKNOWN";
    }
  }
};

DieselPilotComponent *DieselPilotComponent::instance = nullptr;
