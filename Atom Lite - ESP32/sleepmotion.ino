#include <WiFi.h>
#include "BLEDevice.h"
#include "BLEAdvertisedDevice.h"
#include "M5Atom.h"
#include <HTTPUpdate.h>

String version = "bleclient-1013";
String updateUrl = "http://yourupdateserver/index.php";

// Address and service/characteristics of sleepmotion bed
std::string myDevice = "57:4c:54:2c:c6:31";
char bleServiceUUID[] = "0000fee9-0000-1000-8000-00805f9b34fb";
char bleCharUUID[] = "d44bc439-abfd-45a2-b575-925416129600";

// Generic device name
char bleGenericAccessServiceUUID[] = "00001800-0000-1000-8000-00805f9b34fb";
char bleDeviceNameCharUUID[] = "00002a00-0000-1000-8000-00805f9b34fb";

// Control variables
static boolean doConnect = false;
static boolean connected = false;

// BLE objects
BLEScan* pBLEScan;
static BLEAddress* pServerAddress;
static BLERemoteCharacteristic* pRemoteCharacteristic;

byte commands[7][5] = {
  {0x6e, 0x01, 0x00, 0x3c, 0xab}, // light
  {0x6e, 0x01, 0x00, 0x45, 0xb4},  // zg
  {0x6e, 0x01, 0x00, 0x31, 0xa0}, // flat
  {0x6e, 0x01, 0x00, 0x24, 0x93},  // head up
  {0x6e, 0x01, 0x00, 0x25, 0x94}, // head down
  {0x6e, 0x01, 0x00, 0x26, 0x95},  // feet up
  {0x6e, 0x01, 0x00, 0x27, 0x96}   // feet down
};

// WebServer and http request variables 
WiFiServer serverBLE(80);

String header;
String resp = "";
String connectionDetails = "";

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// HTML style
const char *style = R""""(
.accordion > input[name="collapse"] {
  display: none;
  
  /*position: absolute;
  left: -100vw;*/
}

.accordion label,
.accordion .content{
 max-width: 620px;
  margin: 0 auto;
 }

.accordion .content {
  background: #fff;
  overflow: hidden;
  text-align: left;
  height: 0;
  transition: 0.5s;
  box-shadow: 1px 2px 4px rgba(0, 0, 0, 0.3);
}

.accordion > input[name="collapse"]:checked ~ .content {
  height: 380px;
  transition: height 0.5s;
}

.accordion label {
  display: block;
}

.accordion {
  margin-bottom: 1em;
}

.accordion > input[name="collapse"]:checked ~ .content {
  border-top: 0;
  transition: 0.3s;
}

.accordion .handle {
  margin: 0;
  font-size: 16px;
}

.accordion label {
  color: #fff;
  cursor: pointer;
  font-weight: normal;
  padding: 10px;
  background: #b0100c;
  user-select: none;
}

.accordion label:hover,
.accordion label:focus {
  background: #252525;
}

.accordion .handle label:before {
  font-family: FontAwesome;
  content: "\f107";
  display: inline-block;
  margin-right: 10px;
  font-size: 1em;
  line-height: 1.556em;
  vertical-align: middle;
  transition: 0.4s;
}

.accordion > input[name="collapse"]:checked ~ .handle label:before {
    transform: rotate(180deg);
    transform-origin: center;
    transition: 0.4s;
}

*,
*:before,
*:after {
  box-sizing: border-box;
}

body {
  background: #ccc;
  padding: 10px;
}

a {
  color: #06c;
}

p {
  margin: 0 0 1em;
  padding: 10px;
}

h1 {
  margin: 0 0 1.5em;
  font-weight: 600;
  font-size: 1.5em;
}

.accordion p:last-child {
  margin-bottom: 0;
}
.container{
    max-width: 820px;
    margin: 0 auto;
}
)"""";

// BLE scan call back
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    
    connectionDetails = connectionDetails + "<li>BLE Advertised Device found: " + advertisedDevice.toString().c_str();
    connectionDetails = connectionDetails + " <a href=\"/connect/?mydevice=" + advertisedDevice.getAddress().toString().c_str() + "\">Connect</a></li>";
    BLEAddress* bleMyDevice = new BLEAddress(myDevice);

    if (advertisedDevice.getAddress().toString() == bleMyDevice->toString()) {
      Serial.print("Found our device!  address: ");
      connectionDetails = connectionDetails + "<p>Found our device! ";
      
      advertisedDevice.getScan()->stop();
      pBLEScan->stop();
      
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    }
  }
};

void scan(){
  connected = false;
  BLEDevice::deinit(false);
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  connectionDetails = "<ol>";
  pBLEScan->setActiveScan(true);
  pBLEScan->start(10);
  connectionDetails = connectionDetails + "</ol>";
  pBLEScan->clearResults(); 
}

void connectBLE(BLEAddress pAddress) {
  connected = false;
  Serial.println("Connecting to BLE Server");
  connectionDetails = connectionDetails + "<p>Connecting to BLE Server";

  BLEClient* pClient = BLEDevice::createClient();
  pClient->connect(pAddress);
  delay(250);

  if (pClient->isConnected()) {
    Serial.println("Connected!");
    connectionDetails = connectionDetails + "<br>Connected";

    BLEUUID serviceUUID(bleServiceUUID);
    BLEUUID charUUID(bleCharUUID);

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.println("Failed to get service");
      connectionDetails = connectionDetails + "<br>Failed to get service</p>";
      return;
    } else {
      Serial.println("Got service");
      connectionDetails = connectionDetails + "<br>Got service";
    }
   
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.println("Failed to get characteristic");
      connectionDetails = connectionDetails + "<br>Failed to get characteristic</p>";
      return;
    } else {
      Serial.println("Got characteristic");
      connectionDetails = connectionDetails + "<br>Got characteristic";
    }
  } else {
    Serial.println("Failed to connect to BLE Server");
    connectionDetails = connectionDetails + "<br>Failed to connect to BLE Server";
  }
  delay(250);
  if (pClient->isConnected()) {
    connected = true;
  }
  connectionDetails = connectionDetails + "</p>";
  return;
}

void bleWifiClient(WiFiClient client){
  if (pRemoteCharacteristic != nullptr) {
    resp = "Connected";
  } else {
    resp = "Not Connected";
    connected = false;
  }

  connectionDetails = connectionDetails + "<p><a href=\"/scan\">Scan</a></p>";
  if (header.indexOf("GET /connect") >= 0) {
    if (header.indexOf("mydevice=") >= 0) {
      int start = header.indexOf("mydevice=") + 9;
      int end = start + 17;
      myDevice = header.substring(start,end).c_str();
      pRemoteCharacteristic = NULL;
    }
    if (pRemoteCharacteristic == nullptr) {
      scan();
      resp = "Connecting - <a href=\"/\">Refresh</a>";                 
    }
  } else if (header.indexOf("GET /scan") >= 0) {
    myDevice = "00:00:00:00:00:00";
    scan();
    resp = "Scanning - <a href=\"/\">Refresh</a>";
  } else if (header.indexOf("GET /update") >= 0) {
    update();
  } else if (header.indexOf("GET /restart") >= 0) {
    ESP.restart();
  } else if (header.indexOf("GET /light") >= 0) {
    Serial.println("BLE Light");
    pRemoteCharacteristic->writeValue(commands[0], sizeof(commands[0]));
  } else if (header.indexOf("GET /zg") >= 0) {
    Serial.println("BLE ZG");
    pRemoteCharacteristic->writeValue(commands[1], sizeof(commands[1]));
  } else if (header.indexOf("GET /flat") >= 0) {
    Serial.println("BLE Flat");
    pRemoteCharacteristic->writeValue(commands[2], sizeof(commands[2]));
  } else if (header.indexOf("GET /situp") >= 0) {
    Serial.println("BLE Situp");
    int hold = 32;
    while (hold >= 0) {
      pRemoteCharacteristic->writeValue(commands[5], sizeof(commands[5]));
      delay(250);
      hold = hold - 1;
    }
  
    hold = 90;
    while (hold >= 0) {
      pRemoteCharacteristic->writeValue(commands[3], sizeof(commands[3]));
      delay(250);
      hold = hold - 1;
    }
    
  } else if (header.indexOf("GET /feetdown") >= 0) {
    Serial.println("BLE Feet Down");
    int hold = 32;
    while (hold >= 0) {
      pRemoteCharacteristic->writeValue(commands[6], sizeof(commands[6]));
      delay(250);
      hold = hold - 1;
    }
  }
  
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}" + String(style));
  client.println("</style></head>");
  
  client.println("<body><a href=\"/\"><h3>ESP32 Sleepmotion 200i BLE Client</h3></a>");
  client.println("<section class=\"accordion\"><input type=\"radio\" name=\"collapse\" id=\"handle1\"><h3 class=\"handle\"><label for=\"handle1\">" + resp + "</label></h3><div class=\"content\"><p>" + connectionDetails + "</p></div></section>");
  if (connected) {  
    client.println("<section class=\"accordion\"><input type=\"radio\" name=\"collapse\" id=\"handle2\" checked=\"checked\"><h3 class=\"handle\"><label for=\"handle2\">BLE Controls</label></h3><div class=\"content\">");
    client.println("<p><a href=\"/light\">Light</a></p>");
    client.println("<p><a href=\"/zg\">ZG</a></p>");
    client.println("<p><a href=\"/flat\">Flat</a></p>");
    client.println("<p><a href=\"/situp\">Situp</a></p>");
    client.println("<p><a href=\"/feetdown\">Feet Down</a></p>");
    client.println("</div></section>");
  }
  client.println("<section class=\"accordion\"><input type=\"radio\" name=\"collapse\" id=\"handle3\"><h3 class=\"handle\"><label for=\"handle3\">ESP Controls</label></h3><div class=\"content\">");
  client.println("<p><a href=\"/restart\">Restart</a></p>");
  client.println("<p><a href=\"/update\">Update</a></p>");
  client.println("<p>Version: " + version + "</p>");
  client.println("</div></section>");
  client.println("</body></html>");
}

void handleClient(WiFiClient client){
  currentTime = millis();
  previousTime = currentTime;
  Serial.println("New Client.");
  String currentLine = "";       
  while (client.connected() && currentTime - previousTime <= timeoutTime) { 
    currentTime = millis();
    if (client.available()) {             
      char c = client.read();             
      Serial.write(c);        
      header += c;
      if (c == '\n') {      
        if (currentLine.length() == 0) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();

          bleWifiClient(client);
          
          client.println();
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {  
        currentLine += c; 
      }
    }
  }

  header = "";
  client.stop();
  Serial.println("Client disconnected.");
  Serial.println("");
}

// OTA update functionality
void update()
{
  WiFiClient client;
  httpUpdate.update(client, updateUrl);
}

boolean attemptConnect() {
  int i;

  for (i=50; i && (WiFi.status() != WL_CONNECTED); --i) {
    if (i==50){
      Serial.println("Waiting for WiFi");
    }
    delay(500);
    Serial.print(".");
  }
  return (i != 0);
}

void setup() {
  M5.begin(true, false, true); 
  M5.dis.clear(); 

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin();
  
  if ( ! attemptConnect()) {
    WiFi.beginSmartConfig();
  
    Serial.println();
    Serial.println("Waiting for SmartConfig.");
    while (! WiFi.smartConfigDone()) {
      delay(500);
      Serial.print(".");
    }
    WiFi.stopSmartConfig();
    
    Serial.println("\nSmartConfig completed.");
  }
  WiFi.begin();

  if ( ! attemptConnect()) {
    Serial.println("Wifi connect failed (again)");
    ESP.restart();
  }
  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  BLEDevice::init("");
  scan();
  
  serverBLE.begin();
}

void loop(){
  if (doConnect == true) {
    connectBLE(*pServerAddress);
    doConnect = false;
  }

  WiFiClient bclient = serverBLE.available();   
  
  if (bclient) {
    handleClient(bclient);   
  }
}