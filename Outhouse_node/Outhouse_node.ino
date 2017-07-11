// Example sketch showing how to send in OneWire temperature readings
#define MY_REPEATER_FEATURE
#define MY_RADIO_NRF24

#include <MySensors.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define COMPARE_TEMP 1 // Send temperature only if changed? 1 = Yes 0 = No

#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16
unsigned long SLEEP_TIM_SEC = 5 * 60;
unsigned long SLEEP_TIME = SLEEP_TIM_SEC * 1000; // Sleep time between reads (in milliseconds)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors = 0;
boolean receivedConfig = false;
boolean metric = true;
// Initialize temperature message
MyMessage msg(0, V_TEMP);
void before()
{
	sensors.begin();
}

void setup(){

}
void presentation()
{
	sendSketchInfo("Outhouse", "1.0");

	// Fetch the number of attached temperature sensors
	numSensors = sensors.getDeviceCount();

	// Present all sensors to controller
	for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++)
	{
		present(i, S_TEMP);
	}
}


void loop()
{
	// Process incoming messages (like config from server)
	process();

	// Fetch temperatures from Dallas sensors
	sensors.requestTemperatures();

	// Read temperatures and send them to controller
	for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++)
	{
		float temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(i) * 10.)) / 10.;

		// Only send data if temperature has changed and no error
		float diff = lastTemperature[i] - temperature;
		float abs_diff = abs(diff);
		boolean min_diff = abs_diff >= 0.5;
		
		if (min_diff && temperature != -127.00)
		{
			// Send in the new temperature
			send(msg.setSensor(i).set(temperature, 1));
			lastTemperature[i] = temperature;
		}
	}
}



