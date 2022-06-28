#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>
#include "DHTesp.h"
#include <Wire.h>           
#include <LiquidCrystal_I2C.h>    
LiquidCrystal_I2C lcd(0x27,16,2); 

DHTesp dht;
bool NewMsg = false;
char Data[2]; 
int ws = 0;
const int pump = 2;
int sensor_pin = A0;
int mois = 0;
int x=0;

const char* mqtt_server = "4434863454a64154b81bab7294207c19.s1.eu.hivemq.cloud";

BearSSL::CertStore certStore;

WiFiClientSecure espClient;
PubSubClient * client;
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (500)
char msg[MSG_BUFFER_SIZE];
int value = 0;


// WiFi
const char *ssid = "adarsh"; // Enter your WiFi name
const char *password = "adarshsingh";  // Enter WiFi password

void setup_wifi() {
   Serial.begin(115200);
  // connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
      lcd.print("WiFi Connecting...");
      delay(500);
      lcd.clear();
      
  }
  lcd.clear();
  lcd.print("WiFi Connected");
  Serial.println("Connected to the WiFi network");
}


void setDateTime() {
  configTime(TZ_Europe_Berlin, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}



void reconnect() {
  // Loop until we’re reconnected
  while (!client->connected()) {
    Serial.println("Attempting MQTT connection…");
    lcd.clear();
    lcd.print("MQTT connecting…");
    delay(500);
    
    String clientId = "ESP8266Client - MyClient";
    if (client->connect(clientId.c_str(), "weather_score", "Score123")) {
      lcd.clear();
      lcd.print("MQTT connected!");
      delay(1000);
      Serial.println("MQTT connected");
      
      //client->publish("testTopic", "hello world");
      //client->subscribe("weather_score/#");
    } else {
      Serial.print("failed, rc = ");
      Serial.print(client->state());
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(9600);
  lcd.init();      
  lcd.backlight();
  delay(500);
 // mqtt setup
  LittleFS.begin();
  setup_wifi();
  setDateTime();
  pinMode(LED_BUILTIN, OUTPUT); 
  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; 
  }
  BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);
  client = new PubSubClient(*bear);
  client->setServer(mqtt_server, 8883);
  client->setCallback(callback);
  pinMode(pump, OUTPUT);

// other setups

  dht.setup(14, DHTesp::DHT11);
}
void callback(char* topic, byte *payload, unsigned int length) 
{
    ws =  ((payload[0]-'0')*10)+(payload[1]-'0'); 
    //Serial.print("data received: ");
    //Serial.println(ws);
    if ((char)payload[0] != NULL) {
      digitalWrite(LED_BUILTIN, LOW); 
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH); 
    } 
    else digitalWrite(LED_BUILTIN, HIGH);    
}

void motor_delay(int del){
  //use port d4, vcc, gnd
   
  Serial.println("pump is on");
  lcd.clear();
  digitalWrite(pump, LOW);
  lcd.print("Pump is on");
  delay(del*400);
  digitalWrite(pump, HIGH); 
  Serial.println("pump is off");
  lcd.clear();
  lcd.print("Pump is off");
  
}
void loop() {
  if (!client->connected())reconnect();
  client->subscribe("weather_score/#");
  //digitalWrite(pump, HIGH); 
  //client->publish("tt", "hello world");
  x=x+1;
  
  
 // temperature_score
 // use port d5, 3.3v,gnd
  int ts; 
  float temp = dht.getTemperature();
  if(temp<5)ts = 10;
  else if (temp>5  && temp<=10)ts = 20; 
  else if (temp>10 && temp<=15)ts = 30;
  else if (temp>15 && temp<=20)ts = 40;
  else if (temp>20 && temp<=25)ts = 50;
  else if (temp>25 && temp<=30)ts = 60;
  else if (temp>30 && temp<=35)ts = 70;
  else if (temp>35 && temp<=40)ts = 80;
  else if (temp>40 && temp<=45)ts = 90;
  else ts = 100;
  ts = ts/5;
  Serial.print("temperature score: ");
  Serial.println(ts);

//moisture_score // use port A0 and gnd 
  int ms;
  mois= analogRead(sensor_pin);
  mois = map(mois,550,0,0,100);
  if(mois<10)ms = 100;
  else if (mois>10  && mois<=20)ms = 90;
  else if (mois>20 && mois<=30)ms = 80;
  else if (mois>30 && mois<=40)ms = 70;
  else if (mois>40 && mois<=50)ms = 60;
  else if (mois>50 && mois<=60)ms = 50;
  else if (mois>60 && mois<=70)ms = 40;
  else if (mois>70 && mois<=80)ms = 30;
  else if (mois>80 && mois<=90)ms = 20;
  else ms = 10;
  ms = (ms/5)*2;
  
  Serial.print("soil moisture score: ");
  Serial.println(ms);

//humidity_score
// use port d5, 3.3v,gnd
  int hs;
  float humi = dht.getHumidity();
  if(humi<10)hs = 10;
  else if (humi>10  && humi<=20)hs = 90;
  else if (humi>20 && humi<=30)hs = 80;
  else if (humi>30 && humi<=40)hs = 70;
  else if (humi>40 && humi<=50)hs = 60;
  else if (humi>50 && humi<=60)hs = 50;
  else if (humi>60 && humi<=70)hs = 40;
  else if (humi>70 && humi<=80)hs = 30;
  else if (humi>80 && humi<=90)hs = 20;
  else hs = 10;
  hs = hs/5;
  Serial.print("humidity score: ");
  Serial.println(hs);
  
//weather_score
  // use ws variable...
  if (ws>20||ws==0) ws = 8;
  Serial.print("weather score: ");
  Serial.println(ws);
//final score
int fs;
fs = ts+hs+ms+ws;

Serial.print("final weather score is: ");
Serial.println(fs);

Serial.println("------------------------");
lcd.clear();
lcd.setCursor(0,0); 
lcd.print("TS=");
lcd.print(ts);
lcd.print("   ");
lcd.print("HS=");
lcd.print(hs);
lcd.setCursor(0,1);
//next line
lcd.print("MS=");
lcd.print(ms);
lcd.print("   ");
lcd.print("WS=");
lcd.print(ws);
delay(1500);
lcd.clear();
lcd.print("Final Score= ");
lcd.print(fs);
delay(1000);
motor_delay(fs);
//delay(2000);
lcd.clear();
lcd.print("cycle ");
lcd.print(x);
lcd.print(" completed!");
delay(5000);
client->loop(); 
}
