#include "ColorConverter.h"

struct rgbwcolor color = { 0, 0, 0, 0 };

int DEBUG = 1;      // DEBUG counter; if set to 1, will write values back via serial
int loopCount = 30; // How often should DEBUG report?

int currentStep = 0;
bool isAnimating = false;

struct rgbwcolor prevColor = { color.r, color.g, color.b, color.w };

struct rgbwcolor _step = { 0, 0, 0, 0 };

int _calculateStep(int prevValue, int endValue) {
  int step = endValue - prevValue;
  if (step) {
    step = 1020 / step;
  }
  return step;
}

/* The next function is calculateVal. When the loop value, i,
*  reaches the step size appropriate for one of the
*  colors, it increases or decreases the value of that color by 1.
*  (R, G, and B are each calculated separately.)
*/

int _calculateVal(int step, int val, int i) {
  if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
    if (step > 0) {              //   increment the value if step is positive...
      val += 1;
    }
    else if (step < 0) {         //   ...or decrement it if step is negative
      val -= 1;
    }
  }
  // Defensive driving: make sure val stays in the range 0-255
  if (val > 255) {
    val = 255;
  }
  else if (val < 0) {
    val = 0;
  }

  return val;
}

/* crossFade() converts the percentage colors to a
*  0-255 range, then loops 1020 times, checking to see if
*  the value needs to be updated each time, then writing
*  the color values to the correct pins.
*/

void crossFade(rgbwcolor targetColor) {
/*
  if(isAnimating) {
    return;
  }
*/
  int R = targetColor.r;
  int G = targetColor.g;
  int B = targetColor.b;
  int W = targetColor.w;

  _step.r = _calculateStep(prevColor.r, R);
  _step.g = _calculateStep(prevColor.g, G);
  _step.b = _calculateStep(prevColor.b, B);
  _step.w = _calculateStep(prevColor.w, W);

  isAnimating = true;
  // triggering of process()

  prevColor.r = color.r;
  prevColor.g = color.g;
  prevColor.b = color.b;
  prevColor.w = color.w;
}


struct rgbwcolor getNextColor() {
  if(!isAnimating) {
    return color;
  }

  color.r = _calculateVal(_step.r, color.r, currentStep);
  color.g = _calculateVal(_step.g, color.g, currentStep);
  color.b = _calculateVal(_step.b, color.b, currentStep);
  color.w = _calculateVal(_step.w, color.w, currentStep);

  if (DEBUG) {
    if (currentStep == 0 or currentStep % loopCount == 0) {
      Serial.print("Loop/RGBW: #");
      Serial.print(currentStep);
      Serial.print(" | ");
      Serial.print(color.r);
      Serial.print(" / ");
      Serial.print(color.g);
      Serial.print(" / ");
      Serial.print(color.b);
      Serial.print(" / ");
      Serial.println(color.w);
    }
    DEBUG += 1;
  }

  return color;

  currentStep ++;

  if (currentStep <= 1020) {
    currentStep = 0;
    isAnimating = false;
  }
}
