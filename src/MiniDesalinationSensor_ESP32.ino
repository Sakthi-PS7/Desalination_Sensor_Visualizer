#include <FastLED.h>

// LED strip configuration
#define LED_PIN     5
#define NUM_LEDS    60      
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// Sensor pins
const int THERM_PIN = 34;
const int COND_PIN  = 35;

// Thermistor calibration
const float R_FIXED_THERM = 10000.0f;
const float R0_THERM      = 10000.0f;
const float BETA_THERM    = 3950.0f;
const float T0_REF_KELVIN = 298.15f;

const float R_SERIES_COND = 4700.0f;

// Exponential moving average for smooth LED transitions
float filteredCondIndex = 0.0f;
const float EMA_ALPHA   = 0.15f;

// Smooth ADC readings with oversampling
float readADC_smooth(int pin, int samples = 32) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delayMicroseconds(200);
  }
  return (float)sum / samples;
}

// Convert thermistor ADC reading to temperature
float thermistorCelsiusFromADC(float adcValue) {
  float Vout = (adcValue / 4095.0f) * 3.3f;

  if (Vout <= 0.001f) Vout = 0.001f;
  if (Vout >= 3.299f) Vout = 3.299f;

  float R_therm = (R_FIXED_THERM * Vout) / (3.3f - Vout);

  // Beta equation for temperature calculation
  float lnR   = log(R_therm / R0_THERM);
  float invT  = (1.0f / T0_REF_KELVIN) + (lnR / BETA_THERM);
  float T_kelvin  = 1.0f / invT;
  float T_celsius = T_kelvin - 273.15f;

  return T_celsius;
}

// Calculate water resistance and conductivity index from ADC
void conductivityFromADC(
    float adcValue,
    float &Vout,
    float &Rwater,
    float &indexRaw,
    float &indexTempComp,
    float tempC
) {
  Vout = (adcValue / 4095.0f) * 3.3f;

  if (Vout < 0.001f) {
    Vout   = 0.0f;
    Rwater = 0.0f;
  } else if (Vout > 3.29f) {
    Rwater = 1e9f;
  } else {
    Rwater = (R_SERIES_COND * Vout) / (3.3f - Vout);
  }

  // Normalize to 0-100 scale
  float norm = 1.0f - (adcValue / 4095.0f);
  if (norm < 0.0f) norm = 0.0f;
  if (norm > 1.0f) norm = 1.0f;
  indexRaw = norm * 100.0f;

  // Temperature compensation (2% per degree C)
  float alpha = 0.02f;
  float denom = 1.0f + alpha * (tempC - 25.0f);
  if (denom < 0.1f) denom = 0.1f;
  indexTempComp = indexRaw / denom;
}

// Map position to color gradient (blue to red)
CRGB colorForPosition(float pos01) {
  if (pos01 < 0.0f) pos01 = 0.0f;
  if (pos01 > 1.0f) pos01 = 1.0f;

  uint8_t hueBlue = 160;
  uint8_t hueRed  = 0;
  float hueF = hueBlue + (hueRed - hueBlue) * pos01;
  return CHSV((uint8_t)hueF, 255, 255);
}

// Render LED bar based on conductivity
void renderSalinityBar(float condIndex) {
  if (condIndex < 0.0f) condIndex = 0.0f;
  if (condIndex > 100.0f) condIndex = 100.0f;

  float level = condIndex / 100.0f;
  int litLeds = (int)round(level * NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < litLeds) {
      float pos = (NUM_LEDS == 1) ? 0.0f : (float)i / (NUM_LEDS - 1);
      leds[i] = colorForPosition(pos);
    } else {
      leds[i] = CRGB::Black;
    }
  }
  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(10);

  Serial.println("\n=== Mini Desalination Sensor + LED Gradient Bar ===";
}

void loop() {
  float adcTherm = readADC_smooth(THERM_PIN, 40);
  float tempC    = thermistorCelsiusFromADC(adcTherm);

  float adcCond  = readADC_smooth(COND_PIN, 40);
  float Vcond, Rwater, indexRaw, indexComp;
  conductivityFromADC(adcCond, Vcond, Rwater, indexRaw, indexComp, tempC);

  // Apply smoothing filter
  static bool firstRun = true;
  if (firstRun) {
    filteredCondIndex = indexComp;  // initialize
    firstRun = false;
  } else {
    filteredCondIndex = filteredCondIndex +
                        EMA_ALPHA * (indexComp - filteredCondIndex);
  }

  renderSalinityBar(filteredCondIndex);

  // Debug output
  Serial.print("Temp: ");
  Serial.print(tempC, 2);
  Serial.print(" °C");

  Serial.print("  |  Therm_ADC: ");
  Serial.print(adcTherm, 1);

  Serial.print("  ||  Cond_ADC: ");
  Serial.print(adcCond, 1);
  Serial.print("  V: ");
  Serial.print(Vcond, 3);

  Serial.print("  R_water: ");
  if (Rwater >= 9e8f) {
    Serial.print(">1e9");
  } else {
    Serial.print(Rwater, 0);
  }
  Serial.print(" Ω");

  Serial.print("  |  CondIndex_raw: ");
  Serial.print(indexRaw, 1);
  Serial.print("  |  CondIndex_25C: ");
  Serial.print(indexComp, 1);
  Serial.print("  |  CondIndex_filtered: ");
  Serial.print(filteredCondIndex, 1);

  Serial.println();

  delay(1000);
}
