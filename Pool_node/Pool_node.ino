// Example sketch showing how to send in OneWire temperature readings
#include <MySensor.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16
unsigned long SLEEP_TIME_SEC = 1 * 60;
unsigned long SLEEP_TIME = SLEEP_TIME_SEC * 1000; // Sleep time between reads (in milliseconds)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
MySensor gw;
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors = 0;
boolean receivedConfig = false;
boolean metric = true;

int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldBatteryPcnt = 0;

// Initialize temperature message
MyMessage msg(0, V_TEMP);

void handleBatteri()
{
	// get the battery Voltage
	int sensorValue = analogRead(BATTERY_SENSE_PIN);
#ifdef DEBUG
	Serial.println(sensorValue);
#endif

	// 1M, 470K divider across battery and using internal ADC ref of 1.1V
	// Sense point is bypassed with 0.1 uF cap to reduce noise at that point
	// ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
	// 3.44/1023 = Volts per bit = 0.003363075
	float batteryV  = sensorValue * 0.003363075;
	int batteryPcnt = sensorValue / 10;

#ifdef DEBUG
	Serial.print("Battery Voltage: ");
	Serial.print(batteryV);
	Serial.println(" V");

	Serial.print("Battery percent: ");
	Serial.print(batteryPcnt);
	Serial.println(" %");
#endif

	if (oldBatteryPcnt != batteryPcnt)
	{
		// Power up radio after sleep
		gw.sendBatteryLevel(batteryPcnt);
		oldBatteryPcnt = batteryPcnt;
	}
}

void setup()
{
	// Startup OneWire
	sensors.begin();

#if defined(__AVR_ATmega2560__)
	analogReference(INTERNAL1V1);
#else
	analogReference(INTERNAL);
#endif
	// Startup and initialize MySensors library. Set callback for incoming messages.
	gw.begin();

	// Send the sketch version information to the gateway and Controller
	gw.sendSketchInfo("Pool", "1.0");

	// Fetch the number of attached temperature sensors
	numSensors = sensors.getDeviceCount();

	// Present all sensors to controller
	for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++)
	{
		gw.present(i, S_TEMP);
	}
}


void loop()
{
	// Process incoming messages (like config from server)
	gw.process();

	// Fetch temperatures from Dallas sensors
	sensors.requestTemperatures();

	// Read temperatures and send them to controller
	for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++)
	{

		// Fetch and round temperature to one decimal
		//  float temperature = static_cast<float>(static_cast<int>((gw.getConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
		float temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(i) * 10.)) / 10.;

		// Only send data if temperature has changed and no error
		if (lastTemperature[i] != temperature && temperature != -127.00)
		{
			// Send in the new temperature
			gw.send(msg.setSensor(i).set(temperature, 1));
			lastTemperature[i] = temperature;
		}
	}

	handleBatteri();
	gw.sleep(SLEEP_TIME);
}



