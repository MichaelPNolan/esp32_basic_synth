/*
 * I'm taking the Multikey37 library from stand-alone synth and trying to bring the functionality of holding keys and a modifier to change synth module modes
 * Initial Goals:
 * - I want to change the keyboard midi mapping because I have 2 or more midi keyboards with different defaults
 * - I want to have arpeggiator playback
 * - I want to capture a buffer of midi keyboard data and replay it to see what might be causing it to get lost during other processing leaving hanging notes
 * - Maybe I want to create a kind of trigger of other commands that slow down the display processing if there are notes still to be played queued up - ie delay the use of wire
 *
 */

#define KEYQUEUE_SIZE   20
#define USE_MODIFIER_KEYCOMMANDS
#define NORM127MUL  0.007874f

float    volumeParam = 1.0f;
float    semiModifier; //initialize in setupKeyboardCapture update via Custom_SetParam(..)
int  keyMod = 0; //the range of value that can be added to the input note number to determine played note
bool noNote = 0;  //something to stop notes from playing when using a modifier key and triggering commands? if i remember 
bool captureMode = false;
bool queueLock = false;

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

 keyboardMessage keyQueue[KEYQUEUE_SIZE];  //lifo queue but not sure I should be using malloc and pointers ... may cause that heap crash issue
uint8_t  nextkeyIndex;
// these "ready" variables are where we dequeue and process from with however many functions referring to the note that is ready, modifying the note, its velocity 
uint8_t readyNote = 0;
float  readyVelocity = 0;
nState  readyState = Undef;
bool    commandState = false;

void setupKeyboardCapture() {
  
  setupKeyQueue();
  keyboardSetSemiModifier(0.5f);
}

bool getCaptureMode(){
  return captureMode;
}
bool setCaptureMode(bool setC){
  captureMode = setC;
}

uint8_t getKeyIndex(){
  return nextkeyIndex;
}
void setupKeyQueue(){
  
  nextkeyIndex = 0; //means this is an empty queue - increment and decrement as queue items enqueued and dequeued
  for(int i = 0; i < (KEYQUEUE_SIZE); i++){
 //   keyQueue[i] = (keyboardMessage*) malloc( sizeof(keyQueue));
  keyQueue[i].note = 0;
  keyQueue[i].noteState = Undef;
  keyQueue[i].unread = false;
  }
}

 void enqueueNoteMessage(uint8_t *data){ 
 //Serial.printf("enqueue: %02x %02x %02x\n", data[0], data[1], data[2]); //data[1] is the note number 
 queueLock = true;
 if(nextkeyIndex < (KEYQUEUE_SIZE-1)){
    keyQueue[nextkeyIndex].note = data[1];
    keyQueue[nextkeyIndex].velocity = data[2]; 

    switch(data[0] & 0xF0){
      case 0x90:    
        if(data[2] < 2)
          Serial.println("quiet");
        if (data[2] > 0){
          keyQueue[nextkeyIndex].noteState = NoteOn;
        } 
        else
        {
          keyQueue[nextkeyIndex].noteState = NoteOff;
        }
        break;
      case 0x80:
        keyQueue[nextkeyIndex].noteState = NoteOff;
        break;      
      default:
        keyQueue[nextkeyIndex].noteState = Undef;
    } 
    keyQueue[nextkeyIndex].unread = true;
  
    nextkeyIndex++;
    
  } else {
    Serial.println("kQueue over-run"); //but we won't let a noteOff message fail 
    if((data[0] & 0xF0) == 0x80)
      Synth_NoteOff(0, data[1]);
  }
  queueLock = false;
}

inline void dequeueNoteMessage(){
   long count = 0;
   while(queueLock)
     count++;
   if(nextkeyIndex > 0)
   { 
      
      nextkeyIndex--; //safe because we already checked its greater than 0 - decremented because it's always ready for next enqueue so its always +1 to last queue written
      if(keyQueue[nextkeyIndex].unread){
        readyNote = keyQueue[nextkeyIndex].note;
        readyVelocity =  pow(2, ((keyQueue[nextkeyIndex].velocity * NORM127MUL) - 1.0f) * 6); // formula taken from midiInterface because velocity is converted to a float value and "pow(2, ((vel * NORM127MUL) - 1.0f) * 6)"
        readyState = keyQueue[nextkeyIndex].noteState;
        keyQueue[nextkeyIndex].unread = false;
      }
      //Serial.printf("Deq State:%01d Note:%03d Vel:%03d\n", readyState, readyNote, readyVelocity);

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
  keyMod = 80*semiModifier - 40; 
  arpAllOff();
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
  if(noNote == 0 && captureMode)
    if (readyNote > 0 && readyNote < 127){ //some possible/unused keys were mapped to 0 in the keyboard char array to prevent output           
      keyToNote(); // use the readyNote etc globals to process in this library
      readyNote = 0;
    }
}



inline void keyToNote(){
  switch (readyState) {  // Report active keystate based on typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
      case NoteOn:
        if(captureMode)
          if(checkBankValue() != 1){ 
             Synth_NoteOn(0, readyNote+keyMod, readyVelocity); //unchecked if type works as a note - was defaulted to 1.0f for velocity
             //Serial.println("note ON:"+String(keyUS));
          } else {
            Arp_NoteOn(readyNote+keyMod);
           // Serial.println("trigger noteOn");
          }
        break;
      case NoteOff:
         if(captureMode){
           if(checkBankValue() == 1){ 
             Arp_NoteOff(readyNote+keyMod);
           }
           Synth_NoteOff(0, readyNote+keyMod);
         }
       // if(checkArpeggiator()) // this idea - was too slow so I moved the Arp_NoteOn back up to PRESSED status
         // Arp_NoteOn(keyUS);
        // Serial.println("trigger noteOff");
        break;

  }
  readyState = Undef;
//  #ifdef DISPLAY_1306;
  //enqueueDisplayNote(15,1,readyNote,true);
 
//  #endif
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
  
  #endif
}

//Key_NoteOn is mapped in to pass on to Synth_NoteOn or not if we want to use a different play mode
//it is mapped in Z_config struct midiMapping_s midiMapping = ..
inline void Key_NoteOn(uint8_t ch, uint8_t note, float vel){
  if(!captureMode)
    Synth_NoteOn(ch,note,vel);
  
}

//Key_NoteOff is mapped in to pass on to Synth_NoteOff or not if we want to use a different play mode
//it is mapped in Z_config struct midiMapping_s midiMapping = ..
inline void Key_NoteOff(uint8_t ch, uint8_t note){
  if(!captureMode)
    Synth_NoteOff(ch,note);
  
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
