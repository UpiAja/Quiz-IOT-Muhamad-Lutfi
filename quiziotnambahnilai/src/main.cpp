// Nama : Muhamad Lutfi
// NIM : 2502039232
// Kelas : LB40

#include <Arduino.h>
#include <Ticker.h>
#include <DHTesp.h>
#include <BH1750.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define LED_1 5
#define LED_2 18
#define LED_3 19
#define PIN_DHT 15
#define LED_COUNT 3
#define PIN_SW 0
#define PIN_SDA 21
#define PIN_SCL 22
const uint8_t arLed[LED_COUNT] = {LED_3, LED_2, LED_1};

#define WIFI_SSID "PunyaUpi"
#define WIFI_PASSWORD "mamamama"
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_TOPIC_PUBLISH "esp32_upiaja/data"
#define MQTT_TOPIC_PUBLISH1 "esp32_upiaja/temp"
#define MQTT_TOPIC_PUBLISH2 "esp32_upiaja/data/lux"
#define MQTT_TOPIC_PUBLISH3 "esp32_upiaja/data/humid"
#define MQTT_TOPIC_SUBSCRIBE "esp32_upiaja/cmd"

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Ticker timerPublish, ledOff, sendLux, sendTemp, sendHumidity;
DHTesp dht;
BH1750 lightMeter;

char g_szDeviceId[30];
void WifiConnect();
boolean mqttConnect();
void onPublishMessage();
void onPublishTemperature();
void onPublishHumidity();
void onPublishLight();
bool turnOn = false;
float fHumidity = 0;
float fTemperature = 0;
float lux = 0;
char szTopic[50];
char szData[10];

void onPublishTemperature()
{
  fTemperature = dht.getTemperature();
  Serial.printf("\nTemperature: %.2f C\n", fTemperature);
  sprintf(szTopic, "%s/temp", MQTT_TOPIC_PUBLISH);
  sprintf(szData, "%.2f", fTemperature);
  mqtt.publish(szTopic, szData);
}

void onPublishHumidity()
{
  fHumidity = dht.getHumidity();
  Serial.printf("\nHumidity: %.2f\n", fHumidity);
  sprintf(szTopic, "%s/humidity", MQTT_TOPIC_PUBLISH);
  sprintf(szData, "%.2f", fHumidity);
  mqtt.publish(szTopic, szData);
}
void onPublishLight()
{
  lux = lightMeter.readLightLevel();
  Serial.printf("\nLight: %.2f lx\n", lux);
  sprintf(szTopic, "%s/light", MQTT_TOPIC_PUBLISH);
  sprintf(szData, "%.2f", lux);
  mqtt.publish(szTopic, szData);
  ledOff.once_ms(100, []()
                 { digitalWrite(LED_BUILTIN, LOW); });

  digitalWrite(LED_2, (lux > 400) ? HIGH : LOW);
  digitalWrite(LED_3, (lux < 400) ? HIGH : LOW);
  if (lux > 400)
  {
    Serial.printf("\nSafe door opened!");
  }
  else
  {
    Serial.printf("\nSafe door closed!");
  }
}
void setup()
{
  Serial.begin(9600);
  delay(100);
  pinMode(LED_BUILTIN, OUTPUT);
  for (uint8_t i = 0; i < LED_COUNT; i++)
    pinMode(arLed[i], OUTPUT);
  Wire.begin();
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
  Serial.printf("Free Memory: %d\n", ESP.getFreeHeap());
  WifiConnect();
  dht.setup(PIN_DHT, DHTesp::DHT11);
  mqttConnect();
  timerPublish.attach_ms(2000, onPublishMessage);
  sendTemp.attach_ms(7000, onPublishTemperature);
  sendHumidity.attach_ms(5000, []()
                         { onPublishHumidity(); });
  sendLux.attach_ms(4000, []()
                    { onPublishLight(); });
}

void loop()
{
  mqtt.loop();
}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{
  String strTopic = topic;
  int8_t idx = strTopic.lastIndexOf('/') + 1;
  String strDev = strTopic.substring(idx);
  Serial.printf("==> Recv [%s]: ", topic);
  Serial.write(payload, len);
  Serial.println();

  if (strcmp(topic, MQTT_TOPIC_SUBSCRIBE) == 0)
  {
    payload[len] = '\0';
    Serial.printf("==> Recv [%s]: ", payload);
    if (strcmp((char *)payload, "led-on") == 0)
    {
      turnOn = true;
      digitalWrite(LED_1, HIGH);
    }
    else if (strcmp((char *)payload, "led-off") == 0)
    {
      turnOn = false;
      digitalWrite(LED_1, LOW);
    }
  }
}

void onPublishMessage()
{
  szTopic[50];
  szData[10];
  digitalWrite(LED_BUILTIN, HIGH);
}

boolean mqttConnect()
{
  sprintf(g_szDeviceId, "esp32_%08X", (uint32_t)ESP.getEfuseMac());
  mqtt.setServer(MQTT_BROKER, 1883);
  mqtt.setCallback(mqttCallback);
  Serial.printf("Connecting to %s clientId: %s\n", MQTT_BROKER, g_szDeviceId);
  boolean status = mqtt.connect(g_szDeviceId);
  if (status == false)
  {
    Serial.print(" fail, rc=");
    Serial.print(mqtt.state());
    return false;
  }
  Serial.println(" success");
  mqtt.subscribe(MQTT_TOPIC_SUBSCRIBE);
  Serial.printf("Subcribe topic: %s\n", MQTT_TOPIC_SUBSCRIBE);
  onPublishMessage();
  return mqtt.connected();
}

void WifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.print("System connected with IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
}