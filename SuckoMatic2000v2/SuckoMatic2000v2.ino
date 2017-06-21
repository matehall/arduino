#include <LiquidCrystal.h>
#include "SuckoMatic2000v2.h"

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

//Digital pins
const int ZERO_PIN = 0;
const int LAMP_PIN = 1;
const int VAC_ON_BUTTON_PIN = 2;
const int VAC_OFF_BUTTON_PIN = 3;
const int ECHO_PIN = 12;
const int TRIGGER_PIN = 13;

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

//LCD
//const int lcd_rs = 8;
const int lcd_rs = 10;
const int lcd_enable = 9;
const int lcd_d4 = 4;
const int lcd_d5 = 5;
const int lcd_d6 = 6;
const int lcd_d7 = 7;
const int lcd_columns = 16;
const int lcd_row = 2;

LiquidCrystal lcd(lcd_rs, lcd_enable, lcd_d4, lcd_d5, lcd_d6, lcd_d7);


void setup()
{
//Serial.begin(9600);

	pinMode(TRIGGER_PIN, OUTPUT);
	pinMode(VAC_ON_BUTTON_PIN, OUTPUT);
	pinMode(VAC_OFF_BUTTON_PIN, OUTPUT);
	pinMode(ECHO_PIN, INPUT);
	pinMode(LAMP_PIN, OUTPUT);
	pinMode(PHOTORESISTOR_PIN, INPUT);

	lcd.begin(lcd_columns, lcd_row);
}

static Event get_next_event()
{

	photoResistorValue = analogRead(PHOTORESISTOR_PIN);
	fuel_level = get_fuel_level();

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

//void turn_light_on() {
//	digitalWrite(LAMP_PIN, HIGH);
//}
//
//void turn_light_off() {
//	digitalWrite(LAMP_PIN, LOW);
//}

static String get_state_string()
{
	switch (state)
	{
		case ST_CYCLONE_EMPTENING:
			return "Emptening Cyclo";
			break;
		case ST_CYCLONE_FILLING:
			return "Filling Cyclone";
			break;
		case ST_NORMAL:
			return "Normal";
			break;
		case ST_ERR_VAC_ON_TO_LONG:
			return "Vac on to long";
		default:
			return "Err";
	}
}

void updateLCD()
{
	if (state != ST_ERR)
	{
		lcd.setCursor(0, 0);
		lcd.print("R     ");
		lcd.setCursor(1, 0);
		lcd.print(photoResistorValue);

		lcd.setCursor(6, 0);
		lcd.print("L     ");
		lcd.setCursor(7, 0);
		lcd.print(fuel_level);

		lcd.setCursor(12, 0);
		lcd.print("V    ");
		lcd.setCursor(13, 0);
		lcd.print(vac_state * 10 + vac_restarts_attempts);

		lcd.setCursor(0, 1);
		lcd.print("S:               ");
		lcd.setCursor(2, 1);
		lcd.print(get_state_string());
	}
	else
	{
		lcd.setCursor(0, 0);
		lcd.print("ERR:Vac on long.");
		lcd.setCursor(0, 1);
		lcd.print("Max time(sec):");
		lcd.print(MAX_NUMBER_OF_SECONDS);

	}

//	Serial.print("photoResistorValue:");
//	Serial.print(photoResistorValue);
//	Serial.print(" ");
//	Serial.print("fuel_level:");
//	Serial.print(fuel_level);
//	Serial.print(" ");
//	Serial.print("state:");
//	Serial.print(get_state_string());
//	Serial.println();

}

void loop()
{
	int i;
	state = ST_NORMAL;
	while (state != ST_TERM)
	{
		delay(1000);
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
		updateLCD();
	}
}
