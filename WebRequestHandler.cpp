#include "WebRequestHandler.h"

bool WebRequestHandler::canHandle(AsyncWebServerRequest *request)
{
  return true;
}

void WebRequestHandler::handleRequest(AsyncWebServerRequest *request)
{
  extern String ssidList, wifi_ssid, wifi_password, mqtt_host, mqtt_id, mqtt_user, mqtt_pass, eid_webhook, last_reset, em_topic, sgr_type, dongle_type;
  extern int counter, mqtt_port, powerMultiplier, pulseMultiplier, sgr_lim;
  extern unsigned long upload_throttle, pulseOnWidth, pulseOffWidth;;
  extern char apSSID[];
  extern boolean wifiError, mqttHostError, mqttClientError, httpsError, wifiSTA, wifiSave, configSaved, rebootReq, rebootInit, mqttSave, mqtt_en, mqtt_auth, mqtt_tls, updateAvailable, update_start, mTimeFound, meterError, eid_en, eidSave, eidError, ha_en, haSave, em_en, emSave, em_cofy, pulseInversePolarity, data_altherma, sgr_dir, external_temperature;
  if(request->url() == "/"){
    counter++;
    request->send(SPIFFS, "/index.html", "text/html");
  }
  else if(request->url() == "/hostname"){
    request->send(200, "text/plain", apSSID);
  }
  else if(request->url() == "/wifi"){
    if(WiFi.status() == WL_CONNECTED){
      int wifiStrenght = abs(WiFi.RSSI());
      if(wifiStrenght <= 45) request->send(SPIFFS, "/wifi-strength-4.png", "image/png");
      else if(wifiStrenght > 45 && wifiStrenght <= 55) request->send(SPIFFS, "/wifi-strength-3.png", "image/png");
      else if(wifiStrenght > 55 && wifiStrenght <= 65) request->send(SPIFFS, "/wifi-strength-2.png", "image/png");
      else if(wifiStrenght > 65 && wifiStrenght <= 75) request->send(SPIFFS, "/wifi-strength-1.png", "image/png");
      else request->send(SPIFFS, "/wifi-strength-outline.png", "image/png");
    }
    else request->send(SPIFFS, "/wifi-strength-off-outline.png", "image/png");

  }
  else if(request->url() == "/cloud"){
  if(mqtt_en){
    if(wifiSTA && !mqttHostError && !mqttClientError && !httpsError) request->send(SPIFFS, "/cloud-outline.png", "image/png");
    else request->send(SPIFFS, "/cloud-alert.png", "image/png");
  }
  else request->send(SPIFFS, "/cloud-off-outline.png", "image/png");
    
  }
  else if(request->url() == "/meter"){
    if(mTimeFound) request->send(SPIFFS, "/counter.png", "image/png");
    else if (mTimeFound && meterError) request->send(SPIFFS, "/counter-warning.png", "image/png");
    else request->send(SPIFFS, "/counter-off.png", "image/png");
    
  }
  else if(request->url() == "/sensor"){
    request->send(SPIFFS, "/connection-off.png", "image/png");
  }
  else if(request->url() == "/eye"){
    request->send(SPIFFS, "/eye.png", "image/png");
  }
  else if(request->url() == "/config"){
    request->send(SPIFFS, "/config.html", "text/html");
  }
  else if(request->url() == "/wificn"){
    configSaved = false;
    if(request->hasParam("rescan"))
      scanWifi();
    request->send(SPIFFS, "/wifi.html", "text/html");
  }
  else if(request->url() == "/cloudcn"){
    configSaved = false;
    request->send(SPIFFS, "/cloud.html", "text/html");
  }
  else if(request->url() == "/setap"){
    String temp_wifi_ssid, temp_pass;
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
      } else {
        if(p->name() == "ssid"){
          temp_wifi_ssid = p->value();
        }
        else if(p->name() == "pass"){
          temp_pass = p->value();
        }
      }
    }
    if(temp_wifi_ssid.length() > 0){
      wifi_ssid = temp_wifi_ssid;  
      wifi_password = temp_pass;
      wifiSave = true;
      if(saveConfig()){
        //syslog("Saved wifi config", 0);
        configSaved = true;
        rebootReq = true;
      }
      //scanWifi();    
    }
    request->send(SPIFFS, "/wifi.html", "text/html");
  }
  else if(request->url() == "/setcloud"){
    configSaved = false;
    int params = request->params();
    if(request->hasParam("mqtt_en")){
      AsyncWebParameter* p = request->getParam("mqtt_en");
      if(p->value() == "true") mqttSave = true;
    }
    else{
      mqtt_en = false;
      mqttSave = false;
    }
    if(request->hasParam("mqtt_auth")){
      AsyncWebParameter* p = request->getParam("mqtt_auth");
      if(p->value() == "true") mqtt_auth = true;
    }
    else mqtt_auth = false;
    if(request->hasParam("mqtt_tls")){
      AsyncWebParameter* p = request->getParam("mqtt_tls");
      if(p->value() == "true") mqtt_tls = true;
    }
    else mqtt_tls = false;
    if(request->hasParam("eid_en")){
      AsyncWebParameter* p = request->getParam("eid_en");
      if(p->value() == "true") eidSave = true;
    }
    else{
      eid_en = false;
      eidSave = false;
    }
    if(request->hasParam("ha_en")){
      AsyncWebParameter* p = request->getParam("ha_en");
      if(p->value() == "true") haSave = true;
    }
    else{
      ha_en = false;
      haSave = false;
    }
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      //Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      if(p->name() == "mqtt_host"){
        mqtt_host = p->value();
      }
      else if(p->name() == "mqtt_port"){
        mqtt_port = p->value().toInt();
      }
      else if(p->name() == "mqtt_id"){
        mqtt_id = p->value();
      }
      else if(p->name() == "mqtt_user"){
        mqtt_user = p->value();
      }
      else if(p->name() == "mqtt_pass"){
        mqtt_pass = p->value();
      }
      else if(p->name() == "upload_throttle"){
        upload_throttle = atol(p->value().c_str());
      }
      else if(p->name() == "eid_webhook"){
        eid_webhook = p->value();
      }
    }
    if(saveConfig()){
      //syslog("Saved cloud config", 0);
      configSaved = true;
      rebootReq = true;
    }
    request->send(SPIFFS, "/cloud.html", "text/html");
  }
  else if(request->url() == "/ssidlist"){
    request->send(200, "text/plain", ssidList);
  }
  else if(request->url() == "/settingssaved"){
    String save;
    if(configSaved) save = "Settings saved";    
    request->send(200, "text/plain", save);
  }
  else if(request->url() == "/info"){
    String rbr;
    if(wifiError) rbr = rbr + "Could not connect to<br>" + wifi_ssid + "<br>";
    if(rebootReq) rbr = rbr + "Please restart the device<br>to have changes take effect" + "<br>";
    if(updateAvailable) rbr = rbr + "Firmware upgrade available!<br>";
    if(httpsError) rbr = rbr + "Could not initiate TLS/SSL client<br>";
    if(mqttHostError) rbr = rbr + "Could not resolve MQTT broker<br> " + mqtt_host + ":<br>invalid IP/hostname<br>";
    if(mqttClientError) rbr = rbr + "Could not connect to MQTT broker,<br>" + "please check credentials<br>";
    if(eidError) rbr = rbr + "Could not connect to EnergieID,<br>" + "please check credentials<br>";
    //need to reform this to a 'information' endpoint    
    request->send(200, "text/plain", rbr);
  }
  else if(request->url() == "/configData"){
    request->send(200, "text/plain", getConfig());
  }
  else if(request->url() == "/indexData"){
    request->send(200, "text/plain", getIndexData());
  }
  else if(request->url() == "/indexStatic"){
    request->send(200, "text/plain", getIndexStatic());
  }
  else if(request->url() == "/reboot"){
    last_reset = "Reboot requested by user from webmin";
    if(saveConfig()){
      syslog("Reboot requested by user from webmin", 2);
      setReboot();
      request->send(SPIFFS, "/reboot.html", "text/html");
    }
    else request->send(SPIFFS, "/index.html", "text/html");
  }
  else if(request->url() == "/upgrade"){
    if(updateAvailable){
      update_start = true;
      syslog("Restarting for update", 2);
      last_reset = "Restarting for update";
      if(saveConfig()){
        request->send(SPIFFS, "/index.html", "text/html");
        delay(200);
        ESP.restart();
      }
      else request->send(SPIFFS, "/index.html", "text/html");
    }
    else request->send(SPIFFS, "/index.html", "text/html");
  }
  else if(request->url() == "/syslog"){
    request->send(SPIFFS, "/syslog.txt", "text/plain");
  }
  else if(request->url() == "/syslog0"){
    request->send(SPIFFS, "/syslog0.txt", "text/plain");
  }
  else if(request->url() == "/scripts.js"){
    request->send(SPIFFS, "/scripts.js", "text/javascript");
  }
  else if(request->url() == "/style.css"){
    request->send(SPIFFS, "/style.css", "text/css");
  }

  else if(request->url() == "/unitData"){
    request->send(200, "text/plain", getUnit());
  }
  else if(request->url() == "/unit"){
    configSaved = false;
    request->send(SPIFFS, "/unit.html", "text/html");
  }
  /*SGR dongle*/
  else if(request->url() == "/setunit"){
    configSaved = false;
    int params = request->params();
    if(request->hasParam("sgr_type")){
      AsyncWebParameter* p = request->getParam("sgr_type");
      sgr_type = p->value();
    }
    if(request->hasParam("dongle_type")){
      AsyncWebParameter* p = request->getParam("dongle_type");
      dongle_type = p->value();
    }
    if(request->hasParam("em_en")){
      AsyncWebParameter* p = request->getParam("em_en");
      if(p->value() == "true") emSave = true;
    }
    else{
      em_en = false;
      emSave = false;
    }
    if(request->hasParam("em_cofy")){
      AsyncWebParameter* p = request->getParam("em_cofy");
      if(p->value() == "true") em_cofy = true;
    }
    else em_cofy = false;
    if(request->hasParam("em_unit")){
      AsyncWebParameter* p = request->getParam("em_unit");
      if(p->value() == "W") powerMultiplier = 1000;
    }
    else powerMultiplier = 1;
    if(request->hasParam("em_pol")){
      AsyncWebParameter* p = request->getParam("em_pol");
      if(p->value() == "true") pulseInversePolarity = true;
    }
    else pulseInversePolarity = false;
    if(request->hasParam("data_altherma")){
      AsyncWebParameter* p = request->getParam("data_altherma");
      if(p->value() == "true") data_altherma = true;
    }
    else data_altherma = false;
    if(request->hasParam("sgr_dir")){
      AsyncWebParameter* p = request->getParam("sgr_dir");
      if(p->value() == "true") sgr_dir = true;
    }
    else sgr_dir = false;
    if(request->hasParam("external_temperature")){
      AsyncWebParameter* p = request->getParam("external_temperature");
      if(p->value() == "true") external_temperature = true;
    }
    else external_temperature = false;
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      if(p->name() == "em_topic"){
        em_topic = p->value();
      }
      else if(p->name() == "em_pulses"){
        pulseMultiplier = p->value().toInt();
      }
      else if(p->name() == "em_pwon"){
        pulseOnWidth = p->value().toInt();
      }
      else if(p->name() == "em_pwoff"){
        pulseOffWidth = p->value().toInt();
      }
      else if(p->name() == "sgr_lim"){
        sgr_lim = p->value().toInt();
      }
    }
    if(saveConfig()){
      //syslog("Saved cloud config", 0);
      configSaved = true;
      rebootReq = true;
    }
    request->send(SPIFFS, "/unit.html", "text/html");
  }
  else{
    request->send(SPIFFS, "/index.html", "text/html");
    counter++;
  }
}
