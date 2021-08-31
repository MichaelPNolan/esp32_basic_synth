/*
 * pinout of ESP32 DevKit found here:
 * https://circuits4you.com/2018/12/31/esp32-devkit-esp32-wroom-gpio-pinout/
 *
 * Author: Marcel Licence
 */

#include "config.h"

/*
 * required include files
 */
#include <Arduino.h>
#include <WiFi.h>

/* this is used to add a task to core 0 */
TaskHandle_t  Core0TaskHnd ;
boolean       USBConnected;
uint16_t      task0cycles;
uint32_t      refreshScreen;

//#define I2S_NODAC  not really sure what this is since it does try to play samples
#define MIDI_VIA_USB_ENABLED

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
    Blink_Setup();
    setupButtons();
    refreshScreen = SAMPLE_RATE/SCREEN_FPS; //uses modulus to find this interval
    
#ifdef ESP32_AUDIO_KIT
    ac101_setup();
    Serial.printf("Use AC101 features of A1S variant");
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

#ifdef MIDI_VIA_USB_ENABLED
    UsbMidi_Setup();
#endif



    Serial.printf("ESP.getFreeHeap() %d\n", ESP.getFreeHeap());
    Serial.printf("ESP.getMinFreeHeap() %d\n", ESP.getMinFreeHeap());
    Serial.printf("ESP.getHeapSize() %d\n", ESP.getHeapSize());
    Serial.printf("ESP.getMaxAllocHeap() %d\n", ESP.getMaxAllocHeap());

    Serial.printf("Firmware started successfully\n");

#if 0 /* activate this line to get a tone on startup to test the DAC */
    Synth_NoteOn(0, 64, 1.0f);
#endif

#if (defined ADC_TO_MIDI_ENABLED) || (defined MIDI_VIA_USB_ENABLED)
    xTaskCreatePinnedToCore(Core0Task, "Core0Task", 8000, NULL, 999, &Core0TaskHnd, 0);
    Serial.println("Setup Pin To Core 0 loop");
#endif
}


void Core0TaskSetup()
{
    /*
     * init your stuff for core0 here
     */
#ifdef ADC_TO_MIDI_ENABLED
    AdcMul_Init();
#endif
#ifdef DISPLAY_1306
   setup1306(); //display
#endif
   task0cycles = 0;
}

void Core0TaskLoop()
{
    /*
     * put your loop stuff for core0 here
     */
  #ifdef ADC_TO_MIDI_ENABLED
    AdcMul_Process();
  #endif
    //AdcSimple();
    processButtons();
  #ifdef DISPLAY_1306
    displayRefresh();
  #endif
    /*task0cycles++;
    if(task0cycles > 500){
       
       task0cycles = 0;
    }*/
}

void Core0Task(void *parameter)
{
    
    Core0TaskSetup();
    //Serial.print("Core0 Setup- confirm core:");
    //Serial.println(xPortGetCoreID());
    while (true)
    {
        Core0TaskLoop();
       
        delay(1);
        yield();
    }
}

/*
 * use this if something should happen every second
 * - you can drive a blinking LED for example
 */
inline void Loop_1Hz(void)
{
   Blink_Process();
   
}


/*
 * our main loop
 * - all is done in a blocking context
 * - do not block the loop otherwise you will get problems with your audio
 */
float fl_sample, fr_sample;

void loop()
{
    static uint32_t loop_cnt_1hz;
    static uint8_t loop_count_u8 = 0;

    loop_count_u8++;

    loop_cnt_1hz ++;
    if (loop_cnt_1hz >= SAMPLE_RATE)   //I think the timing that makes this actually make sense relates to portMAX_DELAY in the i2s_write function see i2s_interface module
    {
        Loop_1Hz();
        loop_cnt_1hz = 0;
    }
    #ifdef DISPLAY_1306
    if ((loop_cnt_1hz % refreshScreen) == 0)   //I think the timing that makes this actually make sense relates to portMAX_DELAY in the i2s_write function see i2s_interface module
    {
        miniScreenRedraw(0,1); //completely rewrite screen 
    }
       
    #endif
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
    if (loop_count_u8 % 40 == 0)
    {
       
      //if(!USBConnected) {UsbMidi_Loop(); Serial.print(".");}
        Midi_Process();
      #ifdef MIDI_VIA_USB_ENABLED
        UsbMidi_ProcessSync();
      #endif
      UsbMidi_Loop();
             
    }
   
}

    
