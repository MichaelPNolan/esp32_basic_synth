/*
 * I'm taking the Multikey37 library from stand-alone synth and trying to bring the functionality of holding keys and a modifier to change synth module modes
 * Initial Goals:
 * - I want to change the keyboard midi mapping because I have 2 or more midi keyboards with different defaults
 * - I want to have arpeggiator playback
 * - I want to capture a buffer of midi keyboard data and replay it to see what might be causing it to get lost during other processing leaving hanging notes
 * - Maybe I want to create a kind of trigger of other commands that slow down the display processing if there are notes still to be played queued up - ie delay the use of wire
 *
 */
#ifdef KEYCAPTURE
#define KEYQUEUE_SIZE   16
#define USE_MODIFIER_KEYCOMMANDS

float    volumeParam = 1.0f;
float    semiModifier = 0.5f;
uint8_t  keyMod = 0; //the range of value that can be added to the input note number to determine played note
bool noNote = 0;  //something to stop notes from playing when using a modifier key and triggering commands? if i remember 


unsigned long loopCount = 0;
unsigned long startTime = millis();
String msg = "";


enum nState {Undef, NoteOn, NoteOff};
typedef struct keyboardMessage {  //the point of this is to receive keyboard note messages to use for commands or arpeggio mode refactored from the design of stand alone synth
  uint8_t note; //using char array to avoid all the BS of corrupted data hopefully for multi-core
  uint8_t velocity;
  nState noteState;
  bool  unread;
};

keyboardMessage* keyQueue[KEYQUEUE_SIZE];  //lifo queue but not sure I should be using malloc and pointers ... may cause that heap crash issue
uint8_t  nextkeyIndex;
// these "ready" variables are where we dequeue and process from with however many functions referring to the note that is ready, modifying the note, its velocity 
uint8_t readyNote = 0;
uint8_t readyVelocity = 0;
nState  readyState = Undef;
bool    commandState = false;

void setupKeyboardCapture() {
  
  setupKeyQueue();
}

void setupKeyQueue(){
  
  nextkeyIndex = 0; //means this is an empty queue - increment and decrement as queue items enqueued and dequeued
  for(int i = 0; i < (KEYQUEUE_SIZE); i++){
    keyQueue[i] = (keyboardMessage*) malloc( sizeof(keyQueue));
    keyQueue[i]->note = 0;
    keyQueue[i]->noteState = Undef;
    keyQueue[i]->unread = false;
  }
}

void enqueueNoteMessage(uint8_t *data){ 
 Serial.printf("enqueue: %02x %02x %02x\n", data[0], data[1], data[2]); //data[1] is the note number 
 if(nextkeyIndex < (KEYQUEUE_SIZE-1)){
    keyQueue[nextkeyIndex]->note = data[1];
    keyQueue[nextkeyIndex]->velocity = data[2];
    switch(data[0]){
      case 0x90:
        keyQueue[nextkeyIndex]->noteState = NoteOn;
        break;
      case 0x80:
        keyQueue[nextkeyIndex]->noteState = NoteOff;
        break;      
      default:
        keyQueue[nextkeyIndex]->noteState = Undef;
    } 
    keyQueue[nextkeyIndex]->unread = true;
  
    nextkeyIndex++;
    
  } else
    Serial.println("Queue over-run");
}

void dequeueNoteMessage(){
   if(nextkeyIndex > 0)
   {
      nextkeyIndex--; //safe because we already checked its greater than 0 - decremented because it's always ready for next enqueue so its always +1 to last queue written
      readyNote = keyQueue[nextkeyIndex]->note;
      readyVelocity = keyQueue[nextkeyIndex]->velocity;
      readyState = keyQueue[nextkeyIndex]->noteState;

      Serial.printf("Deq State:%01d Note:%03d Vel:%03d\n", readyState, readyNote, readyVelocity);

    } 
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
void serviceKeyQueue() {
  dequeueNoteMessage();
  #ifdef USE_MODIFIER_KEYCOMMANDS
  if(commandState && readyState != Undef){
    keyToCommand(readyNote);

  }
  #endif
  // let it also do the keyboard notes playing effect as well as triggering command in previous call/check
   //when the arpeggiator mode is on don't play
  if(noNote == 0)
    if (readyNote > 0){ //some possible/unused keys were mapped to 0 in the keyboard char array to prevent output
                 
      keyToNote(); // use the readyNote etc globals to process in this library
    }
}



void keyToNote(){
 uint8_t  keyUS = readyNote; //semiModifier is set in ADC from one of the pots to a 1.0f value and keyMod is whatever you want as adjust range
  switch (readyState) {  // Report active keystate based on typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
      case NoteOn:
       // if(checkBankValue() != 4)//(!checkArpeggiator())
      //    Synth_NoteOn(0, keyUS, readyVelocity); //unchecked if type works as a note - was defaulted to 1.0f for velocity
      //  else
       //   Arp_NoteOn(keyUS);
        Serial.println("trigger noteOn");
        break;
      case NoteOff:
       // if(checkArpeggiator()) // this idea - was too slow so I moved the Arp_NoteOn back up to PRESSED status
         // Arp_NoteOn(keyUS);
         Serial.println("trigger noteOff");
        break;

  }
  readyState = Undef;
  #ifdef DISPLAY_1306;
  //enqueueDisplayNote(15,1,readyNote,true);
 
  #endif
}

void keyToArpMap(){ //aparently defunct to remove soon
  uint8_t keyUS = readyNote*semiModifier; //semiModifier is set in ADC from one of the pots to a 1.0f value and keyMod is whatever you want as adjust range
  switch (readyState) {  // Report active keystate based on typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
      case NoteOn:
       //   Arp_NoteOn(keyUS); //unchecked if type works as a note - was defaulted to 1.0f for velocity
          Serial.println("trigger Arp noteOn");
          break;
      case NoteOff:
       //   Arp_NoteOff(keyUS);
          Serial.println("trigger Arp noteOff");
          break;
      case Undef:
          break;
  }
}

void keyToCommand(uint8_t  keyCom){ //assume called after the modifier button was held and a key was released
  
  switch (keyCom) { 
      case 20:
          msg = "B0: vel ADSR"; //use like mousebutton up ... after release trigger
         // setBank(0);
          break;
      case 21:
          msg = "B1: filter ADSR"; //use like mousebutton up ... after release trigger
        //  setBank(1);
          break;
      case 22:
          msg = "B2: filter main"; //use like mousebutton up ... after release trigger
         // setBank(2);
          break;
      case 23:
          msg = "B3: delay"; //use like mousebutton up ... after release trigger
        //  setBank(3);
          break;
      case 24:
          msg = "B4: Arpeggiator";
        //  setBank(4);
          break;
      case 25:
          msg = "B5: Permormance";
        //  setBank(5);
          break;
      case 56:
          if(1){ //if(!checkArpHold()){
          //  arpAllOff();  //i put this here because
            msg = "All Arp Notes off";
          } else {
         //   delTailSeq();
            msg = "RemoveTail Note";
          }
          break;
  }
  noNote = 1;
  #ifdef DISPLAY_1306
  Serial.println(msg);
  //miniScreenString(7,1,msg,HIGH);  //"ArpON+KbdCom:"+String(keyCom)
  
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
#endif
