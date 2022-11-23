// ESP8266 v3.0.2
/*
This code runs in a ESP8266, that acts as a button, away from the "master"


            |-------------|
            |             | 
            |             |
            |             | 
            |     ESP     |
            |     8266    | 
            |             |
            |             | D8 -> To the button
            |     USB     |
            |----....-----|    
*/


//Libraries: 
#include <ESP8266WiFi.h>
#include <espnow.h>


//Definitions
#define TIME_BETWEEN_INTERRUPTIONS 3000     // Time MIN between interruptions
#define DEBUG_MODE 1  // To turn ON the debug mode

//PIN
const int button_pin = D8;
//const int photocell_pin = A0;
const int battery_monitoring_pin = A0;


// Interrupts
void ICACHE_RAM_ATTR button_interrupt ();


// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//VARS>
int long previous_interrupt = 0;


// Variable to store if sending data was successful
String success;

//Structure to send data
//Must match the receiver structure
typedef struct struct_message {
    bool button=0;
    bool check_connection=0;
    float battery;
    float photocell=0;
    float temperature=0;
    float humidity=0;
} struct_message;

//Battery crurve for the us18650gs -> 2200mah
// Analog read from V to % of the battery
float US18650GS_battery_curve[2][12]={{4.2, 4.0, 3.87, 3.82, 3.8, 3.75, 3.70, 3.65, 3.60, 3.55, 3.50, 3.2},     
                                      {100,  90,  80,   70,   60,   50,   40,   30,   20,   10,    5,   0}};
//https://cdn.shopifycdn.net/s/files/1/2713/4408/files/Typical_Lithium-ion_battery_discharge_curves_-_SkyGenius_Blog.jpg?v=1620888810

 
//Create a struck_message to hold the data to be send
struct_message data_to_send;

// Create a struct_message to hold incoming data
struct_message incoming_data;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

// Callback when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&incoming_data, incomingData, sizeof(incoming_data));
  Serial.print("Bytes received: ");
  Serial.println(len);
  
  if(incoming_data.check_connection==1){
    Serial.print("DATA: Check connection");
    esp_now_send(broadcastAddress, (uint8_t *) &data_to_send, sizeof(data_to_send));
    incoming_data.check_connection=1;
    incoming_data.button=0;
    Serial.println(data_to_send.photocell);
  }
  
}


// This function is triggered when the button is pressed
void button_interrupt(){
  
  //Only do the thing if the interrupt is to close to another
  if(millis() > (previous_interrupt + TIME_BETWEEN_INTERRUPTIONS)){
  previous_interrupt = millis();
  if(DEBUG_MODE)
    Serial.print(" INTERRUPT \n");
  data_to_send.button = 1;
  data_to_send.check_connection=0;
  data_to_send.photocell=analogRead(photocell_pin);
  esp_now_send(broadcastAddress, (uint8_t *) &data_to_send, sizeof(data_to_send));
  data_to_send.button = 0;
  
  }
  else{
    if(DEBUG_MODE){
      Serial.print(" CHILL\n");
    }
  }
  
  
  
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  pinMode(button_pin, INPUT_PULLUP);

  //Interrupt
  //attachInterrupt(digitalPinToInterrupt(button_pin), push_interrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(button_pin), button_interrupt, RISING);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
  Serial.print("\nALL IS READY!\n");
}
 
void loop() {
  
}
