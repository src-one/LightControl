#ifndef ColorConverter_h
#define ColorConverter_h

#include <Arduino.h>
#include "ColorConverter.h"

#define max(a, b) ((a)>(b) ? (a) : (b))

ColorConverter::ColorConverter()
{
  Serial.println("converter");
}

rgbwcolor ColorConverter::hue(int bri, int hue, int sat) {
  rgbwcolor color(0, 0, 0, 0);
  double hh, p, q, t, ff, s, v;
  long i;

  color.w = 0;
  s = sat / 255.0;
  v = bri / 255.0;

  if (s <= 0.0) { // < is bogus, just shuts up warnings
    color.r = v;
    color.g = v;
    color.b = v;
    return color;
  }

  hh = hue;

  if (hh >= 65535.0)
    hh = 0.0;

  hh /= 11850, 0;
  i = (long)hh;
  ff = hh - i;
  p = v * (1.0 - s);
  q = v * (1.0 - (s * ff));
  t = v * (1.0 - (s * (1.0 - ff)));

  switch (i) {
  case 0:
    color.r = v * 255.0;
    color.g = t * 255.0;
    color.b = p * 255.0;
    break;
  case 1:
    color.r = q * 255.0;
    color.g = v * 255.0;
    color.b = p * 255.0;
    break;
  case 2:
    color.r = p * 255.0;
    color.g = v * 255.0;
    color.b = t * 255.0;
    break;

  case 3:
    color.r = p * 255.0;
    color.g = q * 255.0;
    color.b = v * 255.0;
    break;
  case 4:
    color.r = t * 255.0;
    color.g = p * 255.0;
    color.b = v * 255.0;
    break;
  case 5:
  default:
    color.r = v * 255.0;
    color.g = p * 255.0;
    color.b = q * 255.0;
    break;
  }

  return color;
}

rgbwcolor ColorConverter::xy(int bri, float x, float y) {
  rgbwcolor color(0, 0, 0, 0);

  //float Y = bri / 255.0f;
  float Y = (bri / 255.0f) * 0.225f;

  float z = 1.0f - x - y;

  float X = (Y / y) * x;
  float Z = (Y / y) * z;

/*
  // sRGB D65 conversion
  float r = X * 1.656492f - Y * 0.354851f - Z * 0.255038f;
  float g = -X * 0.707196f + Y * 1.655397f + Z * 0.036152f;
  float b = X * 0.051713f - Y * 0.121364f + Z * 1.011530f;
*/
  // sRGB D65 conversion
  float r = X * 3.2406f - Y * 1.5372f - Z * 0.4986f;
  float g = -X * 0.9689f + Y * 1.8758f + Z * 0.0415f;
  float b = X * 0.0557f - Y * 0.2040f + Z * 1.0570f;

  if (r > b && r > g && r > 1.0f) {
    // red is too big
    g = g / r;
    b = b / r;
    r = 1.0f;
  } else if (g > b && g > r && g > 1.0f) {
    // green is too big
    r = r / g;
    b = b / g;
    g = 1.0f;
  } else if (b > r && b > g && b > 1.0f) {
    // blue is too big
    r = r / b;
    g = g / b;
    b = 1.0f;
  }

  // Apply gamma correction
  r = r <= 0.0031308f ? 12.92f * r : (1.0f + 0.055f) * (float) pow(r, (1.0f / 2.4f)) - 0.055f;
  g = g <= 0.0031308f ? 12.92f * g : (1.0f + 0.055f) * (float) pow(g, (1.0f / 2.4f)) - 0.055f;
  b = b <= 0.0031308f ? 12.92f * b : (1.0f + 0.055f) * (float) pow(b, (1.0f / 2.4f)) - 0.055f;

  if (r > b && r > g) {
    // red is biggest
    if (r > 1.0f) {
      g = g / r;
      b = b / r;
      r = 1.0f;
    }
  } else if (g > b && g > r) {
    // green is biggest
    if (g > 1.0f) {
      r = r / g;
      b = b / g;
      g = 1.0f;
    }
  } else if (b > r && b > g) {
    // blue is biggest
    if (b > 1.0f) {
      r = r / b;
      g = g / b;
      b = 1.0f;
    }
  }

  r = r < 0 ? 0 : r;
  g = g < 0 ? 0 : g;
  b = b < 0 ? 0 : b;

  color.r = (int)(r * 255.0f);
  color.g = (int)(g * 255.0f);
  color.b = (int)(b * 255.0f);
  color.w = 0;

  return color;
}

/*
rgbwcolor ColorConverter::xy(int brightness_raw, float x, float y) {
  rgbwcolor color(0, 0, 0, 0);

  float brightness = ((float)brightness_raw) / 255.0f;

  float bright_y = brightness / y;
  float X = x * bright_y;
  float Z = (1 - x - y) * bright_y;

  // convert to RGB (0.0-1.0) color space
  float R = X * 1.4628067 - brightness * 0.1840623 - Z * 0.2743606;
  float G = -X * 0.5217933 + brightness * 1.4472381 + Z * 0.0677227;
  float B = X * 0.0349342 - brightness * 0.0968930 + Z * 1.2884099;

  // apply inverse 2.2 gamma
  float inv_gamma = 1.0 / 2.4;
  float linear_delta = 0.055;
  float linear_interval = 1 + linear_delta;

  float r = R <= 0.0031308 ? 12.92 * R : (linear_interval) * pow(R, inv_gamma) - linear_delta;
  float g = G <= 0.0031308 ? 12.92 * G : (linear_interval) * pow(G, inv_gamma) - linear_delta;
  float b = B <= 0.0031308 ? 12.92 * B : (linear_interval) * pow(B, inv_gamma) - linear_delta;

  color.r = (int)(r * COLOR_SATURATION);
  color.g = (int)(g * COLOR_SATURATION);
  color.b = (int)(b * COLOR_SATURATION);
  color.w = 0;

  return color;
}
*/
/*
rgbwcolor ColorConverter::xy(int bri, float x, float y) {
    rgbwcolor color(0, 0, 0, 0);

    float z = 1.0 - x - y;

    float Y = bri / 255.0;
    float X = (Y / y) * x;
    float Z = (Y / y) * z;

    float r = X * 1.612 - Y * 0.203 - Z * 0.302;
    float g = -X * 0.509 + Y * 1.412 + Z * 0.066;
    float b = X * 0.026 - Y * 0.072 + Z * 0.962;

    r = r <= 0.0031308 ? 12.92 * r : (1.0 + 0.055) * pow(r, (1.0 / 2.4)) - 0.055;
    g = g <= 0.0031308 ? 12.92 * g : (1.0 + 0.055) * pow(g, (1.0 / 2.4)) - 0.055;
    b = b <= 0.0031308 ? 12.92 * b : (1.0 + 0.055) * pow(b, (1.0 / 2.4)) - 0.055;

  float maxValue = max(r, g);
        maxValue = max(maxValue, b);

    r /= maxValue;
    g /= maxValue;
    b /= maxValue;

    r = r * 255;
    if (r < 0) {
      r = 255;
    }

    g = g * 255;
    if (g < 0) {
      g = 255;
    }

    b = b * 255;
    if (b < 0) {
      b = 255;
    }

    color.r = (int)(r * 255.0f);
    color.g = (int)(g * 255.0f);
    color.b = (int)(b * 255.0f);
    color.w = 0;

    return color;
}
*/
rgbwcolor ColorConverter::ct(int bri, int ct) {
  rgbwcolor color(0, 0, 0, 0);
  int hectemp = 10000 / ct;
  int r, g, b;

  if (hectemp <= 66) {
    r = 255;
    g = 99.4708025861 * log(hectemp) - 161.1195681661;
    b = hectemp <= 19 ? 0 : (138.5177312231 * log(hectemp - 10) - 305.0447927307);
  } else {
    r = 329.698727446 * pow(hectemp - 60, -0.1332047592);
    g = 288.1221695283 * pow(hectemp - 60, -0.0755148492);
    b = 255;
  }

  r = r > 255 ? 255 : r;
  g = g > 255 ? 255 : g;
  b = b > 255 ? 255 : b;

  color.r = r * (bri / 255.0f);
  color.g = g * (bri / 255.0f);
  color.b = b * (bri / 255.0f);
  color.w = bri;

  return color;
}

int ColorConverter::getHue(hsvcolor hsb) {
  return hsb.h * 360 * 182.04;
}

int ColorConverter::getSaturation(hsvcolor hsb) {
  return hsb.s * COLOR_SATURATION;
}

#endif
