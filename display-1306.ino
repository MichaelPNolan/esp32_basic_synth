/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x32 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/


#ifdef DISPLAY_1306
//#include <SPI.h>
//#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64
// OLED display height, in pixels
#define ROWS         8   // 8 pix high gives 8 * 8 for SCREEN of 64 high which is the 0.96" variation used for basic_synthv4
#define COLUMNS      2 
#define ZONES        16  //simply ROWS by COLUMNS 
#define QUEUE_SIZE   16
#define OLED_RESET      -1// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



bool   wantsDisplayRefresh;  
//these are all arrays of 8 for the 4 rows and 2 columns that fit with text size 1 as set in init
char      zoneCharArray[ZONES][12]; //Room for 8 char (maybe more) per zone (zone)
String    zoneStrings[ZONES];
uint8_t   zoneColor[ZONES]; //monochrome WHITE or BLACK
uint8_t   zoneBarSize[ZONES] = {24,30,32,40,44,48,52,60,64,64,64,64,64,0,0};  //could be a rectangle of 64 pix width 8 pix height
bool      zoneRedraw[ZONES];

typedef struct message {  //the point of this is to receive text but process it from the correct core in multiprocessing since this "display" instance keeps failing or crashing
  char text[12]; //using char array to avoid all the BS of corrupted data hopefully for multi-core
  uint8_t textLen, zoneColor, zone;
  bool  unread;
};


message* messageQueue[QUEUE_SIZE];  //lifo queue but not sure I should be using malloc and pointers ... may cause that heap crash issue
uint8_t  nextMessage;

volatile bool displayReady;
void setup1306() {
  Wire.begin();
  Wire.setClock(10000);
  bool setupDisplayOK = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);  //SSD1306_EXTERNALVCC or SSD1306_SWITCHCAPVCC
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!setupDisplayOK) {
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  } else {

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  
 
  display.display();
  delay(500);
  //display.invertDisplay(false);
  display.clearDisplay();
  //testdrawline(); 
  
  
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
   for(int i = 0; i<ZONES; i++){
      zoneRedraw[i] = false;
    }  

    wantsDisplayRefresh = true;
    Serial.printf("Oled Init started successfully on");
    Serial.println(xPortGetCoreID());
    displayReady = true;
  }

}
void setupDisplayMessageQueue(){
  
  nextMessage = 0; //means this is an empty queue - increment and decrement as queue items enqueued and dequeued
  for(int i = 0; i < (QUEUE_SIZE+1); i++){
    messageQueue[i] = (message*) malloc( sizeof(message));
    //scanf("           ", messageQueue[i]->text);
    messageQueue[i]->text[0] = 0;
    messageQueue[i]->textLen = 0;
    messageQueue[i]->zoneColor = 0;
    messageQueue[i]->zone = 0;
    messageQueue[i]->unread = false;
  }
    miniScreenString(0,1,"VolAttck",false);
    miniScreenString(1,1,"VolDecay",false);
    miniScreenString(2,1,"VolSus",false);
    miniScreenString(3,1,"VolRleas",false);
    miniScreenString(4,1,"FiltEnv_A",false);
    miniScreenString(5,1,"FiltEnv_D",false);
    miniScreenString(6,1,"FiltEnv_S",false);
    miniScreenString(7,1,"FiltEnv_R",false);
    miniScreenString(8,1,"DelyLngth",false);
    miniScreenString(9,1,"DelyMixLv",false);

    miniScreenString(14,1,"Love Supi",false);
}
void enqueueDisplayNote(uint8_t zone, uint8_t c,uint8_t note,bool refresh){
 if(refresh) wantsDisplayRefresh = true; 
 if(nextMessage < (QUEUE_SIZE-1)){
    messageQueue[nextMessage]->text[0]='N';
    messageQueue[nextMessage]->text[1]='#';
    messageQueue[nextMessage]->text[2]=':';
    messageQueue[nextMessage]->text[3]=' ';
    messageQueue[nextMessage]->text[4]=note/10+48;
    messageQueue[nextMessage]->text[5]=note%10+48;
    messageQueue[nextMessage]->text[6]=0;
    messageQueue[nextMessage]->zoneColor = c;
    messageQueue[nextMessage]->zone = zone;
    messageQueue[nextMessage]->unread = true;
  
    nextMessage++;
    
  } else
    Serial.println("Queue over-run");
}
void enqueueDisplayMessage( uint8_t zone, uint8_t c,char* s){
    if(nextMessage < (QUEUE_SIZE-1)){
     //int8_t messageStringLength = strlen(s);
      //if (messageStringLength > 11)
    //    messageStringLength = 11;
     for(int i = 0; i < 11; i++){
       messageQueue[nextMessage]->text[i] = s[i];    
     }  
      messageQueue[nextMessage]->text[11] = 0; //tested Serial.print(messageQueue[nextMessage]->text); works as a char* type string
      //strcpy(messageQueue[nextMessage]->text,s);
      messageQueue[nextMessage]->zoneColor = c;
      messageQueue[nextMessage]->zone = zone;
      messageQueue[nextMessage]->unread = true;
      nextMessage++;
    } else
      Serial.println("Queue over-run");
      
}

void dequeueDisplayMessage(){
   if(nextMessage > 0)
   {
      nextMessage--; //safe because we already checked its greater than 0 - now we want to get the message, so we decrement to prev
      //Serial.print(messageQueue[nextMessage]->text);
      //if(messageQueue[nextMessage]->unread){
        miniScreenLoad(messageQueue[nextMessage]->zone,messageQueue[nextMessage]->zoneColor,messageQueue[nextMessage]->text,true);
        messageQueue[nextMessage]->text[0] = 0;
        messageQueue[nextMessage]->textLen = 0;
        messageQueue[nextMessage]->zoneColor = 0;
        messageQueue[nextMessage]->zone = 0;
        messageQueue[nextMessage]->unread = false;
        //Serial.println(messageQueue[nextMessage]->text);
     // }
    }
}


void miniScreenString(uint8_t zone, uint8_t c, char* s,bool refresh){
  if(zone > (ZONES-1)) return;  //don't try to write or do anything if this is in error
  enqueueDisplayMessage(zone,c,s);
  zoneRedraw[zone] = true;
  if(refresh) wantsDisplayRefresh = true; 
}

void miniScreenLoad(uint8_t zone, uint8_t c, char* s,bool refresh){
  if(zone > (ZONES-1)) return;  //don't try to write or do anything if this is in error
  //zoneStrings[zone] = String(s); //comment out now we switch to char* or array type
  uint8_t messageStringLength = 0;//strlen(s); //alternative coding for not string use theory of fixing lib to work in multiprocessing via core0

  while((s[messageStringLength]!=0) && (messageStringLength<12)){ //calculate string length
    messageStringLength++;
  }
  //strcpy(zoneCharArray[zone],s);
  for(int i = 0; i < messageStringLength; i++){
    zoneCharArray[zone][i] = s[i];
  } 
  zoneCharArray[zone][messageStringLength]=0; //terminate string
  //Serial.print(zoneCharArray[zone]);
  //Serial.println(s);
  zoneColor[zone] = c;
  zoneRedraw[zone] = true;
  if(refresh) wantsDisplayRefresh = true;  //miniScreenRedraw(zone,0) was called in earlier version but we had garbage being written on one version of ESP32 due to different cores
}


//prepares message but doesn't trigger display.display() because that messes with processing samples
//for zones on text size one they are 8 high (32 pix screen height) right half starts at 64
void miniScreenRedraw(uint8_t zone, bool screenRefresh){
  uint8_t row, col;
  if(screenRefresh) {
  display.clearDisplay();
    for(int i = 0; i<ZONES; i++){
       if(strlen(zoneCharArray[i]) > 0){
         if(i>1) row = i / 2; else row = 0;
         col = i % 2;
         display.setCursor(70*col,8*row);
    
         if(zoneColor[i])
            display.setTextColor(WHITE,BLACK);
         else
            display.setTextColor(BLACK,WHITE);
         
         if(zoneBarSize[i] > 0){
            miniScreenBarDraw(i);
         } else
         display.println(zoneCharArray[zone]); //display.println(zoneStrings[i]);
       }
    }
    wantsDisplayRefresh = HIGH;
  } else {
    if(strlen(zoneCharArray[zone]) > 0){
       if(zone>1) row = zone / 2; else row = 0;
       col = zone % 2;
       display.setCursor(70*col,8*row);
      // display.println("         "); //hopefully equivalent to clearDisplay for a single zone at this cursor
      //  display.setCursor(64*col,8*row);
       if(zoneColor[zone])
          display.setTextColor(WHITE,BLACK);
       else
          display.setTextColor(BLACK,WHITE);
       
       if(zoneBarSize[zone] > 0){ // either draw text with bar fill overlay look or if 0 just text only
          miniScreenBarDraw(zone);
       } else
       display.println(zoneCharArray[zone]);   //display.println(zoneStrings[zone]); previously changed to try and use char array strings zero terminated
       //Serial.println(zoneCharArray[zone]);
     }
     wantsDisplayRefresh = HIGH;
  }
  
  
  //  display.display(); is called by displayRefresh on core0task loop but this loading the display memory can be called any time
}
void miniScreenBarSize(uint8_t zone, float param){ //optimise to just update one zone for parameter adjusting to do less processing
  
  zoneBarSize[zone] = 64.0f * param;
  miniScreenRedraw(zone, 0); // set this up to just update one zone
}

void miniScreenBarDraw(uint8_t zone){
   int x,y;
   bool stringTerm = false;
   uint8_t col= (zone % 2); //ie 0 or 1
   uint8_t barLen;
   if(zone>0) y = (zone / 2) * 8; else y = 0;
   x = col *70;  //increased to 70 pix to leave a gap in center between columns was 64 or half way
   barLen = zoneBarSize[zone];
   if(barLen>63-(6*col))
      barLen = 63-(6*col);
   display.drawRect(x,y, barLen, 7, SSD1306_WHITE); 
   display.setCursor(x,y);
   display.setTextColor(BLACK,WHITE);
   
   uint8_t fitInBar = zoneBarSize[zone]/6; //numb of characters to fit in bar
   uint8_t stringL = strlen(zoneCharArray[zone]);
   for(int i=0; i<fitInBar; i++)
     if(zoneCharArray[zone][i]>0 && !stringTerm) //not terminator
        display.print(zoneCharArray[zone][i]);
     else{
        display.print(" ");
        stringTerm = true;
     }
   //display.print(zoneStrings[zone].substring(0,fitInBar)); //reprint the text  myString.substring(from, to)
   display.setTextColor(WHITE,BLACK);
   for(int i=fitInBar; i<(10-col); i++) //if col 1 then there is less space to print
     if(zoneCharArray[zone][i]>0 && !stringTerm) //not terminator
        display.print(zoneCharArray[zone][i]);
     else{
        if(fitInBar<stringL)
           display.print(" ");
        stringTerm = true;
     }
     
   //display.print(zoneStrings[zone].substring(fitInBar));  
  //  display.display(); is called by displayRefresh on core0task loop but this loading the display memory can be called any time
}



void displayRefresh()
{
  
  if(wantsDisplayRefresh){ 
    wantsDisplayRefresh = false;
    //Serial.print(String(nextMessage));
    while(nextMessage>0){
       dequeueDisplayMessage();
       //Serial.printf("ESP.getFreeHeap() %d\n", ESP.getFreeHeap());//Serial.println(random(20));
    }
    
    for(int zone = 0; zone < ZONES; zone++){
      if(zoneRedraw[zone]){
        miniScreenRedraw(zone,0);
        zoneRedraw[zone] = false;
      }
    }
    displayReady = true;
    
   display.display();
  
  }
}

void invokeDisplay(){ //i2s_write_sample_32ch2 is the current location
  //only called from other core0 perhaps - that was idea anyway
  if(displayReady){
    displayReady = false;    

    display.display();
  }
     
}


void testdrawline() {
  int16_t i;

  display.clearDisplay(); // Clear display buffer

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, SSD1306_WHITE);
    display.display(); // Update screen with each newly-drawn line
    delay(1);
  }
  for(i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();
/*
  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  for(i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000); // Pause for 2 seconds */
}

void testdrawrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, SSD1306_WHITE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}

void testfillrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=3) {
    // The INVERSE color is used so rectangles alternate white/black
    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, SSD1306_INVERSE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}

void testdrawcircle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfillcircle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=3) {
    // The INVERSE color is used so circles alternate white/black
    display.fillCircle(display.width() / 2, display.height() / 2, i, SSD1306_INVERSE);
    display.display(); // Update screen with each newly-drawn circle
    delay(1);
  }

  delay(2000);
}

void testdrawroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfillroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    // The INVERSE color is used so round-rects alternate white/black
    display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, SSD1306_INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawtriangle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=5) {
    display.drawTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfilltriangle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=5) {
    // The INVERSE color is used so triangles alternate white/black
    display.fillTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, SSD1306_INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawchar(void) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  for(int16_t i=0; i<256; i++) {
    if(i == '\n') display.write(' ');
    else          display.write(i);
  }

  display.display();
  delay(2000);
}

void testdrawstyles(void) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Hello, world!"));

  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(3.141592);

  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.print(F("0x")); display.println(0xDEADBEEF, HEX);

  display.display();
  delay(2000);
}

void testscrolltext(void) {
  display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(F("scroll"));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}


#endif
