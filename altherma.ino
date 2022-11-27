bool contains(char array[], int size, int value)
{
  for (int i = 0; i < size; i++)
  {
    if (array[i] == value)
      return true;
  }
  return false;
}

//Converts to string, send through MQTT as JSON message
void updateValues(char regID)
{
  LabelDef *labels[128];
  int num = 0;
  converter.getLabels(regID, labels, num);
  for (size_t i = 0; i < num; i++)
  {
    bool alpha = false;
    for (size_t j = 0; j < strlen(labels[i]->asString); j++)
    {
      char c = labels[i]->asString[j];
      if (!isdigit(c) && c!='.'){
        alpha = true;
      }
    }

    char topicBuff[128] = "";
    strcat(topicBuff,labels[i]->label);
    String friendlyName = String(topicBuff);
    String topic = "data/devices/heatpump/" + friendlyName;
    topic.replace(' ', '_');
    topic.replace('.', '\0');
    topic.toLowerCase();
    time_t now;
    unsigned long dtimestamp = time(&now);
    DynamicJsonDocument doc(1024);
    /*Format the COFY required data*/
    doc["value"] = labels[i]->asString;
    if(labels[i]->registryID == 0x20){
      if(labels[i]->offset == 0){
        //SGR Heatpump
        //"Outdoor air temp.(R1T)"}, //! + COFY
        topic = "data/devices/heatpump/outdoor_temperature";
        doc["metric"] = "OutdoorTemperature";
        doc["metricKind"] = "gauge";
        String svalue = String(labels[i]->asString);
        doc["value"] = svalue.toFloat();
      }
    }
    else if(labels[i]->registryID == 0x21){
      if(labels[i]->offset == 0){
        //SGR Heatpump advanced
        //"INV primary current (A)"}, //!
        topic = "data/devices/heatpump/inverter_current";
        String svalue = String(labels[i]->asString);
        doc["value"] = svalue.toFloat();
      }
      else if(labels[i]->offset == 4){
        //SGR Heatpump advanced
        //"Voltage (N-phase) (V)"}, //!
        topic = "data/devices/heatpump/inverter_voltage";
        String svalue = String(labels[i]->asString);
        doc["value"] = svalue.toFloat();
      }
    }
    else if(labels[i]->registryID == 0x60){
      if(labels[i]->offset == 7){
        //SGR Heatpump advanced
        //"DHW setpoint"}, //!
        topic = "data/devices/heatpump/setpoint_dhw_tank_temperature";
        String svalue = String(labels[i]->asString);
        doc["value"] = svalue.toFloat();
      }
      else if(labels[i]->offset == 9){
        //SGR Heatpump advanced
        //"LW setpoint (main)"}, //!
        topic = "data/devices/heatpump/setpoint_outlet";
        String svalue = String(labels[i]->asString);
        doc["value"] = svalue.toFloat();
      }
    }
    else if(labels[i]->registryID == 0x61){
      if(labels[i]->offset == 10){
        //SGR Heatpump
        //"DHW tank temp."}, //! + COFY
        topic = "data/devices/heatpump/dhwtank_temperature";
        doc["metric"] = "DHWTankTemperature";
        doc["metricKind"] = "gauge";
        String svalue = String(labels[i]->asString);
        doc["value"] = svalue.toFloat();
      }
      else if(labels[i]->offset == 12 && !external_temperature){
        //SGR Heatpump
        //"Indoor ambient temp."}, //! + COFY
        topic = "data/devices/heatpump/indoor_temperature";
        doc["metric"] = "IndoorTemperature";
        doc["metricKind"] = "gauge";
        String svalue = String(labels[i]->asString);
        doc["value"] = svalue.toFloat();
      }
      else{
        
      }
    }
    else if(labels[i]->registryID == 0x62){
      if(labels[i]->offset == 5){
        //SGR Heatpump
        //"RT setpoint"}, //! + COFY
        topic = "data/devices/heatpump/indoor_temperature_setpoint";
        doc["metric"] = "IndoorTemperatureSetpoint";
        doc["metricKind"] = "gauge";
        String svalue = String(labels[i]->asString);
        doc["value"] = svalue.toFloat();
      }
    }
    else{
    }
    doc["entity"] = "heat_pump";
    doc["friendly_name"] = "Heat pump " + friendlyName;
    //doc["value"] = labels[i]->asString;
    doc["unit"] = String(labels[i]->unit);
    doc["timestamp"] = dtimestamp;
    String jsonOutput;
    serializeJson(doc, jsonOutput);
    Serial.println(topic);
    Serial.println(jsonOutput);
    pubMqtt(topic, jsonOutput, false);   
  }
}

void initRegistries(boolean eraseMeter){
    //getting the list of registries to query from the selected values  
  for (size_t i = 0; i < sizeof(registryIDs); i++)
  {
    registryIDs[i]=0xff;
  }

  int i = 0;
  for (auto &&label : labelDefs)
  {
    if (!contains(registryIDs, sizeof(registryIDs), label.registryID))
    {
      Serial.printf("Adding registry 0x%2x to be queried.\n", label.registryID);
      /*Perform HA autodisovery
       * if MQTT is enabled
       */
      LabelDef *labels[128];
      int num = 0;
      converter.getLabels(label.registryID, labels, num);
      for (size_t i = 0; i < num; i++)
      {
        bool alpha = false;
        for (size_t j = 0; j < strlen(labels[i]->asString); j++)
        {
          char c = labels[i]->asString[j];
          if (!isdigit(c) && c!='.'){
            alpha = true;
          }
        }
        char topicBuff[128] = "";
        strcat(topicBuff,labels[i]->label);
        String friendlyName = String(topicBuff);
        String topic = friendlyName;
        topic.replace(' ', '_');
        topic.replace('.', '\0');
        topic.toLowerCase();
        boolean controlWidget = false;
        String chanName = "";
        String configTopic = "";
        DynamicJsonDocument doc(1024);
        /*Enumerate channels*/
        if(label.registryID == 0x20){
          if(labels[i]->offset == 0){
            //SGR Heatpump
            //"Outdoor air temp.(R1T)"}, //! + COFY
            chanName = "heatpump_outdoor_temperature";
            doc["name"] = "Outdoor temperature";
            doc["icon"] = "mdi:thermometer";
            doc["state_topic"] = "data/devices/heatpump/outdoor_temperature";
            doc["value_template"] = "{{ value_json.value }}";
            doc["device_class"] = "temperature";
            doc["unit_of_measurement"] = "°C";
            configTopic = "homeassistant/sensor/" + chanName + "/config";
            controlWidget = true;
          }
          else{
            //SGR Heatpump advanced
            //do general autodiscovery
            chanName = topic;
            doc["name"] = friendlyName;
            doc["state_topic"] = "data/devices/heatpump/" + topic;
            doc["value_template"] = "{{ value_json.value }}"; ;
            doc["unit_of_measurement"]  = String(labels[i]->unit);
            configTopic = "homeassistant/sensor/" + chanName + "/config";
          }
        }
        else if(label.registryID == 0x61){
          if(labels[i]->offset == 10){
            //SGR Heatpump
            //"DHW tank temp." //! + COFY
            chanName = "heatpump_dhwtank_temperature";
            doc["name"] = "DHW tank temperature";
            doc["icon"] = "mdi:thermometer";
            doc["state_topic"] = "data/devices/heatpump/dhwtank_temperature";
            doc["value_template"] = "{{ value_json.value }}";
            doc["device_class"] = "temperature";
            doc["unit_of_measurement"] = "°C";
            configTopic = "homeassistant/sensor/" + chanName + "/config";
            controlWidget = true;
          }
          else if(labels[i]->offset == 12){
            //SGR Heatpump
            //"Indoor ambient temp."}, //! + COFY
            chanName = "heatpump_indoor_temperature";
            doc["name"] = "Indoor temperature";
            doc["icon"] = "mdi:thermometer";
            doc["state_topic"] = "data/devices/heatpump/indoor_temperature";
            doc["value_template"] = "{{ value_json.value }}";
            doc["device_class"] = "temperature";
            doc["unit_of_measurement"] = "°C";
            configTopic = "homeassistant/sensor/" + chanName + "/config";
            controlWidget = true;
          }
          else{
            //SGR Heatpump advanced
            //do general autodiscovery
            chanName = topic;
            doc["name"] = friendlyName;
            doc["state_topic"] = "data/devices/heatpump/" + topic;
            doc["value_template"] = "{{ value_json.value }}"; ;
            doc["unit_of_measurement"]  = String(labels[i]->unit);
            configTopic = "homeassistant/sensor/" + chanName + "/config";
          }
        }
        else if(label.registryID == 0x62){
          if(labels[i]->offset == 5){
            //SGR Heatpump
            //"RT setpoint"}, //! + COFY
            chanName = "heatpump_indoor_temperature_setpoint";
            doc["name"] = "Indoor temperature setpoint";
            doc["icon"] = "mdi:gauge";
            doc["state_topic"] = "data/devices/heatpump/indoor_temperature_setpoint";
            doc["value_template"] = "{{ value_json.value }}";
            doc["device_class"] = "temperature";
            doc["unit_of_measurement"] = "°C";
            configTopic = "homeassistant/sensor/" + chanName + "/config";
            controlWidget = true;
          }
          else{
            //SGR Heatpump advanced
            //do general autodiscovery
            chanName = topic;
            doc["name"] = friendlyName;
            doc["state_topic"] = "data/devices/heatpump/" + topic;
            doc["value_template"] = "{{ value_json.value }}"; ;
            doc["unit_of_measurement"]  = String(labels[i]->unit);
            configTopic = "homeassistant/sensor/" + chanName + "/config";
          }
        }
        else if(label.registryID == 0x21){
          if(labels[i]->offset == 0){
            //SGR Heatpump advanced
            //"INV primary current (A)"}, //!
            chanName = "heatpump_inverter_current";
            doc["name"] = "Inverter current";
            doc["state_topic"] = "data/devices/heatpump/inverter_current";
            doc["value_template"] = "{{ value_json.value }}";
            doc["device_class"] = "current";
            doc["unit_of_measurement"] = "A";
            configTopic = "homeassistant/sensor/" + chanName + "/config";
          }
          else if(labels[i]->offset == 4){
            //SGR Heatpump advanced
            //"Voltage (N-phase) (V)"}, //!
            chanName = "heatpump_inverter_voltage";
            doc["name"] = "Inverter voltage";
            doc["state_topic"] = "data/devices/heatpump/inverter_voltage";
            doc["value_template"] = "{{ value_json.value }}";
            doc["device_class"] = "voltage";
            doc["unit_of_measurement"] = "V";
            configTopic = "homeassistant/sensor/" + chanName + "/config";
          }
          else{
            //SGR Heatpump advanced
            //do general autodiscovery
            chanName = topic;
            doc["name"] = friendlyName;
            doc["state_topic"] = "data/devices/heatpump/" + topic;
            doc["value_template"] = "{{ value_json.value }}"; ;
            doc["unit_of_measurement"]  = String(labels[i]->unit);
            configTopic = "homeassistant/sensor/" + chanName + "/config";
          }
        }
        else if(label.registryID == 0x60){
          if(labels[i]->offset == 7){
            //SGR Heatpump advanced
            //"DHW setpoint"}, //!
            chanName = "heatpump_setpoint_dhw_tank_temperature";
            doc["name"] = "Setpoint DHW tank";
            doc["icon"] = "mdi:gauge";
            doc["state_topic"] = "data/devices/heatpump/setpoint_dhw_tank_temperature";
            doc["value_template"] = "{{ value_json.value }}";
            doc["device_class"] = "temperature";
            doc["unit_of_measurement"] = "°C";
            configTopic = "homeassistant/sensor/" + chanName + "/config";
          }
          else if(labels[i]->offset == 9){
            //SGR Heatpump advanced
            //"LW setpoint (main)"}, //!
            chanName = "heatpump_setpoint_outlet";
            doc["name"] = "Setpoint outlet";
            doc["icon"] = "mdi:gauge";
            doc["state_topic"] = "data/devices/heatpump/setpoint_outlet";
            doc["value_template"] = "{{ value_json.value }}";
            doc["device_class"] = "temperature";
            doc["unit_of_measurement"] = "°C";
            configTopic = "homeassistant/sensor/" + chanName + "/config";
          }
          else{
            //SGR Heatpump advanced
            //do general autodiscovery
            chanName = topic;
            doc["name"] = friendlyName;
            doc["state_topic"] = "data/devices/heatpump/" + topic;
            doc["value_template"] = "{{ value_json.value }}"; ;
            doc["unit_of_measurement"]  = String(labels[i]->unit);
            configTopic = "homeassistant/sensor/" + chanName + "/config";
          }
        }
        else{
          //SGR Heatpump advanced
          //do general autodiscovery
          chanName = topic;
          doc["name"] = friendlyName;
          doc["state_topic"] = "data/devices/heatpump/" + topic;
          doc["value_template"] = "{{ value_json.value }}"; ;
          doc["unit_of_measurement"]  = String(labels[i]->unit);
          configTopic = "homeassistant/sensor/" + chanName + "/config";
        }
        doc["unique_id"] = chanName;
        doc["availability_topic"] = "data/devices/heatpump";
        JsonObject device  = doc.createNestedObject("device");
        JsonArray identifiers = device.createNestedArray("identifiers");
        if(controlWidget){
          /*Assign these channels to the basic control widget*/
          identifiers.add("SGR Heatpump control");
          device["name"] = "SGR Heatpump control";
          device["model"] = "SGR dongle for SGR compatible heat pumps";
          device["manufacturer"] = "plan-d.io";
          device["configuration_url"] = "http://" + WiFi.localIP().toString();
          device["sw_version"] = String(fw_ver/100.0);
        }
        else{
          /*All the rest goes into the advanced control widget*/
          identifiers.add("SGR_heatpump advanced control");
          device["name"] = "SGR Heatpump advanced control";
          device["model"] = "SGR dongle for SGR compatible heat pumps - advanced";
          device["manufacturer"] = "plan-d.io";
          device["configuration_url"] = "http://" + WiFi.localIP().toString();
          device["sw_version"] = String(fw_ver/100.0);
        }
        String jsonOutput ="";
        if(eraseMeter){
          //Ensure devices are erased before created again
          if(chanName.length() > 0) pubMqtt(configTopic, jsonOutput, true);
          delay(100);
        }
        serializeJson(doc, jsonOutput);
        if(chanName.length() > 0){
          pubMqtt(configTopic, jsonOutput, true);
          if(eraseMeter) delay(100); //initial HA discovery sometimes fails if sensors are created too quickly
          //Serial.println(configTopic);
          //Serial.println(jsonOutput);
        }
      }
      registryIDs[i++] = label.registryID;
    }
    altherma_mqtt = true;
  }
  if (i == 0)
  {
    Serial.printf("ERROR - No values selected in the include file. Stopping.\n");
    while (true)
    {
    }
  }
}
