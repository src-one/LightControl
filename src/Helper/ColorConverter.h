#ifndef HUECONVERT_h
#define HUECONVERT_h

#include <Arduino.h>
#include <functional>

#define COLOR_SATURATION 255.0f

#define COLOR_MODE_XY 1
#define COLOR_MODE_CT 2
#define COLOR_MODE_HUE 3

#define MIN(a,b) ((a<b)?a:b)
#define MAX(a,b) ((a>b)?a:b)

struct rgbwcolor {
  rgbwcolor(uint8_t r, uint8_t g, uint8_t b, uint8_t w) : r(r), g(g), b(b), w(w) {};
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t w;
};

struct hsvcolor {
  hsvcolor(const rgbwcolor& color) {
    float r = ((float)color.r)/COLOR_SATURATION;
    float g = ((float)color.g)/COLOR_SATURATION;
    float b = ((float)color.b)/COLOR_SATURATION;
    float mi = MIN(MIN(r,g),b);
    float ma = MAX(MAX(r,g),b);
    float diff = ma - mi;

    v = ma;
    h = 0;
    s = (!v)?0:(diff/ma);

    if (diff) {
      if (r == v) {
            h = (g - b) / diff + (g < b ? 6.0f : 0.0f);
        } else if (g == v) {
            h = (b - r) / diff + 2.0f;
        } else {
            h = (r - g) / diff + 4.0f;
        }
        h /= 6.0f;
    }
  };

  float h;
  float s;
  float v;
};

class ColorConverter;

class ColorConverter
{
  public:
    ColorConverter();

    rgbwcolor hue(int bri, int hue, int sat);
    rgbwcolor xy(int bri, float x, float y);
    rgbwcolor ct(int bri, int ct);

    int getHue(hsvcolor hsb);
    int getSaturation(hsvcolor hsb);
};

#endif
