#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "math.h"
#include "LittleFS.h"
//#include "ArduinoJson.h"

        

const int LOGLEVEL = 1;
const String CONFIGFILE = "/config.txt";

const int freqPotPin = A0;
const int lfoFreqPotPin = A1;  // Potentiometer für die Frequenz des LFO
const int lfoAmpPotPin = A2;   // Potentiometer für die Amplitude des LFO

const int waveFormPin_0 = 3; // WaveForm Kippschalter PIN 1
const int waveFormPin_1 = 4; // WaveForm Kippschalter Pin 2
const int waveFormFunctionPin = 6;  // WAVEFORM Funktion

const int wave_output = 5; // Pin, an dem der Rechteckton ausgegeben wird
const int controlPin = 2;  // Shift-Taste
const int firePin1 = 18;  // Steuerpin für die Tonaktivierung1
const int firePin2 = 19;  // Steuerpin für die Tonaktivierung2
const int firePin3 = 20;  // Steuerpin für die Tonaktivierung3
const int firePin4 = 21;  // Steuerpin für die Tonaktivierung4

const int LED1_red = 16;
const int LED1_green = 17;




// Konstanten für den minimalen und maximalen Wert der blanken Frequenz ohne Modulation
const float minVal = 35;
const float maxVal = 8000;
// minimaler und maximaler Wert der Frequenz nach der LFO-Modulation
const float minValMod = 20;
const float maxValMod = 10000;

// Parameter Pitch/Tune
int baseFrequency = 1000;

// Parameter LFO:
float lfoFrequency = 1;  // LFO-Frequenz in Hz (0.5 Hz = 2 Sekunden Periode)
float lfoAmplitude = 0;

// Parameter ENVELOPE-Generator:
float envelopeDuration = 1;  // LFO-Frequenz in Hz (0.5 Hz = 2 Sekunden Periode)
float envelopeAmplitude = 0;
float envelopePitch = 0;

enum LfoWaveform { SQUARE, TRIANGLE, SAWTOOTH };
LfoWaveform lfoWaveform = TRIANGLE;  // Gewünschte LFO-Wellenform: SQUARE, TRIANGLE, SAWTOOTH



// globale Variablen, die der LFO und envelope zurückgibt

volatile float lfoValue = 0;
volatile float envelopeValue = 1;
volatile int lfoDirection = 1;  // Richtung des LFO: 1 aufwärts, -1 abwärts
volatile unsigned long previousMillis = 0;
volatile unsigned long previousMillisEnv = 0;


// Messwerte runden
const int numReadings = 10;
int pitchReadings[numReadings];
int pitchReadIndex = 0;
int pitchTotal = 0;
int valPitch = 0;

// Wenn Ton abgefeuert werden soll:
bool runSound = 1;
bool runSound_oldval = 1;

// shift-taste
bool shiftState = false;
// entprellen der shift-taste
bool shift_bak = 1;

// LED1
uint slice_num_led_red = 0;
uint slice_num_led_green = 0;
int pwm_led = 1000;

///////////////////////////////////



void setup() {
  
  
  // Initialisiere die GPIO-Pin-Funktion für PWM Wave-Output
  gpio_set_function(wave_output, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(wave_output);

  // Setze den PWM-Teilungsverhältnis
  pwm_set_clkdiv(slice_num, 64.f);

  // Starte den PWM-Output
  pwm_set_enabled(slice_num, true); 

  // LED
  pinMode(LED1_red, OUTPUT);
  pinMode(LED1_green, OUTPUT);
  
  gpio_set_function(LED1_green, GPIO_FUNC_PWM);
  gpio_set_function(LED1_red, GPIO_FUNC_PWM);
  
  slice_num_led_green = pwm_gpio_to_slice_num(LED1_green);
  slice_num_led_red = pwm_gpio_to_slice_num(LED1_green);
   
  //pwm_set_clkdiv(slice_num_led, 64.f);
  pwm_set_enabled(slice_num_led_green, true);
  pwm_set_enabled(slice_num_led_red, true);
  
  pwm_set_wrap(slice_num_led_green, pwm_led);
  pwm_set_wrap(slice_num_led_red, pwm_led);
  
  pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green), 0);
  pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red), 0);
  


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
    Serial.begin(57600);
    debugStr("Setup abgeschlossen.");
  }

  if (!LittleFS.begin()) {
      debugStr("LittleFS mount failed");
      return;
   }


  // Lampe an, nur zur Kontrolle, dass die SW läuft
  // digitalWrite(LED_BUILTIN, HIGH);
  
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

void readSettings(){
    // Daten lesen
    File file = LittleFS.open(CONFIGFILE, "r");
    if (file) {
        int value = file.read();  // Lese die gespeicherte Zahl
        Serial.print("Gelesener Wert: ");
        Serial.println(value);
        file.close();
    } else {
        Serial.println("Fehler beim Lesen der Datei");
    }
}

void writeSettings(){
  // Daten schreiben
    File file = LittleFS.open(CONFIGFILE, "w");
    if (file) {
        file.write(42);  // Schreibe eine Zahl
        file.close();
        Serial.println("Daten geschrieben");
    } else {
        Serial.println("Fehler beim Öffnen der Datei");
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
  //debugStr(String(freq)+';'+String(percentage)+';'+String(outputValue));
  return outputValue;
}

// LFO-Variante mit Dreieck, abgeleiteten Rechteck und Sägezahn
float calculateLFOWave(float lfoFrequency, float lfoAmplitude, bool waveFormFunction) {

  float schrittweite = lfoFrequency / 1000 ;
  //float schrittweite = lfoFrequency / 1000 * ((envelopeAmplitude / 100) + 1.0);
  //float schrittweite_envelope = 0.001;
  float schrittweite_envelope = envelopeDuration / 10000;
  unsigned long currentMillis = millis();

  const int lfoPeriod = 100;  // LFO-Periode in Millisekunden (5000 / 50)
  const int envelopePeriod = 100;
  
  

  static float lfovalue_final = 0;

  // Envelope-Generator
  if (currentMillis - previousMillisEnv >= envelopePeriod / 100.0) {
    previousMillisEnv += envelopePeriod / 100.0;
    envelopeValue -= schrittweite_envelope;  // 
        if (envelopeValue <= 0.0) {
          envelopeValue = 0.0;  // Unten begrenzen
     }

  }
  
  
  if (currentMillis - previousMillis >= lfoPeriod / 100.0) {
    previousMillis += lfoPeriod / 100.0;
    
    switch (lfoWaveform) {
      case SQUARE:
        // Dreieck ausrechnen 
        lfoValue += schrittweite * lfoDirection;  // 
        if (lfoValue >= 1.0 || lfoValue <= 0.0) {
          lfoDirection = -lfoDirection;  // Richtung umkehren
        }
        if(waveFormFunction == 0){
          lfovalue_final = (lfoValue >= 0.5) ? 1 : 0;
          
          //lfovalue_final = triggeredLfoValue;
        }else{
          lfovalue_final = (lfoValue <= 0.5) ? 0.5 : -100; // -100 = muting
        }
        
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

  float lfovalue_final1 = 0;
  if(lfovalue_final == -100){
    lfovalue_final1 = lfovalue_final; // negativ für Sonderfunktionen wie Muting (-99)
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green), 0);
    //pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red), 0);
  }else{
    
    // lfovalue_final1 = ((lfovalue_final -0.5) * (lfoAmplitude/100) + 1.0) ;
    float envelope = envelopeValue * (envelopeAmplitude / 100) + 1.0;
     lfovalue_final1 = ((lfovalue_final -0.5) * (lfoAmplitude/100) + 1.0) * envelope ;
     debugFloat(envelope);
    
    // * (envelopeValue) 
    // LED in der Helligkeit der Auslenkung
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green),lfovalue_final * pwm_led);
    //pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red), (1.0 - lfovalue_final) * 500);
  }

  
/*
  if(lfovalue_final < 0.5 ){digitalWrite(LED1, HIGH);}else{digitalWrite(LED1, LOW);}
  
   debugFloat(lfovalue_final1);
   */
  //debugFloat(envelopeValue);
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

int pitchAverage(){
  pitchTotal = pitchTotal - pitchReadings[pitchReadIndex];
  pitchReadings[pitchReadIndex] = analogRead(freqPotPin);
  pitchTotal = pitchTotal + pitchReadings[pitchReadIndex];
  pitchReadIndex = pitchReadIndex + 1;

  if (pitchReadIndex >= numReadings) {
    pitchReadIndex = 0;
  }

  return(pitchTotal / numReadings);
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void soundCreate(float freqVal){
 // float pwm_val = setFrequency(baseFrequency); // 4140 = 500hz; 2070 = 1khz; 1035 = 2khz; 
  float pwm_val = setFrequency(linearToLogarithmic(freqVal));
  // float pwm_val = setFrequency(freqVal);

  // Setze die PWM-Periode
  uint slice_num = pwm_gpio_to_slice_num(wave_output);

  pwm_set_wrap(slice_num, pwm_val);
  
  if ((!runSound)||(freqVal == -100)) {
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(wave_output), 0);
  }else{
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(wave_output), pwm_val / 2);
  }
  
 
}

void loop() {
  runSound = !digitalRead(firePin1);

  if (runSound != runSound_oldval){
    if (runSound) {
        lfoValue = 0;
        envelopeValue = 1;
        lfoDirection = 1;
        previousMillis = millis();
    } 
   //delay(1);  // x millisekunden warten, eine Änderung danach wird wieder eine echte sein.
  }
  runSound_oldval = runSound;

  //taster_val = digitalRead(inputPin);           // Wert auslesen
  
  bool shift = digitalRead(controlPin);
  if(!shift){
    if(shift != shift_bak){
      shiftState = !(shiftState);
      delay(100);
    }
    
  }
  shift_bak = shift;
  
  
  digitalWrite(LED_BUILTIN, shiftState);
  
  
  
  int lfoFreqValue = analogRead(lfoFreqPotPin);
  int lfoAmpValue = analogRead(lfoAmpPotPin);

  

  if(shiftState){
    envelopeDuration = mapFloat(lfoFreqValue, 5, 1023, 1, 100);
    envelopeAmplitude = map(lfoAmpValue, 5, 1023, -100, 100);
  }else{
    // lesen des Pitch-Wertes vom Poti incl. Mittelwert
    valPitch = pitchAverage();
    lfoFrequency = mapFloat(lfoFreqValue, 5, 1023, 0.5, 50);   // LFO-Frequenzbereich von 0.5 Hz bis 50 Hz
    lfoAmplitude = map(lfoAmpValue, 5, 1023, 0, 100);   // LFO-Amplitudenbereich von 0 bis 100, 0 am Anschlag
  }
  //lfoAmplitude = map(lfoAmpValue, 5, 1023, -100, 100);   // LFO-Amplitudenbereich von -100 bis 100, 0 in der Mitte
  
  
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

   //debugStr("Waveform:"+String(lfoWaveform)+" Funktion:"+String(waveFormFunction));

  // debug("LFO1: "+ String(lfoFreqValue)+" LFO1 Amp: "+ String(lfoAmpValue));
  // int baseFrequency = map(valPitch, 0, 1023, 50, 4000); // direkte Ausgabe der Frequenz
  
  // Serial.println(mapFloat(valPitch, 0, 1023, 0, 100));
  // int baseFrequency =  mapFloat(valPitch, 0, 1023, 0, 100) ;

  // Holen der Basisfrequenz
  baseFrequency =  mapFloat(valPitch, 4, 1022, minVal, maxVal);
  // debugFloat(baseFrequency);

  // Modulierte Frequenz berechnen
   float lfoValueActual = calculateLFOWave(lfoFrequency, lfoAmplitude, waveFormFunction);
   float newModulatedFrequency = 0;
   if(lfoValueActual == -100){
    newModulatedFrequency = lfoValueActual;
   }else{
    newModulatedFrequency = baseFrequency * lfoValueActual;
   }
   // Reset LFO values if the start button is false
  
   

  /*
  // Berechne den PWM-Wert für die modulierte Frequenz


  
  // float pwm_val = setFrequency(baseFrequency); // 4140 = 500hz; 2070 = 1khz; 1035 = 2khz; 
  float pwm_val = setFrequency(linearToLogarithmic(newModulatedFrequency));
  
  
  // Setze die PWM-Periode
  // Serial.println(pwm_val);

  
  uint slice_num = pwm_gpio_to_slice_num(wave_output);

  pwm_set_wrap(slice_num, pwm_val);
  if (!runSound) {
    
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(wave_output), 0);
  }else{
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(wave_output), pwm_val / 2);
  }
  */
  // Create Tone
  soundCreate(newModulatedFrequency);
}
