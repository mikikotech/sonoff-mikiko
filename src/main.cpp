#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <OneButton.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <time.h>
#include <sys/time.h>
#include "CronAlarms.h"
#include "EEPROM.h"

#define FIRMWARE_VERSION "0.0.1"

#define LENGTH(x) (strlen(x) + 1)
#define EEPROM_SIZE 200

#define out1 12
#define out2 5
#define out3 4
#define out4 15

#define WIFI_LED 13
// #define WIFI_LED 4

Ticker ticker;
WiFiUDP udp;

WiFiClient espClient;
WiFiClient scheduleClient;
WiFiClient http_notif;
HTTPClient http;
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

String api_endpoint;
CronId id;
DynamicJsonDocument schedule(1024);
DynamicJsonDocument http_data(128);
String string_http_data;

// String uniq_username = String("MIKIKO" + WiFi.macAddress());
String client_id;
const char *mqtt_broker = "broker.hivemq.com";
const char *mqtt_username = "mikiko";
const char *mqtt_password = "mikiko";
const int mqtt_port = 1883;

long lastReconnectAttempt;

String ssid; // string variable to store ssid
String pss;  // string variable to store password
String gmt;
String user;
String MACADD = WiFi.macAddress();

String topic1;
String topic2;
String topic3;
String topic4;
String fwVersion_topic;
String fwUpdate_topic;
String fwRespone_topic;
String schedule_topic;

char udpbuf[3];

static void wifi_led();

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
    size_t i = 0;
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

void notif(uint8_t index, uint8_t state, uint8_t type)
{

    DynamicJsonDocument notif_data(128);
    String string_notif_data;

    api_endpoint = String("http://mikiko.herokuapp.com/notif/" + MACADD);

    notif_data["type"] = type; // 1 schedule // 2 sensor
    notif_data["index"] = index;
    notif_data["state"] = state;

    serializeJson(notif_data, Serial);

    serializeJson(notif_data, string_notif_data);

    http.begin(http_notif, api_endpoint.c_str());
    http.addHeader("Content-Type", "application/json");

    if (http.POST(string_notif_data) > 0)
    {
        Serial.println(http.getString());
    }
    else
    {
        Serial.println("error notif");
    }

    http.end();
}

void out1_on()
{
    digitalWrite(out1, HIGH);
    notif(0, 1, 1);
    client.publish(topic1.c_str(), "true", true);
}

void out1_off()
{
    digitalWrite(out1, LOW);
    notif(0, 0, 1);
    client.publish(topic1.c_str(), "false", true);
}

void out2_on()
{
    digitalWrite(out2, HIGH);
    notif(1, 1, 1);
    client.publish(topic2.c_str(), "true", true);
}

void out2_off()
{
    digitalWrite(out2, LOW);
    notif(1, 0, 1);
    client.publish(topic2.c_str(), "false", true);
}

void out3_on()
{
    digitalWrite(out3, HIGH);
    notif(2, 1, 1);
    client.publish(topic3.c_str(), "true", true);
}

void out3_off()
{
    digitalWrite(out3, LOW);
    notif(2, 0, 1);
    client.publish(topic3.c_str(), "false", true);
}

void out4_on()
{
    digitalWrite(out4, HIGH);
    notif(3, 1, 1);
    client.publish(topic4.c_str(), "true", true);
}

void out4_off()
{
    digitalWrite(out4, LOW);
    notif(3, 0, 1);
    client.publish(topic4.c_str(), "false", true);
}

void removeSchedule(CronID_t triggerCron)
{

    for (size_t i = 0; i < schedule.size(); i++)
    {
        String schedule_id = schedule[i]["id"];
        CronID_t cron_id = schedule[i]["cronId"];
        String cron_data = schedule[i]["data"];

        if (triggerCron == cron_id)
        {

            Serial.print("remove CronId = ");
            Serial.println(cron_id);

            DynamicJsonDocument http_data_remove(128);
            String string_http_data_remove;

            api_endpoint = String("http://mikiko.herokuapp.com/schedule/remove/" + MACADD);

            http_data_remove["id"] = schedule_id;
            http_data_remove["data"] = cron_data;

            serializeJson(http_data_remove, string_http_data_remove);

            http.begin(scheduleClient, api_endpoint.c_str());
            http.addHeader("Content-Type", "application/json");

            if (http.POST(string_http_data_remove) > 0)
            {
                Serial.print("HTTP Response code remove : ");
                Serial.println(http.getString());
            }
            else
            {
                Serial.print("Error code remove : ");
            }
            // Free resources
            http.end();

            // http_data_remove.clear();

            Cron.free(cron_id);
            cron_id = dtINVALID_ALARM_ID;
            schedule.remove(i);

            break;
        }
    }

    serializeJson(schedule, Serial);
}

void out1_on_once()
{
    out1_on();

    removeSchedule(Cron.getTriggeredCronId());
}

void out1_off_once()
{
    out1_off();

    removeSchedule(Cron.getTriggeredCronId());
}

void out2_on_once()
{
    out2_on();

    removeSchedule(Cron.getTriggeredCronId());
}

void out2_off_once()
{
    out2_off();

    removeSchedule(Cron.getTriggeredCronId());
}

void out3_on_once()
{
    out3_on();

    removeSchedule(Cron.getTriggeredCronId());
}

void out3_off_once()
{
    out3_off();

    removeSchedule(Cron.getTriggeredCronId());
}

void out4_on_once()
{
    out4_on();

    removeSchedule(Cron.getTriggeredCronId());
}

void out4_off_once()
{
    out4_off();

    removeSchedule(Cron.getTriggeredCronId());
}

void schedule_check()
{
    for (size_t j = 0; j < schedule.size(); j++)
    {
        String cron_data = schedule[j]["data"];
        String cron_string = getValue(cron_data, 58, 0);                // cron data
        String output = getValue(cron_data, 58, 1);                     // output
        bool state = getValue(cron_data, 58, 2) == "1" ? true : false;  // output state
        bool repeat = getValue(cron_data, 58, 3) == "1" ? true : false; // ?repeat
        bool status = getValue(cron_data, 58, 4) == "1" ? true : false; // ?schedule status

        if (output == "out1" && status == true)
        {
            if (repeat == true)
            {
                if (state == true)
                {
                    id = Cron.create(cron_string.c_str(), out1_on_once, true);
                    schedule[j]["cronId"] = id;
                }
                else
                {
                    id = Cron.create(cron_string.c_str(), out1_off_once, true);
                    schedule[j]["cronId"] = id;
                }
            }
            else
            {
                if (state == true)
                {
                    id = Cron.create(cron_string.c_str(), out1_on, false);
                    schedule[j]["cronId"] = id;
                }
                else
                {
                    id = Cron.create(cron_string.c_str(), out1_off, false);
                    schedule[j]["cronId"] = id;
                }
            }
        }
        else if (output == "out2" && status == true)
        {
            if (repeat == true)
            {
                if (state == true)
                {
                    id = Cron.create(cron_string.c_str(), out2_on_once, true);
                    schedule[j]["cronId"] = id;
                }
                else
                {
                    id = Cron.create(cron_string.c_str(), out2_off_once, true);
                    schedule[j]["cronId"] = id;
                }
            }
            else
            {
                if (state == true)
                {
                    id = Cron.create(cron_string.c_str(), out2_on, false);
                    schedule[j]["cronId"] = id;
                }
                else
                {
                    id = Cron.create(cron_string.c_str(), out2_off, false);
                    schedule[j]["cronId"] = id;
                }
            }
        }
        else if (output == "out3" && status == true)
        {
            if (repeat == true)
            {
                if (state == true)
                {
                    id = Cron.create(cron_string.c_str(), out3_on_once, true);
                    schedule[j]["cronId"] = id;
                }
                else
                {
                    id = Cron.create(cron_string.c_str(), out3_off_once, true);
                    schedule[j]["cronId"] = id;
                }
            }
            else
            {
                if (state == true)
                {
                    id = Cron.create(cron_string.c_str(), out3_on, false);
                    schedule[j]["cronId"] = id;
                }
                else
                {
                    id = Cron.create(cron_string.c_str(), out3_off, false);
                    schedule[j]["cronId"] = id;
                }
            }
        }
        else if (output == "out4" && status == true)
        {
            if (repeat == true)
            {
                if (state == true)
                {
                    id = Cron.create(cron_string.c_str(), out4_on_once, true);
                    schedule[j]["cronId"] = id;
                }
                else
                {
                    id = Cron.create(cron_string.c_str(), out4_off_once, true);
                    schedule[j]["cronId"] = id;
                }
            }
            else
            {
                if (state == true)
                {
                    id = Cron.create(cron_string.c_str(), out4_on, false);
                    schedule[j]["cronId"] = id;
                }
                else
                {
                    id = Cron.create(cron_string.c_str(), out4_off, false);
                    schedule[j]["cronId"] = id;
                }
            }
        }
    }
}

void schedule_edit_check(DynamicJsonDocument schedule_data)
{
    String cron_data = schedule_data["data"];
    String cron_string = getValue(cron_data, 58, 0);                // cron data
    String output = getValue(cron_data, 58, 1);                     // output
    bool state = getValue(cron_data, 58, 2) == "1" ? true : false;  // output state
    bool repeat = getValue(cron_data, 58, 3) == "1" ? true : false; // ?repeat
    bool status = getValue(cron_data, 58, 4) == "1" ? true : false; // ?schedule status
    uint8_t schedule_size = schedule.size();

    if (output == "out1" && status == true)
    {
        if (repeat == true)
        {
            if (state == true)
            {
                id = Cron.create(cron_string.c_str(), out1_on_once, true);
                schedule[schedule_size]["cronId"] = id;
            }
            else
            {
                id = Cron.create(cron_string.c_str(), out1_off_once, true);
                schedule[schedule_size]["cronId"] = id;
            }
        }
        else
        {
            if (state == true)
            {
                id = Cron.create(cron_string.c_str(), out1_on, false);
                schedule[schedule_size]["cronId"] = id;
            }
            else
            {
                id = Cron.create(cron_string.c_str(), out1_off, false);
                schedule[schedule_size]["cronId"] = id;
            }
        }
    }
    else if (output == "out2" && status == true)
    {
        if (repeat == true)
        {
            if (state == true)
            {
                id = Cron.create(cron_string.c_str(), out2_on_once, true);
                schedule[schedule_size]["cronId"] = id;
            }
            else
            {
                id = Cron.create(cron_string.c_str(), out2_off_once, true);
                schedule[schedule_size]["cronId"] = id;
            }
        }
        else
        {
            if (state == true)
            {
                id = Cron.create(cron_string.c_str(), out2_on, false);
                schedule[schedule_size]["cronId"] = id;
            }
            else
            {
                id = Cron.create(cron_string.c_str(), out2_off, false);
                schedule[schedule_size]["cronId"] = id;
            }
        }
    }
    else if (output == "out3" && status == true)
    {
        if (repeat == true)
        {
            if (state == true)
            {
                id = Cron.create(cron_string.c_str(), out3_on_once, true);
                schedule[schedule_size]["cronId"] = id;
            }
            else
            {
                id = Cron.create(cron_string.c_str(), out3_off_once, true);
                schedule[schedule_size]["cronId"] = id;
            }
        }
        else
        {
            if (state == true)
            {
                id = Cron.create(cron_string.c_str(), out3_on, false);
                schedule[schedule_size]["cronId"] = id;
            }
            else
            {
                id = Cron.create(cron_string.c_str(), out3_off, false);
                schedule[schedule_size]["cronId"] = id;
            }
        }
    }
    else if (output == "out4" && status == true)
    {
        if (repeat == true)
        {
            if (state == true)
            {
                id = Cron.create(cron_string.c_str(), out4_on_once, true);
                schedule[schedule_size]["cronId"] = id;
            }
            else
            {
                id = Cron.create(cron_string.c_str(), out4_off_once, true);
                schedule[schedule_size]["cronId"] = id;
            }
        }
        else
        {
            if (state == true)
            {
                id = Cron.create(cron_string.c_str(), out4_on, false);
                schedule[schedule_size]["cronId"] = id;
            }
            else
            {
                id = Cron.create(cron_string.c_str(), out4_off, false);
                schedule[schedule_size]["cronId"] = id;
            }
        }
    }

    schedule[schedule_size]["id"] = schedule_data["id"];
    schedule[schedule_size]["data"] = schedule_data["data"];

    serializeJson(schedule, Serial);
}

static void handleClick1()
{
    if (!btn1.isLongPressed())
    {
        int state = !digitalRead(out1);
        digitalWrite(out1, state);

        client.publish(topic1.c_str(), state == 1 ? "true" : "false");
    }
}

static void handleClick2()
{
    int state = !digitalRead(out2);
    digitalWrite(out2, state);

    client.publish(topic2.c_str(), state == 1 ? "true" : "false");
}

static void handleClick3()
{
    int state = !digitalRead(out3);
    digitalWrite(out3, state);

    client.publish(topic3.c_str(), state == 1 ? "true" : "false");
}

static void handleClick4()
{
    int state = !digitalRead(out4);
    digitalWrite(out4, state);

    client.publish(topic4.c_str(), state == 1 ? "true" : "false");
}

void mqtt_process(char *topic, byte *payload)
{

    String msg;
    String strTopic;

    strTopic = String((char *)topic);

    Serial.println(strTopic);

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

        DynamicJsonDocument schedule_payload(512);

        msg = String((char *)payload);

        deserializeJson(schedule_payload, msg.c_str());

        serializeJson(schedule_payload, Serial);

        Serial.println("---------");

        if (schedule_payload["type"] == "11") // add schedule
        {
            schedule_edit_check(schedule_payload);
        }
        else if (schedule_payload["type"] == "22") // remove schedule
        {
            for (size_t i = 0; i < schedule.size(); i++)
            {
                if (schedule[i]["id"] == schedule_payload["id"])
                {
                    CronID_t cron_id = schedule[i]["cronId"];

                    Cron.free(cron_id);
                    cron_id = dtINVALID_ALARM_ID;
                    schedule.remove(i);

                    Serial.printf("schedule index ke-%d dihapus", i);

                    break;
                }
            }
        }
        else if (schedule_payload["type"] == "33") // edit schedule
        {
            for (size_t i = 0; i < schedule.size(); i++)
            {
                if (schedule[i]["id"] == schedule_payload["id"])
                {
                    CronID_t cron_id = schedule[i]["cronId"];

                    Cron.free(cron_id);
                    cron_id = dtINVALID_ALARM_ID;
                    schedule.remove(i);

                    schedule_edit_check(schedule_payload);

                    break;
                }
            }
        }
    }
    else if (strTopic == fwUpdate_topic)
    {
        msg = String((char *)payload);
    }
}

void callback(char *topic, byte *payload, unsigned int length)
{
    payload[length] = '\0';
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);

    Serial.print("Message:");
    for (size_t i = 0; i < length; i++)
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

uint8_t count = 0;

static void wifi_led_pairing()
{
    count++;

    if (count > 5)
    {
        digitalWrite(WIFI_LED, HIGH);

        if (count > 9)
            count = 0;
    }
    else
    {
        digitalWrite(WIFI_LED, !digitalRead(WIFI_LED));
    }
}

void btnLongPress()
{
    writeStringToFlash("", 0);
    writeStringToFlash("", 20);
    writeStringToFlash("", 40);
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

void reconnect_to_mqtt()
{
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password, MACADD.c_str(), 2, true, "false"))
    {

        client.subscribe(topic1.c_str());
        client.subscribe(topic2.c_str());
        client.subscribe(topic3.c_str());
        client.subscribe(topic4.c_str());

        client.subscribe("data/t");

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

void setup()
{
    Serial.begin(115200);

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

    api_endpoint = String("http://mikiko.herokuapp.com/schedule/getall/" + MACADD);

    topic1 = String("/" + String(MACADD) + "/data/btn1");
    topic2 = String("/" + String(MACADD) + "/data/btn2");
    topic3 = String("/" + String(MACADD) + "/data/btn3");
    topic4 = String("/" + String(MACADD) + "/data/btn4");

    fwVersion_topic = String("/" + MACADD + "/data/firmwareversion");
    fwUpdate_topic = String("/" + String(MACADD) + "/data/ota");
    fwRespone_topic = String("/" + String(MACADD) + "/data/otarespone");

    schedule_topic = String("/" + String(MACADD) + "/data/schedule");

    ssid = readStringFromFlash(0);
    pss = readStringFromFlash(20);
    gmt = readStringFromFlash(40);

    // WiFi.begin("Wifi saya", "1sampai9");

    // while (WiFi.status() != WL_CONNECTED)
    // {
    //   delay(500);
    //   Serial.print(".");
    // }
    // Serial.println("CONNECTED to WIFI");

    // writeStringToFlash("", 0);
    // writeStringToFlash("", 20);
    // writeStringToFlash("", 40);

    if (ssid.length() > 0 && pss.length() > 0)
    {
        ticker.attach(0.4, wifi_led);

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
        ticker.attach(0.1, wifi_led_pairing);

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
                writeStringToFlash(udpbuf, 40);
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
        writeStringToFlash(pss.c_str(), 20);
    }

    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    configTime(0, gmt.toInt() * 3600, "pool.ntp.org");

    struct tm tm_newtime;

    while (getLocalTime(&tm_newtime))
    {
        Serial.print(".");
        // delay(100);
    }

    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);

    client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());

    while (!client.connected())
    {
        reconnect_to_mqtt();
    }

    http.begin(scheduleClient, api_endpoint.c_str());

    if (http.GET() > 0)
    {
        Serial.print("HTTP Response code: ");
        if (http.getString().length() > 0)
        {
            DeserializationError error = deserializeJson(schedule, http.getString().c_str());

            if (error)
            {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }

            // serializeJson(schedule, Serial);

            schedule_check();
        }
    }
    else
    {
        Serial.print("Error code: ");
        // Serial.println(http.GET());
    }
    // Free resources
    http.end();

    ticker.detach();

    digitalWrite(WIFI_LED, LOW);

    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    // LittleFS.end();
}

void loop()
{

    ticker.detach();

    digitalWrite(WIFI_LED, LOW);

    if (!client.connected())
    {
        long now = millis();
        if (now - lastReconnectAttempt > 5000)
        {
            reconnect_to_mqtt();
            // Attempt to reconnect
            lastReconnectAttempt = now;
        }
    }
    else
    {
        client.loop();
    }

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    // Serial.println(asctime(timeinfo));

    // Serial.println(schedule.size());

    Cron.delay();

    btn1.tick();
    btn2.tick();
    btn3.tick();
    btn4.tick();

    delayMicroseconds(5);
}
