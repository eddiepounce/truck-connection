/*
Make a PPM stream from PWM's and Lighting connections on RC Controlled truck.
Copyright (C) 2021  Eddie Pounce

	Channel
		1		Proportional	5th Wheel Locking - to drive trailer legs (PWM from servo)
		2		Analogue		Rear/Stop Lights (uses trailer connector on controller)
		3		On/Off			Indicator	 (uses trailer connector on controller)
		4		On/Off			Indicator	 (uses trailer connector on controller)
		5		On/Off			Reversing Lighting (Opto Isolator in LED circuit)
		6		Proportional	2 way switch - to control Toggle Switch settings on Trailer
												(uses PWM output from reciever)


//#include <Arduino.h>
#include <EnableInterrupt.h>

void interruptReadChannels() {
	nowTime = micros();			// get current time - microseconds
	diffTime = nowTime - oldTime;	// calculate pulse length (time since last rising edge!)
	if (diffTime > minProportionalValue) {		// sort of debounce

setup	
enableInterrupt(inputPin, interruptReadChannels, PULSE_EDGE);

    Copyright

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

	84339861+eddiepounce@users.noreply.github.com

*/

#include <Arduino.h>

// -------------------------
//  Constants and variables to control functionality
//--------------------------
const int outPin 			= 12;	// pin for PPM stream
const int maxChannels		= 8;	// channels to create 
const int framePulseAndAddition = 500;  
				// length of pulses and half the additional time to make PPM give 1 to 2 mS
int debugMode = false;
//int debugMode = true;		// print frame values
// Channels
//		1=legs, 2=rear/stop 3=ind, 4=ind, 5=reversing, 6=TSwitch
//		Throttle monitor.
				
const char channelType[] =   {"-PASSSPSS"};  
//								12345678	// channel
//				P = Proportional (PWM) input
//				A = Analogue input to create 3 state channel (rear/stop lights)
//				//a = Analogue input to create 2 state channel 
//				S = Switch input - light on or off
//				T = Testing (analogue) - put input value in output array

//const int channelPIN[maxChannels+1] = {0,2,A3,4,5,A2,3,0,0};		// channel input pin
const int channelPIN[maxChannels+1] = {0,2,A3,4,5,6,3,0,0};		// channel input pin
//										 1  2 3 4 5 6 7 8  // channel
volatile static int	frameData[maxChannels+1] = {0,0,0,0,0,0,0,498,499};
//		0 - 1000 microS							  1 2 3 4 5 6 7 8  //channel
//volatile static unsigned long propInputTime[maxChannels+1] = {0,0,0,0,0,0,0,0,0};
volatile static unsigned long propInputTime1 = 0;
volatile static unsigned long propInputTime6 = 0;
//		used to store a time (micros) during input for proportional channels (ext interrupts)
// Video Camera Control / Trottle monitoring
const int cameraControlPin = A0;			// Selects forward or reverse camera
const int throttlePin = 8;					// Throttle monitoring pin
const int throttleReverseValue = 600;		// if more - set motion to Reverse
const int throttleForwardValue = 450;		// if less - set motion to Forward
volatile static unsigned long throttlePinTime = 0;
//		used to store a time (micros) during input for pin change interrupt
static int throttleValue = 500;				// initial value - mid point
bool inReverse = false;						// current motion - Forward/Reverse

// --------------------------------
// Proportional settings
// --------------------------------
const int propMinSetting = 0;		// min, mid and max pulse lengths
const int propMidSetting =500;		
const int propMaxSetting = 1000;
const int analogOffValue = 900;		// for Analogue input (inverted)
const int analogOnValue = 350;		// for Analogue input (inverted)

// -------------------------
// for output of frames and debug info
//--------------------------
unsigned long frameTime = 0;
unsigned long nowTime = 0;
unsigned long monTime = 0;

// -------------------------
// Interrupt handler for channelx
//--------------------------
void interruptReadChannel1() {
	if(digitalRead(channelPIN[1])) {
		propInputTime1 = micros();		
	} else {
		frameData[1] = micros() - propInputTime1 - 1000;	// 
	}
}

void interruptReadChannel6() {
	if(digitalRead(channelPIN[6])) {
		propInputTime6 = micros();		
	} else {
		frameData[6] = micros() - propInputTime6 - 1000;	// 
	}
}

void pciSetup(byte pin) {
	// Install Pin Change Interrupt (PCI) for a pin (can be called multiple times)
	//			(care needed in ISR if multiple pins on a port are used)
   *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}
 
ISR (PCINT0_vect) {
    // For PCINT of pins D8 to D13
	// Only 1 pin used in port
	if(digitalRead(throttlePin)) {
		throttlePinTime = micros();		
	} else {
		throttleValue = micros() - throttlePinTime - 1000;	// 
	}
} 
// -------------------------------
//		Setup
//--------------------------------
void setup() {
	Serial.begin(115200);
	if (debugMode) Serial.println("\n--- System Starting ---");
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(outPin, OUTPUT);
	for (int i = 1; i <= maxChannels; i++){
		// set the input pins (hardware) as needed.
		if (channelPIN[i] > 0) pinMode(channelPIN[i], INPUT_PULLUP);		
	}
	attachInterrupt(digitalPinToInterrupt(channelPIN[1]), interruptReadChannel1, CHANGE);
	attachInterrupt(digitalPinToInterrupt(channelPIN[6]), interruptReadChannel6, CHANGE);
	// Camera control pin
	pinMode(cameraControlPin, OUTPUT);
	digitalWrite(cameraControlPin, LOW);	// default is front camera
	pinMode(throttlePin, INPUT);
	// enable interrupt for pin...  -- Pin Change Interrupt (PCI)
	pciSetup(throttlePin);
    //PCICR  |= B00000001;			//"PCIE0" enabeled (PCINT0 to PCINT7)
	//PCMSK0 |= B00000001;			//"PCINT0" enabeled -> D8 will trigger interrupt
}
	
// -------------------------------
//		Main Loop
//--------------------------------
void loop() {
	// read most of the channels 
	//		- proportional ones done via interrupts
	for (int i = 1; i <= maxChannels; i++){
		if (channelPIN[i] > 0) {
			if (channelType[i] == 'A') {
				int valueTemp = analogRead(channelPIN[i]);   // read the input pin
				if (valueTemp > analogOffValue) {
					frameData[i] = propMinSetting;
				} else {
					if (valueTemp < analogOnValue) {
						frameData[i] = propMaxSetting;
					} else {
						frameData[i] = propMidSetting;
					}
				}
			}
			if (channelType[i] == 'S') {
				if (digitalRead(channelPIN[i])) {		// read the input pin
					frameData[i] = propMaxSetting;
				} else {
					frameData[i] = propMinSetting;
				}
			}
//			if (channelType[i] == 'T') {
//				frameData[i] = analogRead(channelPIN[i]);
//				//frameData[i] = digitalRead(channelPIN[i]);
//			}
		}
	}
	
	// =========== output frame =============
	// every 20 milliSeconds
	nowTime = millis();
	if (nowTime - frameTime > 20) {
		for (int i = 1; i <= maxChannels; i++){
			digitalWrite(outPin, HIGH);		// ouput the channel pulse (500uS)
			delayMicroseconds(framePulseAndAddition);
			digitalWrite(outPin, LOW);
			delayMicroseconds(framePulseAndAddition+frameData[i]);
											// wait for the other 500uS and data time
		}
		digitalWrite(outPin, HIGH);			// ouput the end of frame pulse
		delayMicroseconds(framePulseAndAddition);
		digitalWrite(outPin, LOW);
		frameTime = nowTime;				// set frame output time
		digitalWrite(LED_BUILTIN, LOW);		// turn off LED

	// =========== Output Video Camera Control - Front/Rear =============
	// cameraControlPin - controlled by monitoring Throttle and keeping F/R status.
		if (throttleValue > throttleReverseValue) inReverse = true;
		if (throttleValue < throttleForwardValue) inReverse = false;
		if (inReverse) {
			digitalWrite(cameraControlPin, HIGH);
		} else {
			digitalWrite(cameraControlPin, LOW);
		}
		if (debugMode && inReverse) {
			Serial.print(" Reversing. Throttle value: ");
			Serial.println(throttleValue);
		}
	}
			
	// ============== Output Debug info - Frame data and flash LED
	if ((millis() - monTime) > 1000) {  // every second
		digitalWrite(LED_BUILTIN, HIGH);	// turn on LED once a second
				
		if (debugMode) {
			Serial.print("Frame:  ");
			Serial.print("Time: ");
			Serial.print(frameTime);
			Serial.print(";  ");
			for ( int i = 0; i <= maxChannels; i++ ){
				Serial.print("Ch");
				Serial.print(i);
				Serial.print(": ");
				Serial.print(frameData[i]);
				Serial.print("; ");
			}
			Serial.print("Throttle value: ");
			Serial.print(throttleValue);
			Serial.println("");
		}

		// Set debug on or off
		if (Serial.available() > 0) {
			String monSerialRead = Serial.readString();
			if (monSerialRead == "DebugON\n" || monSerialRead == "d\n" || monSerialRead == "d") {
				Serial.println("Debug is now turned on");
				debugMode = true;
			} else {
	//		if (Serial.readString() == "DebugOFF\n") 
				Serial.println("Debug is now turned off.  [reminder: DebugON]");
				debugMode = false;
			}
		}
		monTime = millis();
	}		
}

//   END		END			END			END












