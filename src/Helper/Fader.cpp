#include "ColorConverter.h"
#include "Fader.h"

Fader::Fader()
{
  Serial.println("Fader");
}

void Fader::setState(bool state) {
   if (transitionTime > 0 && !state) {
     transColor.r = 0;
     transColor.g = 0;
     transColor.b = 0;

     stepR = calculateStep(currentColor.r, transColor.r);
     stepG = calculateStep(currentColor.g, transColor.g);
     stepB = calculateStep(currentColor.b, transColor.b);
   } else {
     _setState(state);
   }
}

void Fader::setTransitionTime(uint8_t seconds) {
  transitionTime = seconds;
  startTransTime = millis();
}

void Fader::setColor(uint16_t red, uint16_t green, uint16_t blue) {
  // In transition/fade
   if (transitionTime > 0) {
     transColor.r = red;
     transColor.g = green;
     transColor.b = blue;

     // If light is off, start fading from Zero
     if (!currentState) {
       _setColor(0, 0, 0);
     }

     stepR = calculateStep(currentColor.r, transColor.r);
     stepG = calculateStep(currentColor.g, transColor.g);
     stepB = calculateStep(currentColor.b, transColor.b);

     Serial.print("Set Color: \tR.");
     Serial.print(currentColor.r);
     Serial.print(" -> ");
     Serial.print(transColor.r);
     Serial.print(" \tG. ");
     Serial.print(currentColor.g);
     Serial.print(" -> ");
     Serial.print(transColor.g);
     Serial.print(" \tB. ");
     Serial.println(currentColor.b);
     Serial.print(" -> ");
     Serial.println(transColor.b);

     Serial.print("Set Transition: \tR.");
     Serial.print(stepR);
     Serial.print("\tG. ");
     Serial.print(stepG);
     Serial.print("\tB. ");
     Serial.println(stepB);

     stepCount = 0;
   } else {
     _setColor(red, green, blue);
   }
}

void Fader::setColor(uint16_t red, uint16_t green, uint16_t blue, uint8_t transitionTime) {
  setTransitionTime(transitionTime);
  setColor(red, green, blue);
}

void Fader::setWhite(uint16_t value) {
  // In transition/fade
  if (transitionTime > 0) {
    transColor.w = value;

    // If light is off, start fading from Zero
    if (!currentState) {
      _setWhite(0);
    }

    stepW = calculateStep(currentColor.w, transColor.w);

    stepCount = 0;
  } else {
    _setWhite(value);
  }
}
/*
void Fader::setColourTemp(bool state) {
  // In transition/fade
   if (transitionTime > 0) {
     transColor = AiLight.colorTemperature2RGB(root[KEY_COLORTEMP]);

     // If light is off, start fading from Zero
     if (!AiLight.getState()) {
       AiLight.setColor(0, 0, 0);
     }

     stepR = calculateStep(AiLight.getColor().red, transColor.red);
     stepG = calculateStep(AiLight.getColor().green, transColor.green);
     stepB = calculateStep(AiLight.getColor().blue, transColor.blue);

     stepCount = 0;
   } else {
     AiLight.setColorTemperature(root[KEY_COLORTEMP]);
   }
}
*/
void Fader::setBrightness(uint16_t value) {
    // In transition/fade
    if (transitionTime > 0) {
      transBrightness = value;

      // If light is off, start fading from Zero
      if (!currentState) {
        _setBrightness(0);
      }

      stepBrightness = calculateStep(currentBrightness, transBrightness);
      stepCount = 0;
    } else {
      _setBrightness(value);
    }
}


void Fader::_setState(bool state) {
  //Serial.print("State: ");
  //Serial.println(state ? "true" : "false");

  currentState = state;

  if (_setStateCallback) {
    _setStateCallback(state);
  }
}

void Fader::_setColor(uint16_t red, uint16_t green, uint16_t blue) {
/*
  Serial.print("Color: R.");
  Serial.print(red);
  Serial.print("G. ");
  Serial.print(green);
  Serial.print("B. ");
  Serial.println(blue);

  Serial.print("Transition: R.");
  Serial.print(stepR);
  Serial.print("G. ");
  Serial.print(stepG);
  Serial.print("B. ");
  Serial.println(stepB);
*/
  currentColor.r = red;
  currentColor.g = green;
  currentColor.b = blue;

  if (_setColorCallback) {
    _setColorCallback(red, green, blue);
  }
}

void Fader::_setWhite(uint16_t value) {
  Serial.print("Color: W.");
  Serial.println(value);

  if (_setWhiteCallback) {
    _setWhiteCallback(value);
  }

  currentColor.w = value;
}

void Fader::_setBrightness(uint16_t value) {
  Serial.print("Brightness: ");
  Serial.println(value);

  currentBrightness = value;

  if (_setBrightnessCallback) {
    _setBrightnessCallback(value);
  }
}

/**
 * @brief Process requests and keep on running...
 */
void Fader::loopLight() {
  // Flashing
  if (flash) {
    if (startFlash) {
      startFlash = false;
      flashStartTime = millis();

      _setState(false);
    }

    // Run the flash sequence for the defined period.
    if ((millis() - flashStartTime) <= (flashLength - 100U)) {
      if ((millis() - flashStartTime) % 1000 <= 500) {
        _setColor(flashColor.r, flashColor.g, flashColor.b);
        _setBrightness(flashBrightness);
        _setState(true);
      } else {
        _setState(false);
      }
    } else {
      // Return to the state before the flash
      flash = false;

      _setState(currentState);
      _setColor(currentColor.r, currentColor.g, currentColor.b);
      _setBrightness(currentBrightness);

      //sendState(); // Notify subscribers again about current state
    }
  }

  // Transitioning/Fading
  if (transitionTime > 0) {
    _setState(true);

    uint32_t currentTransTime = millis();

    // Cross fade the RGBW channels every millisecond
    if (currentTransTime - startTransTime > transitionTime) {
      if (stepCount <= 1000) {
        startTransTime = currentTransTime;

        // Transition/fade RGB LEDS (if level is different from current)
        if (stepR != 0 || stepG != 0 || stepB != 0) {
          _setColor(calculateLevel(stepR, currentColor.r, stepCount, transColor.r),
                   calculateLevel(stepG, currentColor.g, stepCount, transColor.g),
                   calculateLevel(stepB, currentColor.b, stepCount, transColor.b));
        }

        // Transition/fade white LEDS (if level is different from current)
        if (stepW != 0) {
          _setWhite(calculateLevel(stepW, currentColor.w, stepCount, transColor.w));
        }

        // Transition/fade brightness (if level is different from current)
        if (stepBrightness != 0) {
          _setBrightness(calculateLevel(stepBrightness, currentBrightness, stepCount, transBrightness));
        }

        stepCount++;
      } else {
        transitionTime = 0;
        stepCount = 0;

        _setState(state);

        //sendState(); // Notify subscribers again about current state
        /*
        // Update settings
        cfg.is_on = AiLight.getState();
        cfg.brightness = AiLight.getBrightness();
        cfg.color = {
          AiLight.getColor().red,
          AiLight.getColor().green,
          AiLight.getColor().blue,
          AiLight.getColor().white
         };
        //EEPROM_write(cfg);
        */
      }
    }
  }
}

/**
 * @brief Determines the step needed to change to the target value
 *
 * @param currentLevel the current level
 * @param targetLevel the target level
 *
 * @return the step value needed to change to the target value
 */
int16_t Fader::calculateStep(uint8_t currentLevel, uint8_t targetLevel) {
  int16_t step = targetLevel - currentLevel;
  if (step) {
    step = 1000 / step;
  }

  return step;
}

/**
 * @brief Calculates the next level of a channel (RGBW/Brightness)
 *
 * @param step the step needed for changing to the target value
 * @param val the current value in the transitioning loop
 * @param i the current index in the transitioning loop
 * @param targetLevel the target level
 *
 * @return the next level of a channel (RGBW/Brightness)
 */
uint8_t Fader::calculateLevel(int step, int val, uint16_t i, uint8_t targetLevel) {
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

  val = constrain(val, 0, LEVEL_MAX); // Force boundaries

  return val;
}
