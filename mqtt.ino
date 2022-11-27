  void setupMqtt() {
  String mqttinfo = "MQTT enabled! Will connect as " + mqtt_id;
  if (mqtt_auth) {
    mqttinfo = mqttinfo + " using authentication, with username " + mqtt_user;
  }
  syslog(mqttinfo, 0);
  if(mqtt_tls) mqttclientSecure.setClient(*client);
  else mqttclient.setClient(wificlient);
  /*Set broker location*/
  IPAddress addr;
  if (mqtt_host.length() > 0) {
    if (addr.fromString(mqtt_host)) {
      syslog("MQTT host has IP address " + mqtt_host, 0);
      if(mqtt_tls) mqttclientSecure.setServer(addr, mqtt_port);
      else{
        mqttclient.setServer(addr, mqtt_port);
        mqttclient.setCallback(mqttCallback);
      }
    }
    else {
      syslog("Trying to resolve MQTT host " + mqtt_host + " to IP address", 0);
      int dotLoc = mqtt_host.lastIndexOf('.');
      String tld = mqtt_host.substring(dotLoc+1);
      if(dotLoc == -1 || tld == "local"){
        if(tld == "local") mqtt_host = mqtt_host.substring(0, dotLoc);
        int mdnsretry = 0;
        while (!WiFi.hostByName(mqtt_host.c_str(), addr) && mdnsretry < 5) {
          Serial.print("...");
          mdnsretry++;
          delay(250);
        }
        while (addr.toString() == "0.0.0.0" && mdnsretry < 10) {
          Serial.print("...");
          addr = MDNS.queryHost(mqtt_host);
          mdnsretry++;
          delay(250);
        }
        if(mdnsretry < 10){
          syslog("MQTT host has IP address " + addr.toString(), 0);
          if(mqtt_tls) {
            mqttclientSecure.setServer(addr, mqtt_port);
            mqttclientSecure.setCallback(mqttCallback);
          }
          else {
            mqttclient.setServer(addr, mqtt_port);
            mqttclient.setCallback(mqttCallback);
          }
        }
        else{
          syslog("MQTT host IP resolving failed", 3);
          mqttHostError = true;
        } 
      }
      else{
        if(mqtt_tls){
          mqttclientSecure.setServer(mqtt_host.c_str(), mqtt_port);
          mqttclientSecure.setCallback(mqttCallback);
        }
        else{
          mqttclient.setServer(mqtt_host.c_str(), mqtt_port);
          mqttclient.setCallback(mqttCallback);
        }
      }
    }
  }
}

void connectMqtt() {
  if(!mqttHostError){
    // Loop until we're (re)connected
    int mqttretry = 0;
    bool disconnected = false;
    if(mqtt_tls && !clientSecureBusy){
      if(!mqttclientSecure.connected()) {
        disconnected = true;
        if(mqttWasConnected) syslog("Lost connection to secure MQTT broker", 2);
        syslog("Trying to connect to secure MQTT broker", 0);
        while(!mqttclientSecure.connected() && mqttretry < 2){
          Serial.print("...");
          if (mqtt_auth) mqttclientSecure.connect(mqtt_id.c_str(), mqtt_user.c_str(), mqtt_pass.c_str());
          else mqttclientSecure.connect(mqtt_id.c_str());
          mqttretry++;
          reconncount++;
          delay(250);
        }
        Serial.println("");
      }
    }
    else{
      if(!mqttclient.connected()) {
        disconnected = true;
        if(mqttWasConnected) syslog("Lost connection to MQTT broker", 2);
        syslog("Trying to connect to MQTT broker", 0);
        while(!mqttclient.connected() && mqttretry < 2){
          Serial.print("...");
          if (mqtt_auth) mqttclient.connect(mqtt_id.c_str(), mqtt_user.c_str(), mqtt_pass.c_str(), "data/devices/heatpump", 1, true, "offline");
          else mqttclient.connect(mqtt_id.c_str(), "data/devices/heatpump", 1, true, "offline");
          mqttretry++;
          reconncount++;
          delay(250);
        }
        Serial.println("");
      }
    }
    if(disconnected){
      if(mqttretry < 2){
        syslog("Connected to MQTT broker", 0);
        if(mqtt_tls){
          mqttclientSecure.publish("data/devices/heatpump", "online", true);
          mqttclientSecure.subscribe("set/devices/heatpump/reboot");
          mqttclientSecure.subscribe(sgr_en_topic.c_str());
          mqttclientSecure.subscribe(sgr_allow_topic.c_str());
          mqttclientSecure.subscribe(sgr_post_topic.c_str());
          if(sgr_dir){
            mqttclientSecure.subscribe(sgr_chan1_topic.c_str());
            mqttclientSecure.subscribe(sgr_chan2_topic.c_str());
          }
          if(em_en && em_topic !=""){
            mqttclientSecure.subscribe(em_topic.c_str());
          }
          if(sgr_topic !=""){
            mqttclientSecure.subscribe(sgr_topic.c_str());
          }
        }
        else{
          mqttclient.publish("data/devices/heatpump", "online", true);
          mqttclient.subscribe("set/devices/heatpump/reboot");
          mqttclient.subscribe(sgr_en_topic.c_str());
          mqttclient.subscribe(sgr_allow_topic.c_str());
          mqttclient.subscribe(sgr_post_topic.c_str());
          if(sgr_dir){
            mqttclient.subscribe(sgr_chan1_topic.c_str());
            mqttclient.subscribe(sgr_chan2_topic.c_str());
          }
          if(em_en && em_topic !=""){
            mqttclient.subscribe(em_topic.c_str());
          }
          if(sgr_topic !=""){
            mqttclient.subscribe(sgr_topic.c_str());
          }
        }
        mqttClientError = false;
        mqttWasConnected = true;
      }
      else{
        syslog("Failed to connect to MQTT broker", 3);
        mqttClientError = true;
      }
    }
  }
  else{
    //setupMqtt();
  }
}

void pubMqtt(String topic, String payload, boolean retain){
  if(mqtt_en && !mqttClientError){
    if(mqtt_tls){
      if(mqttclientSecure.connected()){
        mqttclientSecure.publish(topic.c_str(), payload.c_str(), retain);
      }
    }
    else{
      if(mqttclient.connected()){
        mqttclient.publish(topic.c_str(), payload.c_str(), retain);
      }
    }
  }
}

void haAutoDiscovery(boolean eraseMeter){
  if(ha_en && mqtt_en && !mqttClientError){
    if(!ha_metercreated) syslog("Performing Home Assistant MQTT autodiscovery", 0);
    /*Create the basic control widget*/
    for(int i = 0; i < 2; i++){
      String chanName = "";
      String configTopic = "";
      DynamicJsonDocument doc(1024);
      if(i == 0){
        chanName = "heatpump_allow_sgr_control";
        doc["name"] = "Allow SGR control";
        doc["icon"] = "mdi:pump";
        doc["state_topic"] = "data/devices/heatpump/allow_sgr_control";
        doc["payload_on"] = "{\"value\": \"on\"}";
        doc["payload_off"] = "{\"value\": \"off\"}";
        doc["command_topic"] = "set/devices/heatpump/allow_sgr_control";
        configTopic = "homeassistant/switch/" + chanName + "/config";
      }
      else if(i == 1){
        chanName = "heatpump_current_SGR_state";
        doc["name"] = "Current SGR state";
        doc["state_topic"] = "data/devices/heatpump/sgr_state_verbose";
        doc["value_template"] = "{{ value_json.value }}";
        configTopic = "homeassistant/sensor/" + chanName + "/config";
      }
      else{
        chanName = "";
      }
      doc["unique_id"] = chanName;
      doc["availability_topic"] = "data/devices/heatpump";
      JsonObject device  = doc.createNestedObject("device");
      JsonArray identifiers = device.createNestedArray("identifiers");
      identifiers.add("SGR Heatpump control");
      device["name"] = "SGR Heatpump control";
      device["model"] = "SGR dongle for SGR compatible heat pumps";
      device["manufacturer"] = "plan-d.io";
      device["configuration_url"] = "http://" + WiFi.localIP().toString();
      device["sw_version"] = String(fw_ver/100.0);
      String jsonOutput ="";
      //Ensure devices are erased before created again
      if(eraseMeter){
        if(chanName.length() > 0) pubMqtt(configTopic, jsonOutput, true);
        delay(100);
      }
      serializeJson(doc, jsonOutput);
      if(chanName.length() > 0) {
        pubMqtt(configTopic, jsonOutput, true);
      }
    }
    /*Create the advanced control widget*/
    for(int i = 0; i < 4; i++){
      String chanName = "";
      String configTopic = "";
      DynamicJsonDocument doc(1024);
      if(i == 0){
        chanName = "heatpump_SGR_postpone";
        doc["name"] = "SGR postpone";
        doc["icon"] = "mdi:timer-cog-outline";
        doc["unit_of_measurement"] = "hours";
        doc["min"] = 0;
        doc["max"] = 48;
        doc["step"] = 1;
        doc["state_topic"] = "data/devices/heatpump/sgr_postpone";
        doc["value_template"] = "{{ value_json.value }}";
        doc["command_topic"] = "set/devices/heatpump/sgr_postpone";
        doc["command_template"] = "{\"value\": {{ value }} }";
        configTopic = "homeassistant/number/" + chanName + "/config";
      }
      else if(i == 1){
        chanName = "heatpump_enable_sgr_control";
        doc["name"] = "Enable SGR control";
        doc["state_topic"] = "data/devices/heatpump/enable_sgr_control";
        doc["payload_on"] = "{\"value\": \"on\"}";
        doc["payload_off"] = "{\"value\": \"off\"}";
        doc["command_topic"] = "set/devices/heatpump/enable_sgr_control";
        configTopic = "homeassistant/switch/" + chanName + "/config";
      }
      else if(i == 2){
        chanName = "heatpump_sgr_chan1";
        doc["name"] = "SGR channel 1";
        doc["state_topic"] = "data/devices/heatpump/sgr_chan_1";
        doc["payload_on"] = "{\"value\": \"on\"}";
        doc["payload_off"] = "{\"value\": \"off\"}";
        doc["command_topic"] = "set/devices/heatpump/sgr_chan_1";
        configTopic = "homeassistant/switch/" + chanName + "/config";
      }
      else if(i == 3){
        chanName = "heatpump_sgr_chan2";
        doc["name"] = "SGR channel 2";
        doc["state_topic"] = "data/devices/heatpump/sgr_chan_2";
        doc["payload_on"] = "{\"value\": \"on\"}";
        doc["payload_off"] = "{\"value\": \"off\"}";
        doc["command_topic"] = "set/devices/heatpump/sgr_chan_2";
        configTopic = "homeassistant/switch/" + chanName + "/config";
      }
      else{
        chanName = "";
      }
      doc["unique_id"] = chanName;
      doc["availability_topic"] = "data/devices/heatpump";
      JsonObject device  = doc.createNestedObject("device");
      JsonArray identifiers = device.createNestedArray("identifiers");
      identifiers.add("SGR_heatpump advanced control");
      device["name"] = "SGR Heatpump advanced control";
      device["model"] = "SGR dongle for SGR compatible heat pumps - advanced";
      device["manufacturer"] = "plan-d.io";
      device["configuration_url"] = "http://" + WiFi.localIP().toString();
      device["sw_version"] = String(fw_ver/100.0);
      String jsonOutput ="";
      //Ensure devices are erased before created again
      if(eraseMeter){
        if(chanName.length() > 0) pubMqtt(configTopic, jsonOutput, true);
        delay(100);
      }
      serializeJson(doc, jsonOutput);
      if(chanName.length() > 0){
        pubMqtt(configTopic, jsonOutput, true);
        if(eraseMeter) delay(100);
      }
    }
    /*Create the debug information widget*/
    if(debugInfo){
      firstDebugPush = true;
      for(int i = 0; i < 10; i++){
        String chanName = "";
        DynamicJsonDocument doc(1024);
        if(i == 0){
          chanName = String(apSSID) + "_reboots";
          doc["name"] = String(apSSID ) + " Reboots";
          doc["state_topic"] = "sys/devices/" + String(apSSID) + "/reboots";
        }
        else if(i == 1){
          chanName = String(apSSID) + "_last_reset_reason";
          doc["name"] = String(apSSID ) + " Last reset reason";
          doc["state_topic"] = "sys/devices/" + String(apSSID) + "/last_reset_reason";
        }
        else if(i == 2){
          chanName = String(apSSID) + "_free_heap_size";
          doc["name"] = String(apSSID ) + " Free heap size";
          doc["unit_of_measurement"] = "kB";
          doc["state_topic"] = "sys/devices/" + String(apSSID) + "/free_heap_size";
        }
        else if(i == 3){
          chanName = String(apSSID) + "_max_allocatable_block";
          doc["name"] = String(apSSID ) + " Allocatable block size";
          doc["unit_of_measurement"] = "kB";
          doc["state_topic"] = "sys/devices/" + String(apSSID) + "/max_allocatable_block";
        }
        else if(i == 4){
          chanName = String(apSSID) + "_min_free_heap";
          doc["name"] = String(apSSID ) + " Lowest free heap size";
          doc["unit_of_measurement"] = "kB";
          doc["state_topic"] = "sys/devices/" + String(apSSID) + "/min_free_heap";
        }
        else if(i == 5){
          chanName = String(apSSID) + "_last_reset_reason_verbose";
          doc["name"] = String(apSSID ) + " Last reset reason (verbose)";
          doc["state_topic"] = "sys/devices/" + String(apSSID) + "/last_reset_reason_verbose";
        }
        else if(i == 6){
          chanName = String(apSSID) + "_syslog";
          doc["name"] = String(apSSID ) + " Syslog";
          doc["state_topic"] = "sys/devices/" + String(apSSID) + "/syslog";
        }
        else if(i == 7){
          chanName = String(apSSID) + "_ip";
          doc["name"] = String(apSSID ) + " IP";
          doc["state_topic"] = "sys/devices/" + String(apSSID) + "/ip";
        }
        else if(i == 8){
          chanName = String(apSSID) + "_firmware";
          doc["name"] = String(apSSID ) + " firmware";
          doc["state_topic"] = "sys/devices/" + String(apSSID) + "/firmware";
        }
        else if(i == 9){
          chanName = String(apSSID) + "_reboot";
          doc["name"] = String(apSSID ) + " Reboot";
          doc["payload_on"] = "{\"value\": \"true\"}";
          doc["payload_off"] = "{\"value\": \"false\"}";
          doc["command_topic"] = "set/devices/heatpump/reboot";
        }
        doc["unique_id"] = chanName;
        doc["value_template"] = "{{ value_json.value }}";
        JsonObject device  = doc.createNestedObject("device");
        JsonArray identifiers = device.createNestedArray("identifiers");
        identifiers.add("SGR_dongle");
        device["name"] = apSSID;
        device["model"] = "SGR dongle debug monitoring";
        device["manufacturer"] = "plan-d.io";
        device["configuration_url"] = "http://" + WiFi.localIP().toString();
        device["sw_version"] = String(fw_ver/100.0);
        String configTopic = "";
        if(i == 9) configTopic = "homeassistant/switch/" + chanName + "/config";
        else configTopic = "homeassistant/sensor/" + chanName + "/config";
        String jsonOutput ="";
        //Ensure devices are erased before created again
        if(eraseMeter){
          if(chanName.length() > 0) pubMqtt(configTopic, jsonOutput, true);
          delay(100);
        }
        serializeJson(doc, jsonOutput);
        if(chanName.length() > 0) if(chanName.length() > 0) pubMqtt(configTopic, jsonOutput, true);
      }
    }
    if(!ha_metercreated) Serial.println("Done");
    ha_metercreated = true;
  }
}

void initMqttValues(){
  //syslog("Initialising MQTT topics", 0);
  String reply = "{\"value\": \"";
  if(sgrMode == 0) reply += "Normal operation";
  else if(sgrMode == 1) reply += "Advise on";
  else if(sgrMode == 2) reply += "Block";
  else if(sgrMode == 3) reply += "Forced on";
  else reply += "Normal operation";
  reply += "\"}";
  pubMqtt("data/devices/heatpump/sgr_state_verbose", reply, false);
  reply = "{\"value\": \"";
  if(sgr_all) reply += "on";
  else reply += "off";
  reply += "\"}";
  pubMqtt("data/devices/heatpump/allow_sgr_control", reply, false);
  reply = "{\"value\": \"";
  if(sgr_en) reply += "on";
  else reply += "off";
  reply += "\"}";
  pubMqtt("data/devices/heatpump/enable_sgr_control", reply, false);
  reply = "{\"value\": \"";
  reply += sgr_post;
  reply += "\"}";
  pubMqtt("data/devices/heatpump/sgr_postpone", reply, false);
  reply = "{\"value\": \"";
  if(sgr_chan1 == 0) reply += "off";
  else reply += "on";
  reply += "\"}";
  pubMqtt("data/devices/heatpump/sgr_chan_1", reply, false);
  reply = "{\"value\": \"";
  if(sgr_chan2 == 0) reply += "off";
  else reply += "on";
  reply += "\"}";
  pubMqtt("data/devices/heatpump/sgr_chan_2", reply, false);
}

void processCallback() {
  time_t now;
  unsigned long dtimestamp = time(&now);
  Serial.print("got mqtt message on ");
  Serial.println(mqttTopic);
  Serial.println(mqttPayload);
  if (mqttTopic == "set/devices/heatpump/reboot") {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, mqttPayload);
    if(doc["value"] == "true"){
      last_reset = "Reboot requested by MQTT";
      if(saveConfig()){
        syslog("Reboot requested from MQTT", 2);
        setReboot();
      }
    }
  }
  if (mqttTopic == sgr_topic) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, mqttPayload);
    sgrMode = int(doc["value"]);
  }
  if (mqttTopic == em_topic) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, mqttPayload);
    remotePower = float(doc["value"]);
  }
  if (mqttTopic == sgr_en_topic) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, mqttPayload);
    String reply = "{\"value\": \"";
    if(doc["value"] == "off"){
      sgr_en = false;
      syslog("SGR control disabled by user", 0);
      reply += "off";
    }
    if(doc["value"] == "on"){
      sgr_en = true;
      syslog("SGR control (re-)enabled by user", 0);
      reply += "on";
    }
    reply += "\",\"timestamp\": ";
    reply += dtimestamp;
    reply += "}";
    pubMqtt("data/devices/heatpump/enable_sgr_control", reply, false);
  }
  if (mqttTopic == sgr_allow_topic) {
    Serial.println("Received allow");
    StaticJsonDocument<200> doc;
    deserializeJson(doc, mqttPayload);
    String reply = "{\"value\": \"";
    if(doc["value"] == "off"){
      sgr_all = false;
      syslog("SGR control temporarily disabled by user", 0);
      reply += "off";
    }
    if(doc["value"] == "on"){
      sgr_all = true;
      syslog("SGR control (re-)enabled by user", 0);
      reply += "on";
    }
    reply += "\"}";
    pubMqtt("data/devices/heatpump/allow_sgr_control", reply, false);
  }
  if (mqttTopic == sgr_post_topic) {
    Serial.println("SGR post ");
    StaticJsonDocument<200> doc;
    deserializeJson(doc, mqttPayload);
    sgr_post = int(doc["value"]);
    syslog("SGR control reenable delay set to " + String(sgr_post), 0);
  }
  if (mqttTopic == sgr_chan1_topic) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, mqttPayload);
    if(doc["value"] == "off"){
      sgr_chan1 = false;
    }
    if(doc["value"] == "on"){
      sgr_chan1 = true;
    }
  }
  if (mqttTopic == sgr_chan2_topic) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, mqttPayload);
    if(doc["value"] == "off"){
      sgr_chan2 = false;
    }
    if(doc["value"] == "on"){
      sgr_chan2 = true;
    }
  }
  mqttReceived = false;
}
