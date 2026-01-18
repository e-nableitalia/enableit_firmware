#pragma once

#include <Arduino.h>

#ifdef ARDUINO_M5STACK_ATOMS3

#include <M5GFX.h>
#include <Display.h>

namespace enableit {

class M5Display : public Display, public m5gfx::M5GFX {
public:
   M5Display();

   void begin() override;
   void end() override {} // No specific end for M5GFX

   void clear() override { fillScreen(Color::BLACK); }
   void setTextSize(int size) override { m5gfx::M5GFX::setTextSize(size); }
   void setTextColor(Color color) override { m5gfx::M5GFX::setTextColor(toM5Color(color)); }
   void setCursor(int x, int y) override { m5gfx::M5GFX::setCursor(x, y); }
   void fillScreen(Color color) override { m5gfx::M5GFX::fillScreen(toM5Color(color)); }
   void fillRect(int x, int y, int w, int h, Color color) override { m5gfx::M5GFX::fillRect(x, y, w, h, toM5Color(color)); }
   void print(const char *text) override { m5gfx::M5GFX::print(text); }
   void print(const String &text) override { m5gfx::M5GFX::print(text); }
   void println(const char *text) override { m5gfx::M5GFX::println(text); }
   void println(const String &text) override { m5gfx::M5GFX::println(text); }
   void print(const int value) override { m5gfx::M5GFX::print(value); }
   void println(const int value) override { m5gfx::M5GFX::println(value); }

   DisplayProps getProps() const override {
      return DisplayProps{width(), height(), 8, 21}; // Example values, adjust as needed
   }

   void setTitle(String title) override {
      setCursor(0, 0);
      print(title);
   }
   // Optionally implement setLine to display a line of text at a specific line number
   void setLine(int line, String text) override {
      int lineHeight = 16; // Adjust based on font size
      setCursor(0, line * lineHeight);
      fillRect(0, line * lineHeight, width(), lineHeight, Color::BLACK); // Clear the line
      print(text);
   }
   
   void refresh() override {}

private:
   static uint32_t toM5Color(Color color) {
      switch (color) {
         case Color::BLACK:   return TFT_BLACK;
         case Color::WHITE:   return TFT_WHITE;
         case Color::RED:     return TFT_RED;
         case Color::GREEN:   return TFT_GREEN;
         case Color::BLUE:    return TFT_BLUE;
         case Color::YELLOW:  return TFT_YELLOW;
         case Color::CYAN:    return TFT_CYAN;
         case Color::MAGENTA: return TFT_MAGENTA;
         case Color::GRAY:    return 0x8410; // No direct TFT_ constant
         default:             return TFT_BLACK;
      }
   }
};

} // namespace enableit

#endif
// End of file M5Display.h