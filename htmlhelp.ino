String getConfig() {
  StaticJsonDocument<840> configData;
  configData["innerHTML"]["HostValue"]["value"] = apSSID;
  configData["innerHTML"]["state_value"]["value"] = apSSID;
  configData["innerValues"]["wifi_ssid"]["value"] = wifi_ssid;
  configData["innerValues"]["mqtt_host"]["value"] = mqtt_host;
  configData["innerValues"]["mqtt_id"]["value"]  = mqtt_id;
  configData["innerValues"]["mqtt_user"]["value"]  = mqtt_user;
  configData["innerValues"]["mqtt_port"]["value"]  = mqtt_port;
  configData["innerValues"]["upload_throttle"]["value"]  = upload_throttle;
  configData["innerValues"]["eid_webhook"]["value"]  = eid_webhook;
  configData["innerSrc"]["mqtt_user"]["value"]  = apSSID;
  configData["innerTitles"]["mqtt_user"]["value"]  = apSSID;
  configData["innerChecks"]["mqtt_en"]["value"] = mqttSave;
  configData["innerChecks"]["mqtt_tls"]["value"] = mqtt_tls;
  configData["innerChecks"]["mqtt_auth"]["value"] = mqtt_auth;
  configData["innerChecks"]["eid_en"]["value"] = eidSave;
  configData["innerChecks"]["ha_en"]["value"] = haSave;
  String output;
  serializeJson(configData, output);
  return output;
}

String getIndexData() {
  StaticJsonDocument<840> indexData;
  String hostValue = "N/A";
  if(abs(netPowCon) < 1.0){
    if(mTimeFound) hostValue = String(netPowCon*1000.0) + " W";
    indexData["innerHTML"]["netPowCon"]["value"] = hostValue;
  }
  else{
    if(mTimeFound) hostValue = String(netPowCon) + " kW";
    indexData["innerHTML"]["netPowCon"]["value"] = hostValue;
  }
  if(mTimeFound) hostValue = String(totConToday) + " kWh";
  indexData["innerHTML"]["totConToday"]["value"] = hostValue;
  if(mTimeFound) hostValue = String(totCon) + " kWh";
  indexData["innerHTML"]["totCon"]["value"] = hostValue;
  if(mTimeFound) hostValue = String(totIn) + " kWh";
  indexData["innerHTML"]["totIn"]["value"] = hostValue;
  if(mTimeFound) hostValue = String(gasConToday) + " m³";
  indexData["innerHTML"]["gasConToday"]["value"] = hostValue;
  if(mTimeFound) hostValue = String(totGasCon) + " m³";
  indexData["innerHTML"]["totGasCon"]["value"] = hostValue;
  String output;
  serializeJson(indexData, output);
  return output;
}

String getIndexStatic() {
  StaticJsonDocument<840> indexStatic;
  indexStatic["innerHTML"]["HostValue"]["value"] = apSSID; 
  indexStatic["innerHTML"]["footer"]["value"] = "SGR dongle V" + String(fw_ver/100.0) + " by plan-d.io"; 
  if(wifiSTA) indexStatic["innerTitles"]["WifiStrength"]["value"] = "Wifi connected";
  else indexStatic["innerTitles"]["WifiStrength"]["value"] = "Wifi not connected/configured";
  if(mqtt_en){
    if(wifiSTA && !mqttHostError && !mqttClientError && !httpsError) indexStatic["innerTitles"]["Cloud"]["value"] = "Cloud connected";
    else indexStatic["innerTitles"]["Cloud"]["value"] = "Cloud not connected";
  }
  else indexStatic["innerTitles"]["Cloud"]["value"] = "Cloud not configured";  
  if(mTimeFound) indexStatic["innerTitles"]["DigitalMeter"]["value"] = "Digital meter connected";
  else if (mTimeFound && meterError) indexStatic["innerTitles"]["DigitalMeter"]["value"] = "Unsupported digital meter connected";
  else indexStatic["innerTitles"]["DigitalMeter"]["value"] = "No digital meter connected";
  indexStatic["innerTitles"]["ExternalIO"]["value"] = "No external IO connected";
  String output;
  serializeJson(indexStatic, output);
  return output;
}

String getUnit() {
  StaticJsonDocument<840> unitData;
  unitData["innerHTML"]["HostValue"]["value"] = apSSID;
  unitData["innerValues"]["em_topic"]["value"] = em_topic;
  unitData["innerValues"]["sgr_topic"]["value"] = sgr_topic;
  if(powerMultiplier == 1) unitData["innerValues"]["em_unit"]["value"] = "kW";
  else if(powerMultiplier == 1000) unitData["innerValues"]["em_unit"]["value"] = "W";
  else unitData["innerValues"]["em_unit"]["value"] = "Unknown";
  unitData["innerValues"]["em_pulses"]["value"]  = pulseMultiplier;
  unitData["innerValues"]["em_pwon"]["value"]  = pulseOnWidth;
  unitData["innerValues"]["em_pwoff"]["value"]  = pulseOffWidth;
  unitData["innerChecks"]["em_en"]["value"] = emSave;
  unitData["innerChecks"]["em_cofy"]["value"] = em_cofy;
  unitData["innerChecks"]["em_pol"]["value"] = pulseInversePolarity;
  unitData["innerValues"]["sgr_type"]["value"] = sgr_type;
  unitData["innerValues"]["sgr_lim"]["value"]  = sgr_lim;
  unitData["innerChecks"]["sgr_dir"]["value"] = sgr_dir;
  unitData["innerChecks"]["data_altherma"]["value"] = data_altherma;
  unitData["innerChecks"]["external_temperature"]["value"] = external_temperature;
  String output;
  serializeJson(unitData, output);
  return output;
}
