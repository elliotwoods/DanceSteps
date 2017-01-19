#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

using namespace std;

namespace DanceSteps {
	class Controller;

	//-----------
	class Action {
	public:
		Action(size_t parameterCount = 0);
		virtual ~Action();

		bool ready(); //returns false when not enough parameters

		virtual void init() { }
		//override this, and return true on completion
		virtual bool process() { return true; }
		virtual void close() { }

		void setController(Controller *);

		void pushParameter(float);
		bool hasParameter(size_t position) const;
		float getParameter(size_t position) const;
		size_t getParameterCount() const;
	protected:
		Controller * controller;

		float * parameters;
		size_t parameterCount;
		size_t parameterWritePosition;

		float timeStep = 2.0f;
	};

	//-----------
	class On : public Action {
	public:
		On() : Action(0) { }
		void close() override;
	protected:
		Controller * controller;
	};

	//-----------
	class Off : public Action {
	public:
		Off() : Action(0) { }
		void close() override;
	protected:
		Controller * controller;
	};

	//-----------
	class Stop : public Action {
	public:
		Stop() : Action(0) { }
		void close() override;
	protected:
		Controller * controller;
	};

	//-----------
	class Loop : public Action {
	public:
		Loop() : Action(1) { }
		void close() override;
	protected:
	};

	//-----------
	class Walk : public Action {
	public:
		Walk() : Action(3) { }
		void init() override;
		bool process() override;
	protected:
		float speed;
		bool direction;
		size_t steps;
	};

	//-----------
	class Sine : public Action {
	public:
		Sine() : Action(2) { }
		void init() override;
		bool process() override;
		virtual float function(float x) { return sin(x); }
	protected:
		bool currentDireciton = true;
		unsigned long timeStart;
		long position;
		float period;
		float amplitude;
	};

	//-----------
	class Cosine : public Sine {
	public:
		float function(float x) override { return cos(x); }
	};

	//-----------
	class Controller {
	public:
		struct Pins {
			uint8_t enabled;
			bool enabledIsHigh = false; //true = enabled pin is HIGH during operation
			uint8_t direction;
			uint8_t step;
		};

		Controller();
		void setup(const Pins & pins, unsigned long baudRate = 115200);
		bool process();

		void sendCommand(const char * commandString);
		void setDisableWhilstStationary(bool);

		void setDirection(bool);
		void step();
		void enableStepperDriver();
		void disableStepperDriver();
	protected:
		void processSerial();
		void processCommand();

		void pushByte(char);

		void clearAction();
		void setAction(Action *);

		bool disableWhilstStationary = false;

		Pins pins;

		struct {
			char buffer[1000];
			size_t position = 0;
		} incoming;

		Action * action = NULL;

		struct {
			bool infinite = false;
			int remainingIterations = 0;
		} loop;

		friend Loop;
		friend Stop;
	};
}
