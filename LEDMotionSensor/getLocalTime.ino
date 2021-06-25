void PrintLocalTime(){
  struct tm timeinfo;
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  // strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

int getHour(){
  struct tm timeinfo;
  int timeHour[3];
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return 1;
  }
  return timeinfo.tm_hour; 
}
