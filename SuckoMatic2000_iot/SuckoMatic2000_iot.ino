#include "SuckoMatic2000.h"
#include <SPI.h>
#include <MySensor.h>  

MySensor gw;
#define CHILD_ID 1
MyMessage msg(CHILD_ID, V_DISTANCE);
int last_sent_level = 6000;
boolean metric = true; 

typedef struct
{
	enum State state;
	enum Event event;
	State (*fn)(void);
} tTransition;

tTransition trans[] =
{
	{ST_NORMAL, EV_BOX_EMPTY, &box_empty },
	{ST_CYCLONE_FILLING, EV_BOX_FULL, &box_full },
	{ST_CYCLONE_FILLING, EV_CYCLONE_FULL, &cycone_full },
	{ST_CYCLONE_FILLING, EV_NO_EVENT, &self_trans },
	{ST_CYCLONE_FILLING, EV_VAC_TO_LONG, &vac_to_long },
	{ST_CYCLONE_EMPTENING, EV_CYCLONE_EMPTY, &cyclone_empty },
	{ST_CYCLONE_EMPTENING, EV_BOX_FULL, &box_full },
	{ST_CYCLONE_EMPTENING,	EV_NO_EVENT, &self_trans },
	{ST_ERR_VAC_ON_TO_LONG,	EV_NO_EVENT, &self_trans },
};

#define TRANS_COUNT (sizeof(trans)/sizeof(*trans))

//Levels
const int BOX_EMPTY_LEVEL = 80;
const int BOX_FULL_LEVEL = 45;
const int CYCLONE_FULL = 600;
const int MAX_NUMBER_OF_SECONDS = 120;
const int MAX_VAC_RESTART_ATTEMPTS = 2;
const int MAX_ALLOWED_BOX_LEVEL = 100;
const int MAX_ALLOWED_READING_CHANGE = 5;

//Digital pins
const int ZERO_PIN = 0;
const int LAMP_PIN = 1;
const int VAC_ON_BUTTON_PIN = 4;
const int VAC_OFF_BUTTON_PIN = 3;
const int ECHO_PIN = 7; //12
const int TRIGGER_PIN = 8; //13
const int ON_OFF_SWITCH = 10; //?

//Analog pins
const int PHOTORESISTOR_PIN = 0;
long fuel_level = 0;
int photoResistorValue = 0;
int vac_restarts_attempts = 0;

const int VAC_OFF = 0;
const int VAC_ON = 1;

//Vac control

int vac_state = VAC_OFF;
unsigned long vacc_start_time = 0;

void setup()
{
	pinMode(TRIGGER_PIN, OUTPUT);
	pinMode(VAC_ON_BUTTON_PIN, OUTPUT);
	pinMode(VAC_OFF_BUTTON_PIN, OUTPUT);
	pinMode(ECHO_PIN, INPUT);
	pinMode(LAMP_PIN, OUTPUT);
	pinMode(PHOTORESISTOR_PIN, INPUT);
	pinMode(ON_OFF_SWITCH, INPUT);
	
	gw.begin();

	gw.sendSketchInfo("Suckomatic 2000", "1.0");
	gw.present(CHILD_ID, S_DISTANCE);
	boolean metric = gw.getConfig().isMetric;
}

static Event get_next_event()
{
	photoResistorValue = analogRead(PHOTORESISTOR_PIN);
	fuel_level = get_fuel_level();
	
	if (fuel_level >= last_sent_level + MAX_ALLOWED_READING_CHANGE){
		fuel_level = last_sent_level;
	}
	
	if(has_level_changed()){
		send_level();
	}
	
	switch (state)
	{
		case ST_NORMAL:
			if (box_is_empty())
			{
				return EV_BOX_EMPTY;
			}
			return EV_NO_EVENT;
			break;
		case ST_CYCLONE_FILLING:
			if (vac_has_been_on_more_then(MAX_NUMBER_OF_SECONDS))
			{
				return EV_VAC_TO_LONG;
			}

			if (box_is_full())
			{
				return EV_BOX_FULL;
			}

			if (cyclone_is_full())
			{
				return EV_CYCLONE_FULL;
			}

			return EV_NO_EVENT;
			break;
		case ST_CYCLONE_EMPTENING:
			if (box_is_full())
			{
				return EV_BOX_FULL;
			}

			if (cyclone_is_empty())
			{
				return EV_CYCLONE_EMPTY;
			}

			return EV_NO_EVENT;
			break;
		default:
			return EV_NO_EVENT;
	}
}

static State self_trans(void)
{
	return state;
}

static State box_empty()
{
	turn_vac_on();
	return ST_CYCLONE_FILLING;
}

static State box_full()
{
	turn_vac_off();
	return ST_NORMAL;
}

static State cycone_full()
{
	turn_vac_off();
	vac_restarts_attempts = 0;
	return ST_CYCLONE_EMPTENING;
}

static State cyclone_empty()
{
	turn_vac_on();
	return ST_CYCLONE_FILLING;
}

static State vac_to_long()
{
	vac_restarts_attempts++;
	turn_vac_off();
	if (vac_restarts_attempts > MAX_VAC_RESTART_ATTEMPTS)
	{
		return ST_ERR;
	}
	delay(5000);
	turn_vac_on();
	return ST_CYCLONE_FILLING;
}

boolean vac_has_been_on_more_then(int max_vacc_on_in_seconds)
{

	unsigned long current_time = millis();
	unsigned long elapsed_time = (current_time - vacc_start_time) / 1000;

	if (elapsed_time > max_vacc_on_in_seconds)
	{
		return true;
	}

	return false;
}

long microsecondsToCentimeters(long microseconds)
{
	return microseconds / 29 / 2;
}

long get_fuel_level()
{
	long duration;

	//sending the signal, starting with LOW for a clean signal
	digitalWrite(TRIGGER_PIN, LOW);
	delayMicroseconds(2);
	digitalWrite(TRIGGER_PIN, HIGH);
	delayMicroseconds(5);
	digitalWrite(TRIGGER_PIN, LOW);

	duration = pulseIn(ECHO_PIN, HIGH);
	
	return microsecondsToCentimeters(duration);
}

boolean box_is_empty()
{
	return fuel_level > BOX_EMPTY_LEVEL;
}

boolean box_is_full()
{
	return fuel_level < BOX_FULL_LEVEL;
}

boolean cyclone_is_full()
{
	return photoResistorValue > CYCLONE_FULL;
}

boolean cyclone_is_empty()
{
	return photoResistorValue < CYCLONE_FULL;
}

void turn_vac_on()
{
	if (vac_is_off())
	{
		digitalWrite(VAC_ON_BUTTON_PIN, HIGH);
		delay(500);
		digitalWrite(VAC_ON_BUTTON_PIN, LOW);
		vac_state = VAC_ON;
		vacc_start_time = millis();
	}
}

void turn_vac_off()
{
	if (vac_is_on())
	{
		digitalWrite(VAC_OFF_BUTTON_PIN, HIGH);
		delay(500);
		digitalWrite(VAC_OFF_BUTTON_PIN, LOW);
		vac_state = VAC_OFF;
	}
}

boolean vac_is_on()
{
	return vac_state == VAC_ON;
}

boolean vac_is_off()
{
	return vac_state == VAC_OFF;
}

void send_level(){
	gw.send(msg.set(fuel_level));
	last_sent_level = fuel_level;
}

boolean has_level_changed(){
	if(fuel_level > last_sent_level + 1){ 
		return true; 
	}else if(fuel_level < last_sent_level - 1){
		return true;
	}else{ 
		return false;
	}
}

//void turn_light_on() {
//	digitalWrite(LAMP_PIN, HIGH);
//}
//
//void turn_light_off() {
//	digitalWrite(LAMP_PIN, LOW);
//}

void loop()
{
	gw.process();
	int i;
	state = ST_NORMAL;
	while (state != ST_TERM)
	{
		gw.wait(1000);
		event = get_next_event();
		for (i = 0; i < TRANS_COUNT; i++)
		{
			if ((state == trans[i].state) || (ST_ANY == trans[i].state))
			{
				if ((event == trans[i].event) || (EV_ANY == trans[i].event))
				{
					state = (trans[i].fn)();
					break;
				}
			}
		}
		//updateLCD();
	}
}
