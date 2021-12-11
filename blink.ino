/*
 * this file includes a simple blink task implementation
 *
 * Author: Marcel Licence
 
#ifdef BLINK_LED_PIN
inline
void Blink_Setup(void)
{
    pinMode(BLINK_LED_PIN, OUTPUT);
}


inline
void Blink_Process(void)
{
    static bool ledOn = true;
    if (ledOn)
    {
        digitalWrite(BLINK_LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    }
    else
    {
        digitalWrite(BLINK_LED_PIN, LOW);    // turn the LED off
    }
    ledOn = !ledOn;
}
#endif
*/

const char sixteen[17] = "----------------";
static bool ledOn = false;

uint8_t  stepNum;
inline
void Blink_Setup(void)
{
    pinMode(BLINK_LED_PIN, OUTPUT);
    pinMode(SWITCH2_LED_PIN, OUTPUT);
    digitalWrite(SWITCH2_LED_PIN, false);
    digitalWrite(BLINK_LED_PIN, ledOn);
    stepNum = 0;
}


inline
void Blink_Process(void) //I'm using the blink as a tempo meter and I'll display that 
{
    
    if (ledOn)
    {
        digitalWrite(BLINK_LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        if(checkBankValue() == 1)  //bank 4 is arpeggiator - there you can draw the moving pulse message 'pulse string'
          miniScreenString(8,1,pulseString(),true);


    }
    else
    {
        digitalWrite(BLINK_LED_PIN, LOW);    // turn the LED off
        if(checkBankValue() == 1){
          //miniScreenBarSize(8,0.0f);
          miniScreenString(8,1,pulseString(),true);
        }

    }
    ledOn = !ledOn;
}

char* pulseString(){
  String cursorString,dashProgress,dashesAfter;
  uint8_t arpNotes = readHeldNotes();
  int cursorLen;
  stepNum++;
  if(arpNotes > 0)
    cursorString = String(arpNotes);
  else
    cursorString = ">";
  cursorLen = cursorString.length();
  
  if(stepNum == 16)
    stepNum=0;
  dashProgress = String(sixteen);
  dashProgress.remove(stepNum+cursorLen);
  dashProgress.concat(cursorString);
  dashesAfter = String(sixteen);
  dashesAfter.remove(15 - stepNum);
  dashProgress.concat(dashesAfter);
    
  return string2char(dashProgress);
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}
