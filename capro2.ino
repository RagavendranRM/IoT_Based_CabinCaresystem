#define USE_ARDUINO_INTERRUPTS false
#include <PulseSensorPlayground.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <MQUnifiedsensor.h>

const char ssid[] = "Your WiFi SSID";
const char password[] = "Your WiFi Password";
WiFiClient client;

const long CHANNEL = 2405053; // In this field, enter the Channel ID
const char *WRITE_API = "F4T4SRXVIKJBDTG8"; // Enter the Write API key

long prevMillisThingSpeak = 0;
int intervalThingSpeak = 20000; // 20 seconds to send data to the dashboard

const int OUTPUT_TYPE = SERIAL_PLOTTER;
const int PULSE_INPUT = 32;
const int PULSE_BLINK = 21;
const int PULSE_FADE = 5;
const int THRESHOLD = 550;

byte samplesUntilReport;
const byte SAMPLES_PER_SERIAL_SAMPLE = 10;

PulseSensorPlayground pulseSensor;
const int MQ7_SENSOR_PIN = A0;
MQUnifiedsensor MQ7("Arduino", 5.0, 10, MQ7_SENSOR_PIN, "MQ7");

void setup()
{
  Serial.begin(115200);

  // Initialize PulseSensor
  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.blinkOnPulse(PULSE_BLINK);
  pulseSensor.fadeOnPulse(PULSE_FADE);
  pulseSensor.setSerial(Serial);
  pulseSensor.setOutputType(OUTPUT_TYPE);
  pulseSensor.setThreshold(THRESHOLD);
  samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

  if (!pulseSensor.begin())
  {
    for (;;)
    {
      digitalWrite(PULSE_BLINK, LOW);
      delay(50);
      digitalWrite(PULSE_BLINK, HIGH);
      delay(50);
    }
  }

  // Initialize MQ7 sensor
 
//  MQ7.begin();     // Use begin() instead of the previously suggested method

  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(ssid, password);
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }
}

void loop()
{
  if (pulseSensor.sawNewSample())
  {
    if (--samplesUntilReport == (byte)0)
    {
      samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;
      pulseSensor.outputSample();
      if (pulseSensor.sawStartOfBeat())
      {
        pulseSensor.outputBeat();
      }
    }
    int myBPM = pulseSensor.getBeatsPerMinute();

    if (myBPM < 100 && myBPM > 50)
    {
      int mq7Value = MQ7.readSensor(); // Use readSensor() instead of readCO()

      if (millis() - prevMillisThingSpeak > intervalThingSpeak)
      {
        ThingSpeak.setField(1, String(myBPM).c_str());
        ThingSpeak.setField(2, String(mq7Value).c_str());

        int x = ThingSpeak.writeFields(CHANNEL, WRITE_API);
        if (x == 200)
        {
          Serial.println("Channel update successful.");
        }
        else
        {
          Serial.println("Problem updating channel. HTTP error code " + String(x));
        }

        prevMillisThingSpeak = millis();
      }
    }
  }
}
