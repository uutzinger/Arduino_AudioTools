/**
 * @file read-esp32-multi-channel-csv.ino
 * @author Urs Utzinger
 * @copyright GPLv3
 */

// | attenuation     | range   | accurate range |
// | ------------    | --------| -------------- |
// | ADC_ATTEN_DB_0  | 0..1.1V | 100-950mV      |
// | ADC_ATTEN_DB_2_5| 0..1.5V | 100-1250mV     |
// | ADC_ATTEN_DB_6  | 0..2.2V | 150-1750mV     |
// | ADC_ATTEN_DB_12 | 0..3.9V | 150-2450mV     |

#include "Arduino.h"
#include "AudioTools.h"

#define M_NUM_CHANNELS 6                          // number of channels
#define M_DEC_FACTOR 48                           // decimating
#define M_ADC_ATTENUATION ADC_ATTEN_DB_11         // ADC attenuation, see above
#define M_ADC_BIT_WIDTH 12                        // the ADC bit width 9..12, values will be stored in 16 bit integers
#define M_BIT_DEPTH 16                            // default bit width of data returned from analgo_in. Do not change.

// It looks like Buffer larger than 1024 is capped by stream copy for decimater or csvoutput
#define M_BUFFER_SIZE_FACTOR (1024/ (M_NUM_CHANNELS*M_DEC_FACTOR*M_BIT_DEPTH/8)) // 3.5
#define M_ADC_BUFFER_SIZE (M_NUM_CHANNELS*M_DEC_FACTOR*M_BUFFER_SIZE_FACTOR) // Total number of samples (not bytes), needs to be divisible by 4 // 3*3*96 = 864
#define M_BUFFER_SIZE ADC_BUFFER_SIZE*M_BIT_DEPTH/8 // Number of bytes for processing buffer
#define M_SERIAL_BUFFER_SIZE BUFFER_SIZE/M_DEC_FACTOR   // There will be factor times less data after decimating

// Sampling rates
#define M_SERIAL_SAMPLE_RATE 300                               // samples per channel per second
#define M_SAMPLE_RATE        M_SERIAL_SAMPLE_RATE*M_DEC_FACTOR // number of samples per channel before decimating
#define M_ADC_SAMPLE_RATE    M_SAMPLE_RATE*M_NUM_CHANNELS      // actual sampling rate of ADC
// BAUD rate required: SERIAL_SAMPLE_RATE * (NUM_CHANNELS * approx 6 chars + 2 EOL) * 10

#define M_ADC0 ADC_CHANNEL_7 // ADC channel 0
#define M_ADC1 ADC_CHANNEL_0 // ADC channel 1
#define M_ADC2 ADC_CHANNEL_3 // ADC channel 2
#define M_ADC3 ADC_CHANNEL_6 // ADC channel 3
#define M_ADC4 ADC_CHANNEL_4 // ADC channel 4
#define M_ADC5 ADC_CHANNEL_5 // ADC channel 5
#define M_ADC6 ADC_CHANNEL_1 // ADC channel 6, not routed on the ESP32 WROOM module
#define M_ADC7 ADC_CHANNEL_2 // ADC channel 7, not routed on ESP32 WROOM module
#define M_ADC8 ADC_CHANNEL_8 // ADC channel 8, only ESP32 S3,S2
#define M_ADC9 ADC_CHANNEL_9 // ADC channel 9, only ESP32 S3,S2

AudioInfo                     info_serial(M_SERIAL_SAMPLE_RATE, M_NUM_CHANNELS, M_BIT_DEPTH);

AnalogAudioStream             analog_in; // on board analog to digital converter
Decimate                      decimater(M_DEC_FACTOR, M_NUM_CHANNELS, M_BIT_DEPTH); // skip samples
ConverterStream<int16_t>      decimated_stream(analog_in, decimater); // pipe the sound to the decimater
CsvOutput<int16_t>            serial_out(Serial, M_NUM_CHANNELS, M_SERIAL_BUFFER_SIZE); // serial output
StreamCopy                    copier(serial_out, decimated_stream, M_BUFFER_SIZE); // stream the decimated output to serial port

// Arduino Setup
void setup(void) {

  Serial.begin(115200);
  while (!Serial);

  // Include logging to serial, If you want no logging comment this line
  AudioLogger::instance().begin(Serial, AudioLogger::Warning); // Error, Warning, Info, Debug

  auto adcConfig = analog_in.defaultConfig(RX_MODE);

  // For ESP32 by Espressif Systems version 3.0.0 and later:
  // see examples/README_ESP32.md
  adcConfig.sample_rate = M_SAMPLE_RATE;                          // sample rate per channel
  adcConfig.channels = M_NUM_CHANNELS;                            // up to 6 channels
  adcConfig.adc_bit_width = M_ADC_BIT_WIDTH;                      // Supports only 12
  adcConfig.buffer_size = M_ADC_BUFFER_SIZE;                      // in total number of samples
  adcConfig.adc_calibration_active = true;                        // use and apply the calibration settings of ADC stored in ESP32
  adcConfig.is_auto_center_read = false;                          // do not engage auto-centering of signal (would subtract average of signal)
  adcConfig.adc_attenuation = M_ADC_ATTENUATION;                  // set analog input range
  adcConfig.adc_channels[0] = M_ADC0;                             // ensure configuration for each channel
  adcConfig.adc_channels[1] = M_ADC1;
  adcConfig.adc_channels[2] = M_ADC2; 
  adcConfig.adc_channels[3] = M_ADC3; 
  adcConfig.adc_channels[4] = M_ADC4; 
  adcConfig.adc_channels[5] = M_ADC5; 

  analog_in.begin(adcConfig);
  serial_out.begin(info_serial);

}

// Arduino loop 
void loop() {
  copier.copy();
}