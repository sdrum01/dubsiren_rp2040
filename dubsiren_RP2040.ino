#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "math.h"
#include "LittleFS.h"
#include "Bounce2.h"
#include "ArduinoJson.h"
#include "Wire.h"
#include "GyverOLED.h"

// OLED Display
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;

// #define LONG_PRESS_DURATION 3000

// Version
#define VER "1.2"

const byte LOGLEVEL = 1;

// Definition der IO's
const int freqPotPin = A0;
const int lfoFreqPotPin = A1;  // Potentiometer für die Frequenz des LFO
const int lfoAmpPotPin = A2;   // Potentiometer für die Amplitude des LFO

const int wave_outputPin = 5; // Pin, an dem der Rechteckton ausgegeben wird
const int wave_mutePin = 22; // Pin zum Muten des Ausgangssignals


const int shiftPin1 = 6;  // shift-Button : Envelope / Bank Select (during hold)
const int shiftPin2 = 7;  // SAVE Button
const int selectWaveFormPin = 8; // WaveForm Select Button
const int selectLFOPin = 9;  // Togglebutton LFO Source / Toggle Special Flags (during hold)


const int firePin1 = 18;  // Steuerpin für die Tonaktivierung1
const int firePin2 = 19;  // Steuerpin für die Tonaktivierung2
const int firePin3 = 20;  // Steuerpin für die Tonaktivierung3
const int firePin4 = 21;  // Steuerpin für die Tonaktivierung4


// LED 
const int LEDLfo1 = 17; 
const int LEDLfo2 = 16;
const int LEDShift1 = 14;
const int LEDSave = 15;

const int LEDFire1 = 10;
const int LEDFire2 = 11;
const int LEDFire3 = 12;
const int LEDFire4 = 13;

const int LEDWaveSquare = 2;
const int LEDWaveTri = 3;
const int LEDWaveSaw = 4;


// Variablen/Konstanten for display
unsigned long previousMillisDisplay = 0;  

const int OLED_GRAPH_X_MIN = 1;
const int OLED_GRAPH_X_MAX = 126;
const int OLED_GRAPH_Y_MIN = 12;
const int OLED_GRAPH_Y_MAX = 54;

float waveformDisplay[128];

byte activeLedRow = 0;
byte activeLedCol = 0;

// Bounce Objekte erstellen zur Tastenabfrage
Bounce fire1;
Bounce fire2;
Bounce fire3;
Bounce fire4;
Bounce selectLFO;
Bounce shift1;
Bounce shift2;
Bounce selectWaveForm;


byte actualFireButton = 0;
byte actualFireButtonBak = 0;
byte lastFireButtonPressed = 0;

// 4 Bänke * 4 FireButtons = 16 Speicherplätze;
byte bank = 1;
byte bankBak = 1;

// Zeitmessung, ob die Firebutton lange gedrückt sind
unsigned long firePressedTime = 0;
bool longPressDetected = false;

// globale V. für Schalter für WaveForm, wo der Wert reinwandert, egal ob per Taster oder Kippschalter
byte valLfoWaveformSwitch = 0;
// temporärer  Wertes des Waveformschalters zum detektieren der Statusänderung
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

// die aktuellen Werte für Envelope und LFO
float envelope = 1;
float lfo1ValueActual = 1;
float lfo2ValueActual = 1;

// die finale LFO WaveForm Variable, die durch Schalter oder Speicher gesetzt wird
enum eLfoWaveform { SQUARE, TRIANGLE, SAWTOOTH };
eLfoWaveform lfo1Waveform = TRIANGLE;  //
eLfoWaveform lfo2Waveform = SQUARE;  //

bool dataSaved = false;

// globale Variablen, die der LFO und envelope zurückgibt

volatile float lfo1Value = 0;
volatile float lfo2Value = 0;
volatile float envelopeValue = 1;
volatile int lfo1Direction = 1;  // Richtung des LFO: 1 aufwärts, -1 abwärts
volatile int lfo2Direction = 1;  // Richtung des LFO: 1 aufwärts, -1 abwärts

volatile unsigned long previousMillisLFO1 = 0;
volatile unsigned long previousMillisLFO2 = 0;
volatile unsigned long previousMillisEnv = 0;
volatile unsigned long previousMillisLED = 0;
// volatile unsigned long previousMillisLEDPoll = 0;

volatile float lfovalue_finalLFO1 = 0;
volatile float lfovalue_finalLFO2 = 0;


// Wenn Ton abgefeuert werden soll:
bool runSound = 1;

// shiftToggle-taste
bool lfoToggleState = 0;
bool shiftToggleStateBak = 0;

// shiftState: Byte, in dem eine Kombination aus Shift1 und Shift2 gespeichert wird
byte shiftState = 0;
byte shiftStateBak = 0;

// Modulationsselektor: 0:LFO1, 1: LFO2, 2:Envelope
// wird gesetzt bei pos. Flange LFO1 oder LFO2 oder neg.Flange Shift1
byte modSelect = 0; 
byte modSelectBak = 0;


bool blinkState = 0;
bool blinkState_slow = 0;
byte blinkCounter1 = 0;

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

// Soft-Start gegen Knacken beim Tonbeginn
const float SOFT_START_DUTY = 2.0;            // Start-Duty in %
const unsigned long SOFT_START_US = 15000;    // Dauer der Rampe in µs (xx ms)
bool toneActive = false;                      // läuft der Ton gerade?
unsigned long toneStartMicros = 0;

//  Flags zum ein/Ausschalten von diversen Optionen

byte optionFlags = 0b0000000;  // binär x,x,x,x,x,x,x,Retrigger LFO2
// 0b00000001 : Restart LFO2 on Firepress
// 0b00000010 : Synchronize LFO1 + LFO2
// 0b00000100 : LFO1 Factor Double Amp

// wie viel Prozent darf sich das Poti ändern, bis ein wert als geändert gilt?
const int potiTolerance = 10;

// Flags, ob sich ein Wert geändert hat
bool potiPitchChanged = false;
bool potiFreqLFOChanged = false;
bool potiAmpLFOChanged = false;

// Letzte Werte ausschließlich für die OLED-Bewegungserkennung
int oledPotiPitchBak = 0;
int oledPotiFreqLFOBak = 0;
int oledPotiAmpLFOBak = 0;


// Display-Refresh
bool displayDirty = true;                         // true = beim Start einmal zeichnen
bool displayForce = false;                        // Erzwingen des Display-refresh
unsigned long displaySettleTime = 0;              // Zeitpunkt für den "Nachlauf"-Refresh
const unsigned long OLED_UPDATE_INTERVAL = 100;   // Mindestabstand zwischen zwei Redraws
const unsigned long OLED_SETTLE_DELAY    = 200;   // Nachlauf nach der letzten Potibewegung

bool lfo1WaveformChanged = false;

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
  dataSet["optionFlags"]  = optionFlags;

  // Erstelle einen JSON-String
  String jsonString;
  serializeJson(dataSet, jsonString);

  // JSON-String ausgeben
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
  optionFlags = dataSet["optionFlags"];
}

String readSettings(String configFile){
    // Daten lesen
    String value;
    File file = LittleFS.open(configFile, "r");
    if (file) {
        value = file.readStringUntil('\n');
        //Serial.println("read file "+configFile+": "+value);
        file.close();
    } else {
        //Serial.println("Error during reading of file "+configFile);
        setDefaultDataSet();
        String _json = values2JSON();
        dataSaved = writeSettings(_json,configFile);
    }
  return(value);
}

bool writeSettings(String _json, String configFile){
  // Daten schreiben
  File file = LittleFS.open(configFile, "w");
  if (file) {
      // String in die Datei schreiben
      file.println(_json);  // Schreibt den String und fügt einen Zeilenumbruch hinzu
      file.close();
      return(true);
  } else {
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

  //int rasterpercentage = percentage ;
  
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

// Generators
float calculateEnvelope(float envelopeDuration, float envelopeAmplitude) {

  unsigned long currentMillis = millis();

  // Tatsächlich vergangene Zeit
  float elapsedMillis = currentMillis - previousMillisEnv;
  previousMillisEnv = currentMillis;

  // Geschwindigkeit des Envelope
  float schrittweite_envelope =
    envelopeDuration * elapsedMillis / 5000.0;


  // Envelope-Wert verringern
  envelopeValue -= schrittweite_envelope;

  if (envelopeValue <= 0.0) {
    envelopeValue = 0.0;
  }


  float envelopeValueLog =
    linearToLogarithmic(envelopeValue, 0.0, 1.0);

  float envelope =
    ((envelopeValue - 0.5) * 1.9) *
    (envelopeAmplitude / 100) + 1;

  return envelope;
}


float calculateLFOWave1(float lfoFrequency, float amplitude) {

  unsigned long currentMillis = millis();

  // Tatsächlich vergangene Zeit seit dem letzten LFO-Update
  float elapsedMillis = currentMillis - previousMillisLFO1;
  previousMillisLFO1 = currentMillis;

  // LFO-Schritt entsprechend der tatsächlich vergangenen Zeit
  float schrittweite = lfoFrequency * elapsedMillis / 1000.0;

  switch (lfo1Waveform) {

    case SQUARE:
      // LFO intern als Dreieck laufen lassen
      lfo1Value += schrittweite * lfo1Direction;

      // Auch bei größeren Zeitsprüngen korrekt über die Grenzen gehen
     while (lfo1Value > 1.0 || lfo1Value < 0.0) {
      if (lfo1Value > 1.0) {
        lfo1Value = 2.0 - lfo1Value;
        lfo1Direction = -1;
      }

      if (lfo1Value < 0.0) {
        lfo1Value = -lfo1Value;
        lfo1Direction = 1;
      }
    }
      
      if(amplitude == -100){
        lfovalue_finalLFO1 = (lfo1Value <= 0.5) ? 0.5 : -100; // -100 = muting

      } else {

        if(amplitude < 0){

          // 3-er Raster
          if(lfo1Value < 0.25){
            lfovalue_finalLFO1 = 0;
          }else if(lfo1Value < 0.75){
            lfovalue_finalLFO1 = 0.5;
          }else {
            lfovalue_finalLFO1 = 1;
          }

        }else{
          lfovalue_finalLFO1 = (lfo1Value >= 0.5) ? 1 : 0;
        }
      }

      break;
    case TRIANGLE:

      lfo1Value += schrittweite * lfo1Direction;

      // Auch bei größeren Zeitsprüngen korrekt über die Grenzen gehen

      while (lfo1Value > 1.0 || lfo1Value < 0.0) {
          if (lfo1Value > 1.0) {
              lfo1Value = 2.0 - lfo1Value;
              lfo1Direction = -1;
              //lfo1PeriodenCounter++;
          }

          if (lfo1Value < 0.0) {
              lfo1Value = -lfo1Value;
              lfo1Direction = 1;
              //lfo1PeriodenCounter++;
          }
      }

      lfovalue_finalLFO1 = lfo1Value;
      break;

    case SAWTOOTH:

      lfo1Value -= schrittweite / 2;

      // Auch bei größeren Zeitsprüngen korrekt über den Anfang springen
      while (lfo1Value <= 0) {
        lfo1Value += 1.0;
      }

      lfovalue_finalLFO1 = lfo1Value;

      break;
  }


  // lfovalue_finalLFO1 liegt zwischen 0..1:
  // Verschieben der Mitte auf den Nullpunkt
  // Skalierung mit der LFO-Amplitude
  // Verschieben des Nullpunktes auf 1:
  // Ergebnis kann als Frequenz-Multiplikator verwendet werden

  float lfovalue_final1 = 0;

  if(lfovalue_finalLFO1 == -100){

    // negativ für Sonderfunktionen wie Muting
    lfovalue_final1 = lfovalue_finalLFO1;

  }else{

    lfovalue_final1 =
      ( ((lfovalue_finalLFO1 - 0.5) * 1.5) *
        ((amplitude / 100) *
        (optionFlags & 0b00000100 ? 2 : 1))
      ) + 1.0;
  }

  return(lfovalue_final1);
}

float calculateLFOWave2(float frequency, float amplitude) {

  unsigned long currentMillis = millis();

  // Tatsächlich vergangene Zeit seit dem letzten LFO-Update
  float elapsedMillis = currentMillis - previousMillisLFO2;
  previousMillisLFO2 = currentMillis;

  // LFO-Schritt entsprechend der tatsächlich vergangenen Zeit
  float schrittweite = frequency * elapsedMillis / 1000.0;


  switch (lfo2Waveform) {

    case SQUARE:

      lfo2Value += schrittweite * lfo2Direction;

      // Grenzüberschreitung korrigieren
      while (lfo2Value > 1.0 || lfo2Value < 0.0) {
        if (lfo2Value > 1.0) {
          lfo2Value = 2.0 - lfo2Value;
          lfo2Direction = -1;
        }

        if (lfo2Value < 0.0) {
          lfo2Value = -lfo2Value;
          lfo2Direction = 1;
        }
      }

      lfovalue_finalLFO2 = (lfo2Value >= 0.5) ? 1 : 0;
      break;

    case TRIANGLE:

      lfo2Value += schrittweite * lfo2Direction;

      // Grenzüberschreitung korrigieren
      while (lfo2Value > 1.0 || lfo2Value < 0.0) {
        if (lfo2Value > 1.0) {
          lfo2Value = 2.0 - lfo2Value;
          lfo2Direction = -1;
        }

        if (lfo2Value < 0.0) {
          lfo2Value = -lfo2Value;
          lfo2Direction = 1;
        }
      }

      lfovalue_finalLFO2 = lfo2Value;

      break;


    case SAWTOOTH:

      lfo2Value -= schrittweite / 2;

      while (lfo2Value <= 0) {
        lfo2Value += 1.0;
      }

      lfovalue_finalLFO2 = lfo2Value;

      break;
  }


  // LFO-Wert auf Frequenzmultiplikator umrechnen

  float lfovalue_final1 =
    (((lfovalue_finalLFO2 - 0.5) * 1.5) *
     (amplitude / 100)) + 1.0;

  return(lfovalue_final1);
}

// LFO2-Frequenz: bei Flag 2 auf das nächste ganzzahlige Vielfache von LFO1 einrasten
float getLfo2Frequency() {
  if (!(optionFlags & 0b00000010)) return lfo2Frequency;   // Flag aus: freie Frequenz
  if (lfo1Frequency <= 0.0) return lfo2Frequency;          // Schutz gegen Division durch 0

  float n = roundf(lfo2Frequency / lfo1Frequency);         // nächstes Vielfaches
  if (n < 1.0) n = 1.0;                                    // mindestens 1x LFO1
  return n * lfo1Frequency;
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float setFrequency(float freq){
  if(freq < minValMod){freq = minValMod;}
  if(freq > maxValMod){freq = maxValMod;}
  //return(5140 / (freq/1000));
  return(2070 / (freq/1000));
  //return(1035 / (freq/1000));
}

void muteSound(bool mute){
  digitalWrite(wave_mutePin, mute);
}

// Hilfsfunktion für Rampe beim Tonstart, um Knacksen zu vermeiden
float getSoftStartDuty() {
  if (!toneActive) {              // erster Aufruf nach einer Pause = Tonbeginn
    toneActive = true;
    toneStartMicros = micros();
  }

  unsigned long elapsed = micros() - toneStartMicros;
  if (elapsed >= SOFT_START_US) return duty;      // Rampe fertig: Zielwert

  // Ist der gespeicherte Duty kleiner als der Startwert, nimm den kleineren
  float startDuty = (duty < SOFT_START_DUTY) ? duty : SOFT_START_DUTY;

  // Lineare Rampe von startDuty bis duty
  return startDuty + (duty - startDuty) * (float)elapsed / (float)SOFT_START_US;
}

// Tonerzeugung
void playSound(float freqVal){
  // 4140 = 500hz; 2070 = 1khz; 1035 = 2khz; 
 
  float pwm_val = setFrequency(freqVal);

  // AUSGANG ALS PWM AUSGANG DEFINIEREN, weil er vorher Input sein muss wegen dem Knacks
  // Initialisiere die GPIO-Pin-Funktion für PWM Wave-Output

  // Setze die PWM-Periode
  
  uint slice_num_wave = pwm_gpio_to_slice_num(wave_outputPin);
  
  if (runSound){
    dataSaved = false;
    // Pin ist wieder PWM Ausgang
    gpio_set_function(wave_outputPin, GPIO_FUNC_PWM);

    // Setze den PWM-Teilungsverhältnis
    pwm_set_clkdiv(slice_num_wave, 64.f);
    pwm_set_wrap(slice_num_wave, pwm_val);
    // Starte den PWM-Output
    // pwm_set_enabled(slice_num_wave, true);

    

    if(freqVal == -100) {
      // Ton Stop
      muteSound(true);
      pwm_set_chan_level(slice_num_wave, pwm_gpio_to_channel(wave_outputPin), 0);
      pinMode(wave_outputPin, INPUT);
      toneActive = false;          // Rücksetzen, damit beim nächsten Tonstart die Rampe wieder beginnt
      
    }else{
      // Ton Start
      pwm_set_enabled(slice_num_wave, true);
      pwm_set_chan_level(slice_num_wave, pwm_gpio_to_channel(wave_outputPin), pwm_val * (getSoftStartDuty() / 100));
      muteSound(false);
    }
  }else{
    // Ton Stop
    muteSound(true);
    pwm_set_chan_level(slice_num_wave, pwm_gpio_to_channel(wave_outputPin), 0);
    pinMode(wave_outputPin, INPUT);
    toneActive = false; // Rücksetzen, damit beim nächsten Tonstart die Rampe wieder beginnt
  }
}





void setChangeState(bool p1, bool p2, bool p3, bool s1){
  potiPitchChanged = p1;
  potiFreqLFOChanged = p2;
  potiAmpLFOChanged = p3;
  lfo1WaveformChanged = s1;
  //debug("State Reset");
  requestDisplayUpdate();
}

bool chkLoop(int endCount){
  if(zaehler == endCount){
    zaehler = 0;
    return(true);
  }
  zaehler++;
  return(false);
}

// Setzt den LFO zurück, wenn in den Optionen definiert

void resetLFOParams(){
  unsigned long currentMillis = millis();
  lfo1Value = 0;
  lfo1Direction = 1;
 

  // Bit1: Sync LFO2
  if(optionFlags & 0b00000011){
    lfo2Value = 0;
    lfo2Direction = 1;
  }
  envelopeValue = 1;
  
  //lfo2Direction = 1;
  // mit dem aktualisieren der millisekunden wird die Hüllkurve und LFO neu gestartet
  previousMillisLFO1 = currentMillis;
  previousMillisLFO2 = currentMillis;
  previousMillisEnv = currentMillis;
}

void resetShiftState(){
  dataSaved = true;
  lfoToggleState = 0;
  //shiftToggleState2 = 0;
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

// läd oder speichert einen Datensatz oder macht bei Shiftstate Spezialfunktionen
void loadOrSave(byte fireButton){
  // kein LFO-Taster gedrückt

  // Shifttaste 1 gedrückt : Bank wechseln, aber nicht sofort JSON laden
  if(shiftState == 1){ 
    bank = fireButton; // Bank schreiben
    selectedFireLed = bank;
    //actualFireButton = 0;
    lastFireButtonPressed = 0;
    resetShiftState();
  }else if(shiftState == 4){ // SelectLFO gehalten
    // Maskieren des jeweiligen Bytes (entspricht dem Fire-Button)
    uint8_t mask = 1 << fireButton -1; // weil es bei 0 losgeht und nicht bei 1
    // Bit toggeln mit XOR
    optionFlags ^= mask;
  }else{
    // virtuelle Taste ausrechnen 1..16
    byte buttonName = (bank * 4) - 4 + fireButton;
    // den Filename bestimmen
    String fileName = "fire"+String(buttonName)+".json";

    // LongPress: kein retriggern der LFO und Env., nur Ton Stoppen
    if(longPressDetected == 0){
      resetLFOParams();
    }
    // 
    if(shiftState == 2){ // Shifttaste 2 gedrückt : save
      // Save values
      String _json = values2JSON();
      dataSaved = writeSettings(_json,fileName);
      //debug("Write");
    } else {
      
      // longPressDetected: wenn Ton gerade gehalten wird sollen die Werte nicht neu geladen werden bei drücken des selben Buttons
      if( ((fireButton != actualFireButtonBak) || (bankBak != bank)) && (longPressDetected == 0)){ 
        // load Values
        debug("Load");
        String _json = readSettings(fileName);
        JSON2values(_json);
        setChangeState(0,0,0,0);
        setWaveFormSwitch();
        requestDisplayUpdateNow();   // Display sofort aktualisieren, damit die Werte sofort sichtbar sind

        // Merker, welcher Firebutton als letztes gedrückt wurde
        
        actualFireButtonBak = fireButton;
        //debug("actualFireButtonBak="+actualFireButtonBak);
        debug(_json);
      }
      bankBak = bank;
    }
  }
  return;
}

// Wahlschalter, welche WaveForm der LFO machen soll
void enumWaveForm(){
  // 
  valLfoWaveformSwitch++;
  if( (valLfoWaveformSwitch < 0)||(valLfoWaveformSwitch > 2)){
    valLfoWaveformSwitch = 0;
  }

  lfo1WaveformChanged = true;
  //valLfoWaveformSwitchBak = valLfoWaveformSwitch;
}

void updateFireKeys(){
  // steigende Flanke (Taster gedrückt)
  if (fire1.fell()){
    actualFireButton = 1;
    //firePressedTime = millis();  // Zeit des Tastendrucks speichern
    if(!fire2.read() || !fire3.read() ||!fire4.read()){
      longPressDetected = true;
    }else{
      // loadOrSave(actualFireButton,selectWaveformLFO);
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
      // loadOrSave(actualFireButton,selectWaveformLFO);
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
      // loadOrSave(actualFireButton,selectWaveformLFO);
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
      // loadOrSave(actualFireButton,selectWaveformLFO);
      loadOrSave(actualFireButton);
      longPressDetected = false;     // Reset des Langdruck-Flags
    }
    
  }

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

void updateKeys(){

  shift1.update();
  shift2.update();
  selectLFO.update();
  selectWaveForm.update();

  fire1.update();
  fire2.update();
  fire3.update();
  fire4.update();

    if (shift1.fell()) {
      dataSaved = false;
      //selectedFireLed = bank;
    }
    if (shift2.fell()) {
      dataSaved = false;
    }


 
    // fallende Flanke (Taster losgelassen)
  // LFO1 oder LFO2 wählen  
  if (selectLFO.rose()) {
    if (!dataSaved){
      lfoToggleState = !lfoToggleState;
      modSelect = lfoToggleState; // Modulationsquelle ist 0 oder 1, weil LFO-Selektor gedrückt wurde
      setWaveFormSwitch();
      setChangeState(0,0,0,0);
      
    }
  }
    
  
  // Button Bank Select (Shift1) neg.Flanke
  if (shift1.rose()){
    if(!dataSaved){
      if(modSelect == 2){
        modSelect = lfoToggleState;
      }else{
        modSelect = 2;
        // lfoToggleState = 0;
      }
      
      setChangeState(0,0,0,0);
    }
  }

  if (selectWaveForm.rose()) {
      setChangeState(0,0,0,0);
      enumWaveForm();
  }

  selectedFireLedState = 0;
  shiftState = combineBoolsToByte(!shift1.read(),!shift2.read(),!selectLFO.read(),0,0,0,0,0);
  //Shiftstate ist > 0 wenn irgendeine Shift-taste gedrückt ist, die eine 2.FUnktion während des Drückens haben soll
  if(shiftState == 1){ 
    // Shift 1 gedrückt: Bank wählen
    selectedFireLed = bank;
    selectedFireLedState = blinkState;
  }else if (shiftState == 2){
    // Shift 2 (save) gedrückt: aktueller Firebutton blinkt schnell zum Anzeigen des aktuellen Speicherplatzes
    selectedFireLed = actualFireButton;
    
    if((fire1.read() == LOW)||
      (fire2.read() == LOW)||
      (fire3.read() == LOW)||
      (fire4.read() == LOW)){
        // Sobald ein FIrebutton gedrückt, LED Dauerleuchten als Bestätigung
        selectedFireLedState = 1;
      }else{
        // nur Save gedrückt, Taste flimmert
        selectedFireLedState = blinkState;
      }
  }else if (shiftState == 4){
    // selectLFO gedrückt: Flags für zusätzliche Optionen werden sichtbar
    selectedFireLed = actualFireButton;
    selectedFireLedState = blinkState;
  } else{
    // IDLE: nur ab und zu den aktuellen FIrebutton blitzen lassen
    selectedFireLed = lastFireButtonPressed;
    selectedFireLedState = blinkState_slow;
    
  }

  updateFireKeys();

  bool soundWasRunning = runSound;

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
      shiftState == 0 ||
      (shiftState == 4 && soundWasRunning)   // LFO-Taste gehalten: laufenden Ton nicht abbrechen
    )&&(
      bankBak == bank
    )
  );

  // globale Variable rundsound = wenn high, wird ein Ton abgespielt
  runSound = (anyFireButtonPressed || longPressDetected);
  
  if((runSound)&&(!longPressDetected)){
    lastFireButtonPressed = actualFireButton;
  }

  
  if(shiftState == 0){
    
    selectedFireLed = lastFireButtonPressed;
    
  }
  selectedFireLedState = runSound | selectedFireLedState;
}

void updatePotis(){

  // --------------------------------------------------
  // Potis einlesen
  // --------------------------------------------------

  valPotiPitch   = analogRead(freqPotPin);
  valPotiFreqLFO = analogRead(lfoFreqPotPin);
  valPotiAmpLFO  = analogRead(lfoAmpPotPin);


  // --------------------------------------------------
  // OLED-Aktivierung:
  // Hat sich irgendein Poti bewegt?
  // --------------------------------------------------

  // Neu nach Claude:
  if (abs(valPotiPitch   - oledPotiPitchBak)   > potiTolerance ||
      abs(valPotiFreqLFO - oledPotiFreqLFOBak) > potiTolerance ||
      abs(valPotiAmpLFO  - oledPotiAmpLFOBak)  > potiTolerance) {

    requestDisplayUpdate();
    displaySettleTime = millis() + OLED_SETTLE_DELAY;

    oledPotiPitchBak   = valPotiPitch;
    oledPotiFreqLFOBak = valPotiFreqLFO;
    oledPotiAmpLFOBak  = valPotiAmpLFO;
  }
  

  // --------------------------------------------------
  // Poti-Erkennung für die Parameter
  // --------------------------------------------------

  if ((valPotiPitch < valPotiPitchBak - potiTolerance) ||
      (valPotiPitch > valPotiPitchBak + potiTolerance)) {
    potiPitchChanged = true;
  }


  if ((valPotiFreqLFO < valPotiFreqLFOBak - potiTolerance) ||
      (valPotiFreqLFO > valPotiFreqLFOBak + potiTolerance)) {
    potiFreqLFOChanged = true;
  }


  if ((valPotiAmpLFO < valPotiAmpLFOBak - potiTolerance) ||
      (valPotiAmpLFO > valPotiAmpLFOBak + potiTolerance)) {
    potiAmpLFOChanged = true;
  }
}

// Zyklische Timer
void blink(int interval) {
  // Speichere die aktuelle Zeit
  unsigned long currentMillis = millis();

  // Prüfe, ob das Intervall abgelaufen ist
  if (currentMillis - previousMillisLED >= interval) {
    // Speichere den aktuellen Zeitpunkt als letzten Zeitstempel
    previousMillisLED = currentMillis;

    // Ändere den LED-Zustand
    blinkState = !blinkState;
    blinkState_slow = 0;
    blinkCounter1++;
  }
  if(blinkCounter1 == 16){
    blinkCounter1 = 0;
    blinkState_slow = 1;
  }

}

void updateLEDs(){
  
  uint slice_num_led_green = pwm_gpio_to_slice_num(LEDLfo2);
  uint slice_num_led_red = pwm_gpio_to_slice_num(LEDLfo1);


  // LED Grün
  
  if(lfoToggleState == 0){
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LEDLfo1), 0);
    if(lfovalue_finalLFO1 == -100){
      pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LEDLfo2), 0);
    }else{
      pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LEDLfo2),lfovalue_finalLFO1 * pwm_led);
    }
  }else{
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LEDLfo2), 0);
    pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LEDLfo1),lfovalue_finalLFO2 * pwm_led);
  }
  
  
  // LEDs an Pin
  
  // digitalWrite(LEDShift1,modSelect == 2);
  digitalWrite(LED_BUILTIN, blinkState);

  digitalWrite(LEDShift1,modSelect == 2);
  digitalWrite(LEDSave,shiftState == 2 && blinkState);

  // Debug
  //digitalWrite(LEDSave,oledUpdateActive);
  
  digitalWrite(LEDWaveSquare,valLfoWaveformSwitch == 0);
  digitalWrite(LEDWaveTri,valLfoWaveformSwitch == 1);
  digitalWrite(LEDWaveSaw,valLfoWaveformSwitch == 2);



  if(shiftState == 4){
    digitalWrite(LEDFire1,optionFlags & 0b00000001);
    digitalWrite(LEDFire2,optionFlags & 0b00000010);
    digitalWrite(LEDFire3,optionFlags & 0b00000100);
    digitalWrite(LEDFire4,optionFlags & 0b00001000);
  }else{
    digitalWrite(LEDFire1,selectedFireLedState && selectedFireLed == 1);
    digitalWrite(LEDFire2,selectedFireLedState && selectedFireLed == 2);
    digitalWrite(LEDFire3,selectedFireLedState && selectedFireLed == 3);
    digitalWrite(LEDFire4,selectedFireLedState && selectedFireLed == 4);
  }
  

}
 

void setWaveFormSwitch(){
  // Übertragen des aktuellen LFO auf den LFO-Selekt-LED's, aber nur wenn der Schalter nicht geändert wurde
  switch (lfoToggleState) {
  case 0:
    valLfoWaveformSwitch = lfo1Waveform;
    break;
  case 1:
    valLfoWaveformSwitch = lfo2Waveform;
    break;
  }
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

    StaticJsonDocument<200> dataSet1;
    StaticJsonDocument<200> dataSet2;

    if(receiveStr == "receiveDump"){
        // Verschachtelt
      JsonObject receiveDumpDataSet = dataSet1.createNestedObject("dump");
      for (int i = 1; i <= 16; i++) {
        String _fileName = "fire"+String(i)+".json";

        String _number = String(i);
        String _json = readSettings(_fileName);
        // Deserialisiere den JSON-String
        DeserializationError error = deserializeJson(dataSet2, _json);
        //Serial.println(_fileName);
        // receiveDumpDataSet[_fileName] = dataSet2;
        receiveDumpDataSet[_number] = dataSet2;
      }
      // Erstelle einen JSON-String
      String jsonString;
      serializeJson(dataSet1, jsonString);
      Serial.println(jsonString);
    }
    
    // Universelle Funktion zum Empfangen separater Speicherbereiche 
    if (receiveStr.startsWith("receive_")) {
      int idx = receiveStr.substring(8).toInt();
      if (idx >= 1 && idx <= 16) {
        String _fileName = "fire" + String(idx) + ".json";
        String _json = readSettings(_fileName);
    
        DeserializationError error = deserializeJson(dataSet1, _json);
        if (error) {
          Serial.print("Deserialization failed: ");
          Serial.println(error.f_str());
          return;
        }
    
        String jsonString;
        serializeJson(dataSet1, jsonString);
        Serial.println(jsonString);
      } else {
        Serial.println("Index out of range (1-16)");
      }
    }
 

    if(receiveStr.substring(0, 10) == "setdefault"){
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
      
      String cmd = extractArgument(receiveStr);
      //Serial.println(_json);
      //JSON2values(_json);

      // Inhalt in den Klammern extrahieren: "1:{...}"
      int start = cmd.indexOf('(') + 1;
      int end = cmd.lastIndexOf(')');
      String inner = cmd.substring(start, end);

      // Split bei ':' → Slot | JSON
      int colon = inner.indexOf(':');
      if (colon == -1) {
        //Serial.println("Fehler: kein ':' gefunden");
        return;
      }

      String slotStr = inner.substring(0, colon);
      String jsonStr = inner.substring(colon + 1);

      String fileName = "fire"+String(slotStr)+".json";

      bool success = writeSettings(jsonStr,fileName);

      int slot = slotStr.toInt();

      Serial.println("");
      Serial.print("Saved slot ");
      Serial.print(slot);
      Serial.print(" → ");
      Serial.print(fileName);
      Serial.print(" → ");
      Serial.print(success ? "OK" : "FAIL");

    }

    if (receiveStr.startsWith("load_dump")) {
      String cmd = extractArgument(receiveStr);
    
      // Versuche, den JSON-Inhalt zu deserialisieren
      StaticJsonDocument<4096> doc; // Größe ggf. anpassen
      DeserializationError error = deserializeJson(doc, cmd);
    
      if (error) {
        Serial.print("JSON Deserialization failed: ");
        Serial.println(error.f_str());
      } else {
        JsonObject dump = doc["dump"];
        for (JsonPair kv : dump) {
          String slot = kv.key().c_str(); // z.B. "1", "2", ...
          JsonObject slotData = kv.value().as<JsonObject>();
    
          // Serialisiere das Objekt wieder zu einem String
          String jsonOut;
          serializeJson(slotData, jsonOut);
    
          String fileName = "fire" + slot + ".json";
          bool success = writeSettings(jsonOut, fileName);


          Serial.println("");
          Serial.print("Saved slot ");
          Serial.print(slot);
          Serial.print(" → ");
          Serial.print(fileName);
          Serial.print(" → ");
          Serial.print(success ? "OK" : "FAIL");
        }
      }
    }
    
    // Die Zeichenkette zurücksetzen, um eine neue zu empfangen
    receiveStr = "";
    receiveStrComplete = false;
  }
}

void setup_display(){
  // Setup Oled Display

  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  Wire.setClock(500000);

  oled.init();
  // init buffer
  for (int i = 0; i < 128; i++) {
    waveformDisplay[i] = 1.0;
  }

  oled.clear();
  oled.setCursor(0, 0);

// Außenrahmen
  oled.rect(0, 0, 127, 63, OLED_STROKE);

  // Hauptschrift
  oled.setScale(2);
  oled.setCursor(30, 2);
  oled.print("DUB-IY");

  // Versionsnummer
  oled.setScale(1);
  oled.setCursor(2, 6);
  oled.print("V");
  oled.print(VER);

  oled.update();
  delay(1000);
}

void requestDisplayUpdate() {
  displayDirty = true;
}

void requestDisplayUpdateNow() {                  // neu, eigene Funktion
  displayDirty = true;
  displayForce = true;
}

void updateDisplay() {
  unsigned long now = millis();

  // Ein letzter Refresh, kurz nachdem ein Poti stillsteht
  if (displaySettleTime && now >= displaySettleTime) {
    displaySettleTime = 0;
    displayDirty = true;
  }

  if (!displayDirty) return;                                       // nichts zu tun

  // beim Ton nicht zeichnen (I2C blockiert ca. 25 ms) ausser beim ersten Refresh nach dem Tonstart
  if (!displayForce) {
    if (runSound) return;
    if (now - previousMillisDisplay < OLED_UPDATE_INTERVAL) return;
  }

  bool wasForced = displayForce;

  displayDirty = false;
  displayForce = false;
  previousMillisDisplay = now;
  drawOLEDWaveform();
  if (wasForced) {
    unsigned long t = millis();
    previousMillisLFO1 = previousMillisLFO2 = previousMillisEnv = t;
  }
}


void drawOLEDWaveform()
{
  const int graphLeft   = 0;
  const int graphRight  = 127;
  const int graphTop    = 15;
  const int graphBottom = 60;

  const float displayCyclesLFO1 = 5.5;

  // ------------------------------------------------------------
  // Effektiven Wert eines LFO-Waveforms für die Anzeige berechnen
  // phase: 0.0 ... 1.0
  // ------------------------------------------------------------
  auto getWaveformValue = [](eLfoWaveform waveform, float phase) -> float
  {
    phase -= floor(phase);

    switch (waveform)
    {
      case SQUARE:
        //return (phase < 0.5) ? 1.0 : 0.0;
        return (phase >= 0.25 && phase < 0.75) ? 1.0 : 0.0;

      case TRIANGLE:
        if (phase < 0.5)
          return phase * 2.0;
        else
          return 2.0 - phase * 2.0;

      case SAWTOOTH:
        return 1.0 - phase;      // war: return phase;
    }

    return 0.0;
  };


  // ------------------------------------------------------------
  // Effektiven Frequenz-Multiplikator von LFO1 berechnen
  // Entspricht calculateLFOWave1()
  // ------------------------------------------------------------

  auto triangleValue = [](float phase) -> float
  {
    phase -= floor(phase);
    return (phase < 0.5) ? phase * 2.0 : 2.0 - phase * 2.0;
  };

  const float MUTE_MARK = -100.0;   // Marker für "Ton aus"

  auto getLFO1Multiplier = [&](float phase) -> float
  {
    float amplitudeFactor = lfo1Amplitude / 100.0;

    if (optionFlags & 0b00000100)
      amplitudeFactor *= 2.0;

    float rawValue;

    if (lfo1Waveform == SQUARE && lfo1Amplitude < 0)
    {
      float tri = triangleValue(phase);

      // Maximal negativ: Zerhacker, zweite Hälfte der Dreieck-Rampe = Ton aus
      if (lfo1Amplitude <= -100.0)
        return (tri <= 0.5) ? 1.0 : MUTE_MARK;

      // 3-Stufen-Raster
      if (tri < 0.25)      rawValue = 0.0;
      else if (tri < 0.75) rawValue = 0.5;
      else                 rawValue = 1.0;
    }
    else
    {
      rawValue = getWaveformValue(lfo1Waveform, phase);
    }

    return (((rawValue - 0.5) * 1.5) * amplitudeFactor) + 1.0;
  };


  // ------------------------------------------------------------
  // Effektiven Frequenz-Multiplikator von LFO2 berechnen
  // Entspricht calculateLFOWave2()
  // ------------------------------------------------------------
  auto getLFO2Multiplier = [&](float phase) -> float
  {
    float rawValue = getWaveformValue(lfo2Waveform, phase);

    float amplitudeFactor = (lfo2Amplitude / 100.0);

    return
      (((rawValue - 0.5) * 1.5) * amplitudeFactor) + 1.0;
  };


  // ------------------------------------------------------------
  // Frequenzverhältnis LFO2 zu LFO1
  //
  // Damit LFO1 genau 5 Perioden darstellt, wird LFO2
  // entsprechend seiner Frequenz mitgeführt.
  //
  // Beim SAWTOOTH wird in deinen LFO-Funktionen mit
  // schrittweite / 2 gearbeitet. Deshalb berücksichtigen
  // wir das hier ebenfalls.
  // ------------------------------------------------------------
  

  float frequencyRatio = 0.0;
  // synchronisierter LFO2-Frequenzwert für die Anzeige, damit die Darstellung passt
  float effectiveLFO2Frequency = getLfo2Frequency();

  if (lfo1Frequency > 0.0)
    //frequencyRatio = lfo2Frequency / lfo1Frequency;
    frequencyRatio = effectiveLFO2Frequency / lfo1Frequency;


  // ------------------------------------------------------------
  // Zuerst alle Werte berechnen, damit die Darstellung
  // automatisch auf den tatsächlichen Wertebereich skaliert
  // werden kann.
  // ------------------------------------------------------------
  float waveformValues[128];

  float minValue =  1000000.0;
  float maxValue = -1000000.0;

  for (int x = graphLeft; x <= graphRight; x++)
  {
    float normalizedX = (float)(x - graphLeft) /
      (float)(graphRight - graphLeft);

    // LFO1 läuft exakt 5 Perioden über die Anzeige
    float phaseLFO1 = normalizedX * displayCyclesLFO1;

    // LFO2 läuft entsprechend seines Frequenzverhältnisses
    float phaseLFO2 = phaseLFO1 * frequencyRatio;

    float lfo1Multiplier = getLFO1Multiplier(phaseLFO1);

    float lfo2Multiplier = getLFO2Multiplier(phaseLFO2);

    // Stummgeschaltet = Frequenz 0 = Linie am Boden des Graphen
    // Genau das Produkt, das auch den Frequenzgenerator beeinflusst
    float combinedValue = (lfo1Multiplier == MUTE_MARK)
                          ? MUTE_MARK
                          : lfo1Multiplier * lfo2Multiplier;

    waveformValues[x] = combinedValue;

    // min/max nur für echte Werte (werden aktuell ohnehin nicht mehr verwendet)
    if (combinedValue != MUTE_MARK) {
      if (combinedValue < minValue) minValue = combinedValue;
      if (combinedValue > maxValue) maxValue = combinedValue;
    }

  }


  // ------------------------------------------------------------
  // Display löschen
  // ------------------------------------------------------------
  oled.clear();


  // ------------------------------------------------------------
  // Kopfzeile
  // ------------------------------------------------------------
  oled.setCursor(0, 0);
  oled.print("LFO1 * LFO2");

  oled.setCursor(76, 0);
  oled.print(lfo1Frequency, 1);
  oled.print("/");

  oled.print(effectiveLFO2Frequency, 1);


  // ------------------------------------------------------------
  // Einen Rand um den Wertebereich lassen
  // ------------------------------------------------------------
  // float valueRange = maxValue - minValue;

  float displayMin = 0.0;
  float displayMax = 2.0;

  float valueRange = displayMax - displayMin;


  // ------------------------------------------------------------
  // Nulllinie / Faktor-1-Linie
  //
  // Faktor 1.0 bedeutet:
  // keine Änderung der Grundfrequenz
  // ------------------------------------------------------------
  if (1.0 >= displayMin && 1.0 <= displayMax)
  {
    int yOne =
      graphBottom -
      (int)(
        ((1.0 - displayMin) / valueRange) *
        (graphBottom - graphTop)
      );
  }


  // ------------------------------------------------------------
  // Kombinierte Wellenform zeichnen
  // ------------------------------------------------------------
  bool previousValid = false;   // gibt es einen Vorgängerpunkt, mit dem verbunden wird?
  int previousY = 0;

  for (int x = graphLeft; x <= graphRight; x++)
  {
    // Mute-Phase: nichts zeichnen, Linie unterbrechen
    if (waveformValues[x] == MUTE_MARK)
    {
      previousValid = false;
      continue;
    }

    float normalizedValue = (waveformValues[x] - displayMin) / valueRange;

    int y = graphBottom - (int)(normalizedValue * (graphBottom - graphTop));

    if (y < graphTop)    y = graphTop;
    if (y > graphBottom) y = graphBottom;

    if (previousValid)
      oled.line(x - 1, previousY, x, y);
    else
      oled.dot(x, y);            // Segmentanfang, damit auch einzelne Punkte sichtbar sind

    previousY = y;
    previousValid = true;
  }


  // ------------------------------------------------------------
  // Aktuellen tatsächlichen Multiplikator anzeigen
  // ------------------------------------------------------------
  float currentCombinedValue = lfo1ValueActual * lfo2ValueActual;
  

  oled.setCursor(0, 56);
  oled.print("x");

  oled.print(currentCombinedValue, 2);

  oled.setCursor(75, 56);
  oled.print("A:");

  oled.print(lfo1Amplitude);

  oled.print("/");

  oled.print(lfo2Amplitude);


  oled.update();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  
  if(LOGLEVEL > 0){
    Serial.begin(38400);
  }

  previousMillisLFO1 = millis();
  previousMillisLFO2 = millis();
  previousMillisEnv  = millis();

  // Initialize the display
  setup_display();

  // wave-output defaultmäßig as input, da Hochohmig schalten wegen dem knacks
  pinMode(wave_outputPin, INPUT);
  
  // LED
  uint slice_num_led_green = pwm_gpio_to_slice_num(LEDLfo2);
  uint slice_num_led_red = pwm_gpio_to_slice_num(LEDLfo2);
  
  gpio_set_function(LEDLfo2, GPIO_FUNC_PWM);
  gpio_set_function(LEDLfo1, GPIO_FUNC_PWM);

  // Setze den PWM-Teilungsverhältnis
  pwm_set_clkdiv(slice_num_led_green, 128.f);
  pwm_set_clkdiv(slice_num_led_red, 128.f);

  
  pwm_set_enabled(slice_num_led_green, true);
  pwm_set_enabled(slice_num_led_red, true);

  
  pwm_set_wrap(slice_num_led_green, pwm_led);
  pwm_set_wrap(slice_num_led_red, pwm_led);

  
  pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LEDLfo2), 0);
  pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LEDLfo1), 0);

  
  // Normale IO's
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(wave_mutePin, OUTPUT);
  digitalWrite(wave_mutePin,1);

  pinMode(LEDShift1, OUTPUT);
  pinMode(LEDSave, OUTPUT);

  pinMode(LEDFire1, OUTPUT);
  pinMode(LEDFire2, OUTPUT);
  pinMode(LEDFire3, OUTPUT);
  pinMode(LEDFire4, OUTPUT);

  pinMode(LEDWaveSquare, OUTPUT);
  pinMode(LEDWaveTri, OUTPUT);
  pinMode(LEDWaveSaw, OUTPUT);


  pinMode(selectLFOPin, INPUT_PULLUP); 
  pinMode(shiftPin1, INPUT_PULLUP);  
  pinMode(shiftPin2, INPUT_PULLUP);
  pinMode(selectWaveFormPin, INPUT_PULLUP);  

  pinMode(firePin1, INPUT_PULLUP); 
  pinMode(firePin2, INPUT_PULLUP); 
  pinMode(firePin3, INPUT_PULLUP); 
  pinMode(firePin4, INPUT_PULLUP); 

  // Bounce-Objekte initialisieren

  selectLFO.attach(selectLFOPin);
  selectLFO.interval(50);

  selectWaveForm.attach(selectWaveFormPin);
  selectWaveForm.interval(50);

  shift1.attach(shiftPin1); // Shift / Env / Bank
  shift1.interval(50);  // Entprellintervall in Millisekunden (50 ms)

  shift2.attach(shiftPin2); // Save
  shift2.interval(50);  

  fire1.attach(firePin1);
  fire1.interval(10);  

  fire2.attach(firePin2);
  fire2.interval(10);  

  fire3.attach(firePin3);
  fire3.interval(10);  

  fire4.attach(firePin4);
  fire4.interval(10);
  
  // LittleFS initialisieren und mounten
  if (!LittleFS.begin()) {
      debug("LittleFS mount failed");
      return;
   }

  String _json1 = readSettings("fire1.json");
  JSON2values(_json1);
}

void loop() {
  
  updateKeys();
  updatePotis();
  readSerial();

  ///////////////////////////// LFO1 ///////////////////////////
  if(modSelect == 0){
    
    if(lfo1WaveformChanged){
      
      switch (valLfoWaveformSwitch) {
        case 0:
          lfo1Waveform = SQUARE;
          break;
        case 1:
          lfo1Waveform = TRIANGLE;
          break;
        case 2:
          lfo1Waveform = SAWTOOTH;
          break;
      }
    }
    
    // Pitch
    if(potiPitchChanged){
      int freqLin = map(valPotiPitch, 4, 1023, minVal, maxVal );
      baseFrequency =  linearToLogarithmic(freqLin, minVal, maxVal);
      valPotiPitchBak = valPotiPitch;
    }
    // Frequenz
    if(potiFreqLFOChanged){
      lfo1Frequency = mapFloat(valPotiFreqLFO, 5, 1023, 0.5, 50);   // LFO-Frequenzbereich von 0.5 Hz bis 50 Hz
      valPotiFreqLFOBak = valPotiFreqLFO;
    }
    // Amount
    if(potiAmpLFOChanged){
      lfo1Amplitude = map(valPotiAmpLFO, 5, 1023, -100, 100);   // LFO-Amplitudenbereich von 0 bis 100, 50 Mitte
      valPotiAmpLFOBak = valPotiAmpLFO;
    }
  } else if(modSelect == 1){
    ///////////////////////////// LFO2 ///////////////////////////
    if(lfo1WaveformChanged){
      
      switch (valLfoWaveformSwitch) {
        case 0:
          lfo2Waveform = SQUARE;
          break;
        case 1:
          lfo2Waveform = TRIANGLE;
          break;
        case 2:
          lfo2Waveform = SAWTOOTH;
          break;
      }
    }

    // Pitch
    if(potiPitchChanged){
      int freqLin = map(valPotiPitch, 4, 1023, minVal, maxVal );
      baseFrequency = linearToLogarithmic(freqLin, minVal, maxVal);
      valPotiPitchBak = valPotiPitch;
    }
    // Frequenz
    if(potiFreqLFOChanged){
      lfo2Frequency = mapFloat(valPotiFreqLFO, 5, 1023, 0.1, 20);
      valPotiFreqLFOBak = valPotiFreqLFO;
    }
     // Amount
    if(potiAmpLFOChanged){
      lfo2Amplitude = map(valPotiAmpLFO, 5, 1023, -100, 100);
      valPotiAmpLFOBak = valPotiAmpLFO;
    }
  }  else if(modSelect == 2) {
    ///////////////////////////// Envelope für Speed LFO 1 und 2 ///////////////////////////
    if(lfo1WaveformChanged){
      
      switch (valLfoWaveformSwitch) {
        case 0:
          lfo1Waveform = SQUARE;
          break;
        case 1:
          lfo1Waveform = TRIANGLE;
          break;
        case 2:
          lfo1Waveform = SAWTOOTH;
          break;
      }
    }
    
    // Pitch-Poti = Duty-Cycle 
    if(potiPitchChanged){
      duty = map(valPotiPitch, 4, 1023, 5, 50 );
      valPotiPitchBak = valPotiPitch;
    }
    // LFO-Frequenz-Poti = Geschwindigkeit Envelope
    if(potiFreqLFOChanged){
      envelopeDuration = mapFloat(valPotiFreqLFO, 5, 1023, 1, 10);
      valPotiFreqLFOBak = valPotiFreqLFO;
    }
     // Amount-Poti = Amount Envelope auf LFO-Frequenz
    if(potiAmpLFOChanged){
      envelopeAmplitude = map(valPotiAmpLFO, 5, 1023, -100, 100);
      valPotiAmpLFOBak = valPotiAmpLFO;
    }
  }
  // beide Shift-Tasten zugleich gedrückt, Reset LFO2 Werte
  if(shiftState == 3){
    if (lfo2Amplitude != 0 || envelopeAmplitude != 0) {
      requestDisplayUpdate();      // nur einmal, wenn sich wirklich etwas ändert
    }
    lfo2Amplitude = 0;
    envelopeAmplitude = 0;
    resetShiftState();
  }

   // Modulierte Frequenz berechnen

  envelope = calculateEnvelope(envelopeDuration,envelopeAmplitude);
  lfo1ValueActual = calculateLFOWave1(lfo1Frequency * (envelope), lfo1Amplitude);
  //lfo2ValueActual = calculateLFOWave2(lfo2Frequency * (envelope), lfo2Amplitude);
    lfo2ValueActual = calculateLFOWave2(getLfo2Frequency() * (envelope), lfo2Amplitude);
   
   float newModulatedFrequency = 0;
   if(lfo1ValueActual == -100){
    newModulatedFrequency = lfo1ValueActual;
   }else{
    newModulatedFrequency = baseFrequency * lfo1ValueActual * lfo2ValueActual;
   }
   // Reset LFO values if the start button is false
  // Betriebsanzeige
  blink(50);
  // OLED-Display aktualisieren, wenn nötig
  if (displayForce) updateDisplay();
  // Create Tone
  playSound(newModulatedFrequency);
  // Control LED's updaten
  updateLEDs();
  // Normales Zeichnen NACH dem Ton: beim Loslassen ist er dann schon gestoppt
  updateDisplay();

  // Überwachung und Debugprits
  // if(chkLoop(1000)){
  //   debug(oledUpdateActive?"OLED-Update ON":"OLED-Update OFF");
  // }
}
