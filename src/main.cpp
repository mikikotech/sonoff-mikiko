// #include <Arduino.h>

// void setup()
// {
//   pinMode(4, OUTPUT);
// }

// void loop()
// {
//   digitalWrite(4, HIGH);
//   delay(100);
//   digitalWrite(4, LOW);
//   delay(100);
// }

// #define ARDUINOJSON_ENABLE_PROGMEM 1
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
#include <time.h>
#include <sys/time.h>
#include "CronAlarms.h"
#include "EEPROM.h"

#define FIREBASE_PROJECT_ID "mikiko-c5ca4"
#define STORAGE_BUCKET_ID "gs://mikiko-c5ca4.appspot.com"
#define API_KEY "AIzaSyAMMrTWIU5gKeCDKwiLwO-7liVvfpT8u-M"
#define DATABASE_URL "https://mikiko-c5ca4-default-rtdb.firebaseio.com/"

// device info
#define DEVICE_EMAIL "mikikoSON@mikiko.com"
#define DEVICE_PASS "mikikoSON"
#define FIRMWARE_VERSION "0.0.1"

#define LENGTH(x) (strlen(x) + 1)
#define EEPROM_SIZE 200

#define out1 12
#define out2 5
#define out3 4
#define out4 15

#define WIFI_LED 13

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

Ticker ticker;
WiFiUDP udp;

WiFiClient espClient;
PubSubClient client(espClient);

time_t rawtime;
struct tm *timeinfo;

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

// CronId schedule_id[52];
// int cron_count = 0;

String uniq_username = String("MIKIKO" + WiFi.macAddress());

const char *mqtt_broker = "broker.hivemq.com";
const char *mqtt_username = uniq_username.c_str();
const char *mqtt_password = "mikiko";
const int mqtt_port = 1883;

// const char *ntpServer = "pool.ntp.org";
// const long gmtOffset_sec = 0;
// const int daylightOffset_sec = 3600;

String ssid; // string variable to store ssid
String pss;  // string variable to store password
String gmt;
String MACADD = WiFi.macAddress();

String topic1;
String topic2;
String topic3;
String topic4;
String fwVersion_topic;
String fwUpdate_topic;
String fwRespone_topic;
String schedule_topic;
String documentPath;

bool wifi_state = false;

char udpbuf[3];

static void wifi_led();

void out1_on()
{
  digitalWrite(out1, HIGH);
  Serial.println("out 1 on");

  client.publish(topic1.c_str(), "true", true);
}

void out1_off()
{
  digitalWrite(out1, LOW);
  Serial.println("out 1 off");
  client.publish(topic1.c_str(), "false", true);
}

void out2_on()
{
  digitalWrite(out2, HIGH);
  Serial.println("out 2 on");
  client.publish(topic2.c_str(), "true", true);
}

void out2_off()
{
  digitalWrite(out2, LOW);
  Serial.println("out 2 off");
  client.publish(topic2.c_str(), "false", true);
}

void out3_on()
{
  digitalWrite(out3, HIGH);
  Serial.println("out 3 on");
  client.publish(topic3.c_str(), "true", true);
}

void out3_off()
{
  digitalWrite(out3, LOW);
  Serial.println("out 3 off");
  client.publish(topic3.c_str(), "false", true);
}

void out4_on()
{
  digitalWrite(out4, HIGH);
  Serial.println("out 4 on");
  client.publish(topic4.c_str(), "true", true);
}

void out4_off()
{
  digitalWrite(out4, LOW);
  Serial.println("out 4 off");
  client.publish(topic4.c_str(), "false", true);
}

// void out1_on_once()
// {
//   out1_on();

//   FirebaseJson content;

//   if (schedule["fields"]["schedule"]["arrayValue"]["values"].size() == 1)
//   {
//     content.set("fields/schedule/arrayValue/values/[0]");
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }
//   else
//   {
//     for (int i = 0; i < schedule["fields"]["schedule"]["arrayValue"]["values"].size(); i++)
//     {
//       String id_schedule = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["id"]["stringValue"];
//       String content_data = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["data"]["stringValue"];

//       if (Cron.getTriggeredCronId() != i)
//       {
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/data/stringValue", content_data);
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/id/stringValue", id_schedule);
//       }
//     }
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }

//   content.clear();
// }

// void out1_off_once()
// {
//   out1_off();

//   FirebaseJson content;

//   if (schedule["fields"]["schedule"]["arrayValue"]["values"].size() == 1)
//   {
//     content.set("fields/schedule/arrayValue/values/[0]");
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }
//   else
//   {
//     for (int i = 0; i < schedule["fields"]["schedule"]["arrayValue"]["values"].size(); i++)
//     {
//       String id_schedule = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["id"]["stringValue"];
//       String content_data = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["data"]["stringValue"];

//       if (Cron.getTriggeredCronId() != i)
//       {
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/data/stringValue", content_data);
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/id/stringValue", id_schedule);
//       }
//     }
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }

//   content.clear();
// }

// void out2_on_once()
// {
//   out2_on();

//   FirebaseJson content;

//   if (schedule["fields"]["schedule"]["arrayValue"]["values"].size() == 1)
//   {
//     content.set("fields/schedule/arrayValue/values/[0]");
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }
//   else
//   {
//     for (int i = 0; i < schedule["fields"]["schedule"]["arrayValue"]["values"].size(); i++)
//     {
//       String id_schedule = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["id"]["stringValue"];
//       String content_data = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["data"]["stringValue"];

//       if (Cron.getTriggeredCronId() != i)
//       {
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/data/stringValue", content_data);
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/id/stringValue", id_schedule);
//       }
//     }
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }

//   content.clear();
// }

// void out2_off_once()
// {
//   out2_off();

//   FirebaseJson content;

//   if (schedule["fields"]["schedule"]["arrayValue"]["values"].size() == 1)
//   {
//     content.set("fields/schedule/arrayValue/values/[0]");
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }
//   else
//   {
//     for (int i = 0; i < schedule["fields"]["schedule"]["arrayValue"]["values"].size(); i++)
//     {
//       String id_schedule = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["id"]["stringValue"];
//       String content_data = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["data"]["stringValue"];

//       if (Cron.getTriggeredCronId() != i)
//       {
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/data/stringValue", content_data);
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/id/stringValue", id_schedule);
//       }
//     }
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }

//   content.clear();
// }

// void out3_on_once()
// {
//   out3_on();

//   FirebaseJson content;

//   if (schedule["fields"]["schedule"]["arrayValue"]["values"].size() == 1)
//   {
//     content.set("fields/schedule/arrayValue/values/[0]");
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }
//   else
//   {
//     for (int i = 0; i < schedule["fields"]["schedule"]["arrayValue"]["values"].size(); i++)
//     {
//       String id_schedule = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["id"]["stringValue"];
//       String content_data = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["data"]["stringValue"];

//       if (Cron.getTriggeredCronId() != i)
//       {
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/data/stringValue", content_data);
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/id/stringValue", id_schedule);
//       }
//     }
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }

//   content.clear();
// }

// void out3_off_once()
// {
//   out3_off();

//   FirebaseJson content;

//   if (schedule["fields"]["schedule"]["arrayValue"]["values"].size() == 1)
//   {
//     content.set("fields/schedule/arrayValue/values/[0]");
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }
//   else
//   {
//     for (int i = 0; i < schedule["fields"]["schedule"]["arrayValue"]["values"].size(); i++)
//     {
//       String id_schedule = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["id"]["stringValue"];
//       String content_data = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["data"]["stringValue"];

//       if (Cron.getTriggeredCronId() != i)
//       {
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/data/stringValue", content_data);
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/id/stringValue", id_schedule);
//       }
//     }
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }

//   content.clear();
// }

// void out4_on_once()
// {
//   out4_on();

//   FirebaseJson content;

//   if (schedule["fields"]["schedule"]["arrayValue"]["values"].size() == 1)
//   {
//     content.set("fields/schedule/arrayValue/values/[0]");
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }
//   else
//   {
//     for (int i = 0; i < schedule["fields"]["schedule"]["arrayValue"]["values"].size(); i++)
//     {
//       String id_schedule = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["id"]["stringValue"];
//       String content_data = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["data"]["stringValue"];

//       if (Cron.getTriggeredCronId() != i)
//       {
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/data/stringValue", content_data);
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/id/stringValue", id_schedule);
//       }
//     }
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }

//   content.clear();
// }

// void out4_off_once()
// {
//   out4_off();

//   FirebaseJson content;

//   if (schedule["fields"]["schedule"]["arrayValue"]["values"].size() == 1)
//   {
//     content.set("fields/schedule/arrayValue/values/[0]");
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }
//   else
//   {
//     for (int i = 0; i < schedule["fields"]["schedule"]["arrayValue"]["values"].size(); i++)
//     {
//       String id_schedule = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["id"]["stringValue"];
//       String content_data = schedule["fields"]["schedule"]["arrayValue"]["values"][i]["mapValue"]["fields"]["data"]["stringValue"];

//       if (Cron.getTriggeredCronId() != i)
//       {
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/data/stringValue", content_data);
//         content.set("fields/schedule/arrayValue/values/[" + String(i) + "]/mapValue/fields/id/stringValue", id_schedule);
//       }
//     }
//     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "schedule"))
//       Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//     else
//       Serial.println(fbdo.errorReason());
//   }

//   content.clear();
// }

void schedule_check();

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

static void handleClick1()
{
  if (!btn1.isLongPressed())
  {
    int state = !digitalRead(out1);
    digitalWrite(out1, state);

    if (wifi_state)
    {
      client.publish(topic1.c_str(), state == 1 ? "true" : "false");
    }
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

void fcsDownloadCallback(FCS_DownloadStatusInfo info)
{
  if (info.status == fb_esp_fcs_download_status_init)
  {
    Serial.printf("Downloading firmware %s (%d)\n", info.remoteFileName.c_str(), info.fileSize);
    client.publish(fwRespone_topic.c_str(), "start");

    ticker.attach(0.5, wifi_led);
  }
  else if (info.status == fb_esp_fcs_download_status_download)
  {
    Serial.printf("Downloaded %d%s\n", (int)info.progress, "%");
  }
  else if (info.status == fb_esp_fcs_download_status_complete)
  {
    if (client.publish(fwRespone_topic.c_str(), "done"))
    {
      ticker.detach();
      digitalWrite(WIFI_LED, LOW);
      delay(1000);
      ESP.restart();
    }
  }
  else if (info.status == fb_esp_fcs_download_status_error)
  {
    Serial.printf("Download firmware failed, %s\n", info.errorMsg.c_str());
  }
}

CronId id;

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
  else if (strTopic == fwUpdate_topic)
  {
    if (!Firebase.Storage.downloadOTA(&fbdo, STORAGE_BUCKET_ID, "/SONMIKIKO/firmware.bin", fcsDownloadCallback))
      Serial.println(fbdo.errorReason());
    // gs://mikiko-c5ca4.appspot.com/SONMIKIKO/firmware.bin
  }
  else if (strTopic == schedule_topic)
  {

    DynamicJsonDocument schedule(2045);
    StaticJsonDocument<114> filter;

    filter["fields"]["schedule"]["arrayValue"]["values"][0]["mapValue"] = true;

    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), "schedule"))
    {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());

      for (byte i = 0; i < 51; i++)
      {
        Cron.free(i);
      }

      // Serial.println(fbdo.payloadLength());

      DeserializationError error = deserializeJson(schedule, fbdo.payload().c_str(), DeserializationOption::Filter(filter));

      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      // serializeJson(schedule, Serial);

      // schedule_check();

      for (int j = 0; j < schedule["fields"]["schedule"]["arrayValue"]["values"].size(); j++)
      {
        // String cron_id = schedule["fields"]["schedule"]["arrayValue"]["values"][j]["mapValue"]["fields"]["id"]["stringValue"];
        String cron_data = schedule["fields"]["schedule"]["arrayValue"]["values"][j]["mapValue"]["fields"]["data"]["stringValue"];
        String cron_string = getValue(cron_data, 58, 0);
        String output = getValue(cron_data, 58, 1);
        bool state = getValue(cron_data, 58, 2) == "1" ? true : false;
        bool status = getValue(cron_data, 58, 3) == "1" ? true : false;

        // Serial.print("cron is =");
        // Serial.print(cron_string);
        // Serial.println("end");
        // Serial.print("output is = ");
        // Serial.println(output);
        // Serial.print("status is = ");
        // Serial.println(state);
        // Serial.print("repeat is = ");
        // Serial.println(repeat);
        // Serial.print("status is = ");
        // Serial.println(status);

        if (output == "out1" && status == true)
        {
          // if (repeat == true)
          // {
          //   if (state == true)
          //   {
          //     Cron.create(cron_string.c_str(), out1_on_once, true);
          //   }
          //   else
          //   {
          //     Cron.create(cron_string.c_str(), out1_off_once, true);
          //   }
          // }
          // else
          // {
          if (state == true)
          {
            Cron.create(cron_string.c_str(), out1_on, false);
          }
          else
          {
            Cron.create(cron_string.c_str(), out1_off, false);
          }
          // }
        }
        else if (output == "out2" && status == true)
        {
          // if (repeat == true)
          // {
          //   if (state == true)
          //   {
          //     Cron.create(cron_string.c_str(), out2_on_once, true);
          //   }
          //   else
          //   {
          //     Cron.create(cron_string.c_str(), out2_off_once, true);
          //   }
          // }
          // else
          // {
          if (state == true)
          {
            Cron.create(cron_string.c_str(), out2_on, false);
          }
          else
          {
            Cron.create(cron_string.c_str(), out2_off, false);
          }
          // }
        }
        else if (output == "out3" && status == true)
        {
          // if (repeat == true)
          // {
          //   if (state == true)
          //   {
          //     Cron.create(cron_string.c_str(), out3_on_once, true);
          //   }
          //   else
          //   {
          //     Cron.create(cron_string.c_str(), out3_off_once, true);
          //   }
          // }
          // else
          // {
          if (state == true)
          {
            Cron.create(cron_string.c_str(), out3_on, false);
          }
          else
          {
            Cron.create(cron_string.c_str(), out3_off, false);
          }
          // }
        }
        else if (output == "out4" && status == true)
        {
          // if (repeat == true)
          // {
          //   if (state == true)
          //   {
          //     Cron.create(cron_string.c_str(), out4_on_once, true);
          //   }
          //   else
          //   {
          //     Cron.create(cron_string.c_str(), out4_off_once, true);
          //   }
          // }
          // else
          // {
          if (state == true)
          {
            Cron.create(cron_string.c_str(), out4_on, false);
          }
          else
          {
            Cron.create(cron_string.c_str(), out4_off, false);
          }
          // }
        }
      }
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }

    schedule.clear();
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
  writeStringToFlash("", 0);
  writeStringToFlash("", 40);
  writeStringToFlash("", 80);
  ESP.restart();
}

bool getLocalTime(struct tm *info, uint32_t ms = 5000)
{
  uint32_t start = millis();
  time_t now;
  while ((millis() - start) <= ms)
  {
    time(&now);
    localtime_r(&now, info);
    if (info->tm_year > 1990)
    {
      return true;
    }
    delay(10);
  }
  return false;
}

void setup()
{
  Serial.begin(115200);
  // put your setup code here, to run once:

  EEPROM.begin(EEPROM_SIZE);

  pinMode(out1, OUTPUT);
  pinMode(out2, OUTPUT);
  pinMode(out3, OUTPUT);
  pinMode(out4, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
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

  Serial.println(lround(ESP.getChipId() / 1234));

  String str_reply = String("SON:4CH:" + String(lround(ESP.getChipId() / 1234)) + ":MIKIKO");
  char replyPacket[str_reply.length() + 1];

  strcpy(replyPacket, str_reply.c_str());

  MACADD = getValue(MACADD, 58, 0) + getValue(MACADD, 58, 1) + getValue(MACADD, 58, 2) + getValue(MACADD, 58, 3) + getValue(MACADD, 58, 4) + getValue(MACADD, 58, 5);
  MACADD.toLowerCase();

  topic1 = String("/" + String(MACADD) + "/data/btnone");
  topic2 = String("/" + String(MACADD) + "/data/btntwo");
  topic3 = String("/" + String(MACADD) + "/data/btnthree");
  topic4 = String("/" + String(MACADD) + "/data/btnfour");

  fwVersion_topic = String("/" + MACADD + "/data/firmwareversion");
  fwUpdate_topic = String("/" + String(MACADD) + "/data/ota");
  fwRespone_topic = String("/" + String(MACADD) + "/data/otarespone");

  schedule_topic = String("/" + String(MACADD) + "/data/schedule");
  documentPath = String("devices/" + MACADD);

  // ["fields"]["schedule"]["arrayValue"]["values"];

  ssid = readStringFromFlash(0); // Read SSID stored at address 0
  Serial.print("SSID = ");
  Serial.println(ssid);
  pss = readStringFromFlash(40); // Read Password stored at address 40
  Serial.print("psss = ");
  Serial.println(pss);
  gmt = readStringFromFlash(80);
  Serial.print("gmt = ");
  Serial.println(gmt);

  ticker.attach(0.2, wifi_led);

  Serial.println(MACADD);

  // WiFi.begin("Wifi saya", "1sampai9");

  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("CONNECTED to WIFI");

  // writeStringToFlash("", 0);
  // writeStringToFlash("", 40);
  // writeStringToFlash("", 80);

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
        udp.read(udpbuf, 3);
        Serial.print("message = ");
        Serial.print(udpbuf);
        writeStringToFlash(udpbuf, 80);
        gmt = udpbuf;
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

    writeStringToFlash(ssid.c_str(), 0);
    writeStringToFlash(pss.c_str(), 40);
  }

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  wifi_state = true;

  configTime(0, gmt.toInt() * 3600, "pool.ntp.org");

  struct tm tm_newtime;

  while (getLocalTime(&tm_newtime))
  {
    Serial.print(".");
    // delay(100);
  }

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  while (!client.connected())
  {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password, MACADD.c_str(), 2, true, "false"))
    {
      client.subscribe(topic1.c_str());
      client.subscribe(topic2.c_str());
      client.subscribe(topic3.c_str());
      client.subscribe(topic4.c_str());

      client.subscribe(schedule_topic.c_str());

      client.subscribe(fwUpdate_topic.c_str());
      client.publish(fwVersion_topic.c_str(), FIRMWARE_VERSION, true);

      client.publish(MACADD.c_str(), "true", true);
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = DEVICE_EMAIL;
  auth.user.password = DEVICE_PASS;

  config.token_status_callback = tokenStatusCallback;

  fbdo.setResponseSize(4095 * 2);
  fbdo.setBSSLBufferSize(1024, 1024);
  config.fcs.download_buffer_size = 2048;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  ticker.detach();

  digitalWrite(WIFI_LED, LOW);

  // LittleFS.end();
}

// void schedule_check()
// {

//   for (int j = 0; j < schedule["fields"]["schedule"]["arrayValue"]["values"].size(); j++)
//   {
//     // String cron_id = schedule["fields"]["schedule"]["arrayValue"]["values"][j]["mapValue"]["fields"]["id"]["stringValue"];
//     String cron_data = schedule["fields"]["schedule"]["arrayValue"]["values"][j]["mapValue"]["fields"]["data"]["stringValue"];
//     String cron_string = getValue(cron_data, 58, 0);
//     String output = getValue(cron_data, 58, 1);
//     bool state = getValue(cron_data, 58, 2) == "1" ? true : false;
//     bool status = getValue(cron_data, 58, 3) == "1" ? true : false;

//     // Serial.print("cron is =");
//     // Serial.print(cron_string);
//     // Serial.println("end");
//     // Serial.print("output is = ");
//     // Serial.println(output);
//     // Serial.print("status is = ");
//     // Serial.println(state);
//     // Serial.print("repeat is = ");
//     // Serial.println(repeat);
//     // Serial.print("status is = ");
//     // Serial.println(status);

//     if (output == "out1" && status == true)
//     {
//       // if (repeat == true)
//       // {
//       //   if (state == true)
//       //   {
//       //     Cron.create(cron_string.c_str(), out1_on_once, true);
//       //   }
//       //   else
//       //   {
//       //     Cron.create(cron_string.c_str(), out1_off_once, true);
//       //   }
//       // }
//       // else
//       // {
//       if (state == true)
//       {
//         Cron.create(cron_string.c_str(), out1_on, false);
//       }
//       else
//       {
//         Cron.create(cron_string.c_str(), out1_off, false);
//       }
//       // }
//     }
//     else if (output == "out2" && status == true)
//     {
//       // if (repeat == true)
//       // {
//       //   if (state == true)
//       //   {
//       //     Cron.create(cron_string.c_str(), out2_on_once, true);
//       //   }
//       //   else
//       //   {
//       //     Cron.create(cron_string.c_str(), out2_off_once, true);
//       //   }
//       // }
//       // else
//       // {
//       if (state == true)
//       {
//         Cron.create(cron_string.c_str(), out2_on, false);
//       }
//       else
//       {
//         Cron.create(cron_string.c_str(), out2_off, false);
//       }
//       // }
//     }
//     else if (output == "out3" && status == true)
//     {
//       // if (repeat == true)
//       // {
//       //   if (state == true)
//       //   {
//       //     Cron.create(cron_string.c_str(), out3_on_once, true);
//       //   }
//       //   else
//       //   {
//       //     Cron.create(cron_string.c_str(), out3_off_once, true);
//       //   }
//       // }
//       // else
//       // {
//       if (state == true)
//       {
//         Cron.create(cron_string.c_str(), out3_on, false);
//       }
//       else
//       {
//         Cron.create(cron_string.c_str(), out3_off, false);
//       }
//       // }
//     }
//     else if (output == "out4" && status == true)
//     {
//       // if (repeat == true)
//       // {
//       //   if (state == true)
//       //   {
//       //     Cron.create(cron_string.c_str(), out4_on_once, true);
//       //   }
//       //   else
//       //   {
//       //     Cron.create(cron_string.c_str(), out4_off_once, true);
//       //   }
//       // }
//       // else
//       // {
//       if (state == true)
//       {
//         Cron.create(cron_string.c_str(), out4_on, false);
//       }
//       else
//       {
//         Cron.create(cron_string.c_str(), out4_off, false);
//       }
//       // }
//     }
//   }
// }

unsigned long dataMillis = 0;
int count = 0;

void loop()
{
  // put your main code here, to run repeatedly:

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  // Serial.println(asctime(timeinfo));

  client.loop();

  Cron.delay();

  btn1.tick();
  btn2.tick();
  btn3.tick();
  btn4.tick();

  // Serial.println(Cron.count());

  // Serial.println(ESP.getMaxFreeBlockSize());

  // delay(1000);
  delayMicroseconds(5);
}
