#ifndef CROSSFADER_h
#define CROSSFADER_h

#include <Arduino.h>
#include <functional>

class CrossFader;

class CrossFader
{
  public:
    CrossFader();

    int DEBUG = true;
    int loopCount = 30;

    bool isAnimating = false;

    void setCurrentColor(rgbwcolor color);
    void crossFade(rgbwcolor targetColor);
    struct rgbwcolor getNextColor();

    private:
        int _currentStep = 0;

        struct rgbwcolor _color = { 0, 0, 0, 0 };
        struct rgbwcolor _prevColor = { _color.r, _color.g, _color.b, _color.w };
        struct rgbwcolor _targetColor = { _color.r, _color.g, _color.b, _color.w };
        //struct rgbwcolor _steps = { 0, 0, 0, 0 };

        int _stepsR = 0;
        int _stepsG = 0;
        int _stepsB = 0;
        int _stepsW = 0;

        int16_t _calculateStep(uint16_t currentLevel, uint16_t targetLevel);
        uint16_t _calculateVal(int step, int val, uint16_t i, uint16_t targetLevel);
};

#endif
