#include <Adafruit_MCP4725.h>
#include <Wire.h>
#include <VL53L1X.h>

// DAC
const int I2C_ADDR = 0x60;
Adafruit_MCP4725 dac;
VL53L1X sensor;

// Frequencies (in Hz)
/*const int F = 349;
const int G = 392;
const int A = 440;
const int BB = 466;
const int C = 523;*/

// Measure the communication speed with the DAC
unsigned long usPerCommand;
void calibrate() {
  usPerCommand = micros();
  for(int i = 0; i < 10; i++) {
    dac.setVoltage(0, false);  
  }
  usPerCommand = (micros() - usPerCommand)/10;
}

// Precalculated Voltage Buffer
const int BUFFER_SIZE = 128;
unsigned int voltages[BUFFER_SIZE];

void dacTone(unsigned long freq, int durationMs) {
  // Start time to keep track of the duration
  int startTime = millis();
 
  // Steps need to be a multiple of 4 to keep the sine form
  int raw_steps = 1000000 / (freq * usPerCommand);
  unsigned char steps = (raw_steps / 4) * 4;
  if(steps > BUFFER_SIZE) steps = BUFFER_SIZE;

  // Fill the rest of the time with an delay to achieve the frequency
  int delayUs = 1000000/freq/steps - usPerCommand;

  // Precalculate Sine Values (1 - 4095)
  for(unsigned char i = 0; i < steps; i++) {
    voltages[i] = (sin(i*3.14*2/steps) + 1) * 2047 + 1;
  }

  // Start Output
  while(millis()-startTime < durationMs) {
    for(unsigned char i = 0; i < steps; i++) {
      dac.setVoltage(voltages[i], false);
      delayMicroseconds(delayUs);
    }
  }

  dac.setVoltage(0, false);
}

// Play notes and add a small delay between them
void playNote(int freq, int duration) {
  dacTone(freq, duration-25);
  Serial.println(freq);
  delay(25);
}

void setup() {
  dac.begin(I2C_ADDR);
  
  calibrate();
  
  Serial.begin(115200);
  Wire.begin();
  //Wire.setClock(400000); // use 400 kHz I2C

  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1);
  }
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(50000);
  sensor.startContinuous(50);
}

int recalc(int value)
{
  int L = 12; //amount of notes
  int dists[] = {48, 70, 92, 114, 136, 158, 180, 202, 224, 246, 268, 290}; //{48, 66, 84, 102, 120, 138, 156, 174, 192, 210, 228, 246, 264, 282};
  int x0 = 30; //minimal distance
  int xm = 295; //maximal distance
  int freqss[] = {261, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494/*, 523, 554*/}; //фиксированный набор нот
  int i = 0;
  while ((value >= dists[i]) and (i <= L))
  {
    i = i + 1;
  }
  return freqss[i];
}

void loop() {
  //пересчитать диапазон
  int w = recalc(sensor.read()); /*map(sensor.read(), 30, 295, 261, 587);*/ //какой ноте соответствует полученное значение
  playNote(w, 250);
}
