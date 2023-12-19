#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "./syncMqtt.h"
#include "./credentials.h"


#define PIN_WS2812B  48   // ESP32 pin that connects to WS2812B
#define NUM_PIXELS    1 
// #define NEO_PIXEL 
#ifdef NEO_PIXEL
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel WS2812B(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);
#endif

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

#define MQTT_INPUT_TOPIC "esp32/sync/input"
#define MQTT_OUTPUT_TOPIC "esp32/sync/output"

SyncMqtt syncMqtt(mqtt_client,MQTT_SERVER, mqtt_port, MQTT_CLIENT_NAME, MQTT_INPUT_TOPIC, MQTT_OUTPUT_TOPIC);
const int interval = 5000;
bool first_loop = true;
void setup(){
		Serial.begin(115200);
		#ifdef NEO_PIXEL
		WS2812B.begin();
		WS2812B.setBrightness(128); // a value from 0 to 255
		WS2812B.setPixelColor(0, WS2812B.Color(255, 0, 0));
		WS2812B.show();
		#endif
		pinMode(LED_BUILTIN, OUTPUT);

		syncMqtt.setup_wifi();
		syncMqtt.setCallback(callback);
		syncMqtt.connect();
}
bool toggle = false;
unsigned long previousMillis = 0;
int update = 0;
void loop(){
        if(syncMqtt.synced){
				if(first_loop){
						previousMillis = syncMqtt.syncedMillis() % interval;
						first_loop = false;
						// tune initial toolge state
			  	        }
				unsigned long toggels = syncMqtt.syncedMillis()/interval;
				if(toggels % 2 != 0){
					toggle = true;
				}else{
					toggle = false;
				}
				if(syncMqtt.syncedMillis() % interval <= 20){
				   Serial.print("Time: ");
				   Serial.println(syncMqtt.syncedMillis());
				   Serial.print("Synced Part: ");
				   Serial.println(syncMqtt.synced_millis);
				   Serial.print("Previous millis: ");
				   Serial.println(previousMillis);
				   previousMillis = syncMqtt.syncedMillis();
				   // previousMillis = currentMillis;
				   if(toggle){
				       #ifdef NEO_PIXEL
							   WS2812B.setPixelColor(0, WS2812B.Color(0, 0, 0));
							   WS2812B.show();
				       #endif
					   digitalWrite(LED_BUILTIN, LOW);

				    }else{
				       #ifdef NEO_PIXEL
							   WS2812B.setPixelColor(0, WS2812B.Color(255, 0, 0));
							   WS2812B.show();
				       #endif
					   digitalWrite(LED_BUILTIN, HIGH);
				    }
				}
		}

		syncMqtt.loop();

}

void callback(char* topic, byte* message, unsigned int length) {
		// print the message

		Serial.print("Custom callback: Message arrived in topic: ");
		Serial.println(topic);
		Serial.print("Message:");
		for(int i = 0; i < length; i++){
			Serial.print((char)message[i]);
			Serial.print(" ");
		}
}
