// #include <Arduino.h>
// #define SOUND_VELOCITY 0.034
// #define CM_TO_INCH 0.39370

// void cal_dist();
// void file_write();

// const uint8_t chipSelect = SS;  // SD Card Pin
// unsigned long sendDataPrevMillis = 0;
// int count = 0;
// bool signupOK = false;

// const int trigPin = 10;         // Ultrasonic Triger Pins
// const int echoPin = 9;          // Ultrasonic Echo Pins
// const float error = 0.5;
// unsigned long Duration = 0;
// float currentDistanceCm, previousDistanceCm = 0;

// void setup()
// {
//   Serial.begin(9600);

//   pinMode(trigPin, OUTPUT);
//   pinMode(echoPin, INPUT);
// }

// void loop()
// {

//   cal_dist();

//   if(((previousDistanceCm - error) > currentDistanceCm) || ((previousDistanceCm + error) < currentDistanceCm)){
//     previousDistanceCm = currentDistanceCm;
//     Serial.print("Distance (cm): ");
//     Serial.println(currentDistanceCm);
//     Serial.print("Time : ");
//     Serial.println(millis() / 1000);
  
//     file_write();
//   }
//   delay(1000);
// }


// // Function for calculating distance
// void cal_dist(){
//   digitalWrite(trigPin, LOW);
//   delayMicroseconds(2);
//   digitalWrite(trigPin, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(trigPin, LOW);

//   // Reads the echoPin, returns the sound wave travel time in microseconds
//   unsigned long duration = pulseIn(echoPin, HIGH);
//   // Calculate the distance
//   currentDistanceCm = duration * SOUND_VELOCITY / 2;
//   return;
// }