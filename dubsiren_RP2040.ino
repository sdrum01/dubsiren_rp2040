#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "math.h"
#include "LittleFS.h"
#include "Bounce2.h"
#include "ArduinoJson.h"

   

const int LOGLEVEL = 1;

//const String CONFIG1 = "/config1.txt";
//const String CONFIG2 = "/config2.txt";
//const String CONFIG3 = "/config3.txt";
//const String CONFIG4 = "/config4.txt";

const int freqPotPin = A0;
const int lfoFreqPotPin = A1;  // Potentiometer für die Frequenz des LFO
const int lfoAmpPotPin = A2;   // Potentiometer für die Amplitude des LFO

const int waveFormPin_0 = 3; // WaveForm Kippschalter PIN 1
const int waveFormPin_1 = 4; // WaveForm Kippschalter Pin 2
const int waveFormFunctionPin = 6;  // WAVEFORM Funktion

const int wave_outputPin = 5; // Pin, an dem der Rechteckton ausgegeben wird

const int shiftPin = 2;  // Shift-Taste
const int firePin1 = 18;  // Steuerpin für die Tonaktivierung1
const int firePin2 = 19;  // Steuerpin für die Tonaktivierung2
const int firePin3 = 20;  // Steuerpin für die Tonaktivierung3
const int firePin4 = 21;  // Steuerpin für die Tonaktivierung4

// Bounce Objekte erstellen
Bounce fire1;
Bounce fire2;
Bounce fire3;
Bounce fire4;
Bounce shift;

byte actualFireButton = 0;
byte actualFireButtonBak = 0;

const int LED1_red = 16;
const int LED1_green = 17;


// Konstanten für den minimalen und maximalen Wert der blanken Frequenz ohne Modulation
const float minVal = 35;
const float maxVal = 8000;
// minimaler und maximaler Wert der Frequenz nach der LFO-Modulation
const float minValMod = 20;
const float maxValMod = 10000;

// Variablen
int zaehler = 0;     

// Parameter Pitch/Tune
int baseFrequency = 1000;

// Parameter LFO:
float lfoFrequency = 1;  // LFO-Frequenz in Hz (0.5 Hz = 2 Sekunden Periode)
float lfoAmplitude = 0;

// Parameter ENVELOPE-Generator:
float envelopeDuration = 20;  // LFO-Frequenz in Hz (0.5 Hz = 2 Sekunden Periode)
float envelopeAmplitude = 0;

// die finale LFO WaveForm Variable, die durch Schalter oder Speicher gesetzt wird
enum LfoWaveform { SQUARE, TRIANGLE, SAWTOOTH };
LfoWaveform lfoWaveform = TRIANGLE;  // Gewünschte LFO-Wellenform: SQUARE, TRIANGLE, SAWTOOTH

// Schalter für WaveForm
byte valLfoWaveformSwitch = 0;

// temporäre Sicherung des Wertes des Waveformschalters
byte valLfoWaveformSwitchBak = 0;

bool dataSaved = false;

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
// int valPotiPitch = 0;

// Wenn Ton abgefeuert werden soll:
bool runSound = 1;

// shift-taste
bool shiftState = false;
// entprellen der shift-taste
// bool shift_bak = 1;

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
bool waveFormFunction = 0;
bool waveFormFunctionBak = 0;


//  Flags, wenn es an den Potis gewackelt hat
// byte potisChanged = 0b0000000;  // binär 00000000

bool potiPitchChanged = 1;
bool potiFreqLFOChanged = 1;
bool potiAmpLFOChanged = 1;

const int potiTolerance = 10;

// Flag, wenn es am Waveformschalter gewackelt hat
bool lfoWaveformChanged = 0;
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////




///////////////////////////////////
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

  
  pinMode(shiftPin, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand
  pinMode(firePin1, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand
  pinMode(firePin2, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand
  pinMode(firePin3, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand
  pinMode(firePin4, INPUT_PULLUP);  // Steuerpin als Eingang mit Pull-up-Widerstand

  // Bounce-Objekt initialisieren
  shift.attach(shiftPin);
  shift.interval(50);  // Entprellintervall in Millisekunden (50 ms)

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
  pinMode(waveFormFunctionPin, INPUT_PULLUP);
  
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
  debugStr("Setup abgeschlossen.");
}

// Werte von den Potis und Schaltern in eine JSON datei speicher
String values2JSON(){
  // Erstelle einen JSON-Dokument Puffer mit genügend Speicher
  StaticJsonDocument<200> dataSet;
/*
  JsonObject actualDataset = doc.createNestedObject(fireButton);

  actualDataset["pitch"]     = baseFrequency;
  actualDataset["lfoFreq"]   = lfoFrequency;
  actualDataset["lfoAmount"] = lfoAmplitude;
  actualDataset["envTime"]   = envelopeDuration;
  actualDataset["envAmount"] = envelopeAmplitude;
  actualDataset["waveform"] = lfoWaveform;
*/
  dataSet["pitch"]     = baseFrequency;
  dataSet["lfoFreq"]   = lfoFrequency;
  dataSet["lfoAmount"] = lfoAmplitude;
  dataSet["envTime"]   = envelopeDuration;
  dataSet["envAmount"] = envelopeAmplitude;
  dataSet["waveform"]  = lfoWaveform;

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
  envelopeDuration = dataSet["envTime"];
  envelopeAmplitude = dataSet["envAmount"];
  lfoWaveform = dataSet["waveform"];
}

String readSettings(String configFile){
    // Daten lesen
    String value;
    File file = LittleFS.open(configFile, "r");
    if (file) {
        //String value = file.read();  // Lese die gespeicherte Zahl
        value = file.readStringUntil('\n');
        Serial.println("Gelesener Wert: "+value);
        file.close();
    } else {
        Serial.println("Fehler beim Lesen der Datei "+configFile);
        String _json = values2JSON();
        writeSettings(_json,configFile);
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
      Serial.println("Daten geschrieben: "+s);
  } else {
    Serial.println("Fehler beim Schreiben der Datei "+configFile);
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

// LFO-Variante mit Dreieck, abgeleiteten Rechteck und Sägezahn
float calculateLFOWave(float lfoFrequency, float lfoAmplitude, bool waveFormFunction) {
  
  uint slice_num_led_green = pwm_gpio_to_slice_num(LED1_green);
  uint slice_num_led_red = pwm_gpio_to_slice_num(LED1_green);
  
  float schrittweite = lfoFrequency / 1000 ;
  //float schrittweite = lfoFrequency / 1000 * ((envelopeAmplitude / 100) + 1.0);
  //float schrittweite_envelope = 0.001;
  float schrittweite_envelope = envelopeDuration / 5000;
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
        //lfoAmplitude = lfoAmplitude / 2;
        // Dreieck ausrechnen 
        lfoValue += schrittweite * lfoDirection;  // 
        if (lfoValue >= 1.0 || lfoValue <= 0.0) {
          lfoDirection = -lfoDirection;  // Richtung umkehren
        }

        
        if(lfoAmplitude < 0){
          lfovalue_final = (lfoValue <= 0.5) ? 0.5 : -100; // -100 = muting;
        } else {
          lfovalue_final = (lfoValue >= 0.5) ? 1 : 0;
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
      /*
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
        */
        lfoValue -= schrittweite / 2;  // Schrittweite für den LFO 
        if (lfoValue <= 0) {
          lfoValue = 1;  // Zurücksetzen
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
    
    // LED grün LFO
    pwm_set_chan_level(slice_num_led_green, pwm_gpio_to_channel(LED1_green),lfovalue_final * pwm_led);
    pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red),envelopeValue * pwm_led);
    
    float envelope = envelopeValue * (envelopeAmplitude / 100) + 1.0;
    //pwm_set_chan_level(slice_num_led_red, pwm_gpio_to_channel(LED1_red),envelopeValue * pwm_led);
    
    //lfovalue_final1 = ((lfovalue_final -0.5) * (lfoAmplitude/100) + 1.0);
    lfovalue_final1 = ((lfovalue_final -0.5) * (lfoAmplitude/100) + 1.0) * envelope ;
     //debugFloat(envelope);
  }

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
  //float pwm_val = setFrequency(linearToLogarithmic(freqVal));
  float pwm_val = setFrequency(freqVal);

  // Setze die PWM-Periode
  uint slice_num_wave = pwm_gpio_to_slice_num(wave_outputPin);

  pwm_set_wrap(slice_num_wave, pwm_val);
  
  if (runSound){
  
    if(freqVal == -100) {
      pwm_set_chan_level(slice_num_wave, pwm_gpio_to_channel(wave_outputPin), 0);
      //pwm_val = setFrequency(1);
      //pwm_set_chan_level(slice_num, pwm_gpio_to_channel(wave_outputPin), pwm_val / 2);
    }else{
      pwm_set_chan_level(slice_num_wave, pwm_gpio_to_channel(wave_outputPin), pwm_val / 2);
    }
  }else{
    pwm_set_chan_level(slice_num_wave, pwm_gpio_to_channel(wave_outputPin), 0);
  }
 
}

void setChangeState(bool p1, bool p2, bool p3, bool s1){
  potiPitchChanged = p1;
  potiFreqLFOChanged = p2;
  potiAmpLFOChanged = p3;
  lfoWaveformChanged = s1;
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
  envelopeValue = 1;
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
  if(shift.read() == LOW){
    // Save values
    String _json = values2JSON();
    writeSettings(_json,fileName);
    shiftState = 1;
  } else {
    if(fireButton != actualFireButtonBak){
      // load Values
      String _json = readSettings(fileName);
      JSON2values(_json);
      setChangeState(1,0,0,0);
    }
  }
  return;
}

void updateKeys(){
  shift.update();
  fire1.update();
  fire2.update();
  fire3.update();
  fire4.update();

  if (dataSaved == 0){
    if (shift.rose()) {
      shiftState = !(shiftState);
    }
  }
  
  // Abfrage Schalter, welche Funktion die Potis haben sollen
  waveFormFunction = digitalRead(waveFormFunctionPin);

  // Wenn der Funktionsschalter sich geändert hat
  if(waveFormFunctionBak != waveFormFunction){
    // Flags zurücksetzen, dass die Potis gewackelt haben, sonst springen die Einstellungen sofort auf die neuen Werte
    setChangeState(1,0,0,0);
  }
  waveFormFunctionBak = waveFormFunction;

  // Aus dem Waveformschalter ein Byte machen
  valLfoWaveformSwitch = combineBoolsToByte(digitalRead(waveFormPin_0),digitalRead(waveFormPin_1));
  if(valLfoWaveformSwitchBak != valLfoWaveformSwitch){
    lfoWaveformChanged = 1;
  }
  valLfoWaveformSwitchBak = valLfoWaveformSwitch;

  if(lfoWaveformChanged){
    switch (valLfoWaveformSwitch) {
      case 1:
        lfoWaveform = SQUARE;
        break;
      case 2:
        lfoWaveform = SAWTOOTH;
        break;
      case 3:
        lfoWaveform = TRIANGLE;
        break;
    }
  }
  
  // if (( digitalRead(waveFormPin_0) == 0 )&&( digitalRead(waveFormPin_1) == 1 )){
  //   lfoWaveform = TRIANGLE;
  // } 
  // if(( digitalRead(waveFormPin_0) == 1 )&&( digitalRead(waveFormPin_1) == 1 )){
  //   lfoWaveform = SAWTOOTH;
  // }
  // if ((digitalRead(waveFormPin_0) == 1 )&&( digitalRead(waveFormPin_1) == 0)){
  //   lfoWaveform = SQUARE;
  // }
  
  if (fire1.fell()){
    actualFireButton = 1;
    loadOrSave(actualFireButton);
    // String fileName = "fire"+String(actualFireButton)+".json";
    // resetLFOParams();
    // if(shift.read() == LOW){
    //   String _json = values2JSON();
    //   writeSettings(_json,fileName);
    //   shiftState = 1;
    // } else {
    //   if(actualFireButton != actualFireButtonBak){
    //     String _json = readSettings(fileName);
    //     JSON2values(_json);
    //     setChangeState(1,0,0,0);
    //   }
    // }
  }

  if (fire2.fell()){
    actualFireButton = 2;
    loadOrSave(actualFireButton);
  }
  if (fire3.fell()){
    actualFireButton = 3;
    loadOrSave(actualFireButton);
  }
  if (fire4.fell()){
    actualFireButton = 4;
    loadOrSave(actualFireButton);
  }
  actualFireButtonBak = actualFireButton;

  // globale Variable rundsound = wenn high, wird ein Ton abgespielt
  // runSound = ((fire1.read() == LOW)&&(shift.read() == HIGH));
  runSound = (((fire1.read() == LOW)||(fire2.read() == LOW)||(fire3.read() == LOW)||(fire4.read() == LOW))&&(shift.read() == HIGH));
  
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

////////////////////////////////////////////////////////////



void loop() {
  
  updateKeys();
  updatePotis();
/*
  // Prüfe auf steigende Flanke (Taster wurde gedrückt)
  if (taster.fell()) {
    Serial.println("Taster gedrückt (steigende Flanke erkannt)");
  }

  // Prüfe auf fallende Flanke (Taster wurde losgelassen)
  if (taster.rose()) {
    Serial.println("Taster losgelassen (fallende Flanke erkannt)");
  }

  if (taster.read() == LOW) {
    Serial.println("Taste ist gedrückt!");
  }
*/

  
  // Holen der Basisfrequenz
  if(potiPitchChanged == 1){
    int freqLin = map(valPotiPitch, 4, 1023, minVal, maxVal );
    baseFrequency =  linearToLogarithmic(freqLin );
    valPotiPitchBak = valPotiPitch;
  }

  if(waveFormFunction){
    if(potiFreqLFOChanged == 1){
      envelopeDuration = mapFloat(valPotiFreqLFO, 5, 1023, 1, 100);
      valPotiFreqLFOBak = valPotiFreqLFO;
    }
    if(potiAmpLFOChanged == 1){
      envelopeAmplitude = map(valPotiAmpLFO, 5, 1023, -100, 100);
      valPotiAmpLFOBak = valPotiAmpLFO;
    }
  }else{
    // lesen des Pitch-Wertes vom Poti incl. Mittelwert
    if(potiFreqLFOChanged == 1){
      lfoFrequency = mapFloat(valPotiFreqLFO, 5, 1023, 0.5, 50);   // LFO-Frequenzbereich von 0.5 Hz bis 50 Hz
      valPotiFreqLFOBak = valPotiFreqLFO;
    }
    if(potiAmpLFOChanged == 1){
      lfoAmplitude = map(valPotiAmpLFO, 5, 1023, -100, 100);   // LFO-Amplitudenbereich von 0 bis 100, 50 Mitte
      valPotiAmpLFOBak = valPotiAmpLFO;
    }
  }
  



   if(chkLoop(10000)){
     debugFloat(lfoWaveformChanged);
   }
   

   // Modulierte Frequenz berechnen
   float lfoValueActual = calculateLFOWave(lfoFrequency, lfoAmplitude, waveFormFunction);
   float newModulatedFrequency = 0;
   if(lfoValueActual == -100){
    newModulatedFrequency = lfoValueActual;
   }else{
    newModulatedFrequency = baseFrequency * lfoValueActual;
   }
   // Reset LFO values if the start button is false
  
   
  // LEDs
  digitalWrite(LED_BUILTIN, shiftState);
  
  // Create Tone
  soundCreate(newModulatedFrequency);

}
