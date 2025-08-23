#include <NeoPixelBus.h>

#define LED_PIN     5
#define NUM_LEDS    16

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUM_LEDS, LED_PIN);

void setup() {
  strip.Begin();
  strip.Show();
  
  // Turn on all LEDs to bright red
  // for(int i = 0; i < NUM_LEDS; i++) {
  //   strip.SetPixelColor(i, RgbColor(255, 0, 0));
  // }
  // strip.Show();
}

void loop() {
  // Random color animation - adapted from your Adafruit code
  int randomLED = random(NUM_LEDS);
  int randomR = random(255);
  int randomG = random(255);
  int randomB = random(255);
  
  strip.SetPixelColor(randomLED, RgbColor(randomR, randomG, randomB));
  strip.Show();
  
  // Use a shorter delay to avoid timing issues
  delay(100);
}