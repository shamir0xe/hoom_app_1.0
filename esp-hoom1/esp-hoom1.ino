// HOOM version1
// connecting ESP to router

#include <ESP8266WiFi.h>
#include <aREST.h>
//static const uint8_t D0   = 16;
//static const uint8_t D1   = 5;
//static const uint8_t D2   = 4;
//static const uint8_t D3   = 0;
//static const uint8_t D4   = 2;
//static const uint8_t D5   = 14;
//static const uint8_t D6   = 12;
//static const uint8_t D7   = 13;
//static const uint8_t D8   = 15;
//static const uint8_t D9   = 3;
//static const uint8_t D10  = 1;
const char* ssid     = "vampire";
const char* password = "1412999118";  
// the IP address for the shield:
IPAddress ip(192, 168, 0, 178);  
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
int counter;
int state = 0;
int openPin = D1;
int resetPin = D2;
int wifiStatus;
// Variables to be exposed to the API
int temperature;
int humidity;
// Create aREST instance
aREST rest = aREST();
int dataBaseSize = 3;
// hard code for shamir's door app
#define ARRAYSIZE 3
String usr[ARRAYSIZE] = { "shamir0xe", "h.shapoori" , "zhalehfa"};
String pwd[ARRAYSIZE] = { "02469Amir", "876314" , "631383"};

WiFiServer server(80); //Initialize the server on Port 80 
void setup() {
  pinMode(openPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  digitalWrite(openPin,LOW);
  digitalWrite(resetPin,HIGH);
  Serial.begin(115200);
  server.begin(); // Start the HTTP Server
  // We start by connecting to a WiFi network
  // Init variables and expose them to REST API
  // Function to be exposed
  rest.function("auth",authControl);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("1");
  rest.set_name("SmartDoor");
  
  Serial.print("Your are connecting to;");
  Serial.println(ssid);
  
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);    
}   

void loop() {
  ESP.wdtFeed();
  if (state == 0){
   WiFi.begin(ssid, password);
   WiFi.config(ip, gateway, subnet);
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
  if (WiFi.status() != WL_CONNECTED) {
    state = 0;
    return;
  }
  WiFiClient client = server.available();
  if (!client) { 
   return; 
 }
        //Looking under the hood 
 Serial.println("Somebody has connected :)");
 counter = 0;
 while(!client.available()){
  delay(1);
  counter ++;
  if(counter >= 5555) {
    return;
  }
}
rest.handle(client);
}
}

int authControl(String command){
  int index = command.indexOf('&');
  if (index < 0 || index >= command.length())
    return 0;
  int scndIndx = command.indexOf('!');
  if (scndIndx < 0 || scndIndx >= command.length())
    return 0;
  String reqUsr = command.substring(0, index);
  String reqPwd = command.substring(index+1, scndIndx);
  String reqCmd = command.substring(scndIndx+1);
  if (reqUsr.length() <= 0 || reqPwd.length() <= 0 || reqCmd.length() <= 0)
    return 0;
  boolean validUsr = true;
  int idx;
  //checking if the usr is valid
  for(int i=0 ; i<3 ; i++){
    validUsr = true;
    if (reqUsr.length() != usr[i].length()) {
      validUsr = false;
    }
      for(int j=0 ; j<reqUsr.length() ; j++){
        if (reqUsr[j] != usr[i][j]){
         validUsr = false;
         break;
        }
        idx = i;
      } 
    if (validUsr == true){ // we have found a valid usr, so lets check the password
      Serial.print("user is valid:");
      Serial.println(reqUsr);
      if (passCheck(idx, reqPwd)){
        if (reqCmd == "o"){
          //lets open
          Serial.println("open command");
          digitalWrite(openPin,HIGH);
          delay(1000);
          digitalWrite(openPin,LOW);  
        }
        else if(reqCmd == "r"){
          //lets reset
          Serial.println("reset command");
          digitalWrite(resetPin,LOW);
          delay(1000);
          digitalWrite(resetPin,HIGH);  
        }
        else{
          Serial.println("wrong command");
        }
        return 1;
      }
      else{
        return 0;
      }
      break;
    }
  }
}
boolean passCheck(int index, String reqPwd){
  int i;
  for(i=0 ; i<pwd[index].length() ; i++){
    if (i >= reqPwd.length()) {
      return false;
    }
    if( pwd[index][i] != reqPwd[i] ){
      Serial.println("wrong password");
      return false;
    }
  }
  if (i != reqPwd.length())
    return false;
  Serial.println("correct password");
  return true;
}


