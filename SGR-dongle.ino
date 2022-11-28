#include "rom/rtc.h"
#include <M5Atom.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <Preferences.h>
#include "WebRequestHandler.h"
#include <PubSubClient.h>
#include "time.h"
#include "tls.hpp"
#include "FS.h"
#include <Update.h>
#include "ArduinoJson.h"
#include <elapsedMillis.h>
/*ESPaltherma*/
#include "DEFAULT.h"
#include "converters.h"
#include "comm.h"
#include "ArduinoJson.h"
#define RX_PIN    21// Pin connected to the TX pin of X10A 
#define TX_PIN    25// Pin connected to the RX pin of X10A
#define MySerial Serial1
//

//
unsigned int fw_ver = 105;
unsigned int onlineVersion, fw_new;
DNSServer dnsServer;
AsyncWebServer server(80);
WiFiClient wificlient;
PubSubClient mqttclient(wificlient);
WiFiClientSecure *client = new WiFiClientSecure;
PubSubClient mqttclientSecure(*client);
HTTPClient https;
bool bundleLoaded = true;
bool clientSecureBusy;

elapsedMillis sinceConnCheck, sinceUpdateCheck, sinceClockCheck, sinceLastUpload, sinceEidUpload, sinceLastWebRequest, sinceRebootCheck, sinceMeterCheck, sinceWifiCheck;

float totConDay, totConNight, totCon, totInDay, totInNight, totIn, totPowCon, totPowIn, netPowCon, totGasCon, volt1, volt2, volt3;
RTC_NOINIT_ATTR float totConToday, totConYesterday, gasConToday, gasConYesterday;
struct tm dm_time;  // dm time elements structure
time_t dm_timestamp; // dm timestamp
struct tm mb1_time;  // mbus1 time elements structure
time_t mb1_timestamp; // mbus1 timestamp
int prevDay = -1;

uint8_t DisBuff[2 + 5 * 5 * 3];
elapsedMillis ledTime;
boolean ledState = true;
byte unitState = 0;

unsigned int counter, bootcount, reconncount;
String resetReason, last_reset, last_reset_verbose;
float freeHeap, minFreeHeap, maxAllocHeap;
Preferences preferences;  
String ssidList;
char apSSID[] = "COFY0000";
byte mac[6];

boolean wifiSTA = false;
boolean rebootReq = false;
boolean rebootInit = false;
boolean wifiError, mqttHostError, mqttClientError, mqttWasConnected, httpsError, meterError, eidError, wifiSave, eidSave, mqttSave, haSave, debugInfo, timeconfigured, firstDebugPush;
boolean timeSet, mTimeFound, spiffsMounted;
String wifi_ssid, wifi_password;
String mqtt_host, mqtt_id, mqtt_user, mqtt_pass;
uint8_t prevButtonState = false;
boolean configSaved, resetWifi, resetAll;
boolean mqtt_en, mqtt_tls, mqtt_auth;
boolean update_autoCheck, update_auto, updateAvailable, update_start, update_finish, eid_en, ha_en, ha_metercreated;
unsigned int mqtt_port;
unsigned long upload_throttle;
String eid_webhook;

/*SGR dongle*/
String sgr_topic = "set/devices/heatpump/sgr_mode";
String em_topic, sgr_type, dongle_type;
String sgr_en_topic = "set/devices/heatpump/enable_sgr_control";
String sgr_allow_topic = "set/devices/heatpump/allow_sgr_control";
String sgr_post_topic = "set/devices/heatpump/sgr_postpone";
String sgr_chan1_topic = "set/devices/heatpump/sgr_chan_1";
String sgr_chan2_topic = "set/devices/heatpump/sgr_chan_2";
boolean em_en, emSave, em_cofy, sgr_dir, sgr_chan1, sgr_chan1_prev, sgr_chan2, sgr_chan2_prev;
int pulseState, sgr_lim, sgrMode, sgrPrevMode, pulseMultiplier, powerMultiplier;
int sgr_post = 12;
boolean pulseInversePolarity = false;
boolean sgr_all = true;
boolean sgr_en = true;
unsigned long pulseOnWidth, pulseOffWidth, pulsePeriod;
float remotePower, prevPower;
elapsedMillis pulseTime, prevSGRChange, sgr_all_timeout;
String mqttTopic, mqttPayload;
volatile boolean mqttReceived;

void IRAM_ATTR mqttCallback(char* topic, byte * payload, unsigned int length)
{
  mqttTopic = topic;
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  mqttPayload = messageTemp;
  mqttReceived = true;
}

/*ESPAltherma*/
Converter converter;
char registryIDs[32]; //Holds the registrys to query
elapsedMillis waiter;
int tries = 0;
int activeReg = 0;
unsigned long waitTime;
boolean reStart, data_altherma, altherma_mqtt, external_temperature;

void setup(){
  M5.begin(false, false, true);
  prevSGRChange = 3000*1000;
  delay(10);
  M5.dis.displaybuff(DisBuff);
  Serial.begin(115200);
  MySerial.begin(9600, SERIAL_8E1, RX_PIN, TX_PIN);;
  delay(500);
  getHostname();
  Serial.println();
  syslog("SGR dongle booting", 0);
  preferences.begin("cofy-config");
  delay(100);
  initConfig();
  delay(100);
  restoreConfig();
  delay(100);
  if(dongle_type == "Plan-D"){
    pinMode(19, OUTPUT);
    pinMode(22, OUTPUT);
    digitalWrite(19, LOW);
    digitalWrite(22, LOW);
  }
  else if(dongle_type == "OEM"){
    pinMode(32, OUTPUT);
    pinMode(26, OUTPUT);
    digitalWrite(32, LOW);
    digitalWrite(26, LOW);
  }
  // Initialize SPIFFS
  syslog("Mounting SPIFFS... ", 0);
  if(!SPIFFS.begin(true)){
    syslog("Could not mount SPIFFS", 3);
    return;
  }
  else{
    spiffsMounted = true;
    syslog("SPIFFS used bytes/total bytes:" + String(SPIFFS.usedBytes()) +"/" + String(SPIFFS.totalBytes()), 0);
  }
  syslog("----------------------------", 1);
  syslog("SGR dongle " + String(apSSID) + " type " + dongle_type + " V" + String(fw_ver/100.0) + " by plan-d.io", 1);
  syslog("Checking if internal clock is set", 0);
  printLocalTime(true);
  bootcount = bootcount + 1;
  syslog("Boot #" + String(bootcount), 1);
  saveBoots();
  get_reset_reason(rtc_get_reset_reason(0));
  syslog("Last reset reason: " + resetReason, 1);
  syslog("Last reset reason (verbose): " + last_reset_verbose, 1);
  debugInfo = true;
  data_altherma = true;
  external_temperature = true;
  delay(100);
  //your other setup stuff...
  if(wifiSTA){
    syslog("WiFi mode: station", 1);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    WiFi.setHostname(apSSID);
    elapsedMillis startAttemptTime;
    syslog("Attempting connection to WiFi network " + wifi_ssid, 0);
    while (WiFi.status() != WL_CONNECTED && startAttemptTime < 20000) {
      delay(200);
      Serial.print(".");
    }
    Serial.println("");
    if(WiFi.status() == WL_CONNECTED){
      syslog("Connected to the WiFi network " + wifi_ssid, 1);
      MDNS.begin("apSSID");
      unitState = 5;
      /*Start NTP time sync*/
      setClock(true);
      printLocalTime(true);
      if(client){
        syslog("Setting up SSL client", 0);
        client->setUseCertBundle(true);
        // Load certbundle from SPIFFS
        File file = SPIFFS.open("/cert/x509_crt_bundle.bin", "r");
        if(!file) {
            syslog("Could not load cert bundle from SPIFFS", 2);
            bundleLoaded = false;
        }
        // Load loadCertBundle into WiFiClientSecure
        if(file && file.size() > 0) {
            if(!client->loadCertBundle(file, file.size())){
                syslog("WiFiClientSecure: could not load cert bundle", 2);
                bundleLoaded = false;
            }
        }
        file.close();
      } else {
        syslog("Unable to create SSL client", 2);
        unitState = 5;
        httpsError = true;
      }
      if(update_start){
        startUpdate();
      }
      if(update_finish){
        finishUpdate();
      }
      if(mqtt_en) setupMqtt();
      sinceConnCheck = 60000;
      //if(update_finish) finishUpdate();
      server.addHandler(new WebRequestHandler());
      update_autoCheck = true;
      if(update_autoCheck) {
        sinceUpdateCheck = 86400000-60000;
      }
      if(eid_en) sinceEidUpload = 15*60*900000;
      syslog("Local IP: " + WiFi.localIP().toString(), 0);
    }
    else{
      syslog("Could not connect to the WiFi network", 2);
      wifiError = true;
      wifiSTA = false;
    }
  }
  if(!wifiSTA){
    syslog("WiFi mode: access point", 1);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID);
    dnsServer.start(53, "*", WiFi.softAPIP());
    MDNS.begin("apSSID");
    server.addHandler(new WebRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
    //Serial.println("AP set up");
    unitState = 3;
  }
  scanWifi();
  server.begin();
}

void loop(){
  blinkLed();
  if(sinceRebootCheck > 2000){
    if(rebootInit) ESP.restart();
    sinceRebootCheck = 0;
  }
  if(sinceMeterCheck > 60000){
    //syslog("Meter disconnected", 2);
    //meterError = true;
    sinceMeterCheck = 0;
  }
  if(!wifiSTA){
    dnsServer.processNextRequest();
    if(!timeSet) setMeterTime();
    if(sinceWifiCheck >= 300000){
      if(scanWifi()) setReboot();
      sinceWifiCheck = 0;
    }
    if(sinceClockCheck >= 3600){
      setMeterTime();
      sinceClockCheck = 0;
    }
    if(mTimeFound && ! meterError) unitState = 2;
    else unitState = 3;
  }
  else{
    if(update_autoCheck && sinceUpdateCheck >= 86400000){
      updateAvailable = checkUpdate();
      if(updateAvailable){
        if(saveConfig()) startUpdate();
      }
      sinceUpdateCheck = 0;
    }
    if(sinceClockCheck >= 3600){
      if(timeconfigured) setMeterTime();
      sinceClockCheck = 0;
    }
    if(sinceConnCheck >= 60000){
      checkConnection();
      getHeapDebug();
      sinceConnCheck = 0;
    }
    if(eid_en && mTimeFound && sinceEidUpload > 15*60*1000){
      if(eidUpload()) sinceEidUpload = 0;
      else sinceEidUpload = (15*60*900000)-(5*60*1000);
    }
    if(wifiError || mqttHostError || mqttClientError || httpsError || meterError || eidError) unitState = 5;
    else unitState = 4;
    if(reconncount > 30){
      last_reset = "Rebooting to try fix connections";
      if(saveConfig()){
        syslog("Rebooting to try fix connections", 2);
        setReboot();
      }
      reconncount = 0;
    }
  }
  M5.update();
  if (M5.Btn.pressedFor(2000)) {
    resetWifi = true;
  }
  if (M5.Btn.pressedFor(5000)) {
    resetAll = true;
  }
  if (prevButtonState != M5.Btn.isPressed()) {
    if(!M5.Btn.isPressed()){
      if(resetAll || resetWifi){
        resetConfig();
      }
    }
  }
  prevButtonState = M5.Btn.isPressed();

  /*SGR dongle*/
  mqttclient.loop();
  if(mqttReceived) processCallback();
  if(!sgr_all){
    /*If SGR control has been temporarily disallowed by the user, 
     * wait untill the timeout has passed (default: 12h) before
     * enabling it again.
     */
    if(sgr_all_timeout >= sgr_post*3600000){
      sgr_all = true;
      sgr_all_timeout = 0;
      String reply = "{\"value\": \"on\"}";
      pubMqtt("data/devices/heatpump/allow_sgr_control", reply, false);
      syslog("SGR control re-enabled by time-out", 0);
    }
  }
  else{
    sgr_all_timeout = 0;
  }
  /*If SGR mode change is requested, check if SGR control is enabled,
   * if the change request isn't too close to the previous change,
   * and switch the mode.
   */
  if(sgrMode != sgrPrevMode){
    if(sgr_en){
      if(sgr_all){
        if(prevSGRChange >= sgr_lim*1000){
          String reply = "{\"value\": \"";
          if(sgr_type == "Normal"){
            if(sgrMode == 0){
              //Normal operation
              syslog("SGR mode set to " + String(sgrMode) +": Normal operation", 0);
              if(dongle_type == "Plan-D"){
                setSGRchan(1, 19, 0);
                setSGRchan(2, 22, 0);
                //digitalWrite(19, LOW);
                //digitalWrite(22, LOW);
              }
              else{
                setSGRchan(1, 32, 0);
                setSGRchan(2, 26, 0);
                //digitalWrite(32, LOW);
                //digitalWrite(26, LOW);
              }
              reply += "Normal operation";
            }
            else if(sgrMode == 1){
              //Advise on
              syslog("SGR mode set to " + String(sgrMode) +": Advise on", 0);
              if(dongle_type == "Plan-D"){
                setSGRchan(1, 19, 1);
                setSGRchan(2, 22, 0);
                //digitalWrite(19, HIGH);
                //digitalWrite(22, LOW);
              }
              else{
                setSGRchan(1, 32, 1);
                setSGRchan(2, 26, 0);
                //digitalWrite(32, HIGH);
                //digitalWrite(26, LOW);
              }
              reply += "Advise on";
            }
            else if(sgrMode == 2){
              //Block
              syslog("SGR mode set to " + String(sgrMode) +": Block", 0);
              if(dongle_type == "Plan-D"){
                setSGRchan(1, 19, 0);
                setSGRchan(2, 22, 1);
                //digitalWrite(19, LOW);
                //digitalWrite(22, HIGH);
              }
              else{
                setSGRchan(1, 32, 0);
                setSGRchan(2, 26, 1);
                //digitalWrite(32, LOW);
                //digitalWrite(26, HIGH);
              }
              reply += "Block";
            }
            else if(sgrMode == 3){
              //Must run
              syslog("SGR mode set to " + String(sgrMode) +": Forced on", 0);
              if(dongle_type == "Plan-D"){
                setSGRchan(1, 19, 1);
                setSGRchan(2, 22, 1);
                //digitalWrite(19, HIGH);
                //digitalWrite(22, HIGH);
              }
              else{
                setSGRchan(1, 32, 1);
                setSGRchan(2, 26, 1);
                //digitalWrite(32, HIGH);
                //digitalWrite(26, HIGH);
              }
              reply += "Forced on";
            }
            else{
              syslog("SGR mode set to " + String(sgrMode) +": Normal operation", 0);
              if(dongle_type == "Plan-D"){
                setSGRchan(1, 19, 0);
                setSGRchan(2, 22, 0);
                //digitalWrite(19, LOW);
                //digitalWrite(22, LOW);
              }
              else{
                setSGRchan(1, 32, 0);
                setSGRchan(2, 26, 0);
                //digitalWrite(32, LOW);
                //digitalWrite(26, LOW);
              }
              reply += "Normal operation";
            }
          }
          prevSGRChange = 0;
          reply += "\"}";
          pubMqtt("data/devices/heatpump/sgr_state_verbose", reply, false);
        }
        else{
          syslog("SGR mode change request too fast, ignored", 2);
        }
      }
      else{
        syslog("SGR mode change request rejected, SGR control permanently disabled", 2);
      }
    }
    else{
      syslog("SGR mode change request rejected, SGR control temporarily disabled", 2);
    }
    sgrPrevMode = sgrMode;
  }
  /*If direct control of the SGR outputs is requested,
   * check if it is allowed, and switch outputs.
   */
  if(sgr_chan1 != sgr_chan1_prev){
    if(sgr_dir){
      String reply = "{\"value\": \"";
      if(sgr_chan1 == true){
        if(dongle_type == "Plan-D"){
          setSGRchan(1, 19, 1);
          //digitalWrite(19, HIGH);
        }
        else{
          setSGRchan(1, 32, 1);
          //digitalWrite(32, HIGH);
        }
        syslog("SGR channel 1 set to ON", 0);
        reply += "on";
      }
      else{
        if(dongle_type == "Plan-D"){
          setSGRchan(1, 19, 0);
          //digitalWrite(19, LOW);
        }
        else{
          setSGRchan(1, 32, 0);
          //digitalWrite(32, LOW);
        }
        syslog("SGR channel 1 set to OFF", 0);
        reply += "off";
      }
      reply += "\"}";
      pubMqtt("data/devices/heatpump/sgr_chan_1", reply, false);
    }
    else{
      syslog("SGR channel control ignored, direct control disabled", 2);
    }
    sgr_chan1_prev = sgr_chan1;
  }
  if(sgr_chan2 != sgr_chan2_prev){
    if(sgr_dir){
      String reply = "{\"value\": \"";
      if(sgr_chan2 == true){
        if(dongle_type == "Plan-D"){
          digitalWrite(22, HIGH);
        }
        else{
          digitalWrite(26, HIGH);
        }
        syslog("SGR channel 2 set to ON", 0);
        reply += "on";
      }
      else{
        if(dongle_type == "Plan-D"){
          digitalWrite(22, LOW);
        }
        else{
          digitalWrite(26, LOW);
        }
        syslog("SGR channel 2 set to OFF", 0);
        reply += "off";
      }
      reply += "\"}";
      pubMqtt("data/devices/heatpump/sgr_chan_2", reply, false);
    }
    else{
      syslog("SGR channel control ignored, direct control disabled", 2);
    }
    sgr_chan2_prev = sgr_chan2;
  }
    
  /*If energy meter emulation is enabled, emulate meter pulses*/
  if(em_en){
    //this needs to move into interrupts
    if(prevPower != remotePower){
      syslog("Meter power set to " + String(remotePower), 0);
      Serial.print("Setting energy meter emulation power to ");
      Serial.print(remotePower);
      if(powerMultiplier == 1000) Serial.println("W");
      else Serial.println("kW");
      if(remotePower > 0) pulsePeriod = (3600000/((remotePower/powerMultiplier)*pulseMultiplier))-pulseOnWidth;
      else pulsePeriod = 0;
    }
    prevPower = remotePower;
    if(pulsePeriod >= pulseOnWidth + pulseOffWidth){
      switch (pulseState){
        case 0:
          if(pulseTime >= pulsePeriod){
            if(pulseInversePolarity) digitalWrite(19, HIGH);
            else digitalWrite(23, LOW);
            pulseState = 1;
            pulseTime = 0;
          }
          break;
        case 1:
          if(pulseTime >= pulseOnWidth){
            if(pulseInversePolarity) digitalWrite(19, LOW);
            else digitalWrite(23, HIGH);
            pulseState = 0;
            pulseTime = 0;
          }
          break;
      }
    }
  }
  /*ESPaltherma*/
  if(data_altherma && altherma_mqtt){
    if(waiter >= waitTime){
      //Querying all registries
      if (activeReg < 32 && registryIDs[activeReg] != 0xFF)
      {
        if(reStart){
          Serial.println("Querying Altherma registers");
          reStart = false;
        }
        char buff[64] = {0};
        //int tries = 0;
        if (!queryRegistry(registryIDs[activeReg], buff) && tries++ < 3)
        {
          waitTime = 1000;
        }
        if (registryIDs[activeReg] == buff[1]) //if replied registerID is coherent with the command
        {
          converter.readRegistryValues(buff); //process all values from the register
          updateValues(registryIDs[activeReg]);       //send them in mqtt
          activeReg++;
          tries = 0;
          waitTime = 500;
        }
        if(tries >= 3){
          Serial.print("Time-out on register 0x");
          Serial.print(registryIDs[activeReg], HEX);
          Serial.println(", skipping");
          activeReg++;
          tries = 0;
        }
        if(activeReg >= 32) activeReg = 0;
      }
      if(registryIDs[activeReg] == 0xFF){
        Serial.printf("Done. Waiting %d sec...\n", 30000 / 1000);
        activeReg = 0;
        reStart = true;
        waitTime = 30000;
      }
      waiter = 0;
    }
  }
}

void setSGRchan(int sgrChan, int pinNumber, int sgrValue){
  /*Serial.print("Received command ");
  Serial.print(sgrValue);
  Serial.print(" for chan ");
  Serial.print(sgrChan);
  Serial.print(" on pin ");
  Serial.println(pinNumber);*/
  digitalWrite(pinNumber, sgrValue);
  if(sgrChan == 1){
    if (sgrValue == 1) sgr_chan1 = true;
    else sgr_chan1 = false;
  }
  else if(sgrChan == 2){
    if (sgrValue == 1) sgr_chan2 = true;
    else sgr_chan2 = false;
  }
  String reply = "{\"value\": \"";
  if(sgrValue == 1) reply += "on";
  else reply += "off";
  reply += "\"}";
  String topic = "data/devices/heatpump/";
  if(sgrChan == 1) topic += "sgr_chan_1";
  else topic += "sgr_chan_2";
  pubMqtt(topic, reply, false);
}
