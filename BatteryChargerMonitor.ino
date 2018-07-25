/*
 * First written by David Kanceruk in June, 2018.
 * All Rights Reserved. 
 * 
 * This project uses the Helium Atom and Element to provide a reliable
 * means of communication for remote monitoring of a battery charger.
 * 
 */

#include "Arduino.h"
#include "Board.h"
#include "Helium.h"
#include "HeliumUtil.h"
#include "TimeLib.h"
#include "BlueDot_BME680.h"
#include "TimerSupport.h"


// NOTE: The structure of the payload is:
//       i = incrementing number
//       T = temperature
//       H = relative humidity
//      DT = a TIMESTAMP string
//      BV = battery voltage
//      IP = battery current peak
//      IA = battery current average


#define CHANNEL_NAME "GCIoTCore"
#define XMIT_PERIOD_MS 20000
#define VOLTS_SAMPLE_RATE_MS 250

Helium  helium(&atom_serial);
Channel channel(&helium);
BlueDot_BME680 bme680 = BlueDot_BME680();


// Define some storage space for buffering data
#define TS_BUF_SIZE 40
char  TSbuf[TS_BUF_SIZE];
char  *TSbuf_ptr = TSbuf;
char  buffer[HELIUM_MAX_DATA_SIZE];

// VREF was measured to be 5.00 volts, so 5.00V / 1023bits = 0.00488 V/bit 
#define ADC_VOLTS_PER_BIT 0.00488
int ADCValue = 0; // ADC Value read from an analog input

// The following is used for producing a timestamp 
struct helium_info info;
time_t timestamp; // Seconds since Jan. 1, 1970
TimeElements tm;

// Battery measurement variables
long highCurrentSamples = 0;
long lowCurrentSamples = 0;
float peakCurrent = 0;
float peakVoltage = 0;

#define MAX_JSON_FORMAT_STR 7
// If one of the following strings is increased in length MAX_JSON_FORMAT_STR has to increase too
// since MAX_JSON_FORMAT_STR is used to limit what can be added to a buffer by snprintf(). It must
// be one more than the length of the string in order to allow room for the string terminator.
char JSON_FORMATS[][MAX_JSON_FORMAT_STR] = {
  "{\"i\":",
  ",\"T\":",
  ",\"H\":",
  ",\"DT\":",
  ",\"V\":",
  ",\"IP\":",
  ",\"IA\":",
  "}"
  };


int addToBuffer(char *insertionPoint, int maxLen, char *src)
{
  // This function inserts a string at the insertionPoint
  // and returns the number of characters that were inserted.
  // It will not insert more than maxLen characters.

  snprintf(insertionPoint, maxLen, src);
  return strlen(insertionPoint);  
}

void sendInfoViaHelium()
{
  // This function sends a JSON format payload using the Helium Atom 

  // Local variables
  static unsigned long i = 0;
  float t  = 0;
  float h  = 0;
  float v  = 0;
  float ip = 0;
  float ia = 0;
  char  *buf_ptr = buffer;
  char  tmpBuf[11];
  long totalSamples = highCurrentSamples + lowCurrentSamples;
     

  // Adjust the index number
  i++;
  Serial.print(F("i = "));
  Serial.println(i);
  buf_ptr += addToBuffer(buf_ptr, MAX_JSON_FORMAT_STR, JSON_FORMATS[0]);
  snprintf(tmpBuf, 11, "%lu", i);
  buf_ptr += addToBuffer(buf_ptr, sizeof(tmpBuf), tmpBuf);

  // Read Temperature
  bme680.writeCTRLMeas();
  t = bme680.readTempC();
  Serial.print(F("Temp     = " ));
  Serial.print(t);
  Serial.println(F(" degrees C"));
  buf_ptr += addToBuffer(buf_ptr, MAX_JSON_FORMAT_STR, JSON_FORMATS[1]);
  dtostrf(t, 3, 2, tmpBuf);
  buf_ptr += addToBuffer(buf_ptr, 6, tmpBuf);

  // Read Humidity
  h = bme680.readHumidity();
  Serial.print(F("Humidity = " ));
  Serial.print(h);
  Serial.println(F(" %"));
  buf_ptr += addToBuffer(buf_ptr, MAX_JSON_FORMAT_STR, JSON_FORMATS[2]);
  dtostrf(h, 3, 2, tmpBuf);
  buf_ptr += addToBuffer(buf_ptr, 6, tmpBuf);

  // Get an ntp based, unix epoch based timestamp and break it into time elements
  helium.info(&info);
  timestamp = info.time;
  Serial.print(F("timestamp = "));
  Serial.println(timestamp);
  breakTime(timestamp, tm);

  // Create the DATETIME string
  snprintf(TSbuf_ptr, TS_BUF_SIZE, "\"%d-%d-%d %d:%d:%d\"", tm.Year + 1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
  Serial.print(F("TSbuf is "));
  Serial.println(TSbuf);
  Serial.print(F("TSbuf length is "));
  Serial.println(strlen(TSbuf));
  buf_ptr += addToBuffer(buf_ptr, MAX_JSON_FORMAT_STR, JSON_FORMATS[3]);
  buf_ptr += addToBuffer(buf_ptr, TS_BUF_SIZE, TSbuf);

  // Prepare the Battery Voltage
  v = peakVoltage;
  Serial.print(F("Voltage  = "));
  Serial.print(v);
  Serial.println(F(" Volts"));
  buf_ptr += addToBuffer(buf_ptr, MAX_JSON_FORMAT_STR, JSON_FORMATS[4]);
  dtostrf(v, 3, 2, tmpBuf);
  buf_ptr += addToBuffer(buf_ptr, 6, tmpBuf);

  // Prepare the Battery Current peak value
  ip = peakCurrent;
  Serial.print(F("Curr pk  = "));
  Serial.print(ip);
  Serial.println(F(" Amps"));
  buf_ptr += addToBuffer(buf_ptr, MAX_JSON_FORMAT_STR, JSON_FORMATS[5]);
  dtostrf(ip, 4, 3, tmpBuf);
  buf_ptr += addToBuffer(buf_ptr, 6, tmpBuf);

  // Prepare the Battery current average value 
  ia = peakCurrent * highCurrentSamples / totalSamples;
  Serial.print(F("Curr avg = "));
  Serial.print(ia);
  Serial.println(F(" Amps"));
  buf_ptr += addToBuffer(buf_ptr, MAX_JSON_FORMAT_STR, JSON_FORMATS[6]);
  dtostrf(ia, 8, 6, tmpBuf);
  buf_ptr += addToBuffer(buf_ptr, 9, tmpBuf);

  // Close the JSON record and print it
  buf_ptr += addToBuffer(buf_ptr, MAX_JSON_FORMAT_STR, JSON_FORMATS[7]);
  Serial.println(buffer);
  Serial.print(F("buffer length is "));
  Serial.println(strlen(buffer));

  channel_send(&channel, CHANNEL_NAME, buffer, strlen(buffer));
}

void measureBattery()
{
  // This function measures the battery current every time it is called.
  // It records the peak current and how many samples are considered to be
  // high current and how many are considered to be low current. These counts
  // will be used to calculate a rough average of the current during a period
  // of XMIT_PERIOD_MS   

  float val = 0;
  float threshold = 0;
  
  // Measure Battery Current (connected via the Imeas board to ADC input A1. Scale is 1V per Amp)
  ADCValue = analogRead(A1);
  
  // Convert the ADC count to Amps:
  val = ADCValue * ADC_VOLTS_PER_BIT;

  // Find the maximum current
  if (val > peakCurrent)
  {
    peakCurrent = val;
  }

  // Adjust the sample count
  if (peakCurrent > 0)
  {
    threshold = peakCurrent / 4.0;
    
    // Record if this sample was high or low
    if (val > threshold)
    {
      highCurrentSamples++;
    }
    if (val <= threshold)
    {
      lowCurrentSamples++;
    }
  }
  else
  {
      lowCurrentSamples++;
  }

  // Check if it is time to measure the voltage
  if (!down_timer_running(READ_VOLTS_timer))
  {
    // Restart the timer
    init_down_timer(READ_VOLTS_timer, VOLTS_SAMPLE_RATE_MS);

    // Measure Battery Voltage (connected via a voltage divider to ADC input A0).
    ADCValue = analogRead(A0);
    
    // Convert the ADC count to Volts:
    // V/bit must be multiplied by 4 because of the voltage divider
    // dropping the voltage to a suitable range for the ADC. 
    val = ADCValue * (ADC_VOLTS_PER_BIT * 4);

    // Find the maximum voltage
    if (val > peakVoltage)
    {
      peakVoltage = val;
    }
  }
}

void resetBatteryVars()
{
  highCurrentSamples = 0;
  lowCurrentSamples = 0;
  peakCurrent = 0;
  peakVoltage = 0;
}

void setup()
{
  int retVal;
  int i;
  Serial.begin(9600);
  DBG_PRINTLN(F("Starting"));

  // Begin communication with the Helium Atom
  helium.begin(HELIUM_BAUD_RATE);

  // Connect the Atom to the Helium Network
  helium_connect(&helium);

  // Begin communicating with the channel
  channel_create(&channel, CHANNEL_NAME);

  // Begin communication with the Temp/Humidity Sensor
  bme680.parameter.I2CAddress = 0x77;                  // Choose I2C Address
  bme680.parameter.sensorMode = 0b01;                  // Default sensor mode = Single measurement, then sleep
  bme680.parameter.IIRfilter = 0b100;                  // Setting IIR Filter coefficient to 15 (default)
  bme680.parameter.humidOversampling = 0b101;          // Setting Humidity Oversampling to factor 16 (default) 
  bme680.parameter.tempOversampling = 0b101;           // Setting Temperature Oversampling factor to 16 (default)
  bme680.parameter.pressOversampling = 0b000;          // factor 0 = Disable pressure measurement
  retVal = bme680.init();
  if (retVal != 0x61)
  {    
    DBG_PRINT(F("BME680 could not be found...  Chip ID was 0x"));
    DBG_PRINT(retVal, HEX);
    DBG_PRINTLN(F(" instead of 0x61"));
  }

  // Start a timer for sending data via Helium 
  init_down_timer(SEND_timer, XMIT_PERIOD_MS);

  // Start a timer for reading the voltage 
  init_down_timer(READ_VOLTS_timer, VOLTS_SAMPLE_RATE_MS);

  // Initialize variables for current measurement averaging
  resetBatteryVars();
}

void loop()
{
  // Measure battery values. We need to do this as often as 
  // possible since some chargers apply pulses of less than 1 ms.
  measureBattery();
  
  // Check if it is time to send a packet
  if (!down_timer_running(SEND_timer))
  {
    // Restart the timer
    init_down_timer(SEND_timer, XMIT_PERIOD_MS);
    
    DBG_PRINT(F("highCurrentSamples = "));
    DBG_PRINTLN(highCurrentSamples);
    DBG_PRINT(F("lowCurrentSamples = "));
    DBG_PRINTLN(lowCurrentSamples);
    
    sendInfoViaHelium();
    resetBatteryVars();
  }
}
