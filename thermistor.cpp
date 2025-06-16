#include "thermistor.h"

#ifdef THERM_REPORT_DEGREES  // thermistor reading in °C

#define thermBeta     DEFAULT_THERM_BETA
#define ZERO_KELVIN   273.15f
#define THERM_T0      (25 + ZERO_KELVIN)
#define R0            10000  // Ohms

#ifdef THERM_USE_FLOAT // adds ~1300 bytes for float support

#ifdef THERM_USE_LOG  // for testing: accurate using log: ~9000 bytes?

#include "math.h"

float degreesC(float rTherm){  // -38° to 135°C  in hundredths
  return 1 / (1 / THERM_T0 + log(rTherm / R0) / thermBeta) - ZERO_KELVIN;
}

#else  // !THERM_USE_LOG   approximate float 8482 bytes

float sqrtfn(float x){
  union {
    float x;
    long i;
  } u;
  u.x = x;
  u.i = (1L << 29) + (u.i >> 1) - (1L << 22); // for IEEE 754 F32 format with 127 bias
 /* ((((val_int / 2^m) - b) / 2) + b) * 2^m = ((val_int - 2^m) / 2) + ((b + 1) / 2) * 2^m)
   where
     b = exponent bias = 127 (IEEE & TI)
     m = number of mantissa bits (23)
     val_int -= 1 << 23; // Subtract 2^m.
     val_int >>= 1; // Divide by 2.
     val_int += 1 << 29; // Add ((b + 1) / 2) * 2^m.
 */
  // Approximates sqrt by dividing exponent by 2 (perfect if even, ~ 5% max error)
  // then increases precision using Babylonian Steps, simplified from:
  //    u.x = (u.x + x / u.x) / 2;
  // Use two steps for greater accuracy
  // or none for < 5% error
  u.x += x / u.x;
  u.x = u.x / 4 + x / u.x;
  return u.x;
}

float logfn(float x) { // Borchardts approximation:
  // very good near 1.0; 0.83% error at 10 or 0.1
  return 6 * (x - 1) / (x + 1 + 4 * sqrtfn(x));
}

float degreesC(float rTherm){  // -38° to 135°C 
  return 1 / (1 / THERM_T0  + logfn(rTherm / R0) / thermBeta) - ZERO_KELVIN;
}

#endif // THERM_USE_LOG float methods

#else  // !THERM_USE_FLOAT

// Integer calcs: ~800 bytes

// could be smaller using Look Up Table computed for a particular beta

uint16_t isqrt(uint32_t num) {
  uint32_t res = 0;
  uint32_t bit = 0x4000000L; // 1 << (sizeof(num) * 8 - 2);
  while (bit > num) bit >>= 2;  // bit starts at highest 4 ^ N <= num
  while (bit) {
    if (num >= res + bit) {
      num -= res + bit;
      res = (res >> 1) + bit;
    } else res >>= 1;
    bit >>= 2;
  }
  return (uint16_t) res;
}

#define SCALE1 (1L << 22) // 24 OK on x86
#define SCALE2 (1L << 13) // 15
#define SCALE3 (1L << 12) // 14

long ilog(long x) { // Borchardts approximation:  very good near 1.0; 0.83% error at 10 or 0.1
  long num = SCALE3 * (x - SCALE2);
  long denom = x + SCALE2 + 4L * isqrt(SCALE2 * x);
  return num / denom * 6;
}

int degreesC(int adc){  // -38° to 135°C  in hundredths
  if (!adc) return THERMISTOR_SHORTED;
  return 100 * SCALE1 / ((long)(SCALE1 / THERM_T0) - (SCALE1 / SCALE3) * ilog(SCALE2 * ADC_MAX / adc - SCALE2) / thermBeta) - (int)(100 * ZERO_KELVIN);
}

#endif

#else // !THERM_REPORT_DEGREES: return raw ADC reading to do float computation on Linux side, saves 800 bytes

int degreesC(int adc) {
  return adc;
}

#endif  // THERM_REPORT_DEGREES
