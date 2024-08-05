#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "math.h"

#define PIN 5         // Definiere den Pin, an dem der Rechteckton ausgegeben wird

#define LFO_FREQ 10    // Frequenz des LFO in Hertz (0.5 Hz = 2 Sekunden pro Zyklus)
#define LFO_DEPTH 100   // Tiefe des LFO 

// Konstanten für den minimalen und maximalen Wert der Frequenz
const float minVal = 20.0;
const float maxVal = 8000.0;

const int freqPotPin = A0;

const int controlPin = 2;  // Steuerpin für die Tonaktivierung
bool startButton = 1;


// Messwerte runden
const int numReadings = 50;
int pitchReadings[numReadings];
int pitchReadIndex = 0;
int pitchTotal = 0;
//int pitch_average = 0;
int valPitch = 0;

void setup() {
  // Initialisiere die GPIO-Pin-Funktion für PWM
  gpio_set_function(PIN, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(PIN);

  // Setze den PWM-Teilungsverhältnis
  pwm_set_clkdiv(slice_num, 64.f);

  // Starte den PWM-Output
  pwm_set_enabled(slice_num, true); 

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(controlPin, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand

  Serial.begin(115200);
  while (!Serial) {   }

  Serial.println("Setup abgeschlossen.");
  digitalWrite(LED_BUILTIN, HIGH);

  for (int i = 0; i < numReadings; i++) {
    pitchReadings[i] = 0;
  }
}

// Funktion zur Umwandlung einer linearen Eingangsgröße in eine logarithmische Ausgabe
float linearToLogarithmic(float percentage) {

  
  // Überprüfen, ob die Eingabewerte sinnvoll sind
  if (percentage < 0.0) percentage = 0.0;
  if (percentage > 100.0) percentage = 100.0;
  
  // Skaliere den Prozentwert auf den Bereich [0, 1]
  float scaledPercentage = percentage / 100.0;
  
  // Berechne die logarithmische Ausgabe
  // Wir nehmen an, dass der logarithmische Bereich Basis 10 verwendet
  float logMin = log10(minVal);
  float logMax = log10(maxVal);
  
  // Interpolieren im logarithmischen Raum
  float logValue = logMin + scaledPercentage * (logMax - logMin);
  
  // Rückkonvertieren in den linearen Raum
  float outputValue = pow(10, logValue);
  
  return outputValue;
}

float calculateLFO(float baseFreq, float lfoFreq, float lfoDepth) {
  // Berechne die aktuelle Zeit in Sekunden
  float time = millis() / 1000.0;

  // Berechne die aktuelle LFO-Phase
  float lfoPhase = time * lfoFreq * 2 * PI;

  // Berechne die modulierte Frequenz
  return baseFreq + lfoDepth * sin(lfoPhase);
}

float setFrequency(float freq){
  return(2070 / (freq/1000));
}

void pitchAverage(){
  pitchTotal = pitchTotal - pitchReadings[pitchReadIndex];
  pitchReadings[pitchReadIndex] = analogRead(freqPotPin);
  pitchTotal = pitchTotal + pitchReadings[pitchReadIndex];
  pitchReadIndex = pitchReadIndex + 1;

  if (pitchReadIndex >= numReadings) {
    pitchReadIndex = 0;
  }

  valPitch = pitchTotal / numReadings;
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void loop() {
  startButton = !digitalRead(controlPin);
  // valPitch = analogRead(freqPotPin);
  pitchAverage();

  

  //Serial.println(valPitch);
  
  // int baseFrequency = map(valPitch, 0, 1023, 50, 4000); // direkte Ausgabe der Frequenz
  
  // Serial.println(mapFloat(valPitch, 0, 1023, 0, 100));
  int baseFrequency = linearToLogarithmic( mapFloat(valPitch, 0, 1023, 0, 100) );
  
  // Berechne die modulierte Frequenz
  float modulatedFreq = calculateLFO(baseFrequency, LFO_FREQ, LFO_DEPTH);

  // Berechne den PWM-Wert für die modulierte Frequenz


  
  // float pwm_val = setFrequency(baseFrequency); // 4140 = 500hz; 2070 = 1khz; 1035 = 2khz; 
  float pwm_val = setFrequency(modulatedFreq);
  // Setze die PWM-Periode
  // Serial.println(pwm_val);

  
  uint slice_num = pwm_gpio_to_slice_num(PIN);

  
  pwm_set_wrap(slice_num, pwm_val);
  if (!startButton) {
    
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(PIN), 0);
  }else{
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(PIN), pwm_val / 2);
  }

  delay(1);  // Kurze Pause, um den Ton glatter zu machen
}
