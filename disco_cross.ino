#include <NeoPixelBus.h>
#include <PDM.h>

#define LED_PIN     5
#define NUM_LEDS    16
#define SAMPLE_RATE 16000
#define SAMPLE_BUFFER_SIZE 512

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUM_LEDS, LED_PIN);

// Audio buffer
short sampleBuffer[SAMPLE_BUFFER_SIZE];
volatile int samplesRead;

// Music reactivity
const int BEAT_THRESHOLD = 50; // Much more sensitive for music
const int FADE_SPEED = 30; // Faster fade for snappier response
unsigned long lastBeatTime = 0;
const unsigned long MIN_BEAT_INTERVAL = 30; // Even faster response

// Bass detection
int lastVolume = 0;
const int VOLUME_CHANGE_THRESHOLD = 20; // Detect sudden volume changes
const int BASS_FREQ_MIN = 20;  // Minimum bass frequency (Hz)
const int BASS_FREQ_MAX = 200; // Maximum bass frequency (Hz)

// Current color for all LEDs
int currentRed = 0;
int currentGreen = 0;
int currentBlue = 0;
int currentBrightness = 0;

// Color array for the 5 basic colors
int colors[5][3] = {
  {255, 0, 0},    // Red
  {0, 255, 0},    // Green
  {0, 0, 255},    // Blue
  {255, 0, 255},  // Purple
  {255, 255, 0}   // Yellow
};

int currentColorIndex = 0;
int beatCount = 0;
int beatsUntilColorChange = 8; // Start with 8 beats

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}

void setup() {
  // Initialize LED strip
  strip.Begin();
  strip.Show();
  
  // Initialize PDM microphone
  PDM.onReceive(onPDMdata);
  PDM.begin(1, SAMPLE_RATE);
  
  // Initialize LED state
  currentBrightness = 0;
  
  Serial.begin(9600);
}

void loop() {
      if (samplesRead > 0) {
      // Calculate bass energy using simple low-pass filter
      long bassSum = 0;
      int bassCount = 0;
      
      for (int i = 0; i < samplesRead; i++) {
        // Simple low-pass filter to emphasize bass frequencies
        // This approximates bass detection by averaging samples
        if (i % 4 == 0) { // Take every 4th sample to focus on lower frequencies
          bassSum += abs(sampleBuffer[i]);
          bassCount++;
        }
      }
      
      int bassEnergy = bassCount > 0 ? bassSum / bassCount : 0;
      
      // Check for bass beat - detect both high bass energy and sudden changes
      bool volumeBeat = bassEnergy > BEAT_THRESHOLD;
      bool changeBeat = abs(bassEnergy - lastVolume) > VOLUME_CHANGE_THRESHOLD;
    
    // Check for beat
    if ((volumeBeat || changeBeat) && millis() - lastBeatTime > MIN_BEAT_INTERVAL) {
      // Beat detected! Flash all LEDs with current color
      currentRed = colors[currentColorIndex][0];
      currentGreen = colors[currentColorIndex][1];
      currentBlue = colors[currentColorIndex][2];
      currentBrightness = 255; // Full brightness
      
      beatCount++;
      
      // Check if it's time to change color (independent of flash)
      if (beatCount >= beatsUntilColorChange) {
        beatCount = 0;
        currentColorIndex = (currentColorIndex + 1) % 5;
        // Randomize next color change interval (between 4 and 12 beats)
        beatsUntilColorChange = random(4, 13);
        Serial.print("Color changed to: ");
        Serial.println(currentColorIndex);
      }
      
      lastBeatTime = millis();
      
      Serial.print("Bass! Energy: ");
      Serial.print(bassEnergy);
      Serial.print(" Color: ");
      Serial.print(currentColorIndex);
      Serial.print(" Beat count: ");
      Serial.print(beatCount);
      Serial.print("/");
      Serial.println(beatsUntilColorChange);
    }
    
          lastVolume = bassEnergy;
    
    // Fade brightness
    if (currentBrightness > 0) {
      currentBrightness -= FADE_SPEED;
      if (currentBrightness < 0) currentBrightness = 0;
    }
    
    // Set all LEDs to same color with fade effect
    int red = (currentRed * currentBrightness) / 255;
    int green = (currentGreen * currentBrightness) / 255;
    int blue = (currentBlue * currentBrightness) / 255;
    
    for(int i = 0; i < NUM_LEDS; i++) {
      strip.SetPixelColor(i, RgbColor(red, green, blue));
    }
    
    strip.Show();
    samplesRead = 0;
  }
  
  delay(20); // Slightly longer delay for smoother animation
}