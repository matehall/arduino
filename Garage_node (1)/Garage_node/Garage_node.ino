// Example sketch showing how to send in OneWire temperature readings
#include <MySensor.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16
unsigned long SLEEP_TIME_MINUTES = 5;
unsigned long SLEEP_TIME_SEC = SLEEP_TIME_MINUTES * 60;
unsigned long SLEEP_TIME = SLEEP_TIME_SEC * 1000; // Sleep time between reads (in milliseconds)
unsigned long FORCE_REPORT_TIME_MINUTES = 60; //Fore a report to server

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
MySensor gw;
float lastTemperature[MAX_ATTACHED_DS18B20];
unsigned int time_since_last_report[MAX_ATTACHED_DS18B20];

int numSensors = 0;
boolean receivedConfig = false;
boolean metric = true;
// Initialize temperature message
MyMessage msg(0, V_TEMP);

void setup(){
	sensors.begin();
	gw.begin();
	//gw.begin(NULL, AUTO, true);
	gw.sendSketchInfo("Garage", "1.0");
	numSensors = sensors.getDeviceCount();

	for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++){
		gw.present(i, S_TEMP);
		time_since_last_report[i] = 0;
	}
}

void loop(){
	//gw.process();
	sensors.requestTemperatures();

	for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++){
		float temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(i) * 10.)) / 10.;

		if (reading_differs_from_last_reading(i, temperature) && no_errors(temperature) ){
			gw.send(msg.setSensor(i).set(temperature, 1));
			lastTemperature[i] = temperature;
			time_since_last_report[i] = 0;
		}else{
			time_since_last_report[i] = time_since_last_report[i] + SLEEP_TIME_MINUTES;
		}
	}
	
	gw.sleep(SLEEP_TIME);
}

boolean time_to_force_report(unsigned int time_since_last_report){
	if(time_since_last_report >= FORCE_REPORT_TIME_MINUTES){
		return true;
	}
	return false;
}

boolean reading_differs_from_last_reading(int sensor_index, unsigned long current_value)
{
	float diff = lastTemperature[sensor_index] - current_value;
	float abs_diff = abs(diff);
	boolean min_diff = abs_diff >= 0.5;
	return min_diff;
}

boolean no_errors( unsigned long current_value){
	if(current_value != -127.00){
		return true;
	}
	return false;
}

