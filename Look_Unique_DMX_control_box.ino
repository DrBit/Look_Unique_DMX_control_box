// Doctor Bit 2015
// http://blog.drbit.nl
// Coded for Paradiso Amsterdam
//
// For use with the Conceptinetics DMX Isolated shield.
//
// This piece of code has been designed to work with a custome made shield on top of the Conceptinetics
// Shield. Refer to schematics for further info.
// The main functionality is to listen to a DMX channel and connect or disconnect DMX signal to an attached 
// device, in this case th Look Unique Hazer.
// Since the lack of control channel on the Unique hazer this will act as one with as less as possible
// modifications on the original hardware.


// I'm using DMX library from Conceptinetics with slight modification to be compatible with the 
// new Arduino IDE. (attached in this repository )

/*
  Conceptinetics DMX library
  Copyright (c) 2013 W.A. van der Meeren <danny@illogic.nl>.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#define __DELAY_BACKWARD_COMPATIBLE__ 
#include <Conceptinetics.h>

// PIN setup
const int chan_threshold = 25;    // Threshold in order to activate smoke (min. 1)
const int Relay_PIN = 8;          // Pin conected at the relay
const int TRPin = 2;              // Transmit enable / receive mode pin 0 = receive // 1 = transmit
const int DMX_status = 9;         // Pin to show DMX status (led connected)
const int Debug_switch = 12;      // PIN 12 is connected to switch but not used 

// DMX Shield setup
const int DMX_SLAVE_CHANNELS = 1; // Number of channels to read
const unsigned int DMX_start_addres = 508;    // Starting DMX address
DMX_Slave dmx_slave ( DMX_SLAVE_CHANNELS );   // Configure a DMX slave controller
int dmx_value = 0;                // Var to store DMX readed value
boolean dmx_updated = false;      // Flag for signaling when a DMX frame has been received

// Timing control
boolean HazerShutDown = false;    // Flag to mark when the hazer is on delayed shut down mode 
volatile unsigned long lastFrameReceivedTime =0;      // Marks when the last DMX frame was received
unsigned long Fader_OFF_Time;                         // Record the time fader whent off for calculating the timeout
const unsigned long dmxTimeoutMillis = 5000UL;       // Timeout when we lose DMX connection - Time is set in ms (example: 10000UL = 10 sec)
const unsigned long HazerOFFTimoeutMillis = 10000UL; // Time out when we switch smoke OFF - original -> 300000UL = 5 minutes

///////////////////////////////////////////////////////////////////////////////////////////////////

void setup() { 
  pinMode ( TRPin , OUTPUT ); 
  digitalWrite ( TRPin , LOW );                           // Set to receive mode (my custom shield only)
  pinMode ( DMX_status, OUTPUT );                         // Set realy pin as output pin
  digitalWrite (DMX_status, LOW);
  pinMode ( Relay_PIN, OUTPUT );                          // (relay is set to Default activated so when we activate relay it will disable the smoke machine)          
  digitalWrite (Relay_PIN, HIGH);                         // By default the pin is disconnected so will be connected as soon as we receive the first package
  pinMode(Debug_switch, INPUT_PULLUP);                    // configurte pin to be as internall pull up input for arduino IDE version 1.0.1 and above
  dmx_slave.enable ();                                    // Enable DMX slave interface and start recording DMX data
  dmx_slave.setStartAddress (DMX_start_addres);           // Set start address to 1, this is also the default setting You can change this address at any time during the program
  dmx_slave.onReceiveComplete ( OnFrameReceiveComplete ); // Call back function when a frame is received
  startingdevice ();                                      // Signals starting procedure
}

void loop() 
{
  if (dmx_updated) {                  // If a new DMX frame has been received
    analogWrite (DMX_status, dmx_value);
    if ( dmx_value > chan_threshold ) {   // If DMX value is grater than the preset threshold 
      DMX_to_HAZER_ON ();             // Switch smoke on
    }else{
      DMX_to_HAZER_OFF_DELAYED ();    // Switch smoke off
    }
    dmx_updated = false;
  }

  if (isHazerON && HazerShutDown ) {  // Cheking timer
    if ((millis() - Fader_OFF_Time) > HazerOFFTimoeutMillis) {    // check if time has passed 
      DMX_to_HAZER_OFF ();
    }
  }

  // If we didn't receive a DMX frame within the timeout period  clear all dmx channels  
  if ((millis() - lastFrameReceivedTime) > dmxTimeoutMillis ) {  
    dmx_slave.getBuffer().clear();    // Clear lib data
    dmx_value = 0;                    // clear local data
    dmx_updated = true;               // Update data 1 time (as if we received a DMX frame)
    // Since we are not receiving more packets we will never go into function smoke shut dowm So we have to controll all from here
    DMX_to_HAZER_OFF_DELAYED ();      // Start shut down procedure
  }
}

void OnFrameReceiveComplete (void) {
  lastFrameReceivedTime = millis ();
  dmx_value = dmx_slave.getChannelValue (1);  // Relative to the buffer (1 would be the first starting address)
  dmx_updated = true;                 // Flag that a frame has been received
}

void DMX_to_HAZER_OFF () {
  digitalWrite (Relay_PIN, HIGH);     // Set DMX to hazer HIGH (meaning disconected) 
}

void DMX_to_HAZER_OFF_DELAYED () {
  if (!HazerShutDown && isHazerON){   // We do it only one time or the delay mark  (Fader_OFF_Time) would keep updating
    HazerShutDown = true;             // Flag to mark when the hazer is on shut down mode delayed
    Fader_OFF_Time = millis ();       // Record the time fader whent off for calculating the timeout.
  }
}

void DMX_to_HAZER_ON () {
  digitalWrite (Relay_PIN, LOW);      // Set DMX to hazer LOW (meaning connected)
  HazerShutDown = false;
}

boolean isHazerON () {
  if (digitalRead (Relay_PIN)) return true;
  return false;
}

void startingdevice () {
  // Blink the DMX status led to signal the start
  int N_blink_times = 4;
  for (int a = 0; a<N_blink_times; a++) {
    digitalWrite (DMX_status, HIGH);    // Set led HIGH 
    delay(100);
    digitalWrite (DMX_status, LOW);     // Set led LOW 
    delay(100);
  }

  // If debug button is pressed (reading LOW) we enter debug mode
  int debug_status = digitalRead (Debug_switch);
  if (debug_status == 0) {
    //entering debug mode (not implemented)
  }
}