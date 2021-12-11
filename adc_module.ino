/*
 * This module is run adc with a multiplexer
 * tested with ESP32 Audio Kit V2.2
 * Only tested with 8 inputs
 *
 * Define your adc mapping in the lookup table
 *
 * Author: Marcel Licence
 *
 * Reference: https://youtu.be/l8GrNxElRkc
 */

/* Notes by Michael - hack version of this 
 * Requires Boardmanager 1.0.4 or earlier of ESP32
 * doing math
 * Integrate display calls and how we pick up changes in parameters ... potentiall relocate things out of adc_module from esp32_alone_synth and build up the display-1306 with calls 
 * that can be developed in the code where parameters are changed and we want to reflect that on the screen.
 * From Version 5 of this basic synth I started to integrate arpeggiator from StandaloneSynth37 project
 * that uses analogue controls rather than midi control messages - so here we have to make a mixture for the case that there are no analogue controls
 * wired and mapped to various parameters.
 * So that project uses banks in the display-1306 - this project currently has a bigger resolution but we need to design banks
 * and have arp mode, bank mode calls as that project has - one bank is the arpeggiator bank with modes and tempo and a display that changes using the blink library
*/
// The other project - the stand along keyboard has additional parameter definitions as seen in z_config/easySynth libraries ... eg #define SYNTH_PARAM_VEL_ENV_ATTACK  0
// that introduces some kind of midiControl remapping interception because there is struct midiControllerMapping tunaWorlde[] = that uses midi to map parameters of the synth module
// in theory what would be best is to map them using banks and programming that controller to switch bank and call functions in the arp library the same way
// that the delay library is called  { 0x0, 0x14, "R9", NULL, Delay_SetLength, 2},  because of inline void Midi_ControlChange(uint8_t channel, uint8_t data1, uint8_t data2) mapping those function calls and parameters

#define CONTROL_PARAM_MAX_VOL  14
// The parameters set in z_config for the arpeggiator and performance bank 1
#define CONTROL_SEMITONES 18  // keyboard note modifier
// Arpeggiator parameters  
#define ARP_STATE 20
#define ARP_VARIATION 21
#define ARP_HOLD 22
#define ARP_NOTE_LEN 23
#define ARP_BPM 24
// Performance Parameters
#define PERF_SEMITONES 25   //master semitone tuning
#define PERF_SWING 26       //adjustment for beat spacing to create swing effect (triple skip like)
#define PERF_LFO_PARAM 27   //assign a parameter to sweep with LFO  (low freq oscillator is (usually between .1 and 250Hz) 
#define PERF_LFO_PERIOD 28  //Frequency of LFO wave
#define PERF_BEAT_SKIP 29   //put rests in some pattern into arpeggion - set of variations?


#define upButton 34 // for use with a single POT to select which parameter
#define downButton 35 // ditto
bool upButtonState, downButtonState, lastUpButtonState, lastDownButtonState;
unsigned long lastUBDebounceTime,lastDBDebounceTime;
unsigned long debounceDelay = 50; 

struct adc_to_midi_s
{
    uint8_t ch;
    uint8_t cc;
};

int  analogueParamSet = 0;
int  waveformParamSet = 0; // became unused in v5 when I started remapping for midi 
uint8_t  bankParamSet = 0; //using button to switch banks

static float adcSingle,adcSingleAve; // added by Michael for use when you don't have analogue multiplexer
extern float adcSetpoint=0;
extern struct adc_to_midi_s adcToMidiLookUp[]; /* definition in z_config.ino */

uint8_t lastSendVal[ADC_TO_MIDI_LOOKUP_SIZE];  /* define ADC_TO_MIDI_LOOKUP_SIZE in top level file */
#define ADC_INVERT
#define ADC_THRESHOLD       (1.0f/200.0f)
#define ADC_OVERSAMPLING    2048


//#define ADC_DYNAMIC_RANGE
//#define ADC_DEBUG_CHANNEL0_DATA

static float adcChannelValue[ADC_INPUTS];


void AdcMul_Init(void)
{
    for (int i = 0; i < ADC_INPUTS; i++)
    {
        adcChannelValue[i] = 0.5f;
    }

    memset(lastSendVal, 0xFF, sizeof(lastSendVal));

   // analogReadResolution(10);
   // analogSetAttenuation(ADC_11db);

    //analogSetCycles(1);
    //analogSetClockDiv(1);

    //adcAttachPin(ADC_MUL_SIG_PIN);

    pinMode(ADC_MUL_S0_PIN, OUTPUT);
#if ADC_INPUTS > 2
    pinMode(ADC_MUL_S1_PIN, OUTPUT);
#endif
#if ADC_INPUTS > 4
    pinMode(ADC_MUL_S2_PIN, OUTPUT);
#endif
#if ADC_INPUTS > 8
    pinMode(ADC_MUL_S3_PIN, OUTPUT);
#endif
}

void AdcMul_Process(void)
{
    static float readAccu = 0;
    static float adcMin = 0;//4000;
    static float adcMax = 420453;//410000;

    for (int j = 0; j < ADC_INPUTS; j++)
    {
        digitalWrite(ADC_MUL_S0_PIN, ((j & (1 << 0)) > 0) ? HIGH : LOW);
#if ADC_INPUTS > 2
        digitalWrite(ADC_MUL_S1_PIN, ((j & (1 << 1)) > 0) ? HIGH : LOW);
#endif
#if ADC_INPUTS > 4
        digitalWrite(ADC_MUL_S2_PIN, ((j & (1 << 2)) > 0) ? HIGH : LOW);
#endif
#if ADC_INPUTS > 8
        digitalWrite(ADC_MUL_S3_PIN, ((j & (1 << 3)) > 0) ? HIGH : LOW);
#endif

        /* give some time for transition */
        delay(1);

        readAccu = 0;
       // adcStart(ADC_MUL_SIG_PIN);
        for (int i = 0 ; i < ADC_OVERSAMPLING; i++)
        {

            if (false)//adcBusy(ADC_MUL_SIG_PIN) == false)
            {
                //readAccu += adcEnd(ADC_MUL_SIG_PIN);
               // adcStart(ADC_MUL_SIG_PIN);
            }
        }
        //adcEnd(ADC_MUL_SIG_PIN);

#ifdef ADC_DYNAMIC_RANGE
        if (readAccu < adcMin - 0.5f)
        {
            adcMin = readAccu + 0.5f;
            Serial.printf("adcMin: %0.3f\n", readAccu);
        }

        if (readAccu > adcMax + 0.5f)
        {
            adcMax = readAccu - 0.5f;
            Serial.printf("adcMax: %0.3f\n", readAccu);
        }
#endif

        if (adcMax > adcMin)
        {
            /*
             * normalize value to range from 0.0 to 1.0
             */
            float readValF = (readAccu - adcMin) / ((adcMax - adcMin));
            readValF *= (1 + 2.0f * ADC_THRESHOLD); /* extend to go over thresholds */
            readValF -= ADC_THRESHOLD; /* shift down to allow go under low threshold */

            bool midiMsg = false;

            /* check if value has been changed */
            if (readValF > adcChannelValue[j] + ADC_THRESHOLD)
            {
                adcChannelValue[j] = (readValF - ADC_THRESHOLD);
                midiMsg = true;
            }
            if (readValF < adcChannelValue[j] - ADC_THRESHOLD)
            {
                adcChannelValue[j] = (readValF + ADC_THRESHOLD);
                midiMsg = true;
            }

            /* keep value in range from 0 to 1 */
            if (adcChannelValue[j] < 0.0f)
            {
                adcChannelValue[j] = 0.0f;
            }
            if (adcChannelValue[j] > 1.0f)
            {
                adcChannelValue[j] = 1.0f;
            }

            /* MIDI adoption */
            if (midiMsg)
            {
                uint32_t midiValueU7 = (adcChannelValue[j] * 127.999);
                if (j < ADC_TO_MIDI_LOOKUP_SIZE)
                {
#ifdef ADC_INVERT
                    uint8_t idx = (ADC_INPUTS - 1) - j;
#else
                    uint8_t idx = j;
#endif
                    if (lastSendVal[idx] != midiValueU7)
                    {
                        Midi_ControlChange(adcToMidiLookUp[idx].ch, adcToMidiLookUp[idx].cc, midiValueU7);
                        lastSendVal[idx] = midiValueU7;
                    }
                }
#ifdef ADC_DEBUG_CHANNEL0_DATA
                switch (j == 0)
                {
                    float adcValFrac = (adcChannelValue[j] * 127.999) - midiValueU7;
                    Serial.printf("adcChannelValue[j]: %f -> %0.3f -> %0.3f-> %d, %0.3f\n", readAccu, readValF, adcChannelValue[j], midiValueU7, adcValFrac);
                }
#endif
            }
        }
    }
}

float *AdcMul_GetValues(void)
{
    return adcChannelValue;
}




bool  AdcSimple(){
    unsigned long int pinValue = 0;
    float delta, error;
    bool midiMsg = false;
    
    //for(int i=0; i < 100; i++) //oversample
          pinValue += analogRead(34);
          pinValue += analogRead(34);
          pinValue += analogRead(34);
          pinValue += analogRead(34);
          pinValue += analogRead(34);
          pinValue += analogRead(34);
    pinValue = pinValue / 6;
    adcSingle = float(pinValue)/4096.0f; //(pinValue/10)*10
    delta = adcSingleAve - adcSingle; //floating point absolute get rid of signed
    error = 0.009f+(0.012f*(adcSingle+0.1));  //previous weird idea error = 0.03*((adcSingle+0.25)*0.75); 
    
    
    if (fabs(delta) > error ){
       if(adcSetpoint != adcSingleAve) 
        {
          adcSetpoint = adcSingleAve;
          Serial.println("ADC read: " + String(adcSetpoint));
          adcChannelValue[analogueParamSet] = adcSetpoint;
          Synth_SetParam(analogueParamSet, adcChannelValue[analogueParamSet]*1.2);
          midiMsg = true;
          return 1;
        } 
      
      
    } else {
      
      return 0;
    }
    adcSingleAve = (adcSingleAve+adcSingle)/2;
/*
    if (midiMsg)
    {
        uint32_t midiValueU7 = (adcChannelValue[analogueParamSet] * 127.999);
        if (analogueParamSet < ADC_TO_MIDI_LOOKUP_SIZE)
        {
            #ifdef ADC_INVERT
            uint8_t idx = (ADC_INPUTS - 1) -analogueParamSet;
            #else
            uint8_t idx = analogueParamSet;
            #endif
            if (lastSendVal[idx] != midiValueU7)
            {
                Midi_ControlChange(adcToMidiLookUp[idx].ch, adcToMidiLookUp[idx].cc, midiValueU7);
                lastSendVal[idx] = midiValueU7;
            }
        }
    } */
}

void setupButtons(){
  pinMode(upButton, INPUT_PULLUP);  //pinMode(2, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);

}

void processButtons(){

  // read the state of the switch into a local variable:
  int readUpButton = digitalRead(upButton);
  int readDownButton = digitalRead(downButton);
  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (readUpButton != lastUpButtonState) {
    // reset the debouncing timer
    lastUBDebounceTime = millis();
  }
  digitalWrite(SWITCH2_LED_PIN, readUpButton);
  
  
   if (readDownButton != lastDownButtonState) {
    // reset the debouncing timer
    lastDBDebounceTime = millis();
  }
  
  
  if ((millis() - lastUBDebounceTime) > debounceDelay) {

    // if the button state has changed:
    if (readUpButton != upButtonState) {
      upButtonState = readUpButton;
      
      // only toggle the LED if the new button state is HIGH
      if (upButtonState == HIGH) {
         if (waveformParamSet < MAXBANK-1){
           bankParamSet++;
           displayBankChange(bankParamSet);
           if(bankParamSet > MAXBANK-1)
              bankParamSet = MAXBANK-1;
           Serial.print("BankParamSet: ");
           Serial.println(bankParamSet);
           
         }
      }
    }
  }
  if ((millis() - lastDBDebounceTime) > debounceDelay) {

    // if the button state has changed:
    if (readDownButton != downButtonState) {
      downButtonState = readDownButton;

      // only toggle the LED if the new button state is HIGH
      if (downButtonState == HIGH) {
         if (bankParamSet > 0){
           bankParamSet--;
         if(bankParamSet < 0)
           bankParamSet = 0;
           Serial.print("BankParamSet: ");
           Serial.println(bankParamSet);
         }
      }
      digitalWrite(BLINK_LED_PIN, downButtonState);
    }
  }



  if(bankParamSet != checkBankValue()) //no need to update screen menu if no change
    displayBankChange(bankParamSet);
  
  if(bankParamSet == 1)
    useArpToggle(true);
  else
    useArpToggle(false);
    // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastUpButtonState = readUpButton;
  lastDownButtonState = readDownButton;

}


void Custom_SetParam(uint8_t slider, float value)
{
  switch(slider){
    case CONTROL_PARAM_MAX_VOL:
      keyboardSetVolume(value);  //see multikeyTo37Midi module where keyboard entry calls notes on/off
      miniScreenBarSize(0, value);
      break;
    case CONTROL_SEMITONES:
      keyboardSetSemiModifier(value);
      miniScreenBarSize(1, value);
      break;
    case ARP_STATE:
      setArpState(value);  
      miniScreenBarSize(2, value);
      break;
    case ARP_VARIATION:
      setArpVariation(value); 
      miniScreenBarSize(3, value);
      break;
    case ARP_HOLD:
      setArpHold(value);
      miniScreenBarSize(4, value);  
      break;
    case ARP_NOTE_LEN:
      setArpNoteLength(value);
      miniScreenBarSize(5, value);  
      break;
    case ARP_BPM:
     setBPM(value+0.01);    //set beats per minute
     miniScreenString(6,1,string2char("Tmpo:"+String(checkBPM())),HIGH);
     break;
  }
}
