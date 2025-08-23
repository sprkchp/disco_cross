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

// LED state
bool ledsOn = false;

// Clap detection
unsigned long lastClapTime = 0;
const unsigned long CLAP_COOLDOWN = 500; // 500ms between claps
const int CLAP_THRESHOLD = 1000; // Adjust this value based on your environment

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
    
    // Check for clap
    if (averageVolume > CLAP_THRESHOLD && millis() - lastClapTime > CLAP_COOLDOWN) {
      ledsOn = !ledsOn; // Toggle LED state
      lastClapTime = millis();
      
      if (ledsOn) {
        // Turn on all LEDs to dim red
        for(int i = 0; i < NUM_LEDS; i++) {
          strip.SetPixelColor(i, RgbColor(50, 0, 0)); // Dim red
        }
      } else {
        // Turn off all LEDs
        for(int i = 0; i < NUM_LEDS; i++) {
          strip.SetPixelColor(i, RgbColor(0, 0, 0));
        }
      }
      strip.Show();
      
      Serial.print("Clap detected! Volume: ");
      Serial.print(averageVolume);
      Serial.print(" LEDs: ");
      Serial.println(ledsOn ? "ON" : "OFF");
    }
    
    samplesRead = 0;
  }
  
  delay(10); // Small delay
}