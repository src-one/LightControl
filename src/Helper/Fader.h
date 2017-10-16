#ifndef FADER_h
#define FADER_h

#include <Arduino.h>
#include <functional>

#define LEVEL_MAX 4095

typedef std::function<void(bool)> TSetStateFunction;
typedef std::function<void(uint16_t, uint16_t, uint16_t)> TSetColorFunction;
typedef std::function<void(uint16_t)> TSetWhiteFunction;
typedef std::function<void(uint16_t)> TSetBrightnessFunction;

class Fader;

class Fader
{
  public:
    Fader();

    // flash
    bool flash = false;
    bool startFlash = false;

    uint16_t flashLength = 0;
    uint32_t flashStartTime = 0;
    uint8_t flashBrightness = 0;
    struct rgbwcolor flashColor = { 0, 0, 0, 0 };

    // current state
    struct rgbwcolor currentColor = { 0, 0, 0, 0 };
    uint8_t currentBrightness;
    bool currentState;

    // transition/fade
    bool state = false;
    uint16_t transitionTime = 0;
    uint32_t startTransTime = 0;
    int stepR, stepG, stepB, stepW, stepBrightness = 0;
    uint16_t stepCount = 0;
    struct rgbwcolor transColor = { 0, 0, 0, 0 };
    uint8_t transBrightness = 0;

    void setTransitionTime(uint8_t seconds);
    void setState(bool state);
    void setColor(uint16_t red, uint16_t green, uint16_t blue);
    void setColor(uint16_t red, uint16_t green, uint16_t blue, uint8_t seconds);
    void setWhite(uint16_t value);
    void setBrightness(uint16_t value);

    void onSetState(TSetStateFunction fn) { _setStateCallback = fn; }
    void onSetColor(TSetColorFunction fn) { _setColorCallback = fn; }
    void onSetWhite(TSetWhiteFunction fn) { _setWhiteCallback = fn; }
    void onSetBrightness(TSetBrightnessFunction fn) { _setBrightnessCallback = fn; }

    void loopLight();

    private:
      int16_t calculateStep(uint8_t currentLevel, uint8_t targetLevel);
      uint8_t calculateLevel(int step, int val, uint16_t i, uint8_t targetLevel);

      void _setState(bool state);
      void _setColor(uint16_t red, uint16_t green, uint16_t blue);
      void _setWhite(uint16_t value);
      void _setBrightness(uint16_t value);

      TSetStateFunction _setStateCallback = NULL;
      TSetColorFunction _setColorCallback = NULL;
      TSetWhiteFunction _setWhiteCallback = NULL;
      TSetBrightnessFunction _setBrightnessCallback = NULL;
};

#endif
