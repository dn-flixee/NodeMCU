#define BLYNK_PRINT Serial
#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.39370

#include "keys.h"     // Header File where all the API Keys are store
#include <Arduino.h>
#include <FirebaseESP8266.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>   
#include <BlynkSimpleStream.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <addons/SDHelper.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

void cal_dist();
void file_write();

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
WidgetTerminal terminal(V3);    // Blynk Terminal Virtual Pin

// For Connecting Blynk to WiFi
WiFiClient wifiClient;
bool connectBlynk()
{
  wifiClient.stop();
  return wifiClient.connect(BLYNK_DEFAULT_DOMAIN, BLYNK_DEFAULT_PORT);
}
 
// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
WiFiUDP Udp;
const float timeZone = +5.5;    // Time-Zone of India (IST +05:30)
unsigned int localPort = 8888;
time_t getNtpTime();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

const uint8_t chipSelect = SS;  // SD Card Pin
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

const int trigPin = 10;         // Ultrasonic Triger Pins
const int echoPin = 9;          // Ultrasonic Echo Pins
const float error = 0.5;
unsigned long Duration = 0;
unsigned long currentDistanceCm, previousDistanceCm = 300000;
const long diff  = 300000;

void setup()
{
  Serial.begin(9600);
  SD_Card_Mounting();

  WiFiManager wifiManager;
  wifiManager.autoConnect("ESP 8266 - AP","password");
  Serial.println("connected...yeey :)");
  
  connectBlynk();
  Blynk.begin(wifiClient,BLYNK_AUTH_TOKEN);

  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")){
      Serial.println("ok");
      signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  terminal.clear();
  terminal.println(F("Device started"));
  terminal.flush();

}

void loop()
{

  Blynk.run();
  cal_dist();

  if(currentDistanceCm - previousDistanceCm >= diff){
    previousDistanceCm = currentDistanceCm;
    Serial.print("Distance (cm): ");
    Serial.println(currentDistanceCm);
    Serial.print("Time : ");
    Serial.println(millis() / 1000);
  
    terminal.print("Distance (cm): ");
    terminal.println(currentDistanceCm);
    terminal.print("Time : ");
    terminal.println(millis() / 1000);
    file_write();
  }
  delay(1000);
}
void file_write(){

  // Updating time
  timeStatus();
  String formattedTime = String(hour())+":" + String(minute())+":" + String(second());
  String currentDate = String(day())+"-" + String(month())+"-" + String(year());

  // Firebase Database Upload Function
  if(Firebase.ready() && signupOK){
    
    Firebase.RTDB.setFloat(&fbdo,"UltraSonic/" + currentDate + "/"+formattedTime + "/dist", currentDistanceCm);
    Firebase.RTDB.setInt(&fbdo, "UltraSonic/" + currentDate + "/"+formattedTime + "/time", millis() / 1000);
    Serial.println("Data Uploaded");
  }
  else{
    Serial.println("Data Uploading Unsucessful");
  }

  // Saving data in SD Card
  if (!DEFAULT_SD_FS.exists("UltraSonic/" + currentDate +".txt"))
        DEFAULT_SD_FS.mkdir("UltraSonic/");
    File file = DEFAULT_SD_FS.open("UltraSonic/" + currentDate +".txt", "a");
    
    if(file.println(formattedTime +","+ currentDistanceCm+ ","+ millis()/ 1000)){
        Serial.println("File written");
    } 
    else{
        Serial.println("Write failed");
        SD_Card_Mounting();
    }
    file.close();
}

// Function for calculating distance
void cal_dist(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  unsigned long duration = pulseIn(echoPin, HIGH);
  // Calculate the distance
  currentDistanceCm = duration * SOUND_VELOCITY / 2;
  return;
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}