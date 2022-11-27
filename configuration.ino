boolean restoreConfig() {
  wifi_ssid = preferences.getString("WIFI_SSID");
  wifi_password = preferences.getString("WIFI_PASSWD");  
  wifiSTA = preferences.getBool("WIFI_STA");
  mqtt_en = preferences.getBool("MQTT_EN");
  mqtt_tls = preferences.getBool("MQTT_TLS");
  mqtt_host = preferences.getString("MQTT_HOST");
  mqtt_id = preferences.getString("MQTT_ID");
  mqtt_auth = preferences.getBool("MQTT_AUTH");
  mqtt_user = preferences.getString("MQTT_USER");
  mqtt_pass = preferences.getString("MQTT_PASS");
  mqtt_port = preferences.getUInt("MQTT_PORT");
  upload_throttle = preferences.getULong("UPL_THROTTLE");
  update_auto = preferences.getBool("UPD_AUTO");
  update_autoCheck = preferences.getBool("UPD_AUTOCHK");
  fw_new = preferences.getUInt("FW_NEW");
  update_start = preferences.getBool("UPD_START");
  update_finish = preferences.getBool("UPD_FINISH");
  eid_en = preferences.getBool("EID_EN");
  eid_webhook = preferences.getString("EID_HOOK");
  ha_en = preferences.getBool("HA_EN");
  counter = preferences.getUInt("counter", 0);
  bootcount = preferences.getUInt("reboots", 0);
  last_reset = preferences.getString("LAST_RESET");
  if(mqtt_en) mqttSave = true;
  if(eid_en) eidSave = true;
  if(ha_en) haSave = true;
  /*SGR dongle*/
  em_en = preferences.getBool("EM_EN");
  em_topic = preferences.getString("EM_TOPIC");
  sgr_topic = preferences.getString("SGR_TOPIC");
  em_cofy = preferences.getBool("EM_COFY");
  pulseMultiplier = preferences.getInt("EM_PULSES");
  powerMultiplier = preferences.getInt("EM_MULTI");
  pulseInversePolarity = preferences.getBool("EM_REVPOL");
  pulseOnWidth = preferences.getInt("EM_MINONPULSE");
  pulseOffWidth = preferences.getInt("EM_MINOFFPULSE");
  sgr_type = preferences.getString("SGR_TYPE");
  sgr_dir = preferences.getBool("SGR_DIR");
  sgr_lim = preferences.getInt("SGR_LIM");
  data_altherma = preferences.getBool("DATA_ALT");
  dongle_type = preferences.getString("DONGLE_TYPE");
  external_temperature = preferences.getBool("DONGLE_EXTEMP");
  if(em_en) emSave = true;
  return true;
}

boolean saveConfig() {
  preferences.putString("WIFI_SSID", wifi_ssid);
  preferences.putString("WIFI_PASSWD", wifi_password);
  if(wifiSave) preferences.putBool("WIFI_STA", true);
  else preferences.putBool("WIFI_STA", wifiSTA);
  preferences.putBool("MQTT_EN", mqttSave);
  preferences.putBool("MQTT_TLS", mqtt_tls); 
  preferences.putString("MQTT_HOST", mqtt_host);
  preferences.putString("MQTT_ID", mqtt_id);
  preferences.putBool("MQTT_AUTH", mqtt_auth); 
  preferences.putString("MQTT_USER", mqtt_user);
  preferences.putString("MQTT_PASS", mqtt_pass);
  preferences.putUInt("MQTT_PORT", mqtt_port);
  preferences.putULong("UPL_THROTTLE", upload_throttle);
  preferences.putBool("UPD_AUTO", update_auto);
  preferences.putBool("UPD_AUTOCHK", update_autoCheck);
  preferences.putUInt("FW_NEW", onlineVersion);
  preferences.putBool("UPD_START", update_start);
  preferences.putBool("UPD_FINISH", update_finish);
  preferences.putUInt("counter", counter);
  preferences.putUInt("reboots", bootcount);
  preferences.putBool("EID_EN", eidSave);
  preferences.putString("EID_HOOK", eid_webhook);
  preferences.putString("LAST_RESET", last_reset);
  preferences.putBool("HA_EN", haSave);
  /*SGR dongle*/
  preferences.putBool("EM_EN", emSave);
  preferences.putString("EM_TOPIC", em_topic);
  preferences.putString("SGR_TOPIC", sgr_topic);
  preferences.putBool("EM_COFY", em_cofy);
  preferences.putInt("EM_PULSES", pulseMultiplier);
  preferences.putInt("EM_MULTI", powerMultiplier);
  preferences.putBool("EM_REVPOL", pulseInversePolarity);
  preferences.putInt("EM_MINONPULSE", pulseOnWidth);
  preferences.putInt("EM_MINOFFPULSE", pulseOffWidth);
  preferences.putInt("SGR_LIM", sgr_lim);
  preferences.putString("SGR_TYPE", sgr_type);
  preferences.putBool("SGR_DIR", sgr_dir);
  preferences.putBool("DATA_ALT", data_altherma);
  preferences.putString("DONGLE_TYPE", dongle_type);
  preferences.putBool("DONGLE_EXTEMP", external_temperature);
  return true;
}

boolean saveBoots(){
  preferences.putUInt("reboots", bootcount);
  return true;
}

boolean resetConfig() {
  if(resetAll || resetWifi){
    Serial.print("Executing config reset");
    Serial.print("Executing wifi reset");
    preferences.remove("WIFI_SSID");
    preferences.remove("WIFI_PASSWD");
    preferences.remove("MQTT_HOST");
    preferences.putBool("WIFI_STA", false);
    preferences.putBool("UPD_AUTO", true);
    preferences.putBool("UPD_AUTOCHK", true);
    preferences.putString("LAST_RESET", "Restarting for config reset");
    preferences.end();
    syslog("Restarting for config reset", 2);
    delay(500);
    ESP.restart();
  }
  return true;
}

boolean initConfig() {
  String tempMQTT = preferences.getString("MQTT_HOST");
  if(tempMQTT == ""){
    preferences.putString("MQTT_HOST", "10.42.0.1");
    mqtt_host = "10.42.0.1";
    preferences.putUInt("MQTT_PORT", 1883);
    mqtt_port = 1883;
    tempMQTT = preferences.getString("MQTT_ID");
    if(tempMQTT == ""){
      preferences.putString("MQTT_ID", apSSID);
      mqtt_id = apSSID;
      preferences.putBool("HA_EN", true);
      ha_en = true;
    }
    preferences.putBool("UPD_AUTO", true); 
    preferences.putBool("UPD_AUTOCHK", true);
    /*SGR dongle*/
    preferences.putBool("EM_EN", false);
    preferences.putString("SGR_TOPIC", "set/devices/heatpump/sgr_mode");
    preferences.putString("EM_TOPIC", "data/devices/utility_meter/active_power_injection");
    preferences.putBool("EM_COFY", true);
    preferences.putInt("EM_MULTI", 1);
    preferences.putInt("EM_PULSES", 1000);
    preferences.putBool("EM_REVPOL", false);
    preferences.putInt("EM_MINONPULSE", 20);
    preferences.putInt("EM_MINOFFPULSE", 110);
    preferences.putInt("SGR_LIM", 30);
    preferences.putString("SGR_TYPE", "Normal");
    preferences.putBool("SGR_DIR", true);
    preferences.putString("DONGLE_TYPE", "OEM");
    preferences.putBool("DONGLE_EXTEMP", false);
  }
  unsigned int tempMQTTint = preferences.getUInt("MQTT_PORT");
  if(tempMQTTint == 0){
    preferences.putUInt("MQTT_PORT", 1883);
    mqtt_port = 1883;
  }
  return true;
}
