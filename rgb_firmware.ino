#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <PubSubClient.h>

// ---------- Wi-Fi ----------
const char* ssid = "V_ra";
const char* password = "11111111";

// ---------- MQTT ----------
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

// ---------- Wi-Fi + MQTT setup ----------
WiFiClient espClient;
PubSubClient client(espClient);

void reconnectMQTT(const char* clientID, const char* subTopic) {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect(clientID)) {
      Serial.println("connected!");
      client.subscribe(subTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ---------- OTA FUNCTION ----------
void performGitHubOTA(String versionURL, String binURL, String currentVersion) {
  HTTPClient http;
  http.begin(versionURL);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String newVersion = http.getString();
    newVersion.trim();
    if (newVersion != currentVersion) {
      Serial.println("New version found! Downloading update...");
      WiFiClient client;
      HTTPClient httpBin;
      httpBin.begin(client, binURL);
      int code = httpBin.GET();
      if (code == 200) {
        int len = httpBin.getSize();
        WiFiClient * stream = httpBin.getStreamPtr();
        if (!Update.begin(len)) return;
        Update.writeStream(*stream);
        if (Update.end(true)) {
          Serial.println("✅ Update done! Rebooting...");
          ESP.restart();
        } else Serial.println("❌ Update failed!");
      }
      httpBin.end();
    } else Serial.println("✅ Already latest version.");
  }
  http.end();
}
String currentVersion = "1.0.0";
String versionURL = "https://vrabhaskar.github.io/esp32_ota/rgb_version.txt";
String binURL = "https://vrabhaskar.github.io/esp32_ota/rgb_firmware.bin";
const char* publishTopic = "esp/rgb";
const char* subscribeTopic = "esp/update/3";

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)message[i];
  if (String(topic) == subscribeTopic && msg == "update") {
    performGitHubOTA(versionURL, binURL, currentVersion);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnectMQTT("ESP32_RGB", subscribeTopic);
  client.loop();

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 6000) {
    lastMsg = millis();
    String data = "R=" + String(random(100,255)) +
                  " G=" + String(random(50,200)) +
                  " B=" + String(random(80,255));
    client.publish(publishTopic, data.c_str());
    Serial.println("📤 " + data);
  }
}

