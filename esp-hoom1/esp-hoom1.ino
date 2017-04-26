// Henry's Bench
//Checking to ensure you can connect ESP-12E to a router
     
#include <ESP8266WiFi.h>
#include <aREST.h>
const char* ssid     = "hardtech";
const char* password = "qazwsxedc";    
int state = 0;
int LED1 = 16;
int wifiStatus;
// Variables to be exposed to the API
int temperature;
int humidity;
// Create aREST instance
aREST rest = aREST();

WiFiServer server(80); //Initialize the server on Port 80 
void setup() {
  
  Serial.begin(115200);
  server.begin(); // Start the HTTP Server
  // We start by connecting to a WiFi network
  // Init variables and expose them to REST API
  temperature = 24;
  humidity = 40;
  rest.variable("temperature",&temperature);
  rest.variable("humidity",&humidity);
  // Function to be exposed
  rest.function("led",ledControl);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("1");
  rest.set_name("SmartDoor");
  
  Serial.print("Your are connecting to;");
  Serial.println(ssid);
  pinMode(LED1, OUTPUT);
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);    
}   
     
void loop() {
      ESP.wdtFeed();
      if (state == 0){
         WiFi.begin(ssid, password);
         while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
         }
         Serial.println("");
         Serial.println("Your ESP is connected!");  
         Serial.println("Your IP address is: ");
         Serial.println(WiFi.localIP());
         state = 1;
      }
      else{
       WiFiClient client = server.available();
      if (!client) { 
       return; 
      }
      //Looking under the hood 
      Serial.println("Somebody has connected :)");
      while(!client.available()){
        delay(1);
      }
      rest.handle(client);
      //Read what the browser has sent into a String class and print the request to the monitor
//      String request = client.readString();
//      //Looking under the hood 
//      Serial.println(request);
      // Handle the Request
//      int a = request.indexOf("/");
//      request[a]
      
//       Serial.println("turn off");
//       digitalWrite(LED1, LOW); 
//       client.println("turned off");
//      else if (request.indexOf("/ON") != -1){ 
//       Serial.println("turn on");
//       digitalWrite(LED1, HIGH); 
//       client.println("turned on");
//      } 
      }
}

// Custom function accessible by the API
int ledControl(String command) {

  // Get state from command
  int state = command.toInt();

  digitalWrite(16,state);
  return 1;
}
