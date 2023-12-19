#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <functional>


SyncMqtt::SyncMqtt(PubSubClient client,const char* server, int port, const char* client_name, const char* inputTopic, const char* outputTopic) {
  this->client = client;
  this->server = server;
  this->port = port;
  this->client_name = client_name;
  this->inputTopic = inputTopic;
  this->outputTopic = outputTopic;
}
void SyncMqtt::setCallback(std::function<void(char*, uint8_t*, unsigned int)> callback) {
  this->callback = callback;
}
void SyncMqtt::connect() {
		client.setServer(server, port);
		client.setCallback(std::bind(&SyncMqtt::_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,this));
		if( client.connect(client_name) ){
			Serial.println("Connected to MQTT Broker!");
			client.subscribe(outputTopic); 
		}
		else{
			Serial.println("Connection to MQTT Broker failed...");
		}

}
bool SyncMqtt::connected() {
	return client.connected();
}
void SyncMqtt::sync(){
    StaticJsonDocument<256> doc;
	doc["sync_req"] = client_name;
    doc["t0"] = xTaskGetTickCount()*portTICK_PERIOD_MS;
    char buffer[256];
    serializeJson(doc, buffer);
	waiting_for_sync = millis();
	syncState = WAITING_FOR_SYNC;
	client.publish(inputTopic,buffer);

}
int SyncMqtt::isSyncReq(char message[], unsigned int length){
	// check if message is sync message
	char *sync_rep = "{\"sync_req\":";
	//only check beginning of message
	for(int i = 0; i < 10; i++){
		if(message[i] != sync_rep[i]){
			return 0;
		}
	}
	return 1;
}
int SyncMqtt::isSyncRep(char message[], unsigned int length){
	// check if message is sync message
	char *sync_rep = "{\"sync_rep\":";
	//only check beginning of message
	for(int i = 0; i < 10; i++){
		if(message[i] != sync_rep[i]){
			return 0;
		}
	}
	return 1;
}
void SyncMqtt::_callback(char* topic, byte* message, unsigned int length, SyncMqtt* instance) {
		TickType_t tick_start = xTaskGetTickCount();
		unsigned long current_millis = millis();
		Serial.print("Message arrived in topic: ");
		Serial.println(topic);
		Serial.print("state: ");
		Serial.println(instance->syncState);
		char msg[length];
		for(int i = 0; i < length; i++){
			msg[i] = (char)message[i];
		}
  if(instance->syncState == WAITING_FOR_SYNC){
		  // check if message is sync message
		  if(instance->isSyncRep(msg, length)){
		     StaticJsonDocument<256> receivedDoc;

				// Deserialize the JSON string into the document
		     DeserializationError error = deserializeJson(receivedDoc, msg);

		     // Check for deserialization errors
		     if (error) {
					Serial.print("Error during deserialization: ");
					Serial.println(error.c_str());
					return;
		     } 
			// Access the values from the deserialized document
			const char* receivedClientName = receivedDoc["sync_rep"];

			// Now you can use the receivedClientName as needed
			Serial.print("Received client name: ");
			Serial.println(receivedClientName);
		   //    // get the millis from the message
			  unsigned long t0 = receivedDoc["t0"];
			  unsigned long t1 = receivedDoc["t1"];
			  unsigned long t2 = receivedDoc["t2"];
			  // the latency caused by the PubSubClient is around 30ms but this is not a good approach and it would be bedder
			  // if I could directly get the time when the message was received from the PubSubClient
			  t1 = t1-31;
			  Serial.println("We are slave and received sync message");
			  unsigned long t3 = tick_start*portTICK_PERIOD_MS;
			  // same thing here
			  t3 = t3-31;
			  int latency = ((int)(t3-t0)-(int)(t2-t1))/2;
			  Serial.print("Latency: ");
			  Serial.println(latency);
			  if(t3 < t2){
			      instance->synced_millis = t2-t3+latency;
			  }else{
				  instance->synced_millis = t3-t2-latency;
			  }
			  Serial.print("Synced millis: ");
			  Serial.println(instance->synced_millis);
			  Serial.println(t2);

			  // if it is sync message set us as synced
			  instance->syncState = SYNCED;
			  instance->synced_offered = true;
			  // set us as slave
			  instance->master = false;
			  // set synced to true
			  instance->waiting_for_sync = 0;

		}else{
			// if not call user callback
			instance->callback(topic, message, length);
		}
     

  }else if(instance->syncState == SYNCED){
		
    	// check if message is sync message	and if we are master reply with sync message
		if(instance->isSyncReq(msg, length)){
				Serial.println("sunc req");
				if(instance->master){
		           Serial.println("We are master and seding sync message");
				   StaticJsonDocument<256> receivedDoc;

						// Deserialize the JSON string into the document
					 DeserializationError error = deserializeJson(receivedDoc, msg);

					 // Check for deserialization errors
					 if (error) {
							Serial.print("Error during deserialization: ");
							Serial.println(error.c_str());
							return;
					 } 
				   StaticJsonDocument<256> doc;
				   char buffer[256];
				   doc["sync_rep"] = client_name;
				   doc["t0"] = receivedDoc["t0"];
				   doc["t1"] = tick_start*portTICK_PERIOD_MS;
				   doc["t2"] =  xTaskGetTickCount()*portTICK_PERIOD_MS;
				   serializeJson(doc, buffer);
				   client.publish(instance->outputTopic,buffer);
				   Serial.println("Send message");
				}
		}else{
		// 		// if not call user callback
				instance->callback(topic, message, length);
		}

  }
  else{
    Serial.println("Not a valid state because we cannot be not synced and not waiting for sync");
  }
}

unsigned long SyncMqtt::syncedMillis() {
		if(!master){
		   return millis() + synced_millis;
		}
		return millis() + synced_millis;
}
void SyncMqtt::reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_name)) {
      Serial.println("connected");
      // Subscribe
	  if(this->master){
		 client.unsubscribe(inputTopic);
         client.subscribe(inputTopic);
	  }else{
		 client.unsubscribe(outputTopic);
	     client.subscribe(outputTopic);	
	  }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void SyncMqtt::loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // only continue if we are connected
  if(!client.connected()){
    return;
  }
  if(syncState == NOT_SYNCED){
		  Serial.println("Start syncing");
		  this->sync();
  }
  else if(syncState == WAITING_FOR_SYNC && !synced_offered){
    if(millis() - waiting_for_sync > 10000){
      Serial.println("Sync timeout");
	  // Setting us as synced and there for master
      this->syncState = SYNCED;
	  this->synced = true;
	  this->master = true;
	  client.unsubscribe(outputTopic);
	  client.subscribe(inputTopic);
    }
  }else{
     if(synced_offered){
		 synced = synced_offered;
	 }
    // assert(syncState == SYNCED);
  }
}
void SyncMqtt::setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
