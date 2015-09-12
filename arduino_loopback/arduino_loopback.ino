/*
Copyright 2015 ReliaSolve.com

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// arduino_loopback program.

// This program is meant to be paired with the ReliaSolve/Latent program
// test_arduino_latency, available from https://github.com/ReliaSolve/Latent.git

// It prints the analog input from a single analogs as quickly as possible,
// It also lets the program request the setting and clearing of digital output
// pin 3.  By hooking a jumper between the digital output and the analog
// input, you can send a command to change the voltage, then read the voltage
// and see how long it takes to get a change.  This tests the entire pipeline
// through the system, including any delays due to filter on the analog input.

// INPUT: (optional) An ASCII 0 or 1 followed by a carriage return.  A 0
// will cause the digital output to go low (its default state).  A 1 will
// cause it to go high.

// OUTPUT: Streaming lines.  Each line consists of:
//   An ASCII analog value from Analog0

//*****************************************************
void setup() 
//*****************************************************
{
  // Set the communication speed on the serial port to the fastest we
  // can.
  Serial.begin(115200);
  
  // Set the timeout on input-string parsing to 1ms, so we
  // don't waste a lot of time waiting for the completion of
  // numbers.
  Serial.setTimeout(1);

  // Set the direction of digital pin 3 to be output, and
  // initialize its value to low.
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);
}

//*****************************************************
void readAndParseInput()
//*****************************************************
{
  // Nothing to do if no available characters.
  if (Serial.available() == 0) {
    return;
  }
 
  // Get the number they sent us, so long as we have
  // new ones to get.  For each, set the pin to high if they
  // send a number greater than 0 or low if they send 0 (or
  // something we can't parse).
  while (Serial.available() > 0) {
    int value = Serial.parseInt();
    if (value > 0) {
      digitalWrite(3, HIGH);
    } else {
      digitalWrite(3, LOW);
    }
  }
}

//*****************************************************
void loop() 
//*****************************************************
{
  readAndParseInput();
  Serial.print(analogRead(0));
  Serial.print("\n");
} //end loop()

