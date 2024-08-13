#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "math.h"

#define PIN 5         // Definiere den Pin, an dem der Rechteckton ausgegeben wird


const int LOGLEVEL = 1;

const int freqPotPin = A0;
const int lfoFreqPotPin = A1;  // Potentiometer für die Frequenz des LFO
const int lfoAmpPotPin = A2;   // Potentiometer für die Amplitude des LFO

const int waveFormPin_0 = 3;
const int waveFormPin_1 = 4;


const int controlPin = 2;  // Steuerpin für die Tonaktivierung

// Konstanten für den minimalen und maximalen Wert der Frequenz
const float minVal = 50.0;
const float maxVal = 8000.0;

unsigned int lfoFrequency = 1;  // LFO-Frequenz in Hz (0.5 Hz = 2 Sekunden Periode)
float lfoAmplitude = 0;
bool startButton = 1;

bool oldval;

enum LfoWaveform { SQUARE, TRIANGLE, SAWTOOTH };
LfoWaveform lfoWaveform = TRIANGLE;  // Gewünschte LFO-Wellenform: SQUARE, TRIANGLE, SAWTOOTH

// Variablen für den LFO

volatile float lfoValue = 0;
volatile int lfoDirection = 1;  // Richtung des LFO: 1 aufwärts, -1 abwärts
volatile unsigned long previousMillis = 0;
// float modulatedFrequency = baseFrequency;


// Messwerte runden
const int numReadings = 10;
int pitchReadings[numReadings];
int pitchReadIndex = 0;
int pitchTotal = 0;
int valPitch = 0;

void debugFloat(float f){
  if (LOGLEVEL > 0){

    
    char buffer[10];  // Puffer für die Zeichenkette, stelle sicher, dass er groß genug ist
    dtostrf(f, 6, 4, buffer);  // 6 Zeichen insgesamt, 4 Dezimalstellen
    String myString = String(buffer);


    Serial.println(myString);
  }
}

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
  pinMode(waveFormPin_0, INPUT_PULLUP);
  pinMode(waveFormPin_1, INPUT_PULLUP);
  
  
  digitalWrite(LED_BUILTIN, HIGH);

  for (int i = 0; i < numReadings; i++) {
    pitchReadings[i] = 0;
  }
  if(LOGLEVEL > 0){
    Serial.begin(115200);
    //debug("Setup abgeschlossen.");
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

// LFO-Variante aus dem Arduino-Nano-Script
float calculateLFOWave(float lfoFrequency, float lfoAmplitude) {

  float schrittweite = lfoFrequency / 1000;
  //float schrittweite = 0.01;
  unsigned long currentMillis = millis();

  //float lfoPeriod = 5000 / lfoFrequency;  // LFO-Periode in Millisekunden
  float lfoPeriod = 5000 / 50;  // LFO-Periode in Millisekunden
  //debugFloat(schrittweite);
  static float triggeredLfoValue = 0;

  
  if (currentMillis - previousMillis >= lfoPeriod / 100.0) {
    previousMillis += lfoPeriod / 100.0;
    switch (lfoWaveform) {
      case SQUARE:
        
        
        lfoValue += schrittweite * lfoDirection;  // Schrittweite für den LFO (kann angepasst werden)
        if (lfoValue >= 1.0 || lfoValue <= 0.0) {
          lfoDirection = -lfoDirection;  // Richtung umkehren
        }
        triggeredLfoValue = (lfoValue >= 0.5) ? 1 : 0;
       
        break;
      case TRIANGLE:
        lfoValue += schrittweite * lfoDirection;  // Schrittweite für den LFO (kann angepasst werden)
        if (lfoValue >= 1.0 || lfoValue <= 0.0) {
          lfoDirection = -lfoDirection;  // Richtung umkehren
        }
        break;
      case SAWTOOTH:
        lfoValue += schrittweite / 2;  // Schrittweite für den LFO (kann angepasst werden)
        if (lfoValue >= 1.0) {
          lfoValue = 0;  // Zurücksetzen
        }
        break;
    }
  }

  float lfovalue_final = 0;
  // Skalierung des LFO-Wertes
  if(lfoWaveform != SQUARE){
    lfovalue_final = 1.0 + (lfoValue * (lfoAmplitude/100));
  }else{
    lfovalue_final = 1.0 + (triggeredLfoValue * (lfoAmplitude/100));
  }
  //debug(String(lfovalue_final));
  return lfovalue_final;
}

// LFO-Variante Sinuns
float calculateLFO(float baseFreq, float lfoFreq, float lfoDepth) {
  // Berechne die aktuelle Zeit in Sekunden
  float time = millis() / 1000.0;

  // Berechne die aktuelle LFO-Phase
  float lfoPhase = time * lfoFreq * 2 * PI;
  
  // debug("LFO-Phase: "+String(lfoPhase));

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

  //taster_val = digitalRead(inputPin);           // Wert auslesen
  if (startButton != oldval){
    // normale Bearbeitung: losgelassen / gedrückt etc...
    oldval = startButton;
   delay(10);              // 10 millisekunden warten, eine Änderung danach wird wieder eine echte sein.
  }

  // Reset LFO values if the start button is false
  if (!startButton) {
    lfoValue = 0;
    lfoDirection = 1;
    previousMillis = millis();
  }
  
  //valPitch = analogRead(freqPotPin);
  pitchAverage();
  //debug(String(valPitch));

  int lfoFreqValue = analogRead(lfoFreqPotPin);
  int lfoAmpValue = analogRead(lfoAmpPotPin);

  //lfoFrequency = mapFloat(lfoFreqValue, 5, 1023, 1, 50);   // LFO-Frequenzbereich von 1 Hz bis 30 Hz

  lfoFrequency = map(lfoFreqValue, 5, 1023, 1, 50);   // LFO-Frequenzbereich von 1 Hz bis 30 Hz


  lfoAmplitude = mapFloat(lfoAmpValue, 5, 1023, -100, 100);   // LFO-Amplitudenbereich von 0 bis 100

  if (( digitalRead(waveFormPin_0) == 0 )&&( digitalRead(waveFormPin_1) == 1 )){
    lfoWaveform = TRIANGLE;
  } 
  if(( digitalRead(waveFormPin_0) == 1 )&&( digitalRead(waveFormPin_1) == 1 )){
    lfoWaveform = SQUARE;
  }
  if ((digitalRead(waveFormPin_0) == 1 )&&( digitalRead(waveFormPin_1) == 0)){
    lfoWaveform = SAWTOOTH;
  }

  // debug("Waveform:"+String(lfoWaveform));

  // debug("LFO1: "+ String(lfoFreqValue)+" LFO1 Amp: "+ String(lfoAmpValue));
  // int baseFrequency = map(valPitch, 0, 1023, 50, 4000); // direkte Ausgabe der Frequenz
  
  // Serial.println(mapFloat(valPitch, 0, 1023, 0, 100));
  //int baseFrequency =  mapFloat(valPitch, 0, 1023, 0, 100) ;
  int baseFrequency =  mapFloat(valPitch, 0, 1023, 20, 8000);
  
  // Berechne die modulierte Frequenz
  // float modulatedFreq = (calculateLFO(baseFrequency, lfoFrequency, lfoAmpValue));
  
  // Modulierte Frequenz berechnen
   float newModulatedFrequency = baseFrequency * calculateLFOWave(lfoFrequency, lfoAmplitude);
  //debugFloat(newModulatedFrequency);
  
  // Berechne den PWM-Wert für die modulierte Frequenz


  
  // float pwm_val = setFrequency(baseFrequency); // 4140 = 500hz; 2070 = 1khz; 1035 = 2khz; 
  float pwm_val = setFrequency(newModulatedFrequency);
  
  
  // Setze die PWM-Periode
  // Serial.println(pwm_val);

  
  uint slice_num = pwm_gpio_to_slice_num(PIN);

  
  pwm_set_wrap(slice_num, pwm_val);
  if (!startButton) {
    
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(PIN), 0);
  }else{
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(PIN), pwm_val / 2);
  }

  //delay(1);  // Kurze Pause, um den Ton glatter zu machen
}
