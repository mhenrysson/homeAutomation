#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <Timer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

#define blinker(h, l) blinkOnTime=h;blinkOffTime=l;

#define NORMAL_BLINK_ON 150
#define NORMAL_BLINK_OFF 10000
#define NO_WIFI_BLINK_ON 1500
#define NO_WIFI_BLINK_OFF 300
#define TEMP_WARN_BLINK_ON 75
#define TEMP_WARN_BLINK_OFF 75
#define AP_NO_CLIENT_BLINK_ON 150
#define AP_NO_CLIENT_BLINK_OFF 150
#define AP_CONNECTED_BLINK_ON 300
#define AP_CONNECTED_BLINK_OFF 50

#define SEND_TEMP_INTERVAL 30000
#define AP_SSID "TempSensor"
#define AP_PASSWORD "thereisnospoon"
#define CONFIGFILE "config.json"
#define BUTTONPIN D1
#define ONEWIREPIN D2
#define NO_WIFI_SPEC "NoWiFiYet"
#define BAUDRATE 115200

#define lowarg server.arg("lowerwarn")
#define upparg server.arg("upperwarn")
#define sensorarg server.arg("sensorID")
#define mqttarg server.arg("mqttServer")
#define ssidarg server.arg("ssid")
#define passarg server.arg("password")

char *sensorID = "TempSensor";
char *ssid = NO_WIFI_SPEC;
char *password = "isthereaspoon";
char *mqttServer = "mqtt.example.local";
uint16_t mqttPort = 1883;
char *topic = "/tempReadings";
// TODO: Add MQTT port and topic to web form

bool isAP = false;
int blinkOnTime = 1000;  // Time interval in ms onboard led is on
int blinkOffTime = 100;  // Time interval in ms onboard led is off
char temperatureString[6];
float temp;
float upperWarn = 80.0;
float lowerWarn = -120.0;
int wiFiTimerID = -1;

ESP8266WebServer server(80);
Timer t;
OneWire oneWire(ONEWIREPIN);
DallasTemperature DS18B20(&oneWire);
WiFiClient espClient;
PubSubClient psClient(espClient);

/* Timer-administered onboard led blink.
 *  Use blinkOnTime to set time interval in ms for led to be on
 *  and blinkOffTime to set time interval for led to be off
 *  Call Timer update() function (t.update()) in loop to enable updates to led 
 */
void blinkOn() {
  digitalWrite(LED_BUILTIN, LOW);  
  t.after(blinkOnTime, blinkOff);
}
void blinkOff() {
  digitalWrite(LED_BUILTIN, HIGH);  
  t.after(blinkOffTime, blinkOn);
}

/*
 * Save wifi info to file
 */
bool saveSettings() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["ssid"] = ssid;
  root["password"] = password;
  root["sensorID"] = sensorID;
  root["mqttServer"] = mqttServer;
  root["upperWarn"] = upperWarn;
  root["lowerWarn"] = lowerWarn;
  Serial.println("Updated unit configurations.");
  root.prettyPrintTo(Serial);
  Serial.println();
  File f = SPIFFS.open(CONFIGFILE, "w");
  if(!f) {
    Serial.print("File open for write failed: ");
    Serial.println(CONFIGFILE);
    return false;
  }
  root.prettyPrintTo(f);
  f.close();
  return true;
}

/*
 * Access Point loop
 */
void APLoop() {
  if(WiFi.softAPgetStationNum() == 0) {
    blinker(AP_NO_CLIENT_BLINK_ON, AP_NO_CLIENT_BLINK_OFF);
  } else {
    blinker(AP_CONNECTED_BLINK_ON, AP_CONNECTED_BLINK_OFF);
  }    
  server.handleClient();
}

/*
 * Stop WiFi Access Point mode
 */
void stopAP() {
  WiFi.softAPdisconnect(true);
  isAP = false;  
}

/*
 * Start WiFi Access Point mode
 */
void startAP() {
  if(wiFiTimerID > 0) {
    t.stop(wiFiTimerID);
    wiFiTimerID = -1;  
  }
  Serial.println("Disconnects and clears current WiFi settings.");
  WiFi.disconnect(true);
  Serial.println("Configuring access point...");
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(300);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRequest);
  server.begin();
  Serial.println("HTTP server started");
  t.after(600000, stopAP);
  isAP = true;
}

/* Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it 
 * while WiFi in Access Point mode 
 */
bool handleRequest() {
  Serial.printf("Connected devices: %i devices\n", WiFi.softAPgetStationNum());
  Serial.print("Web server root called. ");
  if(server.args() > 0) {
    Serial.println("Arguments:");
    for(int  i=0;i<server.args();i++) {
      Serial.println("Key: " + server.argName(i) + "  Value: " + server.arg(i));
    }
    if(server.hasArg("sensorID") && sensorarg.length() > 0) {
      sensorID = new char[sensorarg.length() + 1];
      sensorarg.toCharArray(sensorID, sensorarg.length() + 1);
    }
    if(server.hasArg("mqttServer") && mqttarg.length() > 0) {
      mqttServer = new char[mqttarg.length() + 1];
      mqttarg.toCharArray(mqttServer, mqttarg.length() + 1);
    }
    if(server.hasArg("lowerwarn") && lowarg.length() > 0) {
      if(lowarg.toFloat() != 0.00 || lowarg.charAt(0) == '0')
        lowerWarn = lowarg.toFloat();
    }
    if(server.hasArg("upperwarn") && upparg.length() > 0) {
      if(upparg.toFloat() != 0.00 || upparg.charAt(0) == '0')
        upperWarn = upparg.toFloat();
    }    
    if(server.hasArg("ssid") && ssidarg.length() > 0 && server.hasArg("password") && passarg.length() > 0) {
      ssid = new char[ssidarg.length() + 1];
      ssidarg.toCharArray(ssid, ssidarg.length() + 1);
      password = new char[passarg.length() + 1];
      passarg.toCharArray(password, passarg.length() + 1);
    }
    if(saveSettings()) {
      server.send(200, "text/html", "Unit settings updated. Shutting down AP and connecting to IoT network.");
      Serial.println("Settings updated. Stopping AP and connects to WiFi.");
      stopAP();
      delay(500);
      if(startWiFi()) {
        return true;
      } else {
        Serial.println("WiFi config update failed.");
        return false;
      }
    }
  } else {
    String form = "<form method=\"get\"><div><label>SSID</label><input name=\"ssid\" value=\"" + String(ssid) 
        + "\"></div><div><label>Password</label><input name=\"password\" type=\"password\" value=\"" + String(password) 
        + "\"></div><div><label>Sensor ID</label><input name=\"sensorID\" value=\"" + String(sensorID) 
        + "\"></div><div><label>MQTT Server</label><input name=\"mqttServer\" value=\"" + String(mqttServer) 
        + "\"></div><div><label>Warn above</label><input name=\"upperwarn\" value=\"" + String(upperWarn) 
        + "\"></div><div><label>Warn below</label><input name=\"lowerwarn\" value=\"" + String(lowerWarn) 
        + "\"></div><div><button>Submit</button></div></form>";
    Serial.println("No arguments.");
    server.send(200, "text/html", form);
    Serial.println("Form sent.");
    return false;
  }
}

/*
 * Read settings from config.json
 */
bool readSettings() {
  if(!SPIFFS.exists(CONFIGFILE)) {
    Serial.println("No configuration file.");
    return false;
  }
  File f = SPIFFS.open(CONFIGFILE, "r");
  if(!f) {
    Serial.print("File open for read failed: ");
    Serial.println(CONFIGFILE);
    return false;
  }
  Serial.println("Reading config file");
  char json[f.size()];
  f.readBytes(json, f.size());
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);
  if(!root.success()) {
    Serial.print("json parseObject failed on file ");
    Serial.println(CONFIGFILE);
    return false;
  }
  String s = root["ssid"].as<String>();
  ssid = new char[s.length() + 1];
  s.toCharArray(ssid, s.length() + 1);
  s = root["password"].as<String>();
  password = new char[s.length() + 1];
  s.toCharArray(password, s.length() + 1);
  s = root["sensorID"].as<String>();
  sensorID = new char[s.length() + 1];
  s.toCharArray(sensorID, s.length() + 1);
  s = root["mqttServer"].as<String>();
  mqttServer = new char[s.length() + 1];
  s.toCharArray(mqttServer, s.length() + 1);
  lowerWarn = root["lowerWarn"].as<float>();
  upperWarn = root["upperWarn"].as<float>();
  Serial.print("ssid: ");
  Serial.println(ssid);
  Serial.print("password: ");
  Serial.println(password);    // Saved on flash mem, so no secret anyway...        
  Serial.print("sensorID: ");
  Serial.println(sensorID);         
  Serial.print("MQTT Server: ");
  Serial.println(mqttServer);         
  Serial.print("lowerWarn: ");
  Serial.println(lowerWarn);         
  Serial.print("upperWarn: ");
  Serial.println(upperWarn);         
  f.close();
  return true;
}

/*
 * Start WiFi in Station mode
 */
bool startWiFi() {
  if(wiFiTimerID > 0) {
    t.stop(wiFiTimerID);
  }
  if(strcmp(ssid, NO_WIFI_SPEC)==0) {
    Serial.println("No WiFi specfied.");
  } else {
    WiFi.begin(ssid, password);
    Serial.print("Connecting");
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i++ < 50) {
      delay(200);
      Serial.print(".");
    }
    Serial.println();
  }
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Could not connect to WiFi. Aborting.");
    wiFiTimerID = t.after(300000, [](){startWiFi(); return;});
    return false;
  }
  wiFiTimerID = -1;
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

/*
 * Configres and connects psClient
 */
bool connectMQTTClient() {
    Serial.print("Connecting to MQTT server at ");
    Serial.print(mqttServer);
    Serial.print(":");
    Serial.println(mqttPort);
    psClient.setServer(mqttServer, mqttPort);
    if(psClient.connect(sensorID)) {
      Serial.println("MQTT connection successful.");
      return true;
    } else {
      Serial.print("MQTT connection failed. State: ");
      Serial.println(psClient.state());
      return false;
    }    
}

/*
 * Get temperature reading
 */
void updateTemp() {
  Serial.print("Getting temperature reading");
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    Serial.print(".");
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  Serial.println();
}

/*
 * Send temperature reading to meter reading storage
 */
void sendTemperature() {
  updateTemp();
  char t[10];
  dtostrf(temp, 5, 1, t);
  Serial.print("Current temperature: ");
  Serial.println(t);
  if(WiFi.status() == WL_CONNECTED) {
     if(!psClient.connected()) {
       connectMQTTClient();
     }
     if(psClient.connected()) {
       Serial.println("Transmitting temperature.");
       psClient.publish(topic, t);
     }
  }
}

/*
 * Setup nodeMCU unit. Called at boot time.
 */
void setup() {
  Serial.begin(BAUDRATE);
  delay(1000);
  Serial.println();
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTONPIN, INPUT);
  SPIFFS.begin();
  FSInfo fs_info;
  if(!SPIFFS.info(fs_info)) {
    Serial.println("No File System info, formatting.");
    SPIFFS.format();
    SPIFFS.info(fs_info);
  }
  Serial.printf("Total file system size: %i\n", fs_info.totalBytes);
  Serial.printf("Used: %i\n", fs_info.usedBytes);
  blinkOn();
  Serial.println("Initialising temp sensor");
  updateTemp();
  readSettings();   
  startWiFi();
  connectMQTTClient();
  t.every(SEND_TEMP_INTERVAL, sendTemperature);
}

/*
 * Main loop. Called repeatedly after setup finished.
 */
void loop() {
  if(isAP) {
    APLoop();
  } else {
    if(digitalRead(BUTTONPIN) == HIGH) {
      startAP();
    }
    if(WiFi.status() != WL_CONNECTED){
      blinker(NO_WIFI_BLINK_ON, NO_WIFI_BLINK_OFF);
      if(wiFiTimerID < 0) {
        startWiFi();
      }
    } else {
      blinker(NORMAL_BLINK_ON, NORMAL_BLINK_OFF);
    }
    if(temp > upperWarn || temp < lowerWarn) {
      blinker(TEMP_WARN_BLINK_ON, TEMP_WARN_BLINK_OFF);
    }
    psClient.loop();
  }
  t.update();
}


