#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "math.h"
        

const int LOGLEVEL = 1;

const int freqPotPin = A0;
const int lfoFreqPotPin = A1;  // Potentiometer für die Frequenz des LFO
const int lfoAmpPotPin = A2;   // Potentiometer für die Amplitude des LFO

const int waveFormPin_0 = 3; // WaveForm Kippschalter PIN 1
const int waveFormPin_1 = 4; // WaveForm Kippschalter Pin 2
const int waveFormFunctionPin = 6;  // WAVEFORM Funktion

const int wave_output = 5; // Pin, an dem der Rechteckton ausgegeben wird
const int controlPin = 2;  // Steuerpin für die Tonaktivierung

const int firePin1 = 18;  // Steuerpin für die Tonaktivierung
const int firePin2 = 19;  // Steuerpin für die Tonaktivierung
const int firePin3 = 20;  // Steuerpin für die Tonaktivierung
const int firePin4 = 21;  // Steuerpin für die Tonaktivierung





// Konstanten für den minimalen und maximalen Wert der blanken Frequenz ohne Modulation
const float minVal = 20;
const float maxVal = 4000;
// minimaler und maximaler Wert der Frequenz nach der LFO-Modulation
const float minValMod = 5;
const float maxValMod = 14000;

float lfoFrequency = 1;  // LFO-Frequenz in Hz (0.5 Hz = 2 Sekunden Periode)
float lfoAmplitude = 0;
bool startButton = 1;

bool startButton_oldval;

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

///////////////////////////////////

void setup() {
  // Initialisiere die GPIO-Pin-Funktion für PWM Wave-Output
  gpio_set_function(wave_output, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(wave_output);

  // Setze den PWM-Teilungsverhältnis
  pwm_set_clkdiv(slice_num, 64.f);

  // Starte den PWM-Output
  pwm_set_enabled(slice_num, true); 

  // Normale IO's
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(controlPin, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand
  
  pinMode(firePin1, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand
  pinMode(firePin2, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand
  pinMode(firePin3, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand
  pinMode(firePin4, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand
  
  pinMode(waveFormPin_0, INPUT_PULLUP);
  pinMode(waveFormPin_1, INPUT_PULLUP);
  pinMode(waveFormFunctionPin, INPUT_PULLUP);
  
  // Initialisieren des Arrays für die Mittelwertbildung des Pitch
  for (int i = 0; i < numReadings; i++) {
    pitchReadings[i] = 0;
  }
  if(LOGLEVEL > 0){
    Serial.begin(115200);
    //debug("Setup abgeschlossen.");
  }
  // Lampe an, nur zur Kontrolle, dass die SW läuft
  digitalWrite(LED_BUILTIN, HIGH);
}

///////////// allgem. Funktionen

void debugStr(String s){
  if (LOGLEVEL > 0){
    Serial.println(s);
  }
}

// Serielle Ausgabe eines Float oder int, mit mehr Zeichen als die Standardmethode
void debugFloat(float f){
  if (LOGLEVEL > 0){
    char buffer[10];  // Puffer für die Zeichenkette, stelle sicher, dass er groß genug ist
    dtostrf(f, 6, 4, buffer);  // 6 Zeichen insgesamt, 4 Dezimalstellen
    String myString = String(buffer);
    Serial.println(myString);
  }
}

// Funktion zur Umwandlung einer linearen Eingangsgröße in eine logarithmische Ausgabe
float linearToLogarithmic(float freq) {
  
  
  float percentage = (freq - minVal ) / ((maxVal - minVal)/100);
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
  // debugStr(String(freq)+';'+String(percentage)+';'+String(outputValue));
  return outputValue;
}

// LFO-Variante mit Dreieck, abgeleiteten Rechteck und Sägezahn
float calculateLFOWave(float lfoFrequency, float lfoAmplitude, bool waveFormFunction) {

  float schrittweite = lfoFrequency / 1000;
  unsigned long currentMillis = millis();

  const int lfoPeriod = 100;  // LFO-Periode in Millisekunden (5000 / 50)
  //debugFloat(schrittweite);
  static float triggeredLfoValue = 0;
  static float lfovalue_final = 0;

  if (currentMillis - previousMillis >= lfoPeriod / 100.0) {
    previousMillis += lfoPeriod / 100.0;
    switch (lfoWaveform) {
      case SQUARE:
        lfoValue += schrittweite * lfoDirection;  // 
        if (lfoValue >= 1.0 || lfoValue <= 0.0) {
          lfoDirection = -lfoDirection;  // Richtung umkehren
        }
        triggeredLfoValue = (lfoValue >= 0.5) ? 1 : 0;
        lfovalue_final = triggeredLfoValue;
        break;
        
      case TRIANGLE:
        lfoValue += schrittweite * lfoDirection;  // 
        if (lfoValue >= 1.0 || lfoValue <= 0.0) {
          lfoDirection = -lfoDirection;  // Richtung umkehren
        }
        //lfovalue_final = lfoValue;
        lfovalue_final = lfoValue;
        break;
        
      case SAWTOOTH:
        if(waveFormFunction == 0){
          lfoValue += schrittweite / 2;  // Schrittweite für den LFO (kann angepasst werden)
          if (lfoValue >= 1.0) {
            lfoValue = 0;  // Zurücksetzen
          }
        }else{
          lfoValue -= schrittweite / 2;  // Schrittweite für den LFO (kann angepasst werden)
          if (lfoValue <= 0) {
            lfoValue = 1;  // Zurücksetzen
          }
        }
        lfovalue_final = lfoValue;
        break;
    }
  }

  // lfovalue_final liegt zwischen 0..1: verschieben der Mitte auf den NullPunkt
  // Skalierung des LFO-Wertes mit der LFO-Amplitude 
  // verschieben des Nullpunktes auf 1: damit kann der Wert als Multiplikator genutzt werden

  float lfovalue_final1 = (lfovalue_final -0.5) * (lfoAmplitude/100) + 1.0;
  
  // debugFloat(lfovalue_final1);
  return(lfovalue_final1);
}

// LFO-Variante Sinus 
float calculateLFOSin(float baseFreq, float lfoFreq, float lfoDepth) {
  // Berechne die aktuelle Zeit in Sekunden
  float time = millis() / 1000.0;

  // Berechne die aktuelle LFO-Phase
  float lfoPhase = time * lfoFreq * 2 * PI;
  
  // debug("LFO-Phase: "+String(lfoPhase));

  // Berechne die modulierte Frequenz
  return baseFreq + lfoDepth * sin(lfoPhase);
}

float setFrequency(float freq){
  //debugFloat(freq);
  if(freq < minValMod){freq = minValMod;}
  if(freq > maxValMod){freq = maxValMod;}
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

void soundCreate(float freqVal){
  // float pwm_val = setFrequency(baseFrequency); // 4140 = 500hz; 2070 = 1khz; 1035 = 2khz; 
  float pwm_val = setFrequency(linearToLogarithmic(freqVal));
  
  
  // Setze die PWM-Periode
  uint slice_num = pwm_gpio_to_slice_num(wave_output);

  pwm_set_wrap(slice_num, pwm_val);
  if (!startButton) {
    
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(wave_output), 0);
  }else{
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(wave_output), pwm_val / 2);
  }
  //return(true);
}

void loop() {
  startButton = !digitalRead(firePin1);

  //taster_val = digitalRead(inputPin);           // Wert auslesen
  if (startButton != startButton_oldval){
    // normale Bearbeitung: losgelassen / gedrückt etc...
    startButton_oldval = startButton;
   delay(10);              // 10 millisekunden warten, eine Änderung danach wird wieder eine echte sein.
  }

  // Reset LFO values if the start button is false
  if (!startButton) {
    lfoValue = 0;
    lfoDirection = 1;
    previousMillis = millis();
  }
  
  // lesen des Pitch-Wertes vom Poti incl. Mittelwert
  pitchAverage();
  //debug(String(valPitch));

  int lfoFreqValue = analogRead(lfoFreqPotPin);
  int lfoAmpValue = analogRead(lfoAmpPotPin);

  lfoFrequency = mapFloat(lfoFreqValue, 5, 1023, 0.5, 50);   // LFO-Frequenzbereich von 1 Hz bis 50 Hz
  //lfoFrequency = map(lfoFreqValue, 5, 1023, 1, 100);

  //lfoAmplitude = map(lfoAmpValue, 5, 1023, -100, 100);   // LFO-Amplitudenbereich von -100 bis 100, 0 in der Mitte
  lfoAmplitude = map(lfoAmpValue, 5, 1023, 0, 100);   // LFO-Amplitudenbereich von 0 bis 100, 0 am Anschlag
  
  //debugStr(String(lfoFrequency)+';'+String(linearToLogarithmic(lfoFrequency)));

  if (( digitalRead(waveFormPin_0) == 0 )&&( digitalRead(waveFormPin_1) == 1 )){
    lfoWaveform = TRIANGLE;
  } 
  if(( digitalRead(waveFormPin_0) == 1 )&&( digitalRead(waveFormPin_1) == 1 )){
    lfoWaveform = SAWTOOTH;
  }
  if ((digitalRead(waveFormPin_0) == 1 )&&( digitalRead(waveFormPin_1) == 0)){
    lfoWaveform = SQUARE;
  }

  bool waveFormFunction = digitalRead(waveFormFunctionPin);

  // debugStr("Waveform:"+String(lfoWaveform)+" Funktion:"+String(waveFormFunction));

  // debug("LFO1: "+ String(lfoFreqValue)+" LFO1 Amp: "+ String(lfoAmpValue));
  // int baseFrequency = map(valPitch, 0, 1023, 50, 4000); // direkte Ausgabe der Frequenz
  
  // Serial.println(mapFloat(valPitch, 0, 1023, 0, 100));
  //int baseFrequency =  mapFloat(valPitch, 0, 1023, 0, 100) ;
  
  int baseFrequency =  mapFloat(valPitch, 4, 1022, minVal, maxVal);
  //baseFrequency = linearToLogarithmic(baseFrequency);
  
  // Berechne die modulierte Frequenz
  // float modulatedFreq = (calculateLFO(baseFrequency, lfoFrequency, lfoAmpValue));
  
  // Modulierte Frequenz berechnen
   float lfoValueActual = calculateLFOWave(lfoFrequency, lfoAmplitude, waveFormFunction);
   float newModulatedFrequency = baseFrequency * lfoValueActual;
   //debugFloat(lfoValue);

  /*
  // Berechne den PWM-Wert für die modulierte Frequenz


  
  // float pwm_val = setFrequency(baseFrequency); // 4140 = 500hz; 2070 = 1khz; 1035 = 2khz; 
  float pwm_val = setFrequency(linearToLogarithmic(newModulatedFrequency));
  
  
  // Setze die PWM-Periode
  // Serial.println(pwm_val);

  
  uint slice_num = pwm_gpio_to_slice_num(wave_output);

  pwm_set_wrap(slice_num, pwm_val);
  if (!startButton) {
    
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(wave_output), 0);
  }else{
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(wave_output), pwm_val / 2);
  }
  */
  // Create Tone
  soundCreate(newModulatedFrequency);
}
