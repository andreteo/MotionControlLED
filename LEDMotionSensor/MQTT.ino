void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  
  
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(OnboardLED, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(OnboardLED, LOW);
    }
  }
  else if (String(topic) == "esp32/ledstrip") {
    Serial.printf("Changing mode of LED strip to %s\n", messageTemp);
    if (messageTemp == "fire"){
      colourValue = 1;
    }
    /*else if (messageTemp == "meteor"){
      colourValue = 2;
    }
    else if (messageTemp == "rainbow"){
      colourValue = 3;
    }*/
    else {
      colourValue = 0;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
      client.subscribe("esp32/ledstrip");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
