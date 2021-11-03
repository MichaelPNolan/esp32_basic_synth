/*
 * I'm taking the Multikey37 library from stand-alone synth and trying to bring the functionality of holding keys and a modifier to change synth module modes
 * Initial Goals:
 * - I want to change the keyboard midi mapping because I have 2 or more midi keyboards with different defaults
 * - I want to have arpeggiator playback
 * - I want to capture a buffer of midi keyboard data and replay it to see what might be causing it to get lost during other processing leaving hanging notes
 * - Maybe I want to create a kind of trigger of other commands that slow down the display processing if there are notes still to be played queued up - ie delay the use of wire
 *
 */



float volumeParam = 1.0f;
float semiModifier = 0.5f;
bool noNote = 0;


unsigned long loopCount = 0;
unsigned long startTime = millis();
String msg = "";
uint8_t  keyMod = 40; //the range of value that can be added to the input note number to determine played note

void setupKeyboard() {
    //for USB serial switching boards
  Wire.begin( );
  kpd.begin( );                // now does not starts wire library
  kpd.setDebounceTime(1);
  //scan();
  
}

// to receive changes from controller adc input see adc_module
void keyboardSetVolume(float value)
{
  volumeParam = value;
}

float getKeyboardVolume(){
  return volumeParam;
}

void keyboardSetSemiModifier(float value)
{
  semiModifier = value;
}
// for more functionality - we have all the keyboard keys to use - so we check the modifier bank button
void serviceKeyboardMatrix() {
  //const int myLIST_MAX = LIST_MAX - 2; //42
  // Fills kpd.key[ ] array with up-to 10 active keys.
  // Returns true if there are ANY active keys.
 
  if (kpd.getKeys())
  {
    for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.  LIST_MAX
    {
      if ( kpd.key[i].stateChanged )   // Only find keys that have changed state.
      {
        #ifdef USE_MODIFIER_KEYCOMMANDS
        if(!commandState() && kpd.key[i].kstate == PRESSED){
          keyToCommand(uint8_t(kpd.key[i].kchar));
          //arpAllOff();  //test code when it wasn't working well
          
        }
        #endif
        // let it also do the keyboard notes playing effect as well as triggering command in previous call/check
         //when the arpeggiator mode is on don't play
        if(noNote == 0)
          if (uint8_t(kpd.key[i].kchar) > 0){ //some possible/unused keys were mapped to 0 in the keyboard char array to prevent output
                       
            keyToNote(uint8_t(kpd.key[i].kchar),i); // this may fix my bad logic - move arpeggiator test into keytoNote
            
          }
        

      noNote = 0; // reset flag to enable notes - flag was set because of a modifier keyboard command
     }
   }
  }  // End loop
}



void keyToNote(uint8_t  keyUS, int i){
  keyUS += keyMod*semiModifier; //semiModifier is set in ADC from one of the pots to a 1.0f value and keyMod is whatever you want as adjust range
  switch (kpd.key[i].kstate) {  // Report active keystate based on typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
      case PRESSED:
          //msg = " PRESSED.";
          if(checkBankValue() != 4)//(!checkArpeggiator())
            Synth_NoteOn(0, keyUS, volumeParam); //unchecked if type works as a note - was defaulted to 1.0f for velocity
          else
            Arp_NoteOn(keyUS);
          break;
      case HOLD:
          //msg = " HOLD.";
         // if(checkArpeggiator()) // this idea - was too slow so I moved the Arp_NoteOn back up to PRESSED status
           // Arp_NoteOn(keyUS);
          break;
      case RELEASED:
          //msg = " RELEASED.";
          if(checkBankValue() != 4)//(checkArpeggiator())
            Arp_NoteOff(keyUS);
          Synth_NoteOff(0, keyUS);
          break;
      case IDLE:    // there are times when idle needs to be calling noteOff because you had notes on hold in arpeggiator
          //msg = " IDLE.";
          //if(checkArpeggiator())
            Arp_NoteOff(keyUS);
  }
  #ifdef DISPLAY_1306
  miniScreenString(6,1,"N#:"+String(keyUS),HIGH);
  
  #else
  Serial.print("Key :/");//+String(LIST_MAX));
  Serial.print(uint8_t(kpd.key[i].kchar));
  Serial.println(msg);
  #endif
}
//defunct
void keyToArpMap(uint8_t  keyUS, int i){
  keyUS += keyMod*semiModifier; //semiModifier is set in ADC from one of the pots to a 1.0f value and keyMod is whatever you want as adjust range
  switch (kpd.key[i].kstate) {  // Report active keystate based on typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
      case PRESSED:
          //msg = " PRESSED.";
          Arp_NoteOn(keyUS); //unchecked if type works as a note - was defaulted to 1.0f for velocity
          break;
      case HOLD:
          //msg = " HOLD.";
          break;
      case RELEASED:
          //msg = " RELEASED.";
          Arp_NoteOff(keyUS);
          break;
      case IDLE:
          msg = " IDLE.";
  }
}

void keyToCommand(uint8_t  keyCom){ //assume called after the modifier button was held and a key was released
  
  switch (keyCom) { 
      case 20:
          msg = "B0: vel ADSR"; //use like mousebutton up ... after release trigger
          setBank(0);
          break;
      case 21:
          msg = "B1: filter ADSR"; //use like mousebutton up ... after release trigger
          setBank(1);
          break;
      case 22:
          msg = "B2: filter main"; //use like mousebutton up ... after release trigger
          setBank(2);
          break;
      case 23:
          msg = "B3: delay"; //use like mousebutton up ... after release trigger
          setBank(3);
          break;
      case 24:
          msg = "B4: Arpeggiator";
          setBank(4);
          break;
      case 25:
          msg = "B5: Permormance";
          setBank(5);
          break;
      case 56:
          if(!checkArpHold()){
            arpAllOff();  //i put this here because
            msg = "All Arp Notes off";
          } else {
            delTailSeq();
            msg = "RemoveTail Note";
          }
          break;
  }
  noNote = 1;
  #ifdef DISPLAY_1306

  miniScreenString(7,1,msg,HIGH);  //"ArpON+KbdCom:"+String(keyCom)
  
  #else
  Serial.print("Command :/");//
  Serial.print(uint8_t(kpd.key[i].kchar));
  Serial.println(msg);
  #endif
}

void scan() { //spi bus scan for devices
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }
  delay(5000);          
}
