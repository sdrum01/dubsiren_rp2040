#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "math.h"
#include "LittleFS.h"
#include "Bounce2.h"
#include "ArduinoJson.h"

#define LONG_PRESS_DURATION 3000
const int LOGLEVEL = 1;

// Definition der IO's
const int freqPotPin = A0;
const int lfoFreqPotPin = A1;  // Potentiometer für die Frequenz des LFO
const int lfoAmpPotPin = A2;   // Potentiometer für die Amplitude des LFO

const int waveFormPin_0 = 3; // WaveForm Kippschalter PIN 1
const int waveFormPin_1 = 4; // WaveForm Kippschalter Pin 2
//const int waveFormFunctionPin = 6;  // WAVEFORM Funktion

const int wave_outputPin = 5; // Pin, an dem der Rechteckton ausgegeben wird

//const int shiftTogglePin = 2;  // shift-Taste alt
const int shiftPin1 = 6;  // shift-Taste1
const int shiftPin2 = 7;  // shift-Taste2

const int firePin1 = 18;  // Steuerpin für die Tonaktivierung1
const int firePin2 = 19;  // Steuerpin für die Tonaktivierung2
const int firePin3 = 20;  // Steuerpin für die Tonaktivierung3g
const int firePin4 = 21;  // Steuerpin für die Tonaktivierung4

// Bounce Objekte erstellen zur Tastenabfrage
Bounce fire1;
Bounce fire2;
Bounce fire3;
Bounce fire4;
//Bounce shiftToggle;
Bounce shift1;
Bounce shift2;

byte actualFireButton = 0;
byte actualFireButtonBak = 0;

// Zeitmessung, ob die Firebutton lange gedrückt sind
unsigned long firePressedTime = 0;
bool longPressDetected = false;

// globale V. für Schalter für WaveForm
byte valLfoWaveformSwitch = 0;
// temporäre Sicherung des Wertes des Waveformschalters
byte valLfoWaveformSwitchBak = 0;

const int LED1_red = 16;
const int LED1_green = 17;
const int LEDShift1 = 8;
const int LEDShift2 = 9;


// Konstanten für den minimalen und maximalen Wert der blanken Frequenz ohne Modulation
const float minVal = 35;
const float maxVal = 8000;
// minimaler und maximaler Wert der Frequenz nach der LFO-Modulation
const float minValMod = 20;
const float maxValMod = 16000;

// Variablen
int zaehler = 0;     

// Parameter Pitch/Tune
int baseFrequency = 1000;

// Parameter LFO:
float lfoFrequency = 5;  // LFO1-Frequenz in Hz (0.5 Hz = 2 Sekunden Periode)
float lfoAmplitude = 0;

// Parameter LFO2:
float lfo2Frequency = 5;  // LFO2-Frequenz in Hz 
float lfo2Amplitude = 0;

// Parameter ENVELOPE-Generator:
float envelopeDuration = 20;  
float envelopeAmplitude = 0;

float duty = 50;

// die finale LFO WaveForm Variable, die durch Schalter oder Speicher gesetzt wird
enum LfoWaveform { SQUARE, TRIANGLE, SAWTOOTH };
LfoWaveform lfo1Waveform = TRIANGLE;  // 
LfoWaveform lfo2Waveform = TRIANGLE;  //


bool dataSaved = false;

// globale Variablen, die der LFO und envelope zurückgibt

volatile float lfoValue = 0;
volatile float lfo2Value = 0;
volatile float envelopeValue = 1;
volatile int lfoDirection = 1;  // Richtung des LFO: 1 aufwärts, -1 abwärts
volatile int lfo2Direction = 1;  // Richtung des LFO: 1 aufwärts, -1 abwärts
volatile unsigned long previousMillis = 0;
volatile unsigned long previousMillisLFO2 = 0;
volatile unsigned long previousMillisEnv = 0;
volatile float lfovalue_finalLFO1 = 0;
volatile float lfovalue_finalLFO2 = 0;


// Messwerte runden
const int numReadings = 10;
int pitchReadings[numReadings];
int pitchReadIndex = 0;
int pitchTotal = 0;

// Wenn Ton abgefeuert werden soll:
bool runSound = 1;

// shiftToggle-taste
bool shiftToggleState1 = 0;
bool shiftToggleState2 = 0;
bool shiftToggleStateBak = 0;

bool shiftState1 = 0;
bool shiftState2 = 0;

bool shiftState1Bak = 0;
bool shiftState2Bak = 0;

byte shiftState = 0;
byte shiftStateToggle = 0;
byte shiftStateBak = 0;

// LED1+2

int pwm_led = 1000;

/////////// Definition der Werte, die von den 3 Potis gelesen werden
int valPotiPitch = 0;
int valPotiFreqLFO = 0;
int valPotiAmpLFO = 0;

// temporäre Sicherung der Werte der Potis zum Vergleich, obs gewackelt hat
int valPotiPitchBak = 0;
int valPotiFreqLFOBak = 0;
int valPotiAmpLFOBak = 0;



// Funktionsschalter
// bool waveFormFunction = 0;
// bool waveFormFunctionBak = 0;


//  Flags, wenn es an den Potis gewackelt hat
// byte potisChanged = 0b0000000;  // binär 00000000

// wie viel Prozent darf sich das Poti ändern, bis ein wert als Change gilt?
const int potiTolerance = 10;

bool potiPitchChanged = 0;
bool potiFreqLFOChanged = 0;
bool potiAmpLFOChanged = 0;


// Flag, wenn es am Waveformschalter gewackelt hat
bool lfo1WaveformChanged = 0;
bool lfo2WaveformChanged = 0;

String debug ="";

String receiveStr = ""; // Hier wird die empfangene Zeichenkette gespeichert
bool receiveStrComplete = false;
/////////////////////////////////////////////////////////////////////////////////////////////////////
// Beispiel

// 18:00:44.574 -> read file fire1.json: {"pitch":761,"lfoFreq":9.544205,"lfoAmount":97,"lfo2Freq":9.209431,"lfo2Amount":100,"waveform":2,"waveform2":1,"dutyCycle":50}

// 18:00:44.574 -> read file fire2.json: {"pitch":786,"lfoFreq":25.25,"lfoAmount":49,"lfo2Freq":3.247249,"lfo2Amount":0,"waveform":0,"waveform2":0,"dutyCycle":50}

// 18:00:44.574 -> read file fire3.json: {"pitch":968,"lfoFreq":18.19941,"lfoAmount":32,"lfo2Freq":0.197741,"lfo2Amount":100,"waveform":1,"waveform2":1,"dutyCycle":8}

// 18:00:44.574 -> read file fire4.json: {"pitch":1124,"lfoFreq":7.647839,"lfoAmount":34,"lfo2Freq":4.8111,"lfo2Amount":4,"waveform":0,"waveform2":1,"dutyCycle":50}



/////////////////////////////////////////////////////////////////////////////////////////////////////

// Werte von den Potis und Schaltern in eine JSON datei speicher
String values2JSON(){
  // Erstelle einen JSON-Dokument Puffer mit genügend Speicher
  StaticJsonDocument<200> dataSet;
/*
  // Verschachtelt
  JsonObject actualDataset = doc.createNestedObject(fireButton);
  actualDataset["pitch"]     = baseFrequency;
  actualDataset["lfoFreq"]   = lfoFrequency;
  actualDataset["lfoAmount"] = lfoAmplitude;
  actualDataset["envTime"]   = envelopeDuration;
  actualDataset["envAmount"] = envelopeAmplitude;
  actualDataset["waveform"] = lfoWaveform;
*/
  // Plain
  dataSet["pitch"]     = baseFrequency;
  dataSet["lfoFreq"]   = lfoFrequency;
  dataSet["lfoAmount"] = lfoAmplitude;
  dataSet["lfo2Freq"]   = lfo2Frequency;
  dataSet["lfo2Amount"] = lfo2Amplitude;
  //dataSet["envTime"]   = envelopeDuration;
  //dataSet["envAmount"] = envelopeAmplitude;
  dataSet["waveform"]  = lfo1Waveform;
  dataSet["waveform2"]  = lfo2Waveform;
  dataSet["dutyCycle"]  = duty;

  // Erstelle einen JSON-String
  String jsonString;
  serializeJson(dataSet, jsonString);

  // String fileName = fireButton+".json";
  // JSON-String ausgeben
  // return(writeSettings(fileName, jsonString));
  return jsonString;
}

void JSON2values(String jsonString) {
  // Erstelle ein JSON-Dokument für das Parsen
  StaticJsonDocument<200> dataSet;

  // Deserialisiere den JSON-String
  DeserializationError error = deserializeJson(dataSet, jsonString);

  // Überprüfen, ob die Deserialisierung erfolgreich war
  if (error) {
    Serial.print("Fehler beim Parsen: ");
    Serial.println(error.f_str());
    return;
  }

  /*
  // Jetzt können wir die Werte aus dem JSON-Dokument extrahieren
  JsonObject actualDataset = doc["fire1"];

  // Variablen aus dem JSON extrahieren und den globalen Variablen zuweisen
  baseFrequency = actualDataset["pitch"];
  lfoFrequency = actualDataset["lfoFreq"];
  lfoAmplitude = actualDataset["lfoAmount"];
  envelopeDuration = actualDataset["envTime"];
  envelopeAmplitude = actualDataset["envAmount"];
  lfoWaveform = actualDataset["waveform"];
  */
  baseFrequency = dataSet["pitch"];
  lfoFrequency = dataSet["lfoFreq"];
  lfoAmplitude = dataSet["lfoAmount"];
  lfo2Frequency = dataSet["lfo2Freq"];
  lfo2Amplitude = dataSet["lfo2Amount"];
  //envelopeDuration = dataSet["envTime"];
  //envelopeAmplitude = dataSet["envAmount"];
  lfo1Waveform = dataSet["waveform"];
  lfo2Waveform = dataSet["waveform2"];
  duty = dataSet["dutyCycle"];
}

String readSettings(String configFile){
    // Daten lesen
    String value;
    File file = LittleFS.open(configFile, "r");
    if (file) {
        //String value = file.read();  // Lese die gespeicherte Zahl
        value = file.readStringUntil('\n');
        Serial.println("read file "+configFile+": "+value);
        file.close();
    } else {
        Serial.println("Error during reading of file "+configFile);
        String _json = values2JSON();
        dataSaved = writeSettings(_json,configFile);
    }
  return(value);
}

bool writeSettings(String s, String configFile){
  // Daten schreiben
  File file = LittleFS.open(configFile, "w");
  if (file) {
      //file.write(s);  // Schreibe eine Zahl
      // String in die Datei schreiben
      
      file.println(s);  // Schreibt den String und fügt einen Zeilenumbruch hinzu

      file.close();
      return(true);
      //Serial.println("write file "+configFile+": "+s);
  } else {
    //Serial.println("Error during writing of file "+configFile);
    return(false);
  }
}

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
  //debugStr(String(freq)+';'+String(percentage)+';'+String(outputValue));
  return outputValue;
}

// Envelope-Generator
float calculateEnvelope(float envelopeDuration, float envelopeAmplitude) {

  float schrittweite_envelope = envelopeDuration / 5000;
  unsigned long currentMillis = millis();
  const int envelopePeriod = 100;
  
  // Envelope-Generator
  if (currentMillis - previousMillisEnv >= envelopePeriod / 100.0) {
    previousMillisEnv += envelopePeriod / 100.0;
    envelopeValue -= schrittweite_envelope;  // 
    if (envelopeValue <= 0.0){
      envelopeValue = 0.0;  // Unten begrenzen
    }
  }
  float envelope = envelopeValue  * (envelopeAmplitude / 100) + 0.5;
  return envelope;
}

// LFO-1 mit Dreieck, abgeleiteten Rechteck und Sägezahn
float calculateLFOWave1(float lfo1Frequency, float lfo1Amplitude) {

  float schrittweite = lfo1Frequency / 1000 ;
  unsigned long currentMillis = millis();
  const int lfoPeriod = 100;  // LFO-Periode in Millisekunden (5000 / 50)
  

  if (currentMillis - previousMillis >= lfoPeriod / 100.0) {
    previousMillis += lfoPeriod / 100.0;
    
    switch (lfo1Waveform) {
      case SQUARE:
        //lfoAmplitude = lfoAmplitude / 2;
        // Dreieck ausrechnen 
        lfoValue += schrittweite * lfoDirection;  // 
        if (lfoValue >= 1.0 || lfoValue <= 0.0) {
          lfoDirection = -lfoDirection;  // Richtung umkehren
        }
        
        if(lfoAmplitude < 0){
          lfovalue_finalLFO1 = (lfoValue <= 0.5) ? 0.5 : -100; // -100 = muting;
        } else {
          lfovalue_finalLFO1 = (lfoValue >= 0.5) ? 1 : 0;
        }
        break;
        
      case TRIANGLE:
        lfoValue += schrittweite * lfoDirection;  // 
        if (lfoValue >= 1.0 || lfoValue <= 0.0) {
          lfoDirection = -lfoDirection;  // Richtung umkehren
        }
        //lfovalue_finalLFO1 = lfoValue;
        lfovalue_finalLFO1 = lfoValue;
        break;
        
      case SAWTOOTH:
        lfoValue -= schrittweite / 2;  // Schrittweite für den LFO 
        if (lfoValue <= 0) {
          lfoValue = 1;  // Zurücksetzen
        }
        lfovalue_finalLFO1 = lfoValue;
        break;
    }
  }

  // lfovalue_finalLFO1 liegt zwischen 0..1: verschieben der Mitte auf den NullPunkt
  // Skalierung des LFO-Wertes mit der LFO-Amplitude 
  // verschieben des Nullpunktes auf 1: damit kann der Wert als Multiplikator genutzt werden

  float lfovalue_final1 = 0;
  if(lfovalue_finalLFO1 == -100){
    lfovalue_final1 = lfovalue_finalLFO1; // negativ für Sonderfunktionen wie Muting (-99)
   
  }else{
    lfovalue_final1 = ( ((lfovalue_finalLFO1 -0.5)*1.5) * (lfo1Amplitude/100) + 1.0);
  }
  return(lfovalue_final1);
}

// LFO-2 
float calculateLFOWave2(float frequency, float amplitude) {
  //uint slice_num_led_red = pwm_gpio_to_slice_num(LED1_red);
  float schrittweite = frequency / 1000 ;
  unsigned long currentMillis = millis();

  const int lfoPeriod = 100;  // LFO-Periode in Millisekunden (5000 / 50)
  
  if (currentMillis - previousMillisLFO2 >= lfoPeriod / 100.0) {
    previousMillisLFO2 += lfoPeriod / 100.0;
    switch (lfo2Waveform) {
      case SQUARE:
        // Dreieck ausrechnen 
        lfo2Value += schrittweite * lfo2Direction;  // 
        if (lfo2Value >= 1.0 || lfo2Value <= 0.0) {
          lfo2Direction = -lfo2Direction;  // Richtung umkehren
        }
        /*
        if(lfoAmplitude < 0){
          lfovalue_finalLFO2 = (lfo2Value <= 0.5) ? 0.5 : -100; // -100 = muting;
        } else {
          lfovalue_finalLFO2 = (lfo2Value >= 0.5) ? 1 : 0;
        }
        */
        lfovalue_finalLFO2 = (lfo2Value >= 0.5) ? 1 : 0;
        break;
        
      case TRIANGLE:
        lfo2Value += schrittweite * lfo2Direction;  // 
        if (lfo2Value >= 1.0 || lfo2Value <= 0.0) {
          lfo2Direction = -lfo2Direction;  // Richtung umkehren
        }
        lfovalue_finalLFO2 = lfo2Value;
        break;
        
      case SAWTOOTH:
        lfo2Value -= schrittweite / 2;  // Schrittweite für den LFO 
        if (lfo2Value <= 0) {
          lfo2Value = 1;  // Zurücksetzen
        }
        lfovalue_finalLFO2 = lfo2Value;
        break;
    }

  }

  float lfovalue_final1 = 0;
  lfovalue_final1 = ( ((lfovalue_finalLFO2 -0.5)*1.5) * (amplitude/100) + 1.0);
  return(lfovalue_final1);
}

// LFO-Variante Sinus 
/*
float calculateLFOSin(float baseFreq, float lfoFreq, float lfoDepth) {
  // Berechne die aktuelle Zeit in Sekunden
  float time = millis() / 1000.0;

  // Berechne die aktuelle LFO-Phase
  float lfoPhase = time * lfoFreq * 2 * PI;
  
  // debug("LFO-Phase: "+String(lfoPhase));

  // Berechne die modulierte Frequenz
  return baseFreq + lfoDepth * sin(lfoPhase);
}
*/
float setFrequency(float freq){
  //debugFloat(freq);
  if(freq < minValMod){freq = minValMod;}
  if(freq > maxValMod){freq = maxValMod;}
  return(2070 / (freq/1000));
}

/*
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
*/

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Tonerzeugung
void playSound(float freqVal){
  // 4140 = 500hz; 2070 = 1khz; 1035 = 2khz; 
  float pwm_val = setFrequency(freqVal);
  
  // Setze die PWM-Periode
  uint slice_num_wave = pwm_gpio_to_slice_num(wave_outputPin);

  pwm_set_wrap(slice_num_wave, pwm_val);
  
  if (runSound){
    dataSaved = false;
    if(freqVal == -100) {
      pwm_set_chan_level(slice_num_wave, pwm_gpio_to_channel(wave_outputPin), 0);
    }else{
      //pwm_set_chan_level(slice_num_wave, pwm_gpio_to_channel(wave_outputPin), pwm_val * 0.5);
      //dutyCycle = duty / 100;
      pwm_set_chan_level(slice_num_wave, pwm_gpio_to_channel(wave_outputPin), pwm_val * (duty / 100));
    }
  }else{
    pwm_set_chan_level(slice_num_wave, pwm_gpio_to_channel(wave_outputPin), 0);
  }
}

void setChangeState(bool p1, bool p2, bool p3, bool s1){
  potiPitchChanged = p1;
  potiFreqLFOChanged = p2;
  potiAmpLFOChanged = p3;
  lfo1WaveformChanged = s1;
  debugStr("State Reset");
}

bool chkLoop(int endCount){
  if(zaehler == endCount){
    zaehler = 0;
    return(true);
  }
  zaehler++;
  return(false);
}

void resetLFOParams(){
  lfoValue = 0;
  envelopeValue = 2;
  lfoDirection = 1;
  // mit dem aktualisieren der millisekunden wird die Hüllkurve und LFO neu gestartet
  previousMillis = millis();
}

byte combineBoolsToByte(bool b0, bool b1) {
  byte result = 0;
  
  result |= (b0 << 0);  // Bit 0
  result |= (b1 << 1);  // Bit 1
  result |= (0 << 2);  // Bit 2
  result |= (0 << 3);  // Bit 3
  result |= (0 << 4);  // Bit 4
  result |= (0 << 5);  // Bit 5
  result |= (0 << 6);  // Bit 6
  result |= (0 << 7);  // Bit 7
  
  return result;
}

void loadOrSave(byte fireButton){
  String fileName = "fire"+String(fireButton)+".json";
  resetLFOParams();
  // 
  if(shiftState == 2){ // Shifttaste 2 gedrückt
    // Save values
    String _json = values2JSON();
    debugStr("Write");
    dataSaved = writeSettings(_json,fileName);
    //shiftToggleState = 1;
  } else {
    // longPressDetected: wenn Ton gerade gehalten wird, wird sonst der Sound des anderen zuvor gedrückten gespielt
    if((fireButton != actualFireButtonBak) || longPressDetected){ 
      // load Values
      String _json = readSettings(fileName);
      JSON2values(_json);
      setChangeState(0,0,0,0);
    }
  }
  return;
}

void updateKeys(){
  //shiftToggle.update();
  shift1.update();
  shift2.update();

  fire1.update();
  fire2.update();
  fire3.update();
  fire4.update();

    if (shift1.fell()) {
      dataSaved = false;
    }
    if (shift2.fell()) {
      dataSaved = false;
    }

  if (dataSaved == 0){
    // fallende Flanke (Taster losgelassen)
    
    if (shift1.rose()) {
      shiftToggleState1 = !(shiftToggleState1);
      if(shiftToggleState1){shiftToggleState2 = 0;}
    }
    if (shift2.rose()) {
      shiftToggleState2 = !(shiftToggleState2);
      if(shiftToggleState2){shiftToggleState1 = 0;}
    }
  }

/*
  if(shiftToggleStateBak != shiftToggleState){
    // Flags zurücksetzen, dass die Potis gewackelt haben, sonst springen die Einstellungen sofort auf die neuen Werte
    setChangeState(0,0,0,0);
  }
  shiftToggleStateBak = shiftToggleState;
*/  

  
  shiftState = combineBoolsToByte(!shift1.read(),!shift2.read());
  shiftStateToggle = combineBoolsToByte(shiftToggleState1,shiftToggleState2);

  if(shiftStateBak != shiftState){
    // Flags zurücksetzen, dass die Potis gewackelt haben, sonst springen die Einstellungen sofort auf die neuen Werte
    setChangeState(0,0,0,0);
  }
  shiftStateBak = shiftState;



  // Abfrage Schalter, welche Funktion die Potis haben sollen
  /*
  waveFormFunction = digitalRead(waveFormFunctionPin);


  // Wenn der Funktionsschalter sich geändert hat
  if(waveFormFunctionBak != waveFormFunction){
    // Flags zurücksetzen, dass die Potis gewackelt haben, sonst springen die Einstellungen sofort auf die neuen Werte
    setChangeState(1,0,0,0);
  }
  waveFormFunctionBak = waveFormFunction;
*/
  // Aus dem Waveformschalter ein Byte machen
  valLfoWaveformSwitch = combineBoolsToByte(digitalRead(waveFormPin_0),digitalRead(waveFormPin_1));
  if(valLfoWaveformSwitchBak != valLfoWaveformSwitch){
    lfo1WaveformChanged = 1;
  }
  valLfoWaveformSwitchBak = valLfoWaveformSwitch;

  
  
  // steigende Flanke (Taster gedrückt)
  if (fire1.fell()){
    actualFireButton = 1;
    //firePressedTime = millis();  // Zeit des Tastendrucks speichern
    if(!fire2.read() || !fire3.read() ||!fire4.read()){
      longPressDetected = true;
    }else{
      loadOrSave(actualFireButton);
      longPressDetected = false;     // Reset des Langdruck-Flags
    }
    
    
  }
  if (fire2.fell()){
    actualFireButton = 2;
    //firePressedTime = millis();  
    //longPressDetected = false;
    if(!fire1.read() || !fire3.read() ||!fire4.read()){
      longPressDetected = true;
    }else{
      loadOrSave(actualFireButton);
      longPressDetected = false;     // Reset des Langdruck-Flags
    }
  }
  if (fire3.fell()){
    actualFireButton = 3;
    //firePressedTime = millis();  
    //longPressDetected = false;
    if(!fire1.read() || !fire2.read() ||!fire4.read()){
      longPressDetected = true;
    }else{
      loadOrSave(actualFireButton);
      longPressDetected = false;     // Reset des Langdruck-Flags
    }
  }
  if (fire4.fell()){
    actualFireButton = 4;
    //firePressedTime = millis();  
    //longPressDetected = false;
    if(!fire1.read() || !fire2.read() ||!fire3.read()){
      longPressDetected = true;
    }else{
      loadOrSave(actualFireButton);
      longPressDetected = false;     // Reset des Langdruck-Flags
    }
  }

  if (fire1.rose()){
    firePressedTime = 0;  
    //longPressDetected = false;
  }
  if (fire2.rose()){
    firePressedTime = 0;  
    //longPressDetected = false;
  }
  if (fire3.rose()){
    firePressedTime = 0;  
    //longPressDetected = false;
  }
  if (fire4.rose()){
    firePressedTime = 0;  
    //longPressDetected = false;
  }

  bool anyFireButtonPressed = (((fire1.read() == LOW)||(fire2.read() == LOW)||(fire3.read() == LOW)||(fire4.read() == LOW))&&(shiftState != 2));
  
  if (anyFireButtonPressed && !longPressDetected) {
    if (millis() - firePressedTime >= LONG_PRESS_DURATION) {
      // longPressDetected = true;  // Markiere, dass der lange Druck erkannt wurde
      // Serial.println("LongPress detected!");
    }
  }
  
  actualFireButtonBak = actualFireButton;

  // globale Variable rundsound = wenn high, wird ein Ton abgespielt
  runSound = (anyFireButtonPressed || longPressDetected);
  
}

void updatePotis(){
  // Abfrage der 3 Potis
  valPotiPitch = analogRead(freqPotPin);
  valPotiFreqLFO = analogRead(lfoFreqPotPin);
  valPotiAmpLFO = analogRead(lfoAmpPotPin);

  // Wenns gewackelt hat innerhalb einer Toleranz, wird ein Flag gesetzt
  if((valPotiPitch < valPotiPitchBak - potiTolerance)||(valPotiPitch > valPotiPitchBak + potiTolerance)){
    potiPitchChanged = 1;
  }

  if((valPotiFreqLFO < valPotiFreqLFOBak - potiTolerance)||(valPotiFreqLFO > valPotiFreqLFOBak + potiTolerance)){
    potiFreqLFOChanged = 1;
  }

  if((valPotiAmpLFO < valPotiAmpLFOBak - potiTolerance)||(valPotiAmpLFO > valPotiAmpLFOBak + potiTolerance)){
    potiAmpLFOChanged = 1;
  }
}

void ledControl(){
  uint slice_num_led_green = pwm_gpio_to_slice_num(LED1_green);
  uint slice_num_led_red = pwm_gpio_to_slice_num(LED1_red);

  // LED Grün
  if(shiftToggleState1 == 0){
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_red), 0);
    if(lfovalue_finalLFO1 == -100){
      pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green), 0);
    }else{
      pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green),lfovalue_finalLFO1 * pwm_led);
    }
  }else{
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green), 0);
    pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red),lfovalue_finalLFO2 * pwm_led);
  }
  //
  // LEDs
  // digitalWrite(LEDShift1, shiftToggleState1);
  digitalWrite(LEDShift2, shiftToggleState2);
}

void readSerial(){
  while (Serial.available() > 0) {
    char empfangenesZeichen = Serial.read(); // Ein einzelnes Zeichen lesen

    // Wenn das empfangene Zeichen ein Zeilenende ('\n') ist, gilt die Zeichenkette als vollständig
    if (empfangenesZeichen == '\n') {
      receiveStrComplete = true;
    } else {
      receiveStr += empfangenesZeichen; // Zeichen an die Zeichenkette anhängen
    }
  }

  // Wenn die Zeichenkette vollständig empfangen wurde, diese anzeigen
  if (receiveStrComplete) {
    //Serial.print("Empfangen: ");
    //Serial.println(receiveStr);
    if(receiveStr == "dump"){
      String _json1 = readSettings("fire1.json");
      String _json2 = readSettings("fire2.json");
      String _json3 = readSettings("fire3.json");
      String _json4 = readSettings("fire4.json");
      Serial.println(_json1);
      Serial.println(_json2);
      Serial.println(_json3);
      Serial.println(_json4);
     
    }
    // Die Zeichenkette zurücksetzen, um eine neue zu empfangen
    receiveStr = "";
    receiveStrComplete = false;
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  
  if(LOGLEVEL > 0){
    Serial.begin(38400);
  }
  
  // Initialisiere die GPIO-Pin-Funktion für PWM Wave-Output
  gpio_set_function(wave_outputPin, GPIO_FUNC_PWM);
  uint slice_num_wave = pwm_gpio_to_slice_num(wave_outputPin);
  
  // Setze den PWM-Teilungsverhältnis
  pwm_set_clkdiv(slice_num_wave, 64.f);

  // Starte den PWM-Output
  pwm_set_enabled(slice_num_wave, true); 

  // LED

  uint slice_num_led_green = pwm_gpio_to_slice_num(LED1_green);
  uint slice_num_led_red = pwm_gpio_to_slice_num(LED1_green);
  
  gpio_set_function(LED1_green, GPIO_FUNC_PWM);
  gpio_set_function(LED1_red, GPIO_FUNC_PWM);
  
  pwm_set_clkdiv(slice_num_led_green, 128.f);
  pwm_set_clkdiv(slice_num_led_red, 128.f);
  
  pwm_set_enabled(slice_num_led_green, true);
  pwm_set_enabled(slice_num_led_red, true);
  
  pwm_set_wrap(slice_num_led_green, pwm_led);
  pwm_set_wrap(slice_num_led_red, pwm_led);
  
  pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green), 0);
  pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red), 0);
  

  // Normale IO's
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(LEDShift1, OUTPUT);
  pinMode(LEDShift2, OUTPUT);

   
  pinMode(shiftPin1, INPUT_PULLUP);  
  pinMode(shiftPin2, INPUT_PULLUP);
  pinMode(firePin1, INPUT_PULLUP); 
  pinMode(firePin2, INPUT_PULLUP); 
  pinMode(firePin3, INPUT_PULLUP); 
  pinMode(firePin4, INPUT_PULLUP); 

  // Bounce-Objekt initialisieren
  //shiftToggle.attach(shiftPin1);
  //shiftToggle.interval(50);  // Entprellintervall in Millisekunden (50 ms)

  shift1.attach(shiftPin1);
  shift1.interval(50);  // Entprellintervall in Millisekunden (50 ms)

  shift2.attach(shiftPin2);
  shift2.interval(50);  // Entprellintervall in Millisekunden (50 ms)

  fire1.attach(firePin1);
  fire1.interval(10);  

  fire2.attach(firePin2);
  fire2.interval(10);  

  fire3.attach(firePin3);
  fire3.interval(10);  

  fire4.attach(firePin4);
  fire4.interval(10);  
  
  pinMode(waveFormPin_0, INPUT_PULLUP);
  pinMode(waveFormPin_1, INPUT_PULLUP);
  // pinMode(waveFormFunctionPin, INPUT_PULLUP);
  
  // Initialisieren des Arrays für die Mittelwertbildung des Pitch
  /*
  for (int i = 0; i < numReadings; i++) {
    pitchReadings[i] = 0;
  }
 */ 
  if (!LittleFS.begin()) {
      debugStr("LittleFS mount failed");
      return;
   }
  // Lampe an, nur zur Kontrolle, dass die SW läuft
  // debugStr("Setup abgeschlossen.");
  String _json1 = readSettings("fire1.json");
  JSON2values(_json1);
}

void loop() {
  
  updateKeys();
  updatePotis();

  readSerial();


  if(shiftStateToggle == 0){
    // WaveForm für LFO1
    if(lfo1WaveformChanged){
      switch (valLfoWaveformSwitch) {
        case 1:
          lfo1Waveform = SQUARE;
          break;
        case 2:
          lfo1Waveform = SAWTOOTH;
          break;
        case 3:
          lfo1Waveform = TRIANGLE;
          break;
      }
    }
    
    // lesen des Pitch-Wertes vom Poti incl. Mittelwert
    // Holen der Basisfrequenz
    if(potiPitchChanged == 1){
      int freqLin = map(valPotiPitch, 4, 1023, minVal, maxVal );
      baseFrequency =  linearToLogarithmic(freqLin);
      valPotiPitchBak = valPotiPitch;
    }
    if(potiFreqLFOChanged == 1){
      lfoFrequency = mapFloat(valPotiFreqLFO, 5, 1023, 0.5, 50);   // LFO-Frequenzbereich von 0.5 Hz bis 50 Hz
      valPotiFreqLFOBak = valPotiFreqLFO;
    }
    if(potiAmpLFOChanged == 1){
      lfoAmplitude = map(valPotiAmpLFO, 5, 1023, -100, 100);   // LFO-Amplitudenbereich von 0 bis 100, 50 Mitte
      valPotiAmpLFOBak = valPotiAmpLFO;
    }
  } else if(shiftStateToggle == 1){
    // WaveForm für LFO2
    if(lfo1WaveformChanged){
      switch (valLfoWaveformSwitch) {
        case 1:
          lfo2Waveform = SQUARE;
          break;
        case 2:
          lfo2Waveform = SAWTOOTH;
          break;
        case 3:
          lfo2Waveform = TRIANGLE;
          break;
      }
    }

    // Duty-Cycle Tonausgabe
    if(potiPitchChanged == 1){
      duty = map(valPotiPitch, 4, 1023, 5, 50 );
      valPotiPitchBak = valPotiPitch;
    }

    // LFO2
    if(potiFreqLFOChanged == 1){
      //lfo2Frequency = mapFloat(valPotiFreqLFO, 5, 1023, 1, 100);
      lfo2Frequency = mapFloat(valPotiFreqLFO, 5, 1023, 0.1, 20);
      valPotiFreqLFOBak = valPotiFreqLFO;
    }
    if(potiAmpLFOChanged == 1){
      lfo2Amplitude = map(valPotiAmpLFO, 5, 1023, -100, 100);
      valPotiAmpLFOBak = valPotiAmpLFO;
    }
  }  
  // beide Shift-Tasten zugleich gedrückt
  if(shiftState == 3){
    //envelopeAmplitude = 0;
    lfo2Amplitude = 0;
    dataSaved = true;
    shiftToggleState1 = 0;
    shiftToggleState2 = 0;
  }

   
   

   // Modulierte Frequenz berechnen
   // float envelope = calculateEnvelope(5,100);
   float lfo1ValueActual = calculateLFOWave1(lfoFrequency, lfoAmplitude);
   float lfo2ValueActual = calculateLFOWave2(lfo2Frequency, lfo2Amplitude);
   
   float newModulatedFrequency = 0;
   if(lfo1ValueActual == -100){
    newModulatedFrequency = lfo1ValueActual;
   }else{
    newModulatedFrequency = baseFrequency * lfo1ValueActual * lfo2ValueActual;
   }
   // Reset LFO values if the start button is false
  
  ledControl();
  
  // Create Tone
  playSound(newModulatedFrequency);

  // Überwachung und Debugprints
  if(chkLoop(100)){
     //debugFloat(shiftStateToggle);
     debugFloat(newModulatedFrequency);
    //  debugFloat(longPressDetected);
  }

}
