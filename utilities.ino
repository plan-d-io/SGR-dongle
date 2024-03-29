boolean scanWifi(){
  int16_t n = WiFi.scanNetworks();
  String savedSSID = wifi_ssid;
  boolean foundSavedSSID = false;
  String buildSSIDlist = "";
  for (int i = 0; i < n; ++i) {
    if(WiFi.SSID(i) != savedSSID){
      buildSSIDlist += "<option value=\"";
      buildSSIDlist += WiFi.SSID(i);
      buildSSIDlist += "\">";
      buildSSIDlist += WiFi.SSID(i);
      buildSSIDlist += "</option>";
    }
    else{
      foundSavedSSID = true;
    }
  }
  buildSSIDlist += "</select>";  
  ssidList = "<select name=\"ssid\">";
  if(foundSavedSSID){
    ssidList += "<option value=\"";
    ssidList += savedSSID;
    ssidList += "\">";
    ssidList += savedSSID;
    ssidList += "</option>";
  }
  ssidList += buildSSIDlist;
  return foundSavedSSID;
}

String getHostname(){
  WiFi.macAddress(mac);
  char macbuf[] = "00000";
  String macbufs = "";
  macbufs += String(mac[4], HEX);
  macbufs += String(mac[5], HEX);
  macbufs.toUpperCase();
  macbufs.toCharArray(macbuf, 5);
  apSSID[4] = macbuf[0];
  apSSID[5] = macbuf[1];
  apSSID[6] = macbuf[2];
  apSSID[7] = macbuf[3];
  return macbufs;
}

String printLocalTime(boolean verbosePrint){
  struct tm timeinfo;
  String(timestring);
  time_t now;
  if(!getLocalTime(&timeinfo)){
    timestring = "";
    if(verbosePrint) syslog("Failed to obtain time from RTC", 2);
    timeSet = false;
  }
  else{
    char timeStringBuff[30];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y/%m/%d %H:%M:%S", &timeinfo);
    //if(!timeSet) timestring = "Time set: ";
    timestring = timestring + String(timeStringBuff);
    if(verbosePrint) syslog("Time set: " + timestring, 1);
    timeSet = true;
  }
  return(timestring);
}

unsigned long printUnixTime(){
  time_t now;
  return(time(&now));
}

void setClock(boolean firstSync)
{  
  time_t nowSecs = time(nullptr);
  if(firstSync){
    syslog("Configuring NTP time sync", 0);
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    timeconfigured = true;
  }
}

void setMeterTime(){
  if(mTimeFound){
    if(!timeSet) syslog("Syncing to to metertime", 0);
    localtime(&dm_timestamp);
    timeSet = true;
  }
}

void checkConnection(){
  if(WiFi.status() != WL_CONNECTED){
    syslog("Lost WiFi connection, trying to reconnect", 2);
    WiFi.disconnect();
    elapsedMillis restartAttemptTime;
    while (WiFi.status() != WL_CONNECTED && restartAttemptTime < 20000) {
      WiFi.reconnect();
    }
    if(restartAttemptTime >= 20000) {
      syslog("Wifi reconnection failed! Trying again in a minute", 3);
      reconncount++;
      wifiError = true;
    }
  }
  if(wifiError && WiFi.status() == WL_CONNECTED){
    wifiError = false;
    syslog("Reconnected to the WiFi network", 0);
    if(!mqtt_en) reconncount = 0;
  }
  if(WiFi.status() == WL_CONNECTED){
    if(mqtt_en){
      connectMqtt();
      if(data_altherma && !altherma_mqtt){
        initRegistries(true);
        syslog("ESPaltherma integration activated", 1);
      }
      if(ha_en && !ha_metercreated)haAutoDiscovery(true);
      else if(ha_en && ha_metercreated){
        haAutoDiscovery(false);
        initMqttValues();
        if(data_altherma) initRegistries(false);
      }
      initMqttValues();
    }
  }
}

void setReboot(){
  rebootInit = true;
  sinceRebootCheck = 0;
}

void setBuff(uint8_t Rdata, uint8_t Gdata, uint8_t Bdata)
{
    DisBuff[0] = 0x05;
    DisBuff[1] = 0x05;
    for (int i = 0; i < 25; i++)
    {
        DisBuff[2 + i * 3 + 0] = Rdata;
        DisBuff[2 + i * 3 + 1] = Gdata;
        DisBuff[2 + i * 3 + 2] = Bdata;
    }
}

void blinkLed(){
  if(ledTime >= 300){
    //Serial.println(unitState);
    if(ledState){
      if(unitState == 1 || unitState == 3 ||unitState == 5) setBuff(0x00, 0x00, 0x00);
    }
    else{
      if(unitState <= 1) setBuff(0xff, 0x00, 0x00);
      else if(unitState == 2 || unitState == 3) setBuff(0xff, 0x40, 0x00);
      else setBuff(0x00, 0x00, 0x40);
    }
    ledState = !ledState;
    ledTime = 0;
    M5.dis.displaybuff(DisBuff);
  }
}
