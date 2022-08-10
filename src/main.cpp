#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <OneButton.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <PubSubClient.h>
#include <NTPClient.h>
#include "EEPROM.h"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

#define FIREBASE_PROJECT_ID "mikiko-c5ca4"
#define STORAGE_BUCKET_ID "gs://mikiko-c5ca4.appspot.com"
#define API_KEY "AIzaSyAMMrTWIU5gKeCDKwiLwO-7liVvfpT8u-M"
#define DATABASE_URL "https://mikiko-c5ca4-default-rtdb.firebaseio.com/"

// device info
#define DEVICE_EMAIL "mikikoSON@mikiko.com"
#define DEVICE_PASS "mikikoSON"
#define FIRMWARE_VERSION "0.0.1"

const char *mqtt_broker = "broker.hivemq.com";
const char *mqtt_username = "mikiko";
const char *mqtt_password = "mikiko";
const int mqtt_port = 1883;

#define LENGTH(x) (strlen(x) + 1) // length of char string
#define EEPROM_SIZE 200           // EEPROM size

#define out1 12
#define out2 5
#define out3 4
#define out4 15

#define WIFI_LED 13

Ticker ticker;
WiFiUDP udp;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

WiFiClient espClient;
PubSubClient client(espClient);

DynamicJsonDocument schedule(2048);

String ssid; // string variable to store ssid
String pss;  // string variable to store password
String MACADD = WiFi.macAddress();

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void writeStringToFlash(const char *toStore, int startAddr)
{
  int i = 0;
  for (; i < LENGTH(toStore); i++)
  {
    EEPROM.write(startAddr + i, toStore[i]);
  }
  EEPROM.write(startAddr + i, '\0');
  EEPROM.commit();
}

String readStringFromFlash(int startAddr)
{
  char in[128]; // char array of size 128 for reading the stored data
  int i = 0;
  for (; i < 128; i++)
  {
    in[i] = EEPROM.read(startAddr + i);
  }
  return String(in);
}

String topic1;
String topic2;
String topic3;
String topic4;
String schedule_topic;
String documentPath;
String hour;
String minutes;
String sendTime;

int days;

unsigned long int duration1;
unsigned long int duration2;
unsigned long int duration3;
unsigned long int duration4;

unsigned long int millis_duration1;
unsigned long int millis_duration2;
unsigned long int millis_duration3;
unsigned long int millis_duration4;
unsigned long int current_millis;

bool timmerCheck1 = false;
bool timmerCheck2 = false;
bool timmerCheck3 = false;
bool timmerCheck4 = false;

bool schedule_check1 = true;
bool schedule_check2 = true;
bool schedule_check3 = true;
bool schedule_check4 = true;

bool wifi_state = false;

OneButton btn1 = OneButton(
    0,
    true,
    true);
OneButton btn2 = OneButton(
    9,
    true,
    true);
OneButton btn3 = OneButton(
    10,
    true,
    true);
OneButton btn4 = OneButton(
    14,
    true,
    true);

static void handleClick1()
{
  int state = !digitalRead(out1);
  digitalWrite(out1, state);

  if (wifi_state)
  {
    client.publish(topic1.c_str(), state == 1 ? "true" : "false");
  }
}

static void handleClick2()
{
  int state = !digitalRead(out2);
  digitalWrite(out2, state);

  if (wifi_state)
  {
    client.publish(topic2.c_str(), state == 1 ? "true" : "false");
  }
}

static void handleClick3()
{
  int state = !digitalRead(out3);
  digitalWrite(out3, state);

  if (wifi_state)
  {
    client.publish(topic3.c_str(), state == 1 ? "true" : "false");
  }
}

static void handleClick4()
{
  int state = !digitalRead(out4);
  digitalWrite(out4, state);

  if (wifi_state)
  {
    client.publish(topic4.c_str(), state == 1 ? "true" : "false");
  }
}

char udpbuf[255];
char replyPacket[] = "SON:4CH:MIKIKO";

void mqtt_process(char *topic, byte *payload)
{

  String msg;
  String strTopic;

  strTopic = String((char *)topic);
  if (strTopic == topic1)
  {

    msg = String((char *)payload);

    Serial.println(msg);

    if (msg == "true")
    {
      digitalWrite(out1, HIGH);
    }
    else
    {
      digitalWrite(out1, LOW);
    }
  }
  else if (strTopic == topic2)
  {

    msg = String((char *)payload);

    Serial.println(msg);

    if (msg == "true")
    {
      digitalWrite(out2, HIGH);
    }
    else
    {
      digitalWrite(out2, LOW);
    }
  }
  else if (strTopic == topic3)
  {

    msg = String((char *)payload);

    Serial.println(msg);

    if (msg == "true")
    {
      digitalWrite(out3, HIGH);
    }
    else
    {
      digitalWrite(out3, LOW);
    }
  }
  else if (strTopic == topic4)
  {
    msg = String((char *)payload);

    Serial.println(msg);

    if (msg == "true")
    {
      digitalWrite(out4, HIGH);
    }
    else
    {
      digitalWrite(out4, LOW);
    }
  }
  else if (strTopic == schedule_topic)
  {
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), "schedule"))
    {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      deserializeJson(schedule, fbdo.payload());
      schedule["schedule"] = schedule["fields"]["schedule"]["arrayValue"]["values"];
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  payload[length] = '\0';
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  Serial.println("-----------------------");

  mqtt_process(topic, payload);
}

static void wifi_led()
{
  digitalWrite(WIFI_LED, !digitalRead(WIFI_LED));
}

void btnLongPress()
{
  writeStringToFlash("", 0);  // storing ssid at address 0
  writeStringToFlash("", 40); // storing pss at address 40
  ESP.restart();
}

void setup()
{
  Serial.begin(115200);
  // put your setup code here, to run once:

  EEPROM.begin(EEPROM_SIZE);

  // writeStringToFlash("", 0);  // storing ssid at address 0
  // writeStringToFlash("", 40); // storing pss at address 40

  pinMode(out1, OUTPUT);
  pinMode(out2, OUTPUT);
  pinMode(out3, OUTPUT);
  pinMode(out4, OUTPUT);
  pinMode(WIFI_LED, OUTPUT);

  btn1.attachClick(handleClick1);
  btn1.attachLongPressStart(btnLongPress);
  btn1.setPressTicks(3000);
  btn2.attachClick(handleClick2);
  btn3.attachClick(handleClick3);
  btn4.attachClick(handleClick4);

  btn1.tick();
  btn2.tick();
  btn3.tick();
  btn4.tick();

  MACADD = getValue(MACADD, 58, 0) + getValue(MACADD, 58, 1) + getValue(MACADD, 58, 2) + getValue(MACADD, 58, 3) + getValue(MACADD, 58, 4) + getValue(MACADD, 58, 5);
  MACADD.toLowerCase();

  topic1 = String("/" + String(MACADD) + "/data/btnone");
  topic2 = String("/" + String(MACADD) + "/data/btntwo");
  topic3 = String("/" + String(MACADD) + "/data/btnthree");
  topic4 = String("/" + String(MACADD) + "/data/btnfour");
  schedule_topic = String("/" + String(MACADD) + "/data/schedule");
  documentPath = String("devices/" + MACADD);

  ssid = readStringFromFlash(0); // Read SSID stored at address 0
  Serial.print("SSID = ");
  Serial.println(ssid);
  pss = readStringFromFlash(40); // Read Password stored at address 40
  Serial.print("psss = ");
  Serial.println(pss);

  ticker.attach(0.2, wifi_led);

  Serial.println(MACADD);

  if (ssid.length() > 0 && pss.length() > 0)
  {
    WiFi.begin(ssid.c_str(), pss.c_str());

    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED)
    {
      btn1.tick();
      btn2.tick();
      btn3.tick();
      btn4.tick();
      delay(1);
    }
    Serial.println(WiFi.localIP());
  }
  else
  {
    WiFi.mode(WIFI_STA);

    WiFi.beginSmartConfig();

    udp.begin(2255);

    while (!WiFi.smartConfigDone())
    {
      // delayMicroseconds(5);
      delay(1);
      btn1.tick();
      btn2.tick();
      btn3.tick();
      btn4.tick();
      // Serial.print(".");
    }

    while (WiFi.status() != WL_CONNECTED)
    {
      // delayMicroseconds(5);
      delay(1);
      btn1.tick();
      btn2.tick();
      btn3.tick();
      btn4.tick();
      // Serial.print(".");
    }

    while (true)
    {
      btn1.tick();
      btn2.tick();
      btn3.tick();
      btn4.tick();
      if (udp.parsePacket())
      {
        udp.read(udpbuf, 255);
        Serial.print("message = ");
        Serial.print(udpbuf);

        Serial.print(", from =");
        Serial.print(udp.remoteIP());

        udp.beginPacket(udp.remoteIP(), 2255);
        int i = 0;
        while (replyPacket[i] != 0)
          udp.write((uint8_t)replyPacket[i++]);
        Serial.print("udp send = ");
        Serial.println(replyPacket);
        udp.endPacket();
        udp.flush();

        break;
      }
    }

    ssid = WiFi.SSID();
    pss = WiFi.psk();

    writeStringToFlash(ssid.c_str(), 0); // storing ssid at address 0
    writeStringToFlash(pss.c_str(), 40); // storing pss at address 40
  }

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  ticker.detach();

  digitalWrite(WIFI_LED, LOW);

  wifi_state = true;

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  while (!client.connected())
  {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password, MACADD.c_str(), 2, true, "false"))
    {
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  client.subscribe(topic1.c_str());
  client.subscribe(topic2.c_str());
  client.subscribe(topic3.c_str());
  client.subscribe(topic4.c_str());
  client.subscribe(schedule_topic.c_str());
  client.publish(MACADD.c_str(), "true", true);

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  timeClient.begin();
  timeClient.setTimeOffset(8 * 3600);
  timeClient.forceUpdate();

  auth.user.email = DEVICE_EMAIL;
  auth.user.password = DEVICE_PASS;

  config.token_status_callback = tokenStatusCallback;

  fbdo.setResponseSize(4095);
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void schedule_check()
{
  for (int j = 0; j < schedule["schedule"].size(); j++)
  {
    String output = schedule["schedule"][j]["mapValue"]["fields"]["output"]["stringValue"];
    if (output == "out1")
    {
      if (timmerCheck1 == false)
      {
        bool status = schedule["schedule"][j]["mapValue"]["fields"]["status"]["booleanValue"];
        String time = schedule["schedule"][j]["mapValue"]["fields"]["time"]["stringValue"];
        uint8_t day = schedule["schedule"][j]["mapValue"]["fields"]["every"]["integerValue"];
        if (time == sendTime && status == true && days == day)
        {
          duration1 = schedule["schedule"][j]["mapValue"]["fields"]["duration"]["integerValue"];

          timmerCheck1 = true;
        }
        else if (time == sendTime && status == true && day > 7)
        {
          duration1 = schedule["schedule"][j]["mapValue"]["fields"]["duration"]["integerValue"];

          timmerCheck1 = true;
        }
      }
    }
    else if (output == "out2")
    {
      if (timmerCheck2 == false)
      {
        bool status = schedule["schedule"][j]["mapValue"]["fields"]["status"]["booleanValue"];
        String time = schedule["schedule"][j]["mapValue"]["fields"]["time"]["stringValue"];
        uint8_t day = schedule["schedule"][j]["mapValue"]["fields"]["every"]["integerValue"];
        if (time == sendTime && status == true && days == day)
        {
          duration2 = schedule["schedule"][j]["mapValue"]["fields"]["duration"]["integerValue"];

          timmerCheck2 = true;
        }
        else if (time == sendTime && status == true && day > 7)
        {
          duration2 = schedule["schedule"][j]["mapValue"]["fields"]["duration"]["integerValue"];

          timmerCheck2 = true;
        }
      }
    }
    else if (output == "out3")
    {
      if (timmerCheck3 == false)
      {
        bool status = schedule["schedule"][j]["mapValue"]["fields"]["status"]["booleanValue"];
        String time = schedule["schedule"][j]["mapValue"]["fields"]["time"]["stringValue"];
        uint8_t day = schedule["schedule"][j]["mapValue"]["fields"]["every"]["integerValue"];
        if (time == sendTime && status == true && days == day)
        {
          duration3 = schedule["schedule"][j]["mapValue"]["fields"]["duration"]["integerValue"];

          timmerCheck3 = true;
        }
        else if (time == sendTime && status == true && day > 7)
        {
          duration3 = schedule["schedule"][j]["mapValue"]["fields"]["duration"]["integerValue"];

          timmerCheck3 = true;
        }
      }
    }
    else if (output == "out4")
    {
      if (timmerCheck4 == false)
      {
        bool status = schedule["schedule"][j]["mapValue"]["fields"]["status"]["booleanValue"];
        String time = schedule["schedule"][j]["mapValue"]["fields"]["time"]["stringValue"];
        uint8_t day = schedule["schedule"][j]["mapValue"]["fields"]["every"]["integerValue"];
        if (time == sendTime && status == true && days == day)
        {
          duration4 = schedule["schedule"][j]["mapValue"]["fields"]["duration"]["integerValue"];

          timmerCheck4 = true;
        }
        else if (time == sendTime && status == true && day > 7)
        {
          duration4 = schedule["schedule"][j]["mapValue"]["fields"]["duration"]["integerValue"];

          timmerCheck4 = true;
        }
      }
    }
  }
}

void loop()
{
  // put your main code here, to run repeatedly:

  current_millis = millis();

  client.loop();

  timeClient.update();

  hour = String(timeClient.getHours());
  minutes = String(timeClient.getMinutes());

  if (hour.length() == 1)
  {
    hour = String("0" + hour);
  }

  if (minutes.length() == 1)
  {
    minutes = String("0" + minutes);
  }

  sendTime = String(hour + ":" + minutes);

  days = timeClient.getDay();

  schedule_check();

  btn1.tick();
  btn2.tick();
  btn3.tick();
  btn4.tick();

  if (timmerCheck1 == true)
  {
    digitalWrite(out1, HIGH);

    if (schedule_check1)
    {
      millis_duration1 = current_millis;

      schedule_check1 = false;
    }

    if (current_millis - millis_duration1 >= duration1 * 60000)
    {
      digitalWrite(out1, LOW);

      timmerCheck1 = false;
      schedule_check1 = true;
    }
  }
  else if (timmerCheck2 == true)
  {
    digitalWrite(out2, HIGH);

    if (schedule_check2)
    {
      millis_duration2 = current_millis;

      schedule_check2 = false;
    }

    if (current_millis - millis_duration2 >= duration2 * 60000)
    {
      digitalWrite(out2, LOW);

      timmerCheck2 = false;
      schedule_check2 = true;
    }
  }
  else if (timmerCheck3 == true)
  {
    digitalWrite(out3, HIGH);

    if (schedule_check3)
    {
      millis_duration3 = current_millis;

      schedule_check3 = false;
    }

    if (current_millis - millis_duration3 >= duration3 * 60000)
    {
      digitalWrite(out3, LOW);

      timmerCheck3 = false;
      schedule_check3 = true;
    }
  }
  else if (timmerCheck4 == true)
  {
    digitalWrite(out4, HIGH);

    if (schedule_check4)
    {
      millis_duration4 = current_millis;

      schedule_check4 = false;
    }

    if (current_millis - millis_duration4 >= duration4 * 60000)
    {
      digitalWrite(out4, LOW);

      timmerCheck4 = false;
      schedule_check4 = true;
    }
  }

  delayMicroseconds(5);
}