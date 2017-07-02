#ifndef __Hardware_h
#define __Hardware_h

#include <functional>
#include <Wire.h>
#include <Ticker.h>
#include <Adafruit_PWMServoDriver.h>
#include <FS.h>

#include "../Config.h"

class Hardware;

class Hardware
{
  public:
      Hardware();

      void init();
      void scan();
      void tick();

      void setLed(bool enabled);
      void setBlinkLed(float seconds);

      void setYellowLed(bool enabled);
      void setYellowBlinkLed(float seconds);

      void setPWM(uint8_t channel, uint16_t value);

      void setBtnCallback(void (*func)(boolean state));
      boolean isButtonPressed();

      void setChannel(int room, int channel, unsigned long value);
      String updateChannels();

      void updateRelais();
      void updateOnInterrupt();

  private:
      boolean _btnPressed;
      boolean _interruptCalled;

      Ticker _ticker_red;
      Ticker _ticker_yellow;

      static void _tick_red();
      static void _tick_yellow();

      void (*_btnCallback)(boolean state) = NULL;
};

#endif
