/*
 * The GNU GENERAL PUBLIC LICENSE (GNU GPLv3)
 *
 * Copyright (c) 2021 Marcel Licence
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Dieses Programm ist Freie Software: Sie können es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
 * veröffentlichten Version, weiter verteilen und/oder modifizieren.
 *
 * Dieses Programm wird in der Hoffnung bereitgestellt, dass es nützlich sein wird, jedoch
 * OHNE JEDE GEWÄHR,; sogar ohne die implizite
 * Gewähr der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License für weitere Einzelheiten.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <https://www.gnu.org/licenses/>.
 */

/*
 * pinout of ESP32 DevKit found here:
 * https://circuits4you.com/2018/12/31/esp32-devkit-esp32-wroom-gpio-pinout/
 * 
 * This is a fork of Markus project see https://github.com/MichaelPNolan/esp32_basic_synth
 * Refactored my fork with Marcus changes from 26 Oct 2021
 */
#ifdef __CDT_PARSER__
#include <cdt.h>
#endif

#include "config.h"

/*
 * required include files
 */
#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>
#include <WiFi.h>


/* this is used to add a task to core 0 */

boolean       USBConnected;
static bool          videoLockout = false;
uint16_t      task0cycles;
uint32_t      beatcycles,oneSec,noteCycles;
uint32_t      refreshScreen;
static uint32_t CZLoop_cnt_1hz;
unsigned long timeMicros;

//#define I2S_NODAC  not really sure what this is since it does try to play samples
#define MIDI_VIA_USB_ENABLED

void App_UsbMidiShortMsgReceived(uint8_t *msg)
{
    Midi_SendShortMessage(msg);
  #ifdef MIDI_USB_TO_MIDI_SERIAL
    Midi_HandleShortMsg(msg, 8);
  #endif 
}

void setup()
{
    /*
     * this code runs once
     */
    delay(500);

    Serial.begin(115200);

    Serial.println();

    Delay_Init();

    Serial.printf("Initialize Synth Module\n");
    Synth_Init();
    Serial.printf("Initialize I2S Module\n");

    // setup_reverb();
    USBConnected = LOW;

#ifdef BLINK_LED_PIN
    Blink_Setup();
    Serial.printf("blink on\n");
#endif

    setupButtons();
    
    
#ifdef ESP32_AUDIO_KIT
#ifdef ES8388_ENABLED
    ES8388_Setup();
#else
    ac101_setup();
#endif
#endif

    setup_i2s();
    Serial.printf("Initialize Midi Module\n");
    Midi_Setup();
    
    Serial.printf("Turn off Wifi/Bluetooth\n");
#if 0
    setup_wifi();
#else
    WiFi.mode(WIFI_OFF);
#endif

#ifndef ESP8266
    btStop();
    // esp_wifi_deinit();
#endif
    arpeggiatorSetup();
    beatcycles = calcWaitPerBeat();
    noteCycles = noteLengthCycles(); // quarter note default
    oneSec = SAMPLE_RATE;

    Serial.printf("ESP.getFreeHeap() %d\n", ESP.getFreeHeap());
    Serial.printf("ESP.getMinFreeHeap() %d\n", ESP.getMinFreeHeap());
    Serial.printf("ESP.getHeapSize() %d\n", ESP.getHeapSize());
    Serial.printf("ESP.getMaxAllocHeap() %d\n", ESP.getMaxAllocHeap());
  


#if 0 /* activate this line to get a tone on startup to test the DAC */
    Synth_NoteOn(0, 64, 1.0f);
#endif

#if (defined ADC_TO_MIDI_ENABLED) || (defined MIDI_VIA_USB_ENABLED) || (defined DISPLAY_1306)
    Core0TaskInit();
    
#endif
 
    setupKeyboardCapture();
  
    
    Serial.printf("Firmware started successfully\n");

}
TaskHandle_t Core0TaskHnd,Core0USBTaskHnd;

inline
void Core0TaskInit()
{
    /* we need a second task for the terminal output */
   xTaskCreatePinnedToCore(Core0Task, "Core0Task", 4000, NULL, 999, &Core0TaskHnd, 0);  //orig xTaskCreatePinnedToCore(Core0Task, "Core0Task", 8000, NULL, 999, &Core0TaskHnd, 0);
   xTaskCreatePinnedToCore(Core0USBTask, "Core0USBTask", 8000, NULL, 1000, &Core0USBTaskHnd, 0);  //orig xTaskCreatePinnedToCore(Core0Task, "Core0Task", 8000, NULL, 999, &Core0TaskHnd, 0);
}

void Core0TaskSetup()
{
    /*
     * init your stuff for core0 here
     */

#ifdef ADC_TO_MIDI_ENABLED
    AdcMul_Init();
#endif
#ifdef MIDI_VIA_USB_ENABLED
 //   UsbMidi_Setup();

#endif 

#if (defined DISPLAY_1306) && ( DISPLAY_CORE == 0)
   
   refreshScreen = 100/SCREEN_FPS; // 1000/SCREEN_FPS if on core0 loop SAMPLE_RATE/SCREEN_FPS in core1 loop
   setup1306(); //display
   setupDisplayMessageQueue(); //should be initialized (called) from the other core so you can run setup1306() on core0 and other things leave data in the message queue
   
#endif
 task0cycles = 0; //seems defunct


}
void Core0USBTaskSetup()
{
#ifdef MIDI_VIA_USB_ENABLED
    UsbMidi_Setup();      
#endif 
}

void Core0TaskLoop()
{
    /*
     * put your loop stuff for core0 here
     */
  
  #ifdef ADC_TO_MIDI_ENABLED
    AdcMul_Process(); //note there some new code to refactor if you want to use the ADC module latest version including some pre-scaler thingy
  #endif
    //AdcSimple();
    processButtons();
  
  #ifdef MIDI_VIA_USB_ENABLED
   // UsbMidi_Loop();
  #endif
  
   #if (defined DISPLAY_1306) && (DISPLAY_CORE == 0)
    CZLoop_cnt_1hz ++;
    if (CZLoop_cnt_1hz >= refreshScreen)   //I think the timing that makes this actually make sense relates to portMAX_DELAY in the i2s_write function see i2s_interface module
    {
      CZLoop_cnt_1hz = 0;
      //miniScreenRedraw(0,1); //completely rewrite screen
      if(!videoLockout)
        displayRefresh();
      
    } 
    #endif
}
void Core0USBTaskLoop()
{

  #ifdef MIDI_VIA_USB_ENABLED
    UsbMidi_Loop();
    if(usbPollReadSuccess()){
   // while(usbPollReadSuccess()){
      UsbMidi_Loop();
      
    }
  #endif
}

void Core0Task(void *parameter)
{
    
    Core0TaskSetup();

    while (true)
    {
        Core0TaskLoop();
        //delayMicroseconds(1000); //crashes
        delay(30);
       // timeMicros = micros(); this seemed to lead to crashing
        yield();
    }
}
void Core0USBTask(void *parameter)
{
    Core0USBTaskSetup();
    while (true)
    {
        Core0USBTaskLoop();
        delay(1);
        yield();
    }
}

/*
 * use this if something should happen every second
 * - you can drive a blinking LED for example
 */
inline void pulseTempo(void)
{
#ifdef BLINK_LED_PIN
    Blink_Process();
#endif
}

inline void pulseNote(void)
{
  Arpeggiator_Process();
}

/*
 * our main loop
 * - all is done in a blocking context
 * - do not block the loop otherwise you will get problems with your audio
 */
float fl_sample, fr_sample;

void loop()
{

    static uint32_t loop_cnt_1beat;
    static uint32_t loop_cnt_1note;
    static uint8_t loop_count_u8 = 0;

    loop_count_u8++;



    loop_cnt_1beat++;  // related to tempo or bpm
    loop_cnt_1note++;  // related to divisions per beat or notes triggered by arpeggiator module
    
    if (loop_cnt_1beat >= beatcycles) // trigger beat - originally for 1hz blink process - now blink light when arpeggiator is on at tempo
    {

        loop_cnt_1beat = 0;
        beatcycles = calcWaitPerBeat();
        noteCycles = noteLengthCycles(); 
        if(checkArpeggiator())
          pulseTempo();
    }
    if (loop_cnt_1beat == oneSec){ // optional check in case a very slow tempo makes the next beat take a long time - recheck per second
       beatcycles = calcWaitPerBeat();
       noteCycles = noteLengthCycles(); 
       //Serial.println(beatcycles);
    }

    if (loop_cnt_1note >= noteCycles)
    {
      loop_cnt_1note = 0;
       noteCycles = noteLengthCycles(); 
       if(checkArpeggiator())
          pulseNote();
    }
 /*  #if (defined DISPLAY_1306) && (DISPLAY_CORE == 1)
    CZLoop_cnt_1hz ++;
    if (CZLoop_cnt_1hz >= refreshScreen)   //I think the timing that makes this actually make sense relates to portMAX_DELAY in the i2s_write function see i2s_interface module
    {
      CZLoop_cnt_1hz = 0;
      //miniScreenRedraw(0,1); //completely rewrite screen
      displayRefresh();
      
    }  
    #endif
 */
    
   
#ifdef I2S_NODAC
    if (writeDAC(l_sample))
    {
        l_sample = Synth_Process();
    }
#else
    
    if (i2s_write_stereo_samples(&fl_sample, &fr_sample))
    {
        /* nothing for here */
    }
    
    Synth_Process(&fl_sample, &fr_sample);
    
    /*
     * process delay line
     */
    
    Delay_Process(&fl_sample, &fr_sample);
    
#endif

    /*
     * Midi does not required to be checked after every processed sample
     * - we divide our operation by 8
     */
    if (loop_count_u8 % 6 == 0)
    {

      videoLockout = true;
      Midi_Process();
      #ifdef MIDI_VIA_USB_ENABLED

       // midiProc1 = true;
        UsbMidi_ProcessSync();
       // midiProc1 = false;

      #endif
      videoLockout = false;
             
    }

    if (loop_count_u8 % 6 == 1){
     
      if(getKeyIndex()>0){
        //Serial.println("k:"+String(getKeyIndex()));
        videoLockout = true;  

        serviceKeyQueue();
        videoLockout = false;
      } 
    }   
}

    
