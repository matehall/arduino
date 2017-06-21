#include <SPI.h>
#include <MySensor.h>
#include <DHT.h>

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define HUMIDITY_SENSOR_DIGITAL_PIN 5


unsigned long SLEEP_TIME_SEC = 1 * 60;
unsigned long SLEEP_TIME = SLEEP_TIME_SEC * 1000; // Sleep time between reads (in milliseconds)

MySensor gw;
DHT dht;

float lastTemp;
float lastHum;
boolean metric = true;
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

//Batt measure stuff
int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldBatteryPcnt = 0;

void handleDHT()
{
	delay(dht.getMinimumSamplingPeriod());

	float temperature = dht.getTemperature();
	if (isnan(temperature))
	{
		Serial.println("Failed reading temperature from DHT");
	}
	else if (temperature != lastTemp)
	{
		lastTemp = temperature;
		if (!metric)
		{
			temperature = dht.toFahrenheit(temperature);
		}
		gw.send(msgTemp.set(temperature, 1));
		Serial.print("T: ");
		Serial.println(temperature);
	}

	float humidity = dht.getHumidity();
	if (isnan(humidity))
	{
		Serial.println("Failed reading humidity from DHT");
	}
	else if (humidity != lastHum)
	{
		lastHum = humidity;
		gw.send(msgHum.set(humidity, 1));
		Serial.print("H: ");
		Serial.println(humidity);
	}
	
}

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
	dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);
	gw.begin();

#if defined(__AVR_ATmega2560__)
	analogReference(INTERNAL1V1);
#else
	analogReference(INTERNAL);
#endif




	// Send the Sketch Version Information to the Gateway
	gw.sendSketchInfo("Greenhouse", "1.0");

	// Register all sensors to gw (they will be created as child devices)
	gw.present(CHILD_ID_HUM, S_HUM);
	gw.present(CHILD_ID_TEMP, S_TEMP);

	metric = gw.getConfig().isMetric;
}

void loop()
{
	handleDHT();
	handleBatteri();
	gw.sleep(SLEEP_TIME); //sleep a bit
}


