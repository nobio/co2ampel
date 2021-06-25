
#include <driver/rtc_io.h>
#include <WiFi.h>
#include <PubSubClient.h>
#define LED_PIN   22
#define button 33
// Update these with values suitable for your network.

const char* ssid = "WLAN Kabel";
const char* password = "57002120109202250682";
const char* mqtt_server = "192.168.178.24";
const char* mqttUsername = "franz";
const char* mqttPassword = "Gitarre2001:)HA";

char subTopic[] = "heizungsraum/arduino/homeautomation/wlan_Steckdose/zustand";     //payload[0] will control/set LED
char pubTopic[] = "heizungsraum/arduino/homeautomation/wlan_Steckdose/pumpe";       //payload[0] will have ledState value

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  //rtc_gpio_pulldown_en((gpio_num_t)GPIO_NUM_33);   //aktiviert den Systemischen PULLDOWN Widerstand des ESP's, obwohl er im deepsleep ist
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);    //Wenn Pin 33 (dort ist der Button angeschlossen), High ist (durch drücken des Buttons), dann wird Esp32 aufgeweckt

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  if (!client.connected()) {
    blink();
    reconnect();
    blink();
  }
  Serial.println("Switch pressed");
  snprintf (msg, MSG_BUFFER_SIZE, "1", value);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(pubTopic, msg);
}

void loop() {
  client.loop(); //hier wird in der Klasse Client die loop funktion aufgerufen, die die eintreffenden MQTT nachrichten regelmäßig abfragt -> muss gelooped werden
}


void setup_wifi() {
  delay(1000);

  int n = 0;
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);


  //  WiFi.mode(WIFI_STA);
  Serial.printf("Wi-Fi mode set to WIFI_STA %s\n", WiFi.mode(WIFI_STA) ? "" : "Failed!");

  //  delay(100);

  Serial.println(password);
  Serial.println(mqtt_server);
  Serial.println(mqttUsername);
  Serial.println(mqttPassword);

  Serial.println("start connecting");
  WiFi.begin(ssid, password);
  Serial.println("here we go");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);

  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(LED_PIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(LED_PIN, LOW);  // Turn the LED off by making the voltage HIGH
    WiFi.disconnect();
    Serial.println("going to Sleep!");
    esp_deep_sleep_start();

  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUsername, mqttPassword)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(pubTopic, "publish");
      // ... and resubscribe
      client.subscribe(subTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void blink() {
  digitalWrite(LED_PIN, HIGH);  
  delay(30);
  digitalWrite(LED_PIN, LOW);   
  delay(30);
  digitalWrite(LED_PIN, HIGH);  
  delay(30);
  digitalWrite(LED_PIN, LOW);   
}
