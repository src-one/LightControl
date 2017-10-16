#include "ColorConverter.h"
#include "CrossFader.h"

CrossFader::CrossFader()
{
  Serial.println("CrossFader");
}

int16_t CrossFader::_calculateStep(uint16_t currentLevel, uint16_t targetLevel) {
  int16_t step = targetLevel - currentLevel;
  if (step) {
    step = 255 / step;
  }

  return step;
}

uint16_t CrossFader::_calculateVal(int step, int val, uint16_t i, uint16_t targetLevel) {
  if ((step) && i % step == 0) {
    if (step > 0) {
      val++;

      // Prevent overshooting the target level
      if (val > targetLevel) {
        val = targetLevel;
      }
    } else if (step < 0) {
      val--;

      // Prevent undershooting the target level
      if (val < targetLevel) {
        val = targetLevel;
      }
    }
  }

  val = constrain(val, 0, 4095); // Force boundaries

  return val;
}

void CrossFader::setCurrentColor(rgbwcolor color) {
  _prevColor.r = color.r;
  _prevColor.g = color.g;
  _prevColor.b = color.b;
  _prevColor.w = color.w;
}


void CrossFader::crossFade(rgbwcolor targetColor) {
  _stepsR = _calculateStep(_prevColor.r, targetColor.r);
  _stepsG = _calculateStep(_prevColor.g, targetColor.g);
  _stepsB = _calculateStep(_prevColor.b, targetColor.b);
  _stepsW = _calculateStep(_prevColor.w, targetColor.w);

  _targetColor.r = targetColor.r;
  _targetColor.g = targetColor.g;
  _targetColor.b = targetColor.b;
  _targetColor.w = targetColor.w;

  Serial.print(_stepsR);
  Serial.print(", ");
  Serial.print(_stepsG);
  Serial.print(", ");
  Serial.print(_stepsB);
  Serial.print(", ");
  Serial.println(_stepsW);

  isAnimating = true;
}

struct rgbwcolor CrossFader::getNextColor() {
  if(!isAnimating) {
    return _color;
  }

  _color.r = _calculateVal(_stepsR, _color.r, _currentStep, _targetColor.r);
  _color.g = _calculateVal(_stepsG, _color.g, _currentStep, _targetColor.g);
  _color.b = _calculateVal(_stepsB, _color.b, _currentStep, _targetColor.b);
  _color.w = _calculateVal(_stepsW, _color.w, _currentStep, _targetColor.w);

  //stepB, AiLight.getColor().blue, stepCount, transColor.blue

  if (DEBUG) {
    if (_currentStep == 0 or _currentStep % loopCount == 0) {
      Serial.print("Loop/RGBW: #");
      Serial.print(_currentStep);
      Serial.print(" | ");
      Serial.print(_color.r);
      Serial.print(" / ");
      Serial.print(_color.g);
      Serial.print(" / ");
      Serial.print(_color.b);
      Serial.print(" / ");
      Serial.println(_color.w);
    }
    DEBUG += 1;
  }

  _currentStep ++;

  if (_currentStep >= 256) {
    Serial.println("finished");

    _currentStep = 0;
    isAnimating = false;

    _prevColor.r = _color.r;
    _prevColor.g = _color.g;
    _prevColor.b = _color.b;
    _prevColor.w = _color.w;
  }

  return _color;
}
