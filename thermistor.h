// thermistor.h

#define DEFAULT_THERM_BETA 3950 

#define THERM_REPORT_DEGREES  // else returns raw ADC value to do calc on Linux side for smaller code - adds ~800 bytes
#define THERM_USE_FLOAT       // calculate using floating point - adds ~510 bytes for float support over integer version
#define THERM_USE_LOG         // for testing: more accurate far from 25°C - adds ~580 bytes to float version

#define THERMISTOR_SHORTED  -9999  // -99.99° indicates shorted thermistor

float degreesC(float rTherm);

