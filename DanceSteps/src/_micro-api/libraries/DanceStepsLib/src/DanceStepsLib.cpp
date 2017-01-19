#include "DanceStepsLib.h"

namespace DanceSteps {
#pragma mark Action
	//----------
	Action::Action(size_t parameterCount) {
		this->parameterCount = parameterCount;
		this->parameterWritePosition = 0;
		this->parameters = new float[parameterCount];
	}

	//----------
	Action::~Action() {
		delete[] this->parameters;
	}

	//----------
	bool Action::ready() {
		return this->parameterWritePosition == this->parameterCount;
	}

	//----------
	void Action::setController(Controller * controller) {
		this->controller = controller;
	}

	//----------
	void Action::pushParameter(float parameter) {
		if (this->parameterWritePosition < this->parameterCount) {
			this->parameters[this->parameterWritePosition] = parameter;
			this->parameterWritePosition++;

			if (this->parameterWritePosition == this->parameterCount) {
				this->init();
			}
		}
		else {
			Serial.println("ERROR : Parameter overflow");
		}
	}

	//----------
	bool Action::hasParameter(size_t position) const {
		return this->parameterWritePosition > position;
	}

	//----------
	float Action::getParameter(size_t position) const {
		return this->parameters[position];
	}

	//----------
	size_t Action::getParameterCount() const {
		return this->parameterCount;
	}

#pragma mark On
	//----------
	void On::close() {
		this->controller->enableStepperDriver();
	}

#pragma mark Off
	//----------
	void Off::close() {
		this->controller->disableStepperDriver();
	}

#pragma mark Stop
	//----------
	void Stop::close() {
		this->controller->loop.infinite = false;
		this->controller->loop.remainingIterations = 0;
	}

#pragma mark Loop
	//----------
	void Loop::close() {
		if (this->hasParameter(0)) {
			int loopCount = round(this->getParameter(0));
			if (loopCount < 0) {
				this->controller->loop.infinite = true;
				this->controller->loop.remainingIterations = 0;
			}
			else if (loopCount == 0) {
				this->controller->loop.infinite = false;
				this->controller->loop.remainingIterations = 0;
			}
			else { // loopCount > 0
				this->controller->loop.infinite = false;
				this->controller->loop.remainingIterations = loopCount;
			}
		}
	}

#pragma mark Walk
	//----------
	void Walk::init() {
		this->direction = this->getParameter(0);
		this->steps = this->getParameter(1);
		this->speed = this->getParameter(2);

		this->controller->setDirection(this->direction);
	}

	//----------
	bool Walk::process() {
		unsigned long timeStart = micros();
		unsigned long timeEnd = timeStart + 1e6 * this->timeStep;
		unsigned long lastStep = timeStart;
		unsigned long stepPeriod = 1e6 / this->speed;

		//process the time block
		while (micros() < timeEnd) {

			//exit when complete
			if (this->steps == 0) {
				return true;
			}

			//apply speed
			unsigned long now = micros();
			while (lastStep + stepPeriod > now) {
				delayMicroseconds(1);
				now = micros();
			}

			//do the step
			lastStep = now;
			this->controller->step();
			this->steps--;
		}

		return false;
	}

#pragma mark Sine
	//----------
	void Sine::init() {
		this->period = this->getParameter(0);
		this->amplitude = this->getParameter(1);

		this->controller->setDirection(this->currentDireciton);

		this->timeStart = micros();
		this->position = 0;
	}

	//----------
	bool Sine::process() {
		unsigned long timeEnd = micros() + 1e6 * this->timeStep;

		//process the time block
		while (micros() < timeEnd) {
			//catch up logic, i.e. go to where we should already be
			float phase = (float(micros() - this->timeStart) / 1e6) / this->period * PI * 2.0f;

			bool isFinished = phase > 2 * PI;

			long desiredPosition = isFinished ? 0 : this->amplitude * this->function(phase);

			long delta = desiredPosition - position;
			if (delta > 0 != this->currentDireciton) {
				this->currentDireciton ^= true;
				this->controller->setDirection(this->currentDireciton);
			}
			delta = abs(delta);

			for (int i = 0; i < delta; i++) {
				this->controller->step();
			}
			this->position = desiredPosition;

			if (isFinished) {
				return true;
			}
		}

		return false;
	}

#pragma mark Controller
	//----------
	Controller::Controller() {

	}

	//----------
	void Controller::setup(const Pins & pins, unsigned long baudRate /*= 9600*/) {
		Serial.begin(baudRate);
		
		this->pins = pins;

		pinMode(this->pins.enabled, OUTPUT);
		pinMode(this->pins.direction, OUTPUT);
		pinMode(this->pins.step, OUTPUT);
	}

	//----------
	bool Controller::process() {
		this->processSerial();

		if (this->action) {
			if (this->action->ready()) {
				if (this->disableWhilstStationary) {
					this->enableStepperDriver();
				}

				if (this->action->process()) {
					bool needsToLoop = this->loop.infinite || this->loop.remainingIterations > 1;
					if (needsToLoop) {
						if (!this->loop.infinite) {
							this->loop.remainingIterations--;
							Serial.print(this->loop.remainingIterations);
							Serial.print(" ");
						}
						else {
							Serial.println("LOOP");
						}
						this->action->init();
					}
					else {
						if (this->loop.remainingIterations <= 0) {
							this->clearAction();
							Serial.println("COMPLETE");
						}
					}
				}
				return true;
			}
			else {
				if (this->disableWhilstStationary) {
					this->disableStepperDriver();
				}
				return false;
			}
		}
		else {
			if (this->disableWhilstStationary) {
				this->disableStepperDriver();
			}
			return false;
		}
	}

	//----------
	void Controller::sendCommand(const char * commandString) {
		const char * input = commandString;
		while (*input != '\0') {
			this->pushByte(*input);
			input++;
		}
	}

	//----------
	void Controller::setDisableWhilstStationary(bool disableWhilstStationary) {
		this->disableWhilstStationary = disableWhilstStationary;
	}

	//----------
	void Controller::setDirection(bool direction) {
		digitalWrite(this->pins.direction, direction ? HIGH : LOW);
	}

	//----------
	void Controller::step() {
		//we presume that the slowness of this function acts as sufficient delay
		digitalWrite(this->pins.step, HIGH);
		digitalWrite(this->pins.step, LOW);
	}

	//----------
	void Controller::enableStepperDriver() {
		digitalWrite(this->pins.enabled, this->pins.enabledIsHigh ? HIGH : LOW);
	}

	//----------
	void Controller::disableStepperDriver() {
		digitalWrite(this->pins.enabled, this->pins.enabledIsHigh ? LOW : HIGH);
	}

	//----------
	void Controller::processSerial() {
		while (Serial.available() > 0) {
			int readByte = Serial.read();
			this->pushByte(readByte);
		}
	}

	//----------
	bool isNumeric(const char * text) {
		size_t position = 0;
		bool numeric = false;
		for (;;) {
			switch (text[position]) {
			case '\0':
				return numeric;

			case ' ':
			case '\n':
			case '.':
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				numeric = true;
				break;

			default:
				return false;
			}
			position++;
		}
	}

	//----------
	void Controller::processCommand() {
		//empty command (e.g. ";\n")
		if (this->incoming.position == 0) {
			return;
		}

		//set end of string
		this->incoming.buffer[this->incoming.position] = '\0';
		
		if (strcmp("ON", this->incoming.buffer) == 0) {
			this->setAction(new On());
		}
		if (strcmp("OFF", this->incoming.buffer) == 0) {
			this->setAction(new Off());
		}
		if (strcmp("STOP", this->incoming.buffer) == 0) {
			this->setAction(new Stop());
		}
		else if (strcmp("LOOP", this->incoming.buffer) == 0) {
			this->setAction(new Loop());
		}
		else if (strcmp("WALK", this->incoming.buffer) == 0) {
			this->setAction(new Walk());
		}
		else if (strcmp("SINE", this->incoming.buffer) == 0) {
			this->setAction(new Sine());
		}
		else if (strcmp("COSINE", this->incoming.buffer) == 0) {
			this->setAction(new Cosine());
		}
		else if(isNumeric(this->incoming.buffer)) {
			if (this->action) {
				float parameter = atof(this->incoming.buffer);
				this->action->pushParameter(parameter);
				Serial.print("Parameter set to ");
				Serial.println(parameter);
			}
			else {
				Serial.println("ERROR : Cannot add parameter without an Action");
			}
		}
		else {
			Serial.println("ERROR : Invalid command");
		}
	}

	//----------
	void Controller::pushByte(char inputByte) {
		if (inputByte == '\n' || inputByte == ';') {
			this->processCommand();
			this->incoming.position = 0;
		}
		else {
			//capitalize
			if (inputByte >= 'a' && inputByte <= 'z') {
				inputByte += 'A' - 'a';
			}
			this->incoming.buffer[this->incoming.position++] = inputByte;
		}
	}

	//----------
	void Controller::clearAction() {
		if (this->action) {
			this->action->close();
			delete this->action;
			this->action = NULL;
		}
	}

	//----------
	void Controller::setAction(Action * action) {
		this->clearAction();
		this->action = action;
		this->action->setController(this);
		if (!this->loop.infinite && this->loop.remainingIterations == 0) {
			this->loop.remainingIterations = 1;
		}

		if (!this->action->ready()) {
			Serial.print("Action requires ");
			Serial.print(this->action->getParameterCount());
			Serial.println(" parameters.");
		}
	}
}
