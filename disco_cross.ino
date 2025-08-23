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

// Audio processing for better beat detection
int lastVolume = 0;
const int VOLUME_CHANGE_THRESHOLD = 20; // Detect sudden volume changes

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
    // Calculate average volume
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    int averageVolume = sum / samplesRead;
    
    // Check for beat - detect both high volume and sudden volume changes
    bool volumeBeat = averageVolume > BEAT_THRESHOLD;
    bool changeBeat = abs(averageVolume - lastVolume) > VOLUME_CHANGE_THRESHOLD;
    
    if ((volumeBeat || changeBeat) && millis() - lastBeatTime > MIN_BEAT_INTERVAL) {
      // Beat detected! Flash all LEDs with current color
      currentRed = colors[currentColorIndex][0];
      currentGreen = colors[currentColorIndex][1];
      currentBlue = colors[currentColorIndex][2];
      currentBrightness = 255; // Full brightness
      
      beatCount++;
      
      // Shuffle colors every 8 beats
      if (beatCount >= 8) {
        // Shuffle the color array
        for (int i = 0; i < 5; i++) {
          int randomIndex = random(5);
          // Swap colors[i] with colors[randomIndex]
          int tempR = colors[i][0];
          int tempG = colors[i][1];
          int tempB = colors[i][2];
          colors[i][0] = colors[randomIndex][0];
          colors[i][1] = colors[randomIndex][1];
          colors[i][2] = colors[randomIndex][2];
          colors[randomIndex][0] = tempR;
          colors[randomIndex][1] = tempG;
          colors[randomIndex][2] = tempB;
        }
        beatCount = 0;
        currentColorIndex = 0;
      } else {
        // Move to next color
        currentColorIndex = (currentColorIndex + 1) % 5;
      }
      
      lastBeatTime = millis();
      
      Serial.print("Beat! Color: ");
      Serial.print(currentColorIndex);
      Serial.print(" Beat count: ");
      Serial.println(beatCount);
    }
    
    lastVolume = averageVolume;
    
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