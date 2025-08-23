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
const int BEAT_THRESHOLD = 60; // Lower threshold for electronic music
const int FADE_SPEED = 30; // Slightly faster fade for punchy electronic beats
unsigned long lastBeatTime = 0;
const unsigned long MIN_BEAT_INTERVAL = 100; // Shorter interval for faster electronic music

// Music detection and equalizer-style system
const int MUSIC_THRESHOLD = 20; // Lower threshold for background music
bool musicPlaying = false;
int backgroundBrightness = 0;
int maxBackgroundBrightness = 100; // Maximum background brightness

// Electronic music bass detection (60-120 Hz range)
const int BASS_SAMPLE_RATE = 16000;
const int BASS_FREQ_MIN = 60;  // 60 Hz - kick drum territory
const int BASS_FREQ_MAX = 120; // 120 Hz - upper bass, electronic music focus

// Color management - beat-driven
int currentColorIndex = 0;
int beatCount = 0;
int beatsUntilColorChange = 0; // Random number of beats until next color change

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
    
    // Equalizer-style music detection (overall volume controls brightness)
    if (volume > MUSIC_THRESHOLD) {
      musicPlaying = true;
      // Map volume to background brightness (like an equalizer)
      backgroundBrightness = map(volume, MUSIC_THRESHOLD, 200, 20, maxBackgroundBrightness);
      if (backgroundBrightness > maxBackgroundBrightness) backgroundBrightness = maxBackgroundBrightness;
    } else {
      musicPlaying = false;
      if (backgroundBrightness > 0) backgroundBrightness -= 5; // Fade out background
      if (backgroundBrightness < 0) backgroundBrightness = 0;
    }
    
    // Color change (beat-driven)
    if (beatCount >= beatsUntilColorChange) {
      currentColorIndex = random(5); // Random color from 0-4
      beatCount = 0;
      
      // Set random number of beats until next change (3-8 beats)
      beatsUntilColorChange = random(3, 9);
      
      Serial.print("Color changed to: ");
      Serial.print(currentColorIndex);
      Serial.print(" Next change in: ");
      Serial.print(beatsUntilColorChange);
      Serial.println(" beats");
    }
    
    // Electronic music bass beat detection
    bool volumeSpike = volume > (average * 1.8); // 80% above average (electronic music is punchy)
    bool volumeJump = (volume - lastVolume) > 30; // Sudden increase (electronic bass hits are sharp)
    bool enoughTime = (millis() - lastBeatTime) > MIN_BEAT_INTERVAL;
    
    if ((volumeSpike || volumeJump) && enoughTime && musicPlaying) {
      // Beat detected! Flash with fade effect
      currentBrightness = 255; // Full brightness
      lastBeatTime = millis();
      
      beatCount++; // Increment beat counter for color changes
      
      Serial.print("BEAT! Volume: ");
      Serial.print(volume);
      Serial.print(" Background: ");
      Serial.print(backgroundBrightness);
      Serial.print(" Beat: ");
      Serial.print(currentBrightness);
      Serial.print(" Beat count: ");
      Serial.println(beatCount);
    }
    
    lastVolume = volume;
    samplesRead = 0;
  }
  
  // Fade effect for beat flash - runs continuously
  if (currentBrightness > 0) {
    currentBrightness -= FADE_SPEED;
    if (currentBrightness < 0) currentBrightness = 0;
  }
  
  // Set LED brightness - background + beat flash with colors
  for(int i = 0; i < NUM_LEDS; i++) {
    int totalBrightness = backgroundBrightness + currentBrightness;
    if (totalBrightness > 255) totalBrightness = 255; // Cap at max brightness
    
    if (totalBrightness > 0) {
      // Apply color with brightness
      int red = (colors[currentColorIndex][0] * totalBrightness) / 255;
      int green = (colors[currentColorIndex][1] * totalBrightness) / 255;
      int blue = (colors[currentColorIndex][2] * totalBrightness) / 255;
      
      strip.SetPixelColor(i, RgbColor(red, green, blue));
    } else {
      strip.SetPixelColor(i, RgbColor(0, 0, 0)); // Off when no brightness
    }
  }
  strip.Show();
  
  delay(10); // Faster for more responsive fade
}