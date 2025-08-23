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

// Beat detection for electronic music
const int BEAT_THRESHOLD = 80; // Much higher threshold
const int FADE_SPEED = 25; // How fast LEDs fade
unsigned long lastBeatTime = 0;
const unsigned long MIN_BEAT_INTERVAL = 150; // Longer minimum interval

// Music detection
const int MUSIC_THRESHOLD = 20; // Lower threshold for background music
bool musicPlaying = false;
int backgroundBrightness = 0;

// Volume tracking
float average = 0;
int volumes[10] = {0}; // Shorter history for faster response
int volumeIndex = 0;
int lastVolume = 0;

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
    // Calculate current volume
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    int volume = sum / samplesRead;
    
    // Update rolling average (shorter window for faster response)
    for(int i = 9; i > 0; i--) {
      volumes[i] = volumes[i - 1];
    }
    volumes[0] = volume;
    
    // Calculate average of last 10 samples
    float avgSum = 0;
    for(int i = 0; i < 10; i++) {
      avgSum += volumes[i];
    }
    average = avgSum / 10;
    
    // Music detection (background lighting)
    if (volume > MUSIC_THRESHOLD) {
      musicPlaying = true;
      if (backgroundBrightness < 50) backgroundBrightness = 50; // Set background brightness
    } else {
      musicPlaying = false;
      if (backgroundBrightness > 0) backgroundBrightness -= 5; // Fade out background
      if (backgroundBrightness < 0) backgroundBrightness = 0;
    }
    
    // Beat detection: much more selective for bass beats
    bool volumeSpike = volume > (average * 2.0); // 100% above average (much higher)
    bool volumeJump = (volume - lastVolume) > 40; // Much larger sudden increase
    bool enoughTime = (millis() - lastBeatTime) > MIN_BEAT_INTERVAL;
    
    if ((volumeSpike || volumeJump) && enoughTime && musicPlaying) {
      // Beat detected! Flash with fade effect
      currentBrightness = 255; // Full brightness
      lastBeatTime = millis();
      
      Serial.print("BEAT! Background: ");
      Serial.print(backgroundBrightness);
      Serial.print(" Beat: ");
      Serial.println(currentBrightness);
    }
    
    lastVolume = volume;
    samplesRead = 0;
  }
  
  // Fade effect for beat flash - runs continuously
  if (currentBrightness > 0) {
    currentBrightness -= FADE_SPEED;
    if (currentBrightness < 0) currentBrightness = 0;
  }
  
  // Set LED brightness - background + beat flash
  for(int i = 0; i < NUM_LEDS; i++) {
    int totalBrightness = backgroundBrightness + currentBrightness;
    if (totalBrightness > 255) totalBrightness = 255; // Cap at max brightness
    
    if (totalBrightness > 0) {
      strip.SetPixelColor(i, RgbColor(totalBrightness, 0, 0)); // Red with background + beat
    } else {
      strip.SetPixelColor(i, RgbColor(0, 0, 0)); // Off when no brightness
    }
  }
  strip.Show();
  
  delay(10); // Faster for more responsive fade
}