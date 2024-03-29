boolean checkUpdate(){
  if(update_autoCheck){
    clientSecureBusy = true;
    boolean needUpdate = false;
    boolean mqttPaused;
    if(mqttclientSecure.connected()){
      Serial.println("Disconnecting TLS MQTT connection");
      mqttclientSecure.disconnect();
      mqttPaused = true;
    }
    if(bundleLoaded){
      syslog("Checking repository for firmware update... ", 0);
      if (https.begin(*client, "https://raw.githubusercontent.com/plan-d-io/SGR-dongle/main/version")) {  
        int httpCode = https.GET();
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            onlineVersion = atoi(payload.c_str());
          }
        } else {
          syslog("Could not connect to repository, HTTPS code " + String(https.errorToString(httpCode)), 2);
        }
        https.end();
      } else {
        syslog("Unable to connect to repository", 2);
      }
    }
    client->stop();
    clientSecureBusy = false;
    if(mqttPaused){
      sinceConnCheck = 10000;
    }
    //Serial.println("");
    syslog("Current firmware: " + String(fw_ver/100.0) + ", online version: " + String(onlineVersion/100.0), 0);
    if(onlineVersion > fw_ver){
      needUpdate = true;
      syslog("Firmware update available", 2);
    }
    else syslog("No firmware update available", 0);
    return needUpdate;
  }
}

boolean startUpdate(){
  if(update_auto){
    if(fw_ver < onlineVersion || update_start){
      syslog("Preparing firmware upgrade", 1);
      clientSecureBusy = true;
      boolean mqttPaused; //,needUpdate
      if(mqttclientSecure.connected()){
        syslog("Disconnecting TLS MQTT connection to fetch firmware upgrade", 2);
        mqttclientSecure.disconnect();
        mqttPaused = true;
      }
      if(bundleLoaded){
        //String baseUrl = "https://raw.githubusercontent.com/plan-d-io/SGR-dongle/main";
        //String fileUrl = baseUrl + "/bin/SGR-dongle-V" + String(onlineVersion/100.0) +"ino.bin";
        String fileUrl ="https://raw.githubusercontent.com/plan-d-io/SGR-dongle/main/bin/SGR-dongle.ino.bin";
        //String fileUrl = baseUrl + String(onlineVersion/100.0) +".ino.bin";
        syslog("Getting new firmware over HTTPS/TLS", 0);
        syslog("Found new firmware at "+ fileUrl, 0);
        if (https.begin(*client, fileUrl)) {  
          int httpCode = https.GET();
          Serial.println(httpCode);
          if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              long contentLength = https.getSize();
              syslog("Firmware size: " + String(contentLength), 0);
              // Check if there is enough to OTA Update
              bool canBegin = Update.begin(contentLength);
              // If yes, begin
              if (canBegin) {
                unitState = 0;
                blinkLed();
                syslog("Beginning firmware upgrade. This may take 2 - 5 mins to complete. Things might be quiet for a while.. Patience!", 2);
                // No activity would appear on the Serial monitor
                // So be patient. This may take 2 - 5mins to complete
                size_t written = Update.writeStream(*client);
                if (written == contentLength) {
                  syslog("Written : " + String(written) + " successfully", 0);
                } else {
                  syslog("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?", 3);
                  update_start = false;
                }
                if (Update.end()) {
                  if (Update.isFinished()) {
                    syslog("Firmware upgrade successfully completed. Rebooting to finish update.", 1);
                    last_reset = "Firmware upgrade successfully completed. Rebooting to finish update";
                    fw_ver = onlineVersion;
                    update_start = false;
                    update_finish = true;
                    saveConfig();
                    delay(500);
                    ESP.restart();
                  } else {
                    syslog("Firmware upgrade not finished? Something went wrong!", 3);
                    update_start = false;
                  }
                } else {
                  syslog("Firmware upgrade error occurred. Error #: " + String(Update.getError()), 3);
                  update_start = false;
                }
              } 
              else {
                // Not enough space to begin OTA
                syslog("Not enough space to begin OTA upgrade", 3);
                update_start = false;
                client->flush();
              }
              while (client->available()) {
                // read line till /n
                String line = client->readString();
                // remove space, to check if the line is end of headers
                Serial.print(line);
              }
            }
            else{
              syslog("Could not connect to repository, HTTPS code " + String(https.errorToString(httpCode)), 2);
              update_start = false;
              unitState = 4;
            }
          } 
          else {
            syslog("Could not connect to repository, HTTPS code " + String(https.errorToString(httpCode)), 2);
            update_start = false;
            unitState = 4;
          }
          https.end(); 
        } 
        else {
          Serial.print("Unable to connect");
          update_start = false;
          unitState = 4;
        }
      }
      client->stop();
      clientSecureBusy = false;
      if(mqttPaused){
        sinceConnCheck = 10000;
      }
      update_start = false;
      return true;
    }
    else{
      syslog("No firmware upgrade available", 0);
      update_start = false;
      unitState = 4;
      return false;
    }
    update_start = false;
    saveConfig();
    delay(500);
    return true;
  }
}

boolean finishUpdate(){
  clientSecureBusy = true;
  boolean mqttPaused;
  boolean filesUpdated = false;
  if(mqttclientSecure.connected()){
    syslog("Disconnecting TLS MQTT connection to fetch update", 2);
    mqttclientSecure.disconnect();
    mqttPaused = true;
  }
  if(bundleLoaded){
    syslog("Finishing upgrade. Preparing to download static files.", 1);
    String baseUrl = "https://raw.githubusercontent.com/plan-d-io/SGR-dongle/main";
    String fileUrl = baseUrl + "/bin/files";
    String payload;
    if (https.begin(*client, fileUrl)) {  
      int httpCode = https.GET();
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          payload = https.getString();
        }
      } 
      else {
        syslog("Could not connect to repository, HTTPS code " + String(https.errorToString(httpCode)), 2);
      }
      https.end();
      unsigned long eof = payload.lastIndexOf('\n');
      if(eof > 0){
        syslog("Downloading static files", 2);
        unitState = 0;
        blinkLed();
        unsigned long delimStart = 0;
        unsigned long delimEnd = 0;
        while(delimEnd < eof){
          delimEnd = payload.indexOf('\n', delimStart);
          String s = "/" + payload.substring(delimStart, delimEnd);
          delimStart = delimEnd+1;
          fileUrl = baseUrl + "/data" + s;
          Serial.println(fileUrl);
          File f = SPIFFS.open(s, FILE_WRITE);
          Serial.println(s);
          if (f) {
            if (https.begin(*client, fileUrl)) {
              int httpCode = https.GET();
              if (httpCode > 0) {
                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                  long contentLength = https.getSize();
                  Serial.print("File size: ");
                  Serial.println(contentLength);
                  Serial.println("Begin download");
                  size_t written = https.writeToStream(&f);
                  if (written == contentLength) {
                    Serial.println("Written : " + String(written) + " successfully");
                    filesUpdated = true;
                  }
                }
              } 
              else {
                syslog("Could not connect to repository, HTTPS code " + String(https.errorToString(httpCode)), 2);
              }
              https.end();
            }
          }
          else{
            syslog("Could not write files!", 3);
          }
        }
      }   
    } 
    else {
      syslog("Unable to connect to repository", 2);
    }
  }
  client->stop();
  clientSecureBusy = false;
  update_finish = false;
  if(filesUpdated){
    update_finish = false;
    syslog("Static files successfully updated. Rebooting to finish update.", 1);
    last_reset = "Static files successfully updated. Rebooting to finish update.";
    saveConfig();
    delay(500);
    ESP.restart();
  }
  if(mqttPaused){
    sinceConnCheck = 10000;
  }
  saveConfig();
  unitState = 4;
  delay(500);
  return true;
}
