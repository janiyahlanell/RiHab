#include <Wire.h>
//===================
//Hardware Pins & Address Definitions
//===================================

const int ROTARY_PIN = A0; // anolog pin coonnected to rotary angle sensor
const int BUTTON_PIN = 2; // Digital Pin connected fo rpush button
const byte ACCEL_ADDRESS = 0x4c;  // hardware address for our accelleromoter

int startAngle = 0; // baseline resting angle
int maxAngle = 0; // deepest point of teh knee bend
// for the python script 
unsigned long repStartTime = 0; 
int totalFlux = 0; //score the last of z-axis vibration
int prevZ = 0; // stores teh last samples z-axis acceleration value
// previous z will remember teh last moment z-axis and total flux will calculate teh difference between current z-reading and previous z-reading
bool inMotion = false; //bool flag tracking whether a rep is active

bool isRecording = false;
bool lastButtonState = LOW;

void setup(){
  //STep1. open serial communication pipeline to your laptop at 9600 baud
  Serial.begin(9600);
  //Step2. Initialize the I2C wire library for accelerometer
  Wire.begin();

  pinMode(BUTTON_PIN, INPUT);
/*
  //wake up accellerometer
  Wire.beginTransmission(ACCEL_ADDRESS);
  Wire.write(0x07);
  Wire.write(0x01);
  Wire.endTransmission();
*/
  delay(500); // pause to etthe hardware fully stabilize

  Serial.println("System initialized. Press the button to start recording.");

}

void loop(){
  //============================
  //PHYSICAL BUTTON START/STOP LOOP
  //==============================

  bool currButtonState = digitalRead(BUTTON_PIN); // 

  //To detect button press (transition for mlow to high)
  if(lastButtonState == LOW && currButtonState == HIGH) {
    isRecording = !isRecording; // flips the status from what it was opposite (we need it to flip and not stay on "on" or "off")

    if(isRecording){
      Serial.println("\n-- RECORDING ACTIVE ---");
      //INSTANT CALIBRATION FOR WHICHEVER LEG THE HARDWARE IS STRAPPED TO ; makes teh sensor go back to 0
      startAngle = analogRead(ROTARY_PIN);
      maxAngle = startAngle; // setting your baseline
    } else{
      Serial.println("\n--- PAUSED: SWAP LEGS NOW ---");
      inMotion = false; //reset motion stste safely
    }
    delay(250); //debounce delay to prevent accidental clicks
  }
  lastButtonState = currButtonState; // flips it back

  if(isRecording){
    int currAngle = analogRead(ROTARY_PIN);

    //request the z-axis data from the accelerometer
    Wire.beginTransmission(ACCEL_ADDRESS);
    Wire.write(0x02);
    Wire.endTransmission();
    Wire.requestFrom((int)ACCEL_ADDRESS, 1);

    int currZ = 0; 
    if(Wire.available() > 0){
      currZ = Wire.read();
      if(currZ > 31){
        currZ -= 64; // convert 6-bit signed format
      }

    if (abs(currAngle - startAngle) > 20 && !inMotion){
      inMotion = true;
      repStartTime = millis();
      maxAngle = currAngle;
      totalFlux = 0;
      prevZ = currZ;
    }
    //Process Movement Data
    if(inMotion){
      if(currAngle > maxAngle){
        maxAngle = currAngle;
      }
      totalFlux += abs(currZ - prevZ); //accumulate instability
      prevZ = currZ;

      //Detetct the end of teh rep
      if(abs(currAngle - startAngle) < 10){
        int finalAngle = maxAngle - startAngle;
        unsigned long finalSpeed = millis() - repStartTime;

        //ouput clean telemetry for python ingestion
        Serial.print("Angle:");
        Serial.print(finalAngle);
        Serial.print(",Speed:");
        Serial.print(finalSpeed);
        Serial.print(",Flux:");
        Serial.println(totalFlux);

        inMotion = false;
        delay(1000);
      }
    }
  }
  delay(50);
}
}
