#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "math.h"
#include "LittleFS.h"
#include "Bounce2.h"
#include "ArduinoJson.h"

#define LONG_PRESS_DURATION 3000

const byte LOGLEVEL = 1;

// Definition der IO's
const int freqPotPin = A0;
const int lfoFreqPotPin = A1;  // Potentiometer für die Frequenz des LFO
const int lfoAmpPotPin = A2;   // Potentiometer für die Amplitude des LFO

//const int waveFormPin_0 = 3; // WaveForm Kippschalter PIN 1
//const int waveFormPin_1 = 4; // WaveForm Kippschalter Pin 2

const int selectWaveFormLFO1Pin = 3; // WaveForm Kippschalter PIN 1
const int selectWaveFormLFO2Pin = 4; // WaveForm Kippschalter Pin 2

//const int waveFormFunctionPin = 6;  // WAVEFORM Funktion

const int wave_outputPin = 5; // Pin, an dem der Rechteckton ausgegeben wird

//const int selectWaveformPin = 2;  // shift-Taste alt
const int shiftPin1 = 6;  // shift-Taste1
const int shiftPin2 = 7;  // shift-Taste2

const int firePin1 = 18;  // Steuerpin für die Tonaktivierung1
const int firePin2 = 19;  // Steuerpin für die Tonaktivierung2
const int firePin3 = 20;  // Steuerpin für die Tonaktivierung3g
const int firePin4 = 21;  // Steuerpin für die Tonaktivierung4

const int LED1_red = 16;
const int LED1_green = 17;
const int LED2_red = 14;
const int LED2_green = 15;
const int LEDShift1 = 8;
const int LEDShift2 = 9;

const int LEDFire1 = 10;
const int LEDFire2 = 11;
const int LEDFire3 = 12;
const int LEDFire4 = 13;

// Bounce Objekte erstellen zur Tastenabfrage
Bounce fire1;
Bounce fire2;
Bounce fire3;
Bounce fire4;
//Bounce shiftToggle;
Bounce shift1;
Bounce shift2;

Bounce selectWaveformLFO1;
Bounce selectWaveformLFO2;

byte actualFireButton = 0;
byte actualFireButtonBak = 0;

// 4 Bänke * 4 FireButtons = 16 Speicherplätze;
byte bank = 1;
byte bankBak = 1;

// Zeitmessung, ob die Firebutton lange gedrückt sind
unsigned long firePressedTime = 0;
bool longPressDetected = false;

// globale V. für Schalter für WaveForm
byte valLfoWaveformSwitch = 0;
// temporäre Sicherung des Wertes des Waveformschalters
byte valLfoWaveformSwitchBak = 0;


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
float lfo1Frequency = 5;  // LFO1-Frequenz in Hz (0.5 Hz = 2 Sekunden Periode)
float lfo1Amplitude = 0;

// Parameter LFO2:
float lfo2Frequency = 5;  // LFO2-Frequenz in Hz 
float lfo2Amplitude = 0;

// Parameter ENVELOPE-Generator:
float envelopeDuration = 1;  
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
volatile unsigned long previousMillisLED = 0;

volatile float lfovalue_finalLFO1 = 0;
volatile float lfovalue_finalLFO2 = 0;

/*
// Messwerte runden
const int numReadings = 10;
int pitchReadings[numReadings];
int pitchReadIndex = 0;
int pitchTotal = 0;
*/

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

// shiftState: Byte, in dem eine Kombination aus Shift1 und Shift2 gespeichert wird
byte shiftState = 0;
//byte shiftStateToggle = 0;
byte shiftStateBak = 0;

// Modulationsselektor: 0:LFO1, 1: LFO2, 2:Envelope
// wird gesetzt bei pos. Flange LFO1 oder LFO2 oder neg.Flange Shift1
byte modSelect = 0; 

// Kombination aus LFO1 und LFO2, wird gesetzt, wenn LFO-Button gehalten wird zur Waveform Auswahl mit den Fire-Buttons
byte lfoSelectState = 0;

bool blinkState = 0;
// LEDs Firebutton 1 aus 4 mit Status an oder aus
byte selectedFireLed = 0;
bool selectedFireLedState = 0;

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

String debugString = "";

String receiveStr = ""; // Hier wird die empfangene Zeichenkette gespeichert
bool receiveStrComplete = false;
/////////////////////////////////////////////////////////////////////////////////////////////////////
// Beispiel

// 18:00:44.574 -> read file fire1.json: {"pitch":761,"lfoFreq":9.544205,"lfoAmount":97,"lfo2Freq":9.209431,"lfo2Amount":100,"waveform":2,"waveform2":1,"dutyCycle":50}

// 18:00:44.574 -> read file fire2.json: {"pitch":786,"lfoFreq":25.25,"lfoAmount":49,"lfo2Freq":3.247249,"lfo2Amount":0,"waveform":0,"waveform2":0,"dutyCycle":50}

// 18:00:44.574 -> read file fire3.json: {"pitch":968,"lfoFreq":18.19941,"lfoAmount":32,"lfo2Freq":0.197741,"lfo2Amount":100,"waveform":1,"waveform2":1,"dutyCycle":8}

// 18:00:44.574 -> read file fire4.json: {"pitch":1124,"lfoFreq":7.647839,"lfoAmount":34,"lfo2Freq":4.8111,"lfo2Amount":4,"waveform":0,"waveform2":1,"dutyCycle":50}


/////////////////////////////////////////////////////////////////////////////////////////////////////

void setDefaultDataSet(){
  baseFrequency     = 1000;
  lfo1Frequency     = 15;
  lfo1Amplitude     = 0.0;
  lfo2Frequency     = 5.0;
  lfo2Amplitude     = 0.0;
  envelopeDuration  = 2;
  envelopeAmplitude = 0;
  lfo1Waveform      = TRIANGLE;
  lfo2Waveform      = SQUARE;
  duty = 50;
}


// Werte von den Potis und Schaltern in eine JSON datei speicher
String values2JSON(){
  // Erstelle einen JSON-Dokument Puffer mit genügend Speicher
  StaticJsonDocument<200> dataSet;
/*
  // Verschachtelt
  JsonObject actualDataset = doc.createNestedObject(fireButton);
  actualDataset["pitch"]     = baseFrequency;
  actualDataset["lfoFreq"]   = lfo1Frequency;
  actualDataset["lfoAmount"] = lfoAmplitude;
  actualDataset["envTime"]   = envelopeDuration;
  actualDataset["envAmount"] = envelopeAmplitude;
  actualDataset["waveform"] = lfoWaveform;
*/
  // Plain
  dataSet["pitch"]     = baseFrequency;
  dataSet["lfoFreq"]   = lfo1Frequency;
  dataSet["lfoAmount"] = lfo1Amplitude;
  dataSet["lfo2Freq"]   = lfo2Frequency;
  dataSet["lfo2Amount"] = lfo2Amplitude;
  dataSet["envTime"]   = envelopeDuration;
  dataSet["envAmount"] = envelopeAmplitude;
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
  lfo1Frequency = actualDataset["lfoFreq"];
  lfoAmplitude = actualDataset["lfoAmount"];
  envelopeDuration = actualDataset["envTime"];
  envelopeAmplitude = actualDataset["envAmount"];
  lfoWaveform = actualDataset["waveform"];
  */
  baseFrequency = dataSet["pitch"];
  lfo1Frequency = dataSet["lfoFreq"];
  lfo1Amplitude = dataSet["lfoAmount"];
  lfo2Frequency = dataSet["lfo2Freq"];
  lfo2Amplitude = dataSet["lfo2Amount"];
  envelopeDuration = dataSet["envTime"];
  envelopeAmplitude = dataSet["envAmount"];
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
        setDefaultDataSet();
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

bool deleteSettings(String configFile){
 // Datei löschen
  if (LittleFS.remove(configFile)) {
    Serial.println("Datei erfolgreich gelöscht.");
    return true;
  } else {
    Serial.println("Datei konnte nicht gelöscht werden!");
    return false;
  }

}

void debug(String s){
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
float linearToLogarithmic(float value,float min, float max) {
  
  float percentage = (value - min ) / ((max - min)/100);
  // Überprüfen, ob die Eingabewerte sinnvoll sind
  if (percentage < 0.0) percentage = 0.0;
  if (percentage > 100.0) percentage = 100.0;
  
  // Skaliere den Prozentwert auf den Bereich [0, 1]
  float scaledPercentage = percentage / 100.0;
  
  // Berechne die logarithmische Ausgabe
  // logarithmischer Bereich mit Basis 10
  float logMin = log10(min);
  float logMax = log10(max);
  
  // Interpolieren im logarithmischen Raum
  float logValue = logMin + scaledPercentage * (logMax - logMin);
  
  // Rückkonvertieren in den linearen Raum
  float outputValue = pow(10, logValue);
  //debug(String(value)+';'+String(percentage)+';'+String(outputValue));
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
  float envelopeValueLog = linearToLogarithmic(envelopeValue,0.0,1.0);
  float envelope = ((envelopeValue -0.5)*1.9)  * (envelopeAmplitude / 100) +1;
  return envelope;
}

// LFO-1 mit Dreieck, abgeleiteten Rechteck und Sägezahn
float calculateLFOWave1(float lfoFrequency, float lfoAmplitude) {

  float schrittweite = lfoFrequency / 1000 ;
  unsigned long currentMillis = millis();
  const int lfoPeriod = 100;  // LFO-Periode in Millisekunden (5000 / 50)
  

  if (currentMillis - previousMillis >= lfoPeriod / 100.0) {
    previousMillis += lfoPeriod / 100.0;
    
    switch (lfo1Waveform) {
      case SQUARE:
        //lfo1Amplitude = lfoAmplitude / 2;
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

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
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
  if(freq < minValMod){freq = minValMod;}
  if(freq > maxValMod){freq = maxValMod;}
  //return(5140 / (freq/1000));
  return(2070 / (freq/1000));
  //return(1035 / (freq/1000));
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
  debug("State Reset");
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
  unsigned long currentMillis = millis();
  lfoValue = 0;
  //lfo2Value = 0;
  envelopeValue = 1;
  lfoDirection = 1;
  //lfo2Direction = 1;
  // mit dem aktualisieren der millisekunden wird die Hüllkurve und LFO neu gestartet
  previousMillis = currentMillis;
  previousMillisLFO2 = currentMillis;
}

void resetShiftState(){
  dataSaved = true;
  shiftToggleState1 = 0;
  shiftToggleState2 = 0;
  modSelect = 0;
}

byte combineBoolsToByte(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7) {
  byte result = 0;
  
  result |= (b0 << 0);  // Bit 0
  result |= (b1 << 1);  // Bit 1
  result |= (b2 << 2);  // Bit 2
  result |= (b3 << 3);  // Bit 3
  result |= (b4 << 4);  // Bit 4
  result |= (b5 << 5);  // Bit 5
  result |= (b6 << 6);  // Bit 6
  result |= (b7 << 7);  // Bit 7
  
  return result;
}

void loadOrSave(byte fireButton,byte selectWaveformLFO){
  selectedFireLed = fireButton; // LED ansteuern
  // Shifttaste 1 gedrückt : Bank wechseln, aber nicht sofort JSON laden
  if(selectWaveformLFO == 0){
    if(shiftState == 1){ 
    bank = fireButton; // Bank schreiben
    selectedFireLed = bank;
    resetShiftState();
    }else{
      // virtuelle Taste ausrechnen 1..16
      byte buttonName = (bank * 4) - 4 + fireButton;

      String fileName = "fire"+String(buttonName)+".json";

      // LongPress: kein retriggern der LFO und Env., nur Ton Stoppen
      if(longPressDetected == 0){
        resetLFOParams();
      }
      // 
      if(shiftState == 2){ // Shifttaste 2 gedrückt : save
        // Save values
        String _json = values2JSON();
        debug("Write");
        dataSaved = writeSettings(_json,fileName);
      } else {
        
        // longPressDetected: wenn Ton gerade gehalten wird sollen die Werte nicht neu geladen werden bei drücken des selben Buttons
        if( ((fireButton != actualFireButtonBak) || (bankBak != bank)) && (longPressDetected == 0)){ 
          // load Values
          String _json = readSettings(fileName);
          JSON2values(_json);
          setChangeState(0,0,0,0);
        }
        bankBak = bank;
      }
    }
  }else{
    // wenn einer der LFO-Taster gedrückt wurde, muss der jeweiilige LFO umgeschalten werden
    
    valLfoWaveformSwitch = fireButton;

    if(valLfoWaveformSwitchBak != valLfoWaveformSwitch){
      lfo1WaveformChanged = 1;
    }
    valLfoWaveformSwitchBak = valLfoWaveformSwitch;
  }
  
  return;
}

void updateKeys(){

  shift1.update();
  shift2.update();

  fire1.update();
  fire2.update();
  fire3.update();
  fire4.update();

  selectWaveformLFO1.update();
  selectWaveformLFO2.update();


    if (shift1.fell()) {
      dataSaved = false;
      //selectedFireLed = bank;
    }
    if (shift2.fell()) {
      dataSaved = false;
    }

  /*
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
  */

/*
  if(shiftToggleStateBak != shiftToggleState){
    // Flags zurücksetzen, dass die Potis gewackelt haben, sonst springen die Einstellungen sofort auf die neuen Werte
    setChangeState(0,0,0,0);
  }
  shiftToggleStateBak = shiftToggleState;
*/  
  // Button LFO1 pos. Flanke
  if(selectWaveformLFO1.fell()){
    modSelect = 0;
  }
  // Button LFO2 pos. Flanke
  if(selectWaveformLFO2.fell()){
    modSelect = 1;
  }
  // Button Bank Select (Shift1) neg.Flanke
  if (shift1.rose()){
    if(!dataSaved){modSelect = 2;}
  }

  

  selectedFireLedState = 0;
  shiftState = combineBoolsToByte(!shift1.read(),!shift2.read(),0,0,0,0,0,0);
  //shiftStateToggle = combineBoolsToByte(shiftToggleState1,shiftToggleState2,0,0,0,0,0,0);
  if(shiftState == 1){
    // Fire-Leds sollen blinken
    selectedFireLed = bank;
    selectedFireLedState = blinkState;
  }else if (shiftState == 2){

  }else if (shiftState == 3){

  }

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

/*
  valLfoWaveformSwitch = combineBoolsToByte(digitalRead(waveFormPin_0),digitalRead(waveFormPin_1));
  if(valLfoWaveformSwitchBak != valLfoWaveformSwitch){
    lfo1WaveformChanged = 1;
  }
  valLfoWaveformSwitchBak = valLfoWaveformSwitch;
*/  

  // zum Setzen des Byte lfoSelectState um den LFO zu wählen
  //bool selectWaveformLFOpressed = (!selectWaveformLFO1.read())||(!selectWaveformLFO2.read());
  byte selectWaveformLFO = combineBoolsToByte(!selectWaveformLFO1.read(),!selectWaveformLFO2.read(),0,0,0,0,0,0);
  /*
  if(selectWaveformLFOpressed){
    
    if (fire1.fell()){
      valLfoWaveformSwitch = 1;
    }
    if (fire2.fell()){
      valLfoWaveformSwitch = 2;
    }
    if (fire3.fell()){
      valLfoWaveformSwitch = 3;
    }
    if (fire4.fell()){
      valLfoWaveformSwitch = 4;
    }
    // Kippschalter emulieren
    if(valLfoWaveformSwitchBak != valLfoWaveformSwitch){
      lfo1WaveformChanged = 1;
    }
    valLfoWaveformSwitchBak = valLfoWaveformSwitch;

  }else
  */
  //{
    // steigende Flanke (Taster gedrückt)
    if (fire1.fell()){
      actualFireButton = 1;
      //firePressedTime = millis();  // Zeit des Tastendrucks speichern
      if(!fire2.read() || !fire3.read() ||!fire4.read()){
        longPressDetected = true;
      }else{
        loadOrSave(actualFireButton,selectWaveformLFO);
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
        loadOrSave(actualFireButton,selectWaveformLFO);
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
        loadOrSave(actualFireButton,selectWaveformLFO);
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
        loadOrSave(actualFireButton,selectWaveformLFO);
        longPressDetected = false;     // Reset des Langdruck-Flags
      }
    //}

    if (fire1.rose()){
      firePressedTime = 0;
    }
    if (fire2.rose()){
      firePressedTime = 0;
    }
    if (fire3.rose()){
      firePressedTime = 0;
    }
    if (fire4.rose()){
      firePressedTime = 0;
    }

    
  }
  
  
  
  // AnyfireButton = Tonerzeugung
  // Bedingungen: 
  // Firebutton gedrückt
  // keine Shifttaste
  // nicht nach einem Bankwechsel, sost dudelt es gleich los
  bool anyFireButtonPressed = (
                                (
                                  (fire1.read() == LOW)||
                                  (fire2.read() == LOW)||
                                  (fire3.read() == LOW)||
                                  (fire4.read() == LOW)
                                )&&(
                                  shiftState != 1
                                )&&(
                                  shiftState != 2
                                )&&(
                                  selectWaveformLFO == 0
                                )&&(
                                  bankBak == bank
                                )
                              );
/*
  if (anyFireButtonPressed && !longPressDetected) {
    if (millis() - firePressedTime >= LONG_PRESS_DURATION) {
      // longPressDetected = true;  // Markiere, dass der lange Druck erkannt wurde
      // Serial.println("LongPress detected!");
    }
  }
*/  
  
  
/*
  if(selectWaveform.read() == LOW){
    debugString = "Waveform selected";
  }else{
    debugString = "";
  }
*/

  
  actualFireButtonBak = actualFireButton;
  
  // globale Variable rundsound = wenn high, wird ein Ton abgespielt
  runSound = (anyFireButtonPressed || longPressDetected);
  selectedFireLedState = runSound | selectedFireLedState;
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

void blink(int interval) {
  // Speichere die aktuelle Zeit
  unsigned long currentMillis = millis();

  // Prüfe, ob das Intervall abgelaufen ist
  if (currentMillis - previousMillisLED >= interval) {
    // Speichere den aktuellen Zeitpunkt als letzten Zeitstempel
    previousMillisLED = currentMillis;

    // Ändere den LED-Zustand
    blinkState = !blinkState;
  }
}

void controlFireLed(){
  digitalWrite(LEDFire1,(selectedFireLedState && selectedFireLed == 1));
  digitalWrite(LEDFire2,(selectedFireLedState && selectedFireLed == 2));
  digitalWrite(LEDFire3,(selectedFireLedState && selectedFireLed == 3));
  digitalWrite(LEDFire4,(selectedFireLedState && selectedFireLed == 4));
}

void ledControl(){
  
  uint slice_num_led_green = pwm_gpio_to_slice_num(LED1_green);
  uint slice_num_led_red = pwm_gpio_to_slice_num(LED1_red);

  uint slice_num_led2_green = pwm_gpio_to_slice_num(LED2_green);
  uint slice_num_led2_red = pwm_gpio_to_slice_num(LED2_red);

  // LED Grün
  /*
  if(shiftToggleState1 == 0){
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_red), 0);
    if(lfovalue_finalLFO1 == -100){
      pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green), 0);
    }else{
      pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green),lfovalue_finalLFO1 * pwm_led);
      //pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red),lfovalue_finalLFO2 * pwm_led);
    }
  }else{
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green), 0);
    pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red),lfovalue_finalLFO2 * pwm_led);
  }
  */
  
  pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red), 0);
  pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green), 0);
  pwm_set_chan_level(slice_num_led2_red, pwm_gpio_to_channel(LED2_red), 0);
  pwm_set_chan_level(slice_num_led2_green, pwm_gpio_to_channel(LED2_green), 0);

  if(lfovalue_finalLFO1 == -100){
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green), 0);
  }else{
    if(modSelect == 0){
      pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red),lfovalue_finalLFO1 * (pwm_led/8));
    }else{
      
    }
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green),lfovalue_finalLFO1 * pwm_led);
  }

  if(modSelect == 1){
    pwm_set_chan_level(slice_num_led2_red, pwm_gpio_to_channel(LED2_red),lfovalue_finalLFO2 * (pwm_led/8));
  }else{
    
  }
  pwm_set_chan_level(slice_num_led2_green, pwm_gpio_to_channel(LED2_green),lfovalue_finalLFO2 * pwm_led);

  //
  // LEDs
  digitalWrite(LEDShift1,modSelect == 2);
  //digitalWrite(LEDShift1, shiftToggleState1);
  //digitalWrite(LEDShift2, shiftToggleState2);
  digitalWrite(LED_BUILTIN, blinkState);

  controlFireLed();
}


String extractArgument(String inputString){
  // Position der ersten Klammer '(' und der schließenden Klammer ')'
  int startIndex = inputString.indexOf('(');
  int endIndex = inputString.indexOf(')');

  // Überprüfen, ob beide Klammern vorhanden sind
  if (startIndex != -1 && endIndex != -1 && endIndex > startIndex) {
    // Extrahiere den Teilstring zwischen den Klammern
    String extractedString = inputString.substring(startIndex + 1, endIndex);
    return extractedString;
    // Serial.println("Extrahierter Teilstring: " + extractedString);
  } else {
    return "";
  }
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

    if(receiveStr.substring(0, 4) == "test"){
      // extractArgument();
      // Serial.println(extractArgument(receiveStr));
      setDefaultDataSet();
      //  deleteSettings(String configFile)
    }
    if(receiveStr.substring(0, 10) == "deleteFile"){
      // extractArgument();
       Serial.println("DELETE "+extractArgument(receiveStr));
       deleteSettings(extractArgument(receiveStr));
    }

    if(receiveStr.substring(0, 4) == "load"){
      // Beispiel: load({"pitch":1000,"lfoFreq":10,"lfoAmount":50,"lfo2Freq":5,"lfo2Amount":50,"envTime":2,"envAmount":0,"waveform":0,"waveform2":0,"dutyCycle":50}) 
      
      String _json = extractArgument(receiveStr);
      Serial.println(_json);
      JSON2values(_json);
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
  uint slice_num_led2_green = pwm_gpio_to_slice_num(LED2_green);
  uint slice_num_led2_red = pwm_gpio_to_slice_num(LED2_green);
  
  gpio_set_function(LED1_green, GPIO_FUNC_PWM);
  gpio_set_function(LED1_red, GPIO_FUNC_PWM);
  gpio_set_function(LED2_green, GPIO_FUNC_PWM);
  gpio_set_function(LED2_red, GPIO_FUNC_PWM);
  
  pwm_set_clkdiv(slice_num_led_green, 128.f);
  pwm_set_clkdiv(slice_num_led_red, 128.f);
  pwm_set_clkdiv(slice_num_led2_green, 128.f);
  pwm_set_clkdiv(slice_num_led2_red, 128.f);
  
  pwm_set_enabled(slice_num_led_green, true);
  pwm_set_enabled(slice_num_led_red, true);
  pwm_set_enabled(slice_num_led2_green, true);
  pwm_set_enabled(slice_num_led2_red, true);
  
  pwm_set_wrap(slice_num_led_green, pwm_led);
  pwm_set_wrap(slice_num_led_red, pwm_led);
  pwm_set_wrap(slice_num_led2_green, pwm_led);
  pwm_set_wrap(slice_num_led2_red, pwm_led);
  
  pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green), 0);
  pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red), 0);
  pwm_set_chan_level(slice_num_led2_green, pwm_gpio_to_channel(LED2_green), 0);
  pwm_set_chan_level(slice_num_led2_red, pwm_gpio_to_channel(LED2_red), 0);
  

  // Normale IO's
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(LEDShift1, OUTPUT);
  pinMode(LEDShift2, OUTPUT);

  pinMode(LEDFire1, OUTPUT);
  pinMode(LEDFire2, OUTPUT);
  pinMode(LEDFire3, OUTPUT);
  pinMode(LEDFire4, OUTPUT);

  // pinMode(selectWaveformPin, INPUT_PULLUP); 
  pinMode(shiftPin1, INPUT_PULLUP);  
  pinMode(shiftPin2, INPUT_PULLUP);
  pinMode(selectWaveFormLFO1Pin, INPUT_PULLUP);  
  pinMode(selectWaveFormLFO2Pin, INPUT_PULLUP);
  pinMode(firePin1, INPUT_PULLUP); 
  pinMode(firePin2, INPUT_PULLUP); 
  pinMode(firePin3, INPUT_PULLUP); 
  pinMode(firePin4, INPUT_PULLUP); 

  // Bounce-Objekt initialisieren
  //shiftToggle.attach(shiftPin1);
  //shiftToggle.interval(50);  // Entprellintervall in Millisekunden (50 ms)

  selectWaveformLFO1.attach(selectWaveFormLFO1Pin);
  selectWaveformLFO1.interval(50);

  selectWaveformLFO2.attach(selectWaveFormLFO2Pin);
  selectWaveformLFO2.interval(50);

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
  
  // pinMode(waveFormPin_0, INPUT_PULLUP);
  // pinMode(waveFormPin_1, INPUT_PULLUP);
  // pinMode(waveFormFunctionPin, INPUT_PULLUP);
  
  // Initialisieren des Arrays für die Mittelwertbildung des Pitch
  /*
  for (int i = 0; i < numReadings; i++) {
    pitchReadings[i] = 0;
  }
 */ 
  if (!LittleFS.begin()) {
      debug("LittleFS mount failed");
      return;
   }

  String _json1 = readSettings("fire1.json");
  JSON2values(_json1);

  
  
}

void loop() {
  float envelope = 1;
  float lfo1ValueActual = 1;
  float lfo2ValueActual = 1;
  updateKeys();
  updatePotis();
  readSerial();


  if(modSelect == 0){
    ///////////////////////////// LFO1 ///////////////////////////
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
    
    // Pitch
    if(potiPitchChanged == 1){
      int freqLin = map(valPotiPitch, 4, 1023, minVal, maxVal );
      baseFrequency =  linearToLogarithmic(freqLin, minVal, maxVal);
      valPotiPitchBak = valPotiPitch;
    }
    // Frequenz
    if(potiFreqLFOChanged == 1){
      lfo1Frequency = mapFloat(valPotiFreqLFO, 5, 1023, 0.5, 50);   // LFO-Frequenzbereich von 0.5 Hz bis 50 Hz
      valPotiFreqLFOBak = valPotiFreqLFO;
    }
    // Amount
    if(potiAmpLFOChanged == 1){
      lfo1Amplitude = map(valPotiAmpLFO, 5, 1023, -100, 100);   // LFO-Amplitudenbereich von 0 bis 100, 50 Mitte
      valPotiAmpLFOBak = valPotiAmpLFO;
    }
  } else if(modSelect == 1){
    ///////////////////////////// LFO2 ///////////////////////////
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

    // Pitch
    if(potiPitchChanged == 1){
      int freqLin = map(valPotiPitch, 4, 1023, minVal, maxVal );
      baseFrequency =  linearToLogarithmic(freqLin, minVal, maxVal);
      valPotiPitchBak = valPotiPitch;
    }
    // Frequenz
    if(potiFreqLFOChanged == 1){
      lfo2Frequency = mapFloat(valPotiFreqLFO, 5, 1023, 0.1, 20);
      valPotiFreqLFOBak = valPotiFreqLFO;
    }
     // Amount
    if(potiAmpLFOChanged == 1){
      lfo2Amplitude = map(valPotiAmpLFO, 5, 1023, -100, 100);
      valPotiAmpLFOBak = valPotiAmpLFO;
    }
  }  else if(modSelect == 2) {
    
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
    
    // Pitch-Poti = Duty-Cycle 
    if(potiPitchChanged == 1){
      duty = map(valPotiPitch, 4, 1023, 5, 50 );
      valPotiPitchBak = valPotiPitch;
    }
    // LFO-Frequenz-Poti = Geschwindigkeit Envelope
    if(potiFreqLFOChanged == 1){
      envelopeDuration = mapFloat(valPotiFreqLFO, 5, 1023, 1, 10);
      valPotiFreqLFOBak = valPotiFreqLFO;
    }
     // Amount-Poti = Amount Envelope auf LFO-Frequenz
    if(potiAmpLFOChanged == 1){
      envelopeAmplitude = map(valPotiAmpLFO, 5, 1023, -100, 100);
      valPotiAmpLFOBak = valPotiAmpLFO;
    }
  }
  // beide Shift-Tasten zugleich gedrückt, Reset LFO2 Werte
  if(shiftState == 3){
    //envelopeAmplitude = 0;
    lfo2Amplitude = 0;
    envelopeAmplitude = 0;
    resetShiftState();
  }

   // Modulierte Frequenz berechnen
  // float envelopeLin = calculateEnvelope(envelopeDuration,envelopeAmplitude);
  // envelope = linearToLogarithmic(envelopeLin,0.1,1.9);
  envelope = calculateEnvelope(envelopeDuration,envelopeAmplitude);
  lfo1ValueActual = calculateLFOWave1(lfo1Frequency * (envelope), lfo1Amplitude);
  lfo2ValueActual = calculateLFOWave2(lfo2Frequency * (envelope), lfo2Amplitude);
   
   float newModulatedFrequency = 0;
   if(lfo1ValueActual == -100){
    newModulatedFrequency = lfo1ValueActual;
   }else{
    newModulatedFrequency = baseFrequency * lfo1ValueActual * lfo2ValueActual;
   }
   // Reset LFO values if the start button is false
  
  // Betriebsanzeige
  blink(100);
  
  // Create Tone
  playSound(newModulatedFrequency);
  // Control LED's
  ledControl();


  // Überwachung und Debugprints
  if(chkLoop(5000)){
     //debug("ShiftstateToggle: "+String(shiftStateToggle)+" Shiftstate: "+String(shiftState));
     //debug("Env Duration: "+String(envelopeDuration)+" Env Amount: "+String(envelopeAmplitude)+"Env Value: "+String(envelope));
     //debug("LFO1: "+String(previousMillis)+" LFO2: "+String(previousMillisLFO2));
     //debugFloat(newModulatedFrequency);
     //debug("Bank: "+String(bank)+"Firebutton: "+String(actualFireButton));
    // debug(debugString);
    debug(String(shiftState));
  }

}
