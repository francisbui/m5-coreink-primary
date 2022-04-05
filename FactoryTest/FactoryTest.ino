
#include "M5CoreInk.h"
#include <WiFi.h>
#include "icon.h"
#include "esp_adc_cal.h"
#include <String>

TaskHandle_t Task1;

Ink_Sprite TimePageSprite(&M5.M5Ink);
Ink_Sprite TimeSprite(&M5.M5Ink);
Ink_Sprite DateSprite(&M5.M5Ink);

RTC_TimeTypeDef RTCtime,RTCTimeSave;
RTC_DateTypeDef RTCDate;
uint8_t second = 0 ,minutes = 0;

bool testMode = false;

void drawImageToSprite(int posX,int posY,image_t* imagePtr,Ink_Sprite* sprite)
{
    sprite->drawBuff(   posX, posY,
                        imagePtr->width, imagePtr->height, imagePtr->ptr);
}

void drawTime( RTC_TimeTypeDef *time )
{
    int hrs;
    if (time->Hours <= 12)
    {
      hrs = time->Hours;
    }
    else
    {
      hrs = time->Hours-12;
    }
    drawImageToSprite(10 ,48,&num55[hrs/10],&TimePageSprite);
    drawImageToSprite(50 ,48,&num55[hrs%10],&TimePageSprite);
    drawImageToSprite(90 ,48,&num55[10],&TimePageSprite);
    drawImageToSprite(110,48,&num55[time->Minutes/10],&TimePageSprite);
    drawImageToSprite(150,48,&num55[time->Minutes%10],&TimePageSprite);
}

void darwDate( RTC_DateTypeDef *date)
{
    int posX = 15, num = 0;
    drawImageToSprite(posX,124,&num18x29[date->Month / 10 % 10],&TimePageSprite);posX += 17;
    drawImageToSprite(posX,124,&num18x29[date->Month % 10],&TimePageSprite);posX += 17;

    drawImageToSprite(posX,124,&num18x29[10],&TimePageSprite);posX += 17;

    drawImageToSprite(posX,124,&num18x29[date->Date  / 10 % 10 ],&TimePageSprite);posX += 17;
    drawImageToSprite(posX,124,&num18x29[date->Date  % 10 ],&TimePageSprite);posX += 17;
  
    drawImageToSprite(posX,124,&num18x29[10],&TimePageSprite);posX += 17;

    for( int i = 0; i < 4; i++ )
    {
        num = ( date->Year / int(pow(10,3-i)) % 10);
        drawImageToSprite(posX,124,&num18x29[num],&TimePageSprite);
        posX += 17;
    }
}

float getBatVoltage()
{
    analogSetPinAttenuation(35,ADC_11db);
    esp_adc_cal_characteristics_t *adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 3600, adc_chars);
    uint16_t ADCValue = analogRead(35);
    
    uint32_t BatVolmV  = esp_adc_cal_raw_to_voltage(ADCValue,adc_chars);
    float BatVol = float(BatVolmV) * 25.1 / 5.1 / 1000;
    return BatVol;
}

void drawScanWifi()
{
    M5.M5Ink.clear();
    TimePageSprite.clear();
}

void drawWarning(const char *str)
{
    M5.M5Ink.clear();
    TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
    drawImageToSprite(76,40,&warningImage,&TimePageSprite);
    int length = 0;
    while(*( str + length ) != '\0') length++;
    TimePageSprite.drawString((200 - length * 8) / 2,100,str,&AsciiFont8x16);
    TimePageSprite.pushSprite();
}

void drawTimePage( )
{
    M5.rtc.GetTime(&RTCtime);
    drawTime(&RTCtime);
    minutes = RTCtime.Minutes;
    M5.rtc.GetDate(&RTCDate);
    darwDate(&RTCDate);
    TimePageSprite.pushSprite();
}

void setRTC()
{
  RTCtime.Hours = 22;
  RTCtime.Minutes = 11;
  RTCtime.Seconds = 00;
  M5.rtc.SetTime(&RTCtime); //Set RTC time
  
  RTCDate.Month = 2;
  RTCDate.Date = 18;
  RTCDate.Year = 2022;
  M5.rtc.SetDate(&RTCDate); //Set RTC date.
}

void testPage()
{
    uint8_t buttonMark = 0x00, buttonMarkSave = 0;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.scanNetworks(true);

    M5.M5Ink.clear();
    TimePageSprite.clear();

    const char BtnUPStr[] =     "Btn UP Pressed";
    const char BtnDOWNStr[] =   "Btn DOWN Pressed";
    const char BtnMIDStr[] =    "Btn MID Pressed";
    const char BtnEXTStr[] =    "Btn EXT Pressed";
    const char BtnPWRStr[] =    "Btn PWR Pressed";

    const char* strPtrBuff[5] = {BtnUPStr,BtnDOWNStr,BtnMIDStr,BtnEXTStr,BtnPWRStr};

    char timeStrbuff[64];
    M5.rtc.GetTime(&RTCtime);
    M5.rtc.GetDate(&RTCDate);

    sprintf(timeStrbuff,"%d/%02d/%02d %02d:%02d:%02d",
                        RTCDate.Year,RTCDate.Month,RTCDate.Date,
                        RTCtime.Hours,RTCtime.Minutes,RTCtime.Seconds);

    TimePageSprite.drawString(10,5,timeStrbuff);

    //char batteryStrBuff[64];
    //sprintf(batteryStrBuff,"Battery:%.2fV",getBatVoltage());
    //TimePageSprite.drawString(10,23,batteryStrBuff,&AsciiFont8x16);
    
    char wifiStrBuff[64];
        
    Wire.begin(32,33,100000UL);
    int groveCheck = 0;

    Wire.beginTransmission(0x76);
    Wire.write(0xD0);
    if( Wire.endTransmission() != 0 )
    {
        groveCheck = -1;
    }
    else
    {
        Wire.requestFrom(0x76,1); 
        uint8_t chipID = Wire.read();
        Serial.printf("Read ID = %02X\r\n",chipID);
        if( chipID != 0x58 )
        {
            groveCheck = -1;
        }
    }
    
    if( groveCheck != 0 )
    {
        drawWarning("GROVE Check Error");
        while(1){ delay(10);}
    }

    while(1)
    {
        int WifiRes = WiFi.scanComplete();
        if( WifiRes == -2 ){}
        else if( WifiRes == -1 ){}
        else if( WifiRes == 0 )
        {
            TimePageSprite.drawString(10,41,"no networks found",&AsciiFont8x16);
            break;
        }
        else
        {
            String SSIDStr = WiFi.SSID(0);
            if( SSIDStr.length() > 11 )
            {
                SSIDStr = SSIDStr.substring(0,8);
                SSIDStr += "...";
            }
            int32_t rssi = ( WiFi.RSSI(0) < -100 ) ? -100 : WiFi.RSSI(0);
            sprintf(wifiStrBuff,"wifi %s  r:%d",SSIDStr.c_str(),rssi);
            TimePageSprite.drawString(10,41,wifiStrBuff,&AsciiFont8x16);
            break;
        }
        delay(100);
    }
    TimePageSprite.pushSprite();

    Wire1.begin(-1,-1);
    pinMode(21,OUTPUT);
    pinMode(22,OUTPUT);

    digitalWrite(21,HIGH);
    digitalWrite(22,HIGH);

    while(1)
    {
        if( buttonMark != buttonMarkSave )
        {
            uint8_t mark = buttonMarkSave ^ buttonMark;
            int index = 0;
            while(!(( mark >> index ) & 0x01)) index ++;
            Serial.printf("index = %d\r\n",index);
            TimePageSprite.drawString(10,59 + ( index * 18),strPtrBuff[index],&AsciiFont8x16);
            TimePageSprite.pushSprite();
            buttonMarkSave = buttonMark;
            //digitalWrite(21,(pinFlag)?HIGH:LOW);
            //digitalWrite(22,(pinFlag)?HIGH:LOW);
            //pinFlag = !pinFlag;
        }

        if( M5.BtnUP.wasPressed())  buttonMark |= 0x01;
        if( M5.BtnDOWN.wasPressed())buttonMark |= 0x02;
        if( M5.BtnMID.wasPressed()) buttonMark |= 0x04;
        if( M5.BtnEXT.wasPressed()) buttonMark |= 0x08;
        if( M5.BtnPWR.wasPressed()) buttonMark |= 0x10;

        M5.update();
        if( buttonMarkSave == 0x1f )
        {
            Wire1.begin(21,22);
            break;
        }
    }

    uint8_t pinCheckMark = 0;

    uint8_t testWritPinMap[4] = {13,18,26,23};
    uint8_t testReadPinMap[4] = {14,36,34,25};

    ink_spi.end();
    for( int i = 0; i < 4; i++ )
    {
        pinMode(testWritPinMap[i],OUTPUT);
        pinMode(testReadPinMap[i],INPUT);

        digitalWrite( testWritPinMap[i], HIGH );
        delay(2);
        if( digitalRead(testReadPinMap[i]) == HIGH )
        {
            pinCheckMark |= ( 1 << i );
        }

        digitalWrite( testWritPinMap[i], LOW );
        delay(2);
        if( digitalRead(testReadPinMap[i]) == LOW )
        {
            pinCheckMark |= ( 1 << ( i + 4 ));
        }
    }
    Serial.printf("EXT Check Mark %02X\r\n",pinCheckMark);
    ink_spi.begin(INK_SPI_SCK, -1, INK_SPI_MOSI, -1);

    char pinStrBuff[64];
    if( pinCheckMark != 0xff )
    {
        sprintf(pinStrBuff,"EXT PIN check %02X Error",pinCheckMark);
    }
    else
    {
        sprintf(pinStrBuff,"EXT PIN check %02X Ok",pinCheckMark);
    }
    M5.Speaker.tone(2700);
    TimePageSprite.drawString(10,149,pinStrBuff,&AsciiFont8x16);
    TimePageSprite.pushSprite();
    M5.Speaker.mute();

    if( pinCheckMark != 0xff )
    {
        while(1){ delay(100);}
    }
    while(1)
    {
        delay(10);
        M5.update();
        if( M5.BtnDOWN.wasPressed()|| M5.BtnUP.wasPressed())break;
    }

    M5.Speaker.tone(2700);
    delay(100);
    M5.Speaker.mute();

    for( int i = 0; i < 4; i++ )
    {
        testWritPinMap[i] = ( i % 2 == 0 ) ? 26 : 25;
        testReadPinMap[i] = 36;
    }
    pinCheckMark = 0;
    for( int i = 0; i < 4; i++ )
    {
        pinMode(testWritPinMap[i],OUTPUT);
        pinMode(testReadPinMap[i],INPUT);

        digitalWrite( testWritPinMap[i], HIGH );
        delay(2);
        if( digitalRead(testReadPinMap[i]) == HIGH )
        {
            pinCheckMark |= ( 1 << i );
        }

        digitalWrite( testWritPinMap[i], LOW );
        delay(2);
        if( digitalRead(testReadPinMap[i]) == LOW )
        {
            pinCheckMark |= ( 1 << ( i + 4 ));
        }
    }
    Serial.printf("HAT Check Mark %02X\r\n",pinCheckMark);

    if( pinCheckMark != 0xff )
    {
        sprintf(pinStrBuff,"HAT PIN check %02X Error",pinCheckMark);
    }
    else
    {
        sprintf(pinStrBuff,"HAT PIN check %02X Ok",pinCheckMark);
    }
    TimePageSprite.drawString(10,167,pinStrBuff,&AsciiFont8x16);
    TimePageSprite.pushSprite();
    
    M5.M5Ink.clear();
    TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
}

// My WEB SERVER ----------------------------------------------------------------------
void WebServerPage()
{
  drawImageToSprite(0,0,&webServerImage,&TimePageSprite);
  TimePageSprite.drawString(10,60,"Connected to:",&AsciiFont8x16);
  TimePageSprite.drawString(10,75,"...",&AsciiFont8x16);
  TimePageSprite.pushSprite();
  webRun();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
  String strLocalIp = WiFi.localIP().toString();
  const char* cstrIp = strLocalIp.c_str();
  TimePageSprite.drawString(10,90,cstrIp,&AsciiFont8x16);
  TimePageSprite.pushSprite();
  xTaskCreatePinnedToCore(
      webRunning, /* Function to implement the task */
      "Task1", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &Task1,  /* Task handle. */
      0); /* Core where the task should run */
  
  while(1)
  {
    M5.update(); //clears button press
    if( M5.BtnEXT.wasPressed()) //if back button clicked; exit back to menu
    {
      vTaskDelete(Task1);
      M5.M5Ink.clear();
      TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
      M5.update(); //clears button press
      break;
    }
  }
}

int menuPosition = 0;
void MenuPosition()
{
  // infinite scrolling
  if (menuPosition > 4) {menuPosition = 0;}
  if (menuPosition < 0) {menuPosition = 4;}
  // non-infinite scrolling
  //if (menuPosition > 4) {menuPosition = 4;}
  //if (menuPosition < 0) {menuPosition = 0;}

    M5.M5Ink.clear();
    TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
    M5.update(); //clears button press
    drawImageToSprite(0,0,&backgroundImage,&TimePageSprite);
    drawImageToSprite(0,0,&mainMenuImage,&TimePageSprite);
  
  switch(menuPosition)
  {
    case 0:
      drawImageToSprite(100,90,&startIcon,&TimePageSprite);
      TimePageSprite.drawString(0,75,"HELLO WORLD!",&AsciiFont8x16);
      TimePageSprite.drawString(0,150,"Use the scroll wheel",&AsciiFont8x16);
      TimePageSprite.drawString(0,165,"to navigate and select",&AsciiFont8x16);
      TimePageSprite.pushSprite();
      break;
    case 1:
      drawImageToSprite(100,90,&calendarIcon,&TimePageSprite);
      TimePageSprite.drawString(0,75,"DATE TIME",&AsciiFont8x16);
      TimePageSprite.drawString(0,150,"Gets the date and time",&AsciiFont8x16);
      TimePageSprite.drawString(0,165,"from the onboard RTC",&AsciiFont8x16);
      TimePageSprite.pushSprite();
      break;
    case 2:
      drawImageToSprite(100,90,&webIcon,&TimePageSprite);
      TimePageSprite.drawString(0,75,"WEB SERV",&AsciiFont8x16);
      TimePageSprite.drawString(0,150,"Launches your web app",&AsciiFont8x16);
      TimePageSprite.drawString(0,165,"to your local browser",&AsciiFont8x16);
      TimePageSprite.pushSprite();
      break;
    case 3:
      drawImageToSprite(100,90,&wifiIcon,&TimePageSprite);
      TimePageSprite.drawString(0,75,"WIFI SCAN",&AsciiFont8x16);
      TimePageSprite.drawString(0,150,"Search for nearby",&AsciiFont8x16);
      TimePageSprite.drawString(0,165,"wifi signals",&AsciiFont8x16);
      TimePageSprite.pushSprite();
      break;
    case 4:
      drawImageToSprite(100,90,&weatherIcon,&TimePageSprite);
      TimePageSprite.drawString(0,75,"WEATHER",&AsciiFont8x16);
      TimePageSprite.drawString(0,150,"Get the current",&AsciiFont8x16);
      TimePageSprite.drawString(0,165,"weather and temperature",&AsciiFont8x16);
      TimePageSprite.pushSprite();
      break;
    default:
    break;
  }
}

// MENU ----------------------------------------------------------------------
void MainMenu()
{
  // on startup the menu position is reset to 0;
  MenuPosition();
  while(1)
  {
    M5.update(); //clears button press
    if( M5.BtnEXT.wasPressed()) //if back button clicked; exit back to menu
    {
      break;
    }
    if (M5.BtnMID.wasPressed())
    { 
      switch(menuPosition)
      {
        case 0:
          break;
        case 1:
          M5.M5Ink.clear();
          TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
          flushTimePage();
          break;
        case 2:
          M5.M5Ink.clear();
          TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
          WebServerPage();
          break;
        case 3:
          M5.M5Ink.clear();
          TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
          WifiScanPage();
          break;
        case 4:
          break;
        default:
          break;
      }
      break;
    }
    if( M5.BtnUP.wasPressed())
    {
      menuPosition++;
      Serial.println("UP was pressed");
      MenuPosition();
    }
    if( M5.BtnDOWN.wasPressed())
    {
      menuPosition--;
      Serial.println("DOWN was pressed");
      MenuPosition();
    }
    if( M5.BtnPWR.wasPressed())
    {
        Serial.println("POWER was pressed");
        digitalWrite(LED_EXT_PIN,LOW);
        M5.shutdown();
    }
  }
}

void WifiScanPage()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.scanNetworks(true);

    //M5.M5Ink.clear();
    //TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
    drawImageToSprite(0,0,&wifiScanImage,&TimePageSprite);
    TimePageSprite.pushSprite();

    char wifiStrBuff[64];
    int flushCount = 1000;
    bool wifiReadyFlag = false;
    while(1)
    {
        int WifiRes = WiFi.scanComplete();
        if( WifiRes == -2 ){}
        else if( WifiRes == -1 ){}
        else if( WifiRes == 0 )
        {
            TimePageSprite.drawString(10,41,"no networks found",&AsciiFont8x16);
            break;
        }
        else
        {
            TimePageSprite.clear();
            drawImageToSprite(0,0,&wifiScanImage,&TimePageSprite);
            WifiRes = ( WifiRes > 8 ) ? 8 : WifiRes;
            for( int i = 0; i < WifiRes; i++ )
            {
                String SSIDStr = WiFi.SSID(i);
                if( SSIDStr.length() > 11 )
                {
                    SSIDStr = SSIDStr.substring(0,8);
                    SSIDStr += "...";
                }
                int32_t rssi = ( WiFi.RSSI(i) < -100 ) ? -100 : WiFi.RSSI(i);
                sprintf(wifiStrBuff,"SSID:%s",SSIDStr.c_str());
                TimePageSprite.drawString(10,50 + i * 18,wifiStrBuff,&AsciiFont8x16);

                sprintf(wifiStrBuff,"%02ddb",rssi);
                TimePageSprite.drawString(150,50 + i * 18,wifiStrBuff,&AsciiFont8x16);
            }
            wifiReadyFlag = true;
            WiFi.scanDelete();
            WiFi.scanNetworks(true);
        }
        delay(10);
        if(( flushCount > 1000 )&&( wifiReadyFlag == true ))
        {
            TimePageSprite.pushSprite();
            flushCount = 0;
        }
        flushCount ++;
        M5.update();
        if( M5.BtnPWR.wasPressed())
        {
            digitalWrite(LED_EXT_PIN,LOW);
            M5.shutdown();
        }
        if( M5.BtnEXT.wasPressed()) break;
    }
    M5.M5Ink.clear();
    TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
}

void flushTimePage()
{
    //M5.M5Ink.clear();
    //TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
    drawTimePage();
    while(1)
    {
        M5.rtc.GetTime(&RTCtime);
        if( minutes != RTCtime.Minutes )
        {
            M5.rtc.GetTime(&RTCtime);
            M5.rtc.GetDate(&RTCDate);
            
            if( RTCtime.Minutes % 10 == 0 )
            {
                M5.M5Ink.clear();
                TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
            }
            drawTime(&RTCtime);
            darwDate(&RTCDate);
            TimePageSprite.pushSprite();
            minutes = RTCtime.Minutes;
        }

        delay(10);
        M5.update();
        if( M5.BtnPWR.wasPressed())
        {
            digitalWrite(LED_EXT_PIN,LOW);
            M5.shutdown();
        }
        if( M5.BtnDOWN.wasPressed() || M5.BtnUP.wasPressed() || M5.BtnMID.wasPressed() || M5.BtnEXT.wasPressed())
        {
          M5.update();
          break;
        }
    }
    M5.M5Ink.clear();
    TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
}

void checkBatteryVoltage( bool powerDownFlag )
{
    float batVol = getBatVoltage();
    Serial.printf("Bat Voltage %.2f\r\n",batVol);

    if( batVol > 3.2 ) return;

    drawWarning("Battery voltage is low");
    if( powerDownFlag == true )
    {
         M5.shutdown();
    }
    while( 1 )
    {
        batVol = getBatVoltage();
        if( batVol > 3.2 ) return;
    }
}

void checkRTC()
{
    M5.rtc.GetTime(&RTCtime);
    if( RTCtime.Seconds == RTCTimeSave.Seconds )
    {
        drawWarning("RTC Error");
        while( 1 )
        {
            if( M5.BtnMID.wasPressed()) return;
            delay(10);
            M5.update();
        }
    }
}

void setup()
{
    M5.begin();
    M5.M5Ink.isInit();

    digitalWrite(LED_EXT_PIN,LOW);

    Serial.println(__TIME__);

    M5.rtc.GetTime(&RTCTimeSave);
    M5.update();
    if( M5.BtnMID.isPressed())
    {
        M5.Speaker.tone(261,200);
        delay(100);
        testMode = true;
        M5.Speaker.mute();
    }
    
    //M5.M5Ink.clear();
    M5.M5Ink.drawBuff((uint8_t *)image_CoreInkWelcome);
    delay(5000);
    M5.M5Ink.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );

    checkRTC();
    checkBatteryVoltage(false);

    TimePageSprite.creatSprite(0,0,200,200);
    TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
    if( testMode )
    {
        testPage();
    }
    //drawTimePage();
    // M5.Speaker.tone(261,200);
}

void loop()
{
  MainMenu();
  M5.update();
}
