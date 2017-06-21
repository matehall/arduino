/* 
This basic example turns the built in LED on for one second,
then off for one second, repeatedly.
*/

// Pin 13 has a LED connected so we give it a name:
int led = 13;

// the setup routine runs once when you press reset:
void setup()
{
	// initialize the digital pin as an output.
	pinMode(led, OUTPUT);
}

// the loop routine runs over and over again forever:
void loop()
{
	//Set the LED pin to HIGH. This gives power to the LED and turns it on
	digitalWrite(led, HIGH);
	//Wait for a second
	delay(1000);
	//Set the LED pin to LOW. This turns it off
	digitalWrite(led, LOW);
	//Wait for a second
	delay(1000);
}