#include "DanceStepsLib.h"

DanceSteps::Controller danceSteps;

const char * regularCommand = "LOOP;12;COSINE;20;8000;"; //4 minutes on
unsigned long regularCommandInterval = 6UL * 60UL * 1000UL; //6 minutes between iterations
unsigned long regularCommandLastActivation = -1;

// the setup function runs once when you press reset or power the board
void setup() {
	DanceSteps::Controller::Pins pins;
	{
		pins.enabled = 5;
		pins.enabledIsHigh = false;
		pins.direction = 6;
		pins.step = 7;
	}
	
	danceSteps.setup(pins);
	danceSteps.setDisableWhilstStationary(true);
}

// the loop function runs over and over again until power down or reset
void loop() {
	if (!danceSteps.process()) {
		delay(10);
	}

	unsigned long now = millis();
	unsigned long durationSinceLastCall = now - regularCommandLastActivation;
	if (regularCommandLastActivation == -1UL
		|| (durationSinceLastCall >= regularCommandInterval)) {
		danceSteps.sendCommand(regularCommand);
		
		Serial.print("Sent command at : ");
		Serial.print(now);
		Serial.print("ms, which is ");
		Serial.print(durationSinceLastCall);
		Serial.println("ms since last call.");

		regularCommandLastActivation = now;
	}
}
