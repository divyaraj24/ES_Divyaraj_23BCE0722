/*
 * Cold Chain Breach Predictor with TinyML Impact Detection
 * ESP32 + MPU6050 + I2C LCD for Wokwi Simulation
 *
 * VIT Embedded Systems Project - 23BCE0722
 *
 * Temperature Monitoring: Rule-based thresholds (WHO Cold Chain Guidelines)
 * Impact Detection: TinyML Neural Network (6->10->4, 344 bytes)
 */

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// ==================== TinyML MODEL ====================
const float vib_mean[6] = {1.0, 0.1, 0.02, 2.0, 2.0, 100.0};
const float vib_std[6] = {0.3, 0.2, 0.05, 3.0, 3.0, 50.0};

// Layer 1 (input -> hidden): 6 normalized features to 10 hidden neurons.
const float W1_vib[6][10] = {
    {0.2, -0.1, 0.3, 0.1, -0.2, 0.15, -0.1, 0.25, -0.15, 0.1},
    {2.5, 1.8, -0.5, -1.2, 0.3, -0.8, 1.2, -0.3, 0.5, -0.6},
    {-0.8, 0.6, 2.2, 1.5, -1.0, 0.4, -0.6, 1.8, -0.4, 0.7},
    {0.3, -0.4, 0.8, 0.5, -0.3, 0.6, -0.5, 0.4, 0.2, -0.3},
    {-0.2, 0.3, 0.6, 0.4, -0.5, 0.3, -0.4, 0.5, -0.2, 0.4},
    {0.1, -0.1, 0.15, -0.1, 0.2, -0.15, 0.1, -0.2, 0.15, -0.1}};
// Layer 1 bias for the 10 hidden neurons (ReLU activation after this layer).
const float b1_vib[10] = {-0.5, -0.3, -0.8, -0.4, 0.2,
                          -0.2, -0.3, -0.5, 0.1,  0.3};

// Layer 2 (hidden -> output): 10 hidden activations to 4 class logits.
const float W2_vib[10][4] = {{-0.8, 2.5, 0.5, -1.5},  {-0.5, 2.0, 0.8, -1.2},
                             {0.8, -0.5, 1.8, -1.0},  {0.6, -0.3, 1.5, -0.8},
                             {1.5, -0.8, -0.5, -0.3}, {0.4, -0.2, 0.6, 0.8},
                             {-0.3, 1.2, 0.4, -0.5},  {0.5, -0.4, 1.2, -0.6},
                             {0.3, -0.2, -0.3, 1.5},  {-0.4, 0.3, -0.2, 1.8}};
// Layer 2 bias for 4 classes: MOVING, IMPACT!, ROUGH, STATIC.
const float b2_vib[4] = {0.5, -1.5, -0.8, 1.2};

// ==================== Hardware Configuration ====================
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define MPU_ADDR 0x68
#define POT_INTERNAL 34
#define POT_AMBIENT 35
#define LED_GREEN 25
#define LED_YELLOW 26
#define LED_RED 27
#define BUZZER 18
#define BTN_DOOR 12

const float TEMP_MIN_SAFE = 2.0;
const float TEMP_MAX_SAFE = 8.0;
const float TEMP_WARNING = 10.0;
const float TEMP_BREACH = 12.0;

// ==================== Variables ====================
float internalTemp = 5.0, ambientTemp = 25.0;
float accelX = 0, accelY = 0, accelZ = 0;
float gyroX = 0, gyroY = 0, gyroZ = 0;
float gyroMagnitude = 0;
float accelHistory[20][3];
int accelIdx = 0;
float lastMagnitude = 1.0;

int tempStatus = 0;
int lastTempStatus = -1;
int vibClass = 3;
int lastVibClass = 3;
float vibConf = 0;
int stableCount = 0;

const char* tempNames[] = {"SAFE", "WARNING", "CRITICAL"};
const char* vibNames[] = {"MOVING", "IMPACT!", "ROUGH", "STATIC"};

int impactCount = 0;
int roughCount = 0;
int loopCount = 0;

// ==================== Temperature Classification ====================
int classifyTemperature(float temp) {
  if (temp >= TEMP_BREACH) return 2;
  if (temp >= TEMP_WARNING || temp < TEMP_MIN_SAFE) return 1;
  if (temp >= TEMP_MIN_SAFE && temp <= TEMP_MAX_SAFE) return 0;
  return 1;
}

// ==================== TinyML Inference ====================
int runVibrationML(float magnitude, float jerk, float variance, float peakFreq,
                   float zeroCross, float duration) {
  float input[6] = {(magnitude - vib_mean[0]) / vib_std[0],
                    (jerk - vib_mean[1]) / vib_std[1],
                    (variance - vib_mean[2]) / vib_std[2],
                    (peakFreq - vib_mean[3]) / vib_std[3],
                    (zeroCross - vib_mean[4]) / vib_std[4],
                    (duration - vib_mean[5]) / vib_std[5]};

  float hidden[10];
  for (int j = 0; j < 10; j++) {
    hidden[j] = b1_vib[j];
    for (int i = 0; i < 6; i++) {
      hidden[j] += input[i] * W1_vib[i][j];
    }
    if (hidden[j] < 0) hidden[j] = 0;
  }

  float output[4], maxVal = -1000;
  int maxIdx = 0;
  for (int j = 0; j < 4; j++) {
    output[j] = b2_vib[j];
    for (int i = 0; i < 10; i++) {
      output[j] += hidden[i] * W2_vib[i][j];
    }
    if (output[j] > maxVal) {
      maxVal = output[j];
      maxIdx = j;
    }
  }

  float sumExp = 0;
  for (int j = 0; j < 4; j++) sumExp += exp(output[j] - maxVal);
  vibConf = 1.0 / sumExp;

  return maxIdx;
}

// ==================== MPU6050 Functions ====================
void initMPU6050() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x08);
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B);
  Wire.write(0x00);
  Wire.endTransmission(true);
}

void readMPU6050() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  int bytesRead = Wire.requestFrom(MPU_ADDR, 14, true);
  if (bytesRead < 14 || Wire.available() < 14) {
    return;
  }

  int16_t ax = Wire.read() << 8 | Wire.read();
  int16_t ay = Wire.read() << 8 | Wire.read();
  int16_t az = Wire.read() << 8 | Wire.read();

  Wire.read();
  Wire.read();

  int16_t gx = Wire.read() << 8 | Wire.read();
  int16_t gy = Wire.read() << 8 | Wire.read();
  int16_t gz = Wire.read() << 8 | Wire.read();

  accelX = ax / 8192.0;
  accelY = ay / 8192.0;
  accelZ = az / 8192.0;

  gyroX = gx / 131.0;
  gyroY = gy / 131.0;
  gyroZ = gz / 131.0;
  gyroMagnitude = sqrt(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ);

  accelHistory[accelIdx][0] = accelX;
  accelHistory[accelIdx][1] = accelY;
  accelHistory[accelIdx][2] = accelZ;
  accelIdx = (accelIdx + 1) % 20;
}

void extractVibrationFeatures(float& magnitude, float& jerk, float& variance,
                              float& peakFreq, float& zeroCross) {
  float rawMag = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);

  if (rawMag < 0.05) {
    magnitude = 1.0;
    rawMag = 0.0;
  } else {
    magnitude = 1.0 + rawMag;
  }

  jerk = abs(magnitude - lastMagnitude) * 20;
  lastMagnitude = magnitude;

  float sum = 0, sumSq = 0;
  for (int i = 0; i < 20; i++) {
    float m = sqrt(accelHistory[i][0] * accelHistory[i][0] +
                   accelHistory[i][1] * accelHistory[i][1] +
                   accelHistory[i][2] * accelHistory[i][2]);
    if (m < 0.05)
      m = 1.0;
    else
      m = 1.0 + m;
    sum += m;
    sumSq += m * m;
  }
  float mean = sum / 20;
  variance = (sumSq / 20) - (mean * mean);
  if (variance < 0) variance = 0;

  if (variance < 0.001) variance = 0;

  zeroCross = 0;
  for (int i = 1; i < 20; i++) {
    float curr = sqrt(accelHistory[i][0] * accelHistory[i][0] +
                      accelHistory[i][1] * accelHistory[i][1] +
                      accelHistory[i][2] * accelHistory[i][2]);
    float prev = sqrt(accelHistory[i - 1][0] * accelHistory[i - 1][0] +
                      accelHistory[i - 1][1] * accelHistory[i - 1][1] +
                      accelHistory[i - 1][2] * accelHistory[i - 1][2]);
    if ((curr - mean) * (prev - mean) < 0) {
      zeroCross++;
    }
  }
  peakFreq = zeroCross * 2.5;
}

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=============================================");
  Serial.println("  COLD CHAIN MONITOR + IMPACT DETECTION");
  Serial.println("  Temperature: Rule-based (WHO Guidelines)");
  Serial.println("  Impact: TinyML Neural Network (344 bytes)");
  Serial.println("=============================================\n");

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.print("ColdChain+Impact");
  lcd.setCursor(0, 1);
  lcd.print("TinyML 344 bytes");

  initMPU6050();

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BTN_DOOR, INPUT_PULLUP);

  digitalWrite(LED_GREEN, HIGH);

  for (int i = 0; i < 20; i++) {
    accelHistory[i][0] = 0;
    accelHistory[i][1] = 0;
    accelHistory[i][2] = 1.0;
  }

  tone(BUZZER, 1000, 200);
  delay(2000);
  lcd.clear();

  Serial.println("\n========== COLD CHAIN DEBUG MODE ==========");
  Serial.println(
      "Loop | Temp | AmbientTemp | TempClass | Mag  Jrk Var | Class");
  Serial.println(
      "------------------------------------------------------------------------"
      "--------------------------");
}

// ==================== Main Loop ====================
void loop() {
  int potInt = analogRead(POT_INTERNAL);
  int potAmb = analogRead(POT_AMBIENT);
  internalTemp = (potInt / 4095.0) * 20.0;
  ambientTemp = 15.0 + (potAmb / 4095.0) * 25.0;

  readMPU6050();

  tempStatus = classifyTemperature(internalTemp);
  if (tempStatus != lastTempStatus) {
    Serial.print("TEMP CLASS CHANGE -> ");
    Serial.print(tempNames[tempStatus]);
    Serial.print(" at ");
    Serial.print(internalTemp, 1);
    Serial.println("C");
    lastTempStatus = tempStatus;
  }

  float magnitude, jerk, vibVar, peakFreq, zeroCross;
  extractVibrationFeatures(magnitude, jerk, vibVar, peakFreq, zeroCross);

  float rawMag = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
  float accelDeltaFrom1G = abs(rawMag - 1.0);
  float dynamicAccel = min(rawMag, accelDeltaFrom1G);

  if (dynamicAccel < 0.06 && jerk < 0.5 && vibVar < 0.005 &&
      gyroMagnitude < 5.0) {
    vibClass = 3;
    vibConf = 0.95;
    stableCount = 0;
  } else {
    int newVibClass =
        runVibrationML(magnitude, jerk, vibVar, peakFreq, zeroCross, 100);

    if (newVibClass == 3 && gyroMagnitude > 20.0) {
      newVibClass = 0;
      vibConf = min(vibConf, 0.70f);
    }

    if (dynamicAccel >= 0.06 && newVibClass == 3) {
      newVibClass = 0;
      vibConf = max(vibConf, 0.80f);
    }

    if (newVibClass == lastVibClass) {
      stableCount++;
      if (stableCount >= 3) {
        vibClass = newVibClass;
      }
    } else {
      stableCount = 0;
      lastVibClass = newVibClass;
    }
  }

  if (vibClass == 1) impactCount++;
  if (vibClass == 2) roughCount++;

  bool critical = (tempStatus == 2) || (vibClass == 1);
  bool warning = (tempStatus == 1) || (vibClass == 2);

  digitalWrite(LED_GREEN, !critical && !warning);
  digitalWrite(LED_YELLOW, warning && !critical);
  digitalWrite(LED_RED, critical);

  if (critical) {
    tone(BUZZER, vibClass == 1 ? 2500 : 2000);
  } else if (warning && (millis() / 500) % 2) {
    tone(BUZZER, 1000, 100);
  } else {
    noTone(BUZZER);
  }

  lcd.setCursor(0, 0);
  lcd.print(tempNames[tempStatus]);
  lcd.print(" ");
  lcd.print(internalTemp, 1);
  lcd.print("C    ");

  lcd.setCursor(0, 1);
  lcd.print(vibNames[vibClass]);
  lcd.print(" ");
  lcd.print((int)(vibConf * 100));
  lcd.print("% I:");
  lcd.print(impactCount);
  lcd.print("  ");

  loopCount++;

  Serial.print(loopCount);
  Serial.print(" | ");

  Serial.print(internalTemp, 1);
  Serial.print("C | ");

  Serial.print(ambientTemp, 1);
  Serial.print("C | ");

  Serial.print(tempNames[tempStatus]);
  Serial.print(" | ");

  Serial.print(magnitude, 2);
  Serial.print(" ");
  Serial.print(jerk, 2);
  Serial.print(" ");
  Serial.print(vibVar, 4);
  Serial.print(" | ");

  Serial.print(vibNames[vibClass]);
  Serial.print(" ");
  Serial.print((int)(vibConf * 100));
  Serial.println("%");

  delay(50);
}
