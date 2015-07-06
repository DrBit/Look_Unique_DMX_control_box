/*
  DMX_Slave.ino - Example code for using the Conceptinetics DMX library
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

//
// CTC-DRA-13-1 ISOLATED DMX-RDM SHIELD JUMPER INSTRUCTIONS
//
// If you are using the above mentioned shield you should 
// place the RXEN jumper towards G (Ground), This will turn
// the shield into read mode without using up an IO pin
//
// The !EN Jumper should be either placed in the G (GROUND) 
// position to enable the shield circuitry 
//   OR
// if one of the pins is selected the selected pin should be
// set to OUTPUT mode and set to LOGIC LOW in order for the 
// shield to work
//

//
// If the start address is for example 56, then the channels kept
// by the dmx_slave object is channel 56-66
//
#define DMX_SLAVE_CHANNELS   1

#define hz_IDLE 0
#define hz_ON 1
#define hz_count_down_OFF 2
int hazer_mode = hz_IDLE;
unsigned int Frame_received=0;

const int chan_threshold = 25;    // Threshold in order to activate smoke (min. 1)
const int Relay_PIN = 8;    // Pin conected at the relay
const int TRPin = 2;  // Transmit enable / receive mode pin 0 = receive // 1 = transmit
const int DMX_status = 9;
DMX_Slave dmx_slave ( DMX_SLAVE_CHANNELS );  // Configure a DMX slave controller
int dmx_value = 0;
boolean dmx_updated = false;




// timing
boolean HazerShutDown = false;
volatile unsigned long       lastFrameReceivedTime =0;
unsigned long       Fader_OFF_Time;
// Time is set in ms (example: 10000UL = 10 sec)
const unsigned long dmxTimeoutMillis = 10000UL; // 10 seconds
const unsigned long HazerOFFTimoeutMillis = 5000UL; // original -> 900000UL; // = 15 minutes
boolean count_down_off = false;



// the setup routine runs once when you press reset:
void setup() { 
  pinMode ( TRPin , OUTPUT ); 
  digitalWrite ( TRPin , LOW );  // Set to receive mode (my custom shield only)
  pinMode ( DMX_status, OUTPUT ); // Set realy pin as output pin
  digitalWrite (DMX_status, LOW);
  pinMode ( Relay_PIN, OUTPUT ); // (relay is set to Default activated so when we activate relay it will disable the smoke machine)          
  digitalWrite (Relay_PIN, HIGH); // By default the pin is disconnected so will be connected as soon as we receive the first package
  dmx_slave.enable ();    // Enable DMX slave interface and start recording DMX data
  dmx_slave.setStartAddress (1);  // Set start address to 1, this is also the default setting You can change this address at any time during the program
  dmx_slave.onReceiveComplete ( OnFrameReceiveComplete ); // Call back function when a frame is received
  startingdevice ();
}



// the loop routine runs over and over again forever:
void loop() 
{
  if (dmx_updated) {
    analogWrite (DMX_status, dmx_value);
    if ( dmx_value > chan_threshold ) { 
      DMX_to_HAZER_ON ();
    }else{
      DMX_to_HAZER_OFF_DELAYED ();
    }
    dmx_updated = false;
  }

  if (isHazerON && HazerShutDown ) {      // Cheking timer
    if ((millis() - Fader_OFF_Time) > HazerOFFTimoeutMillis) {    // check if time has passed 
      DMX_to_HAZER_OFF ();
    }
  }

  // If we didn't receive a DMX frame within the timeout period  clear all dmx channels  
  if ((millis() - lastFrameReceivedTime) > dmxTimeoutMillis ) {  
    dmx_slave.getBuffer().clear();  // Clear lib data
    dmx_value = 0;    // clear local data
    dmx_updated = true;   // Update data 1 time
    // Since we are not receiving more packets we will never go into function So we have to controll all from here
    DMX_to_HAZER_OFF_DELAYED ();    // Start shut down procedure
  }
}

void OnFrameReceiveComplete (void) {
  lastFrameReceivedTime = millis ();
  dmx_value = dmx_slave.getChannelValue (1);
  dmx_updated = true;
}

void DMX_to_HAZER_OFF () {
  digitalWrite (Relay_PIN, HIGH); // Set DMX to hazer HIGH (meaning disconected) 
}

void DMX_to_HAZER_OFF_DELAYED () {
  if (!HazerShutDown && isHazerON){    // We doit only one time or the dealy mark  (Fader_OFF_Time) would keep updating
    HazerShutDown = true;
    Fader_OFF_Time = millis (); // Record the time fader whent off for calculating the timeout.
  }
}

void DMX_to_HAZER_ON () {
  digitalWrite (Relay_PIN, LOW); // Set DMX to hazer LOW (meaning connected)
  HazerShutDown = false;
}

boolean isHazerON () {
  if (digitalRead (Relay_PIN)) return true;
  return false;
}

void startingdevice () {
  digitalWrite (DMX_status, HIGH);  // Set led HIGH 
  delay(500);
  digitalWrite (DMX_status, LOW);  // Set led LOW 
  delay(500);
  digitalWrite (DMX_status, HIGH);  // Set led HIGH 
  delay(500);
  digitalWrite (DMX_status, LOW);  // Set led LOW
}



 /*
  if ((millis() - lastFrameReceivedTime) > dmxTimeoutMillis ) {  // If we didn't receive a DMX frame within the timeout period  clear all dmx channels  
    dmx_slave.getBuffer().clear();
    // Since we are not receiving more packets we will never go into function
    // So we have to controll all from here
    DMX_to_HAZER_OFF ();
    count_down_off = false;
    hazer_mode = hz_IDLE;
  }

  if (count_down_off) {
    if ((millis() - Fader_OFF_Time) > HazerOFFTimoeutMillis) {    // check if time has passed 
      DMX_to_HAZER_OFF ();
      count_down_off = false;
      hazer_mode = hz_IDLE;
    }
  }
  */
  

  /*
  switch (hazer_mode) {
    case hz_IDLE: {   // wait until we get the data.
      if (Frame_received >0) {
        if (IsDMXON ()) {
          hazer_mode = hz_ON;
          DMX_to_HAZER_ON ();
        }else{
          DMX_to_HAZER_OFF ();
        }
      }
    break;}
    
    case hz_ON: {
      if (Frame_received >0) {
        if (!IsDMXON ()) {
          hazer_mode = hz_count_down_OFF;
          count_down_off = true;
        }
      }
    break;}
    
    case hz_count_down_OFF: {
      if (Frame_received >0) {
        if (IsDMXON ()) {   
          count_down_off = false;
          hazer_mode = hz_ON;
          DMX_to_HAZER_ON ();
        }
      }
    break;}
  } 
  */