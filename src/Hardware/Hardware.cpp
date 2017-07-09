#include "Hardware.h"
#include "PinConfig.h"

Adafruit_PWMServoDriver _pwm = Adafruit_PWMServoDriver(0x40);

Hardware::Hardware()
{
  _btnPressed = false;
}

void Hardware::updateOnInterrupt() {
  _interruptCalled = true;
}

void Hardware::init()
{
    Serial.begin(115200);
    Serial.println("");
    Serial.println("");
    //system_update_cpu_freq(SYS_CPU_160MHZ);

    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(FAN, OUTPUT);

    digitalWrite(RELAY_1, true);
    digitalWrite(RELAY_2, true);
    digitalWrite(FAN, false);

    pinMode(BTN, INPUT);

    delay(10);

    Wire.begin(SDA, SCL);

    SPIFFS.begin();

    scan();

    _pwm.begin();
    _pwm.setPWMFreq(1600);
    //_pwm.setInvertedLogicMode();
    //_pwm.enableOutputDriverMode();
}

void Hardware::scan()
{
  Serial.println(F(""));
  Serial.println(F("Scanning I2C ..."));

  byte count = 0;

  for (byte i = 8; i < 120; i++) {
      Wire.beginTransmission(i);

      if (Wire.endTransmission() == 0) {
        Serial.print(F("Found i2c Device Address: "));
        Serial.print(i, DEC);
        Serial.print(F(" (0x"));
        Serial.print(i, HEX);
        Serial.println(F(")"));

        count++;

        delay(1);
      }
    }

    Serial.println(F("Done."));
    Serial.print(F("Found "));
    Serial.print(count, DEC);
    Serial.println(F(" device(s)."));
}

void Hardware::tick()
{
  if(_interruptCalled == true) {
    _interruptCalled = false;

    boolean _newBtnState = !digitalRead(BTN);

    if (_newBtnState != _btnPressed && _btnCallback != NULL) {
      _btnCallback(_newBtnState);
    }
    _btnPressed = _newBtnState;
  }
}

// **************************** LED ****************************

void Hardware::setLed(bool enabled)
{
  _ticker_red.detach();

  digitalWrite(LED_RED, enabled);
}

void Hardware::setBlinkLed(float seconds)
{
  _ticker_red.detach();
  _ticker_red.attach(seconds, &Hardware::_tick_red);
}

void Hardware::setYellowLed(bool enabled)
{
  _ticker_yellow.detach();

  digitalWrite(LED_RED, enabled);
}

void Hardware::setYellowBlinkLed(float seconds)
{
  _ticker_yellow.detach();
  _ticker_yellow.attach(seconds, &Hardware::_tick_yellow);
}

void Hardware::_tick_red() {
  int state = digitalRead(LED_RED);

  digitalWrite(LED_RED, !state);
}

void Hardware::_tick_yellow() {
  int state = digitalRead(LED_YELLOW);

  digitalWrite(LED_YELLOW, !state);
}

// **************************** FAN ****************************

void Hardware::setFan(bool enabled)
{
  digitalWrite(FAN, enabled);
}

// **************************** BTN ****************************

boolean Hardware::isButtonPressed()
{
  return !digitalRead(BTN);
}

void Hardware::setBtnCallback(void (*func)(boolean state)) {
  _btnCallback = func;
}

// **************************** PWM ****************************

int channelNum;

void ICACHE_FLASH_ATTR Hardware::setPWM(uint8_t channel, uint16_t value)
{
  /*
  uint16_t _value = map(value, 0, 4095, channelLowerBoundary[channel], 0);

  if(value == 0) {
    _value = 4095;
  }
  */
  //Serial.println(String("channel: ") + channel + String(" value: ") + value + String(" _value: ") + _value);

  _pwm.setPWM(channel, 0, value);
}

void ICACHE_FLASH_ATTR Hardware::setChannel(int roomNum, int channelNum, unsigned long value)
{
  channel[roomNum][channelNum] = value;
}

void ICACHE_FLASH_ATTR Hardware::updateChannels()
{
  for (uint8_t roomNum = 0; roomNum <  NUM_ROOMS; roomNum++) {
      for (uint8_t channelNum = 0; channelNum < NUM_CHANNELS; channelNum++ ) {
        _pwm.setPWM((roomNum * NUM_CHANNELS) + channelNum, 0, channel[roomNum][channelNum]);
      }
   }
}

// **************************** RELAY ****************************

boolean relay1 = false;
boolean relay2 = false;

void Hardware::_tick_relay_1() {
  relay1 = false;
  digitalWrite(RELAY_1, true);
  digitalWrite(FAN, relay1 || relay2);
}

void Hardware::_tick_relay_2() {
  relay2 = false;
  digitalWrite(RELAY_2, true);
  digitalWrite(FAN, relay1 || relay2);
}

void ICACHE_FLASH_ATTR Hardware::updateRelay()
{
  if(channel[0][0] == 0 &&
     channel[0][1] == 0 &&
     channel[0][2] == 0 &&
     channel[0][3] == 0) {
    relay1 = false;
    _ticker_relay_1.detach();
    _ticker_relay_1.once(500, &Hardware::_tick_relay_1);
   } else {
    relay1 = true;
   _ticker_relay_1.detach();
    digitalWrite(RELAY_1, false);
    digitalWrite(FAN, true);
   }

  if(channel[1][0] == 0 &&
     channel[1][1] == 0 &&
     channel[1][2] == 0 &&
     channel[1][3] == 0) {
    relay2 = false;
    _ticker_relay_2.detach();
    _ticker_relay_2.once(5, &Hardware::_tick_relay_2);
   } else {
   relay2 = true;
   _ticker_relay_2.detach();
    digitalWrite(RELAY_2, false);
    digitalWrite(FAN, true);
   }

  //digitalWrite(RELAY_1, !relay1);
  //digitalWrite(RELAY_2, !relay2);
  digitalWrite(LED_RED, !relay1 && !relay2);
}
