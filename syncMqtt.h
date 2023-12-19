#include <PubSubClient.h>
class SyncMqtt {
  public:
    SyncMqtt(PubSubClient client,const char* server, int port, const char* client_name, const char* inputTopic, const char* outputTopic);
    void setup_wifi();
	void connect();
	bool connected();
    void reconnect();
	void setCallback(std::function<void(char*, uint8_t*, unsigned int)> callback);
	int isSyncReq(char message[], unsigned int length);
	int isSyncRep(char message[], unsigned int length);
    void loop();
	unsigned long syncedMillis();
	unsigned long synced_millis = 0;
	bool synced = false;
    enum SyncState {
			NOT_SYNCED,
			WAITING_FOR_SYNC,
			SYNCED
		} syncState = NOT_SYNCED;
	void sync();
  private:
    const char* server;
    int port;
	bool master = false;
	// we need this to be able to control the callback
	bool synced_offered = false;
    const char* client_name;
    const char* inputTopic;
    const char* outputTopic;
	PubSubClient client;
	long waiting_for_sync = 0;
    std::function<void(char*, uint8_t*, unsigned int)> callback;
	void _callback(char* topic, byte* message, unsigned int length, SyncMqtt *instance);
    static void callbackWrapper(char* topic, byte* payload, unsigned int length) {
  }

	
	
};
