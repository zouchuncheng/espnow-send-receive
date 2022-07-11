#include <esp_now.h>
#include <WiFi.h>
//#include "../src/Logging.h"

// Define a data structure
typedef struct struct_message {
    char  a[32];
    int   b;
    float c;
    bool  d;
} struct_message;

// Create a structured object
struct_message toSendData;
// Global copy of slave
#define NUMSLAVES 20
esp_now_peer_info_t slaves[NUMSLAVES] = {};
int                 SlaveCnt          = 0;

#define CHANNEL 1
#define PRINTSCANRESULTS 0

// Init ESP Now with fallback
void InitESPNow() {
    WiFi.disconnect();
    if (esp_now_init() == ESP_OK) {
        Serial.println("ESPNow Init Success");
    } else {
        Serial.println("ESPNow Init Failed");
        // Retry InitESPNow, add a counte and then restart?
        // InitESPNow();
        // or Simply Restart
        ESP.restart();
    }
}

// Scan for slaves in AP mode
void ScanForSlave() {
    int8_t scanResults = WiFi.scanNetworks();
    //reset slaves
    memset(slaves, 0, sizeof(slaves));
    SlaveCnt = 0;
    Serial.println("");
    if (scanResults == 0) {
        Serial.println("No WiFi devices in AP Mode found");
    } else {
        Serial.print("Found ");
        Serial.print(scanResults);
        Serial.println(" devices ");
        for (int i = 0; i < scanResults; ++i) {
            // Print SSID and RSSI for each device found
            String  SSID     = WiFi.SSID(i);
            int32_t RSSI     = WiFi.RSSI(i);
            String  BSSIDstr = WiFi.BSSIDstr(i);

            if (PRINTSCANRESULTS) {
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(SSID);
                Serial.print(" [");
                Serial.print(BSSIDstr);
                Serial.print("]");
                Serial.print(" (");
                Serial.print(RSSI);
                Serial.print(")");
                Serial.println("");
            }
            delay(10);
            // Check if the current device starts with `Slave`
            if (SSID.indexOf("Slave") == 0) {
                // SSID of interest
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(SSID);
                Serial.print(" [");
                Serial.print(BSSIDstr);
                Serial.print("]");
                Serial.print(" (");
                Serial.print(RSSI);
                Serial.print(")");
                Serial.println("");
                // Get BSSID => Mac Address of the Slave
                int mac[6];

                if (6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])) {
                    for (int ii = 0; ii < 6; ++ii) {
                        slaves[SlaveCnt].peer_addr[ii] = (uint8_t)mac[ii];
                    }
                }
                slaves[SlaveCnt].channel = CHANNEL;  // pick a channel
                slaves[SlaveCnt].encrypt = 0;        // no encryption
                SlaveCnt++;
            }
        }
    }

    if (SlaveCnt > 0) {
        Serial.print(SlaveCnt);
        Serial.println(" Slave(s) found, processing..");
    } else {
        Serial.println("No Slave Found, trying again.");
    }

    // clean up ram
    WiFi.scanDelete();
}

// Check if the slave is already paired with the master.
// If not, pair the slave with master
void manageSlave() {
    if (SlaveCnt > 0) {
        for (int i = 0; i < SlaveCnt; i++) {
            Serial.print("Processing: ");
            for (int ii = 0; ii < 6; ++ii) {
                //Serial.println((uint8_t) slaves[i].peer_addr[ii], HEX);
                if (ii != 5)
                    Serial.print(":");
            }
             Serial.println(" ");
            Serial.print(" Status: ");
            // check if the peer exists
            bool exists = esp_now_is_peer_exist(slaves[i].peer_addr);
            if (exists) {
                // Slave already paired.
                Serial.println("Already Paired");
            } else {
                // Slave not paired, attempt pair
                esp_err_t addStatus = esp_now_add_peer(&slaves[i]);
                if (addStatus == ESP_OK) {
                    // Pair success
                    Serial.println("Pair success");
                } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
                    // How did we get so far!!
                    Serial.println("ESPNOW Not Init");
                } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
                    Serial.println("Add Peer - Invalid Argument");
                } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
                    Serial.println("Peer list full");
                } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
                    Serial.println("Out of memory");
                } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
                    Serial.println("Peer Exists");
                } else {
                    Serial.println("Not sure what happened");
                }
                delay(100);
            }
        }
    } else {
        // No slave found to process
        Serial.println("No Slave found to process");
    }
}

// send data
void SendData() {
    // Create test data

    // Variables for test data
    int   int_value;
    float float_value;
    bool  bool_value = true;

    for (int i = 0; i < SlaveCnt; i++) {
        const uint8_t* peer_addr = slaves[i].peer_addr; 
        // Generate a random integer
        int_value = random(1, 20);

        // Use integer to make a new float
        float_value = 1.3 * int_value;

        // Invert the boolean value
        bool_value = !bool_value;

        // Format structured data
        strcpy(toSendData.a, "Welcome to the Workshop!");
        toSendData.b = int_value;
        toSendData.c = float_value;
        toSendData.d = bool_value;

        // Send message via ESP-NOW
        esp_err_t result = esp_now_send(peer_addr, (uint8_t*)&toSendData, sizeof(toSendData));

        Serial.print("Send Status: ");
        if (result == ESP_OK) {
            Serial.println("Success");
        } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
            // How did we get so far!!
            Serial.println("ESPNOW not Init.");
        } else if (result == ESP_ERR_ESPNOW_ARG) {
            Serial.println("Invalid Argument");
        } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
            Serial.println("Internal Error");
        } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
            Serial.println("ESP_ERR_ESPNOW_NO_MEM");
        } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
            Serial.println("Peer not found.");
        } else {
            Serial.println("Not sure what happened");
        }
        delay(100);
    }
}

// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(
        macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print("Last Packet Sent to: ");
    Serial.println(macStr);
    String success;
    if (status == 0) {
        success = "  Delivery Success :)";
    } else {
        success = "Delivery Fail :(";
    }
    Serial.println(success);
}

// Callback function executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  memcpy(&toSendData, incomingData, sizeof(toSendData));
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Data received: ");
  Serial.println(len);
  Serial.print("Character Value: ");
  Serial.println(toSendData.a);
  Serial.print("Integer Value: ");
  Serial.println(toSendData.b);
  Serial.print("Float Value: ");
  Serial.println(toSendData.c);
  Serial.print("Boolean Value: ");
  Serial.println(toSendData.d);
}

void setup() {
    Serial.begin(115200);
    //Set device in STA mode to begin with WIFI_MODE_APSTA
    WiFi.mode(WIFI_MODE_APSTA);
    Serial.println("ESPNow Master Example");
    // This is the mac address of the Master in Station Mode
   Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
   Serial.print("STA MAC: ");   Serial.println(WiFi.macAddress());
    // Init ESPNow with a fallback logic
    InitESPNow();
    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    // In the loop we scan for slave
    ScanForSlave();
    // If Slave is found, it would be populate in `slave` variable
    // We will check if `slave` is defined and then we proceed further
    if (SlaveCnt > 0) {  // check if slave channel is defined
        // `slave` is defined
        // Add slave as peer if it has not been added already
        manageSlave();
        // pair success or already paired
        // Send data to device
        SendData();
    } else {
        // No slave found to process
    }

    // wait for 3seconds to run the logic again
    delay(1000);
}
