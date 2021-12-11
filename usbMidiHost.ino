/*
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

/**
 * @file usbMidiHost.ino
 * @author Marcel Licence
 * @date 18.05.2021
 *
 * @brief This file includes the implementation for MIDI in/out
 * via the USB Host module using the "Revision 2.0 of MAX3421E-based USB Host Shield Library"
 *
 * You will require to use following library:
 * https://github.com/felis/USB_Host_Shield_2.0
 *
 * Please check out USB-MIDI dump utility from Yuuichi Akagawa
 *
 * @see Mini USB host shield with ESP32 as MIDI interface (MAX3421E add-on for arduino synthesizer projects) - https://youtu.be/Mt3rT-SVZww
 * @see https://github.com/felis/USB_Host_Shield_2.0/blob/master/examples/USBH_MIDI/USBH_MIDI_dump/USBH_MIDI_dump.ino
 */

 /* Michael Nolan - notes - mi - providing 5v vbus power the communications from my Novation mini launch v2 was unstable. I dropped voltage to 4.35
  *  I don't know exactly what voltage is returning from it but it would get stuck.
  *  Update/refactor Oct 26 to new changes by Marcel License
 * Dec 8 - i wired GPIO17 to the int pin (first on right side 
 * https://felis.github.io/USB_Host_Shield_2.0/#documentation suggests ESP32 pins and INT is GPIO17 but it was left unused in Marcel's library here
 * After reading a lot from usbh_midi.h I moved onto usbhub.h and start working on SetHubFeature()
 * 
   In future if you want to send USB midi then https://github.com/YuuichiAkagawa/Arduino-UHS2MIDI
  */

#ifdef __CDT_PARSER__
#include <cdt.h>
#endif

#ifdef MIDI_VIA_USB_ENABLED
/*
 * Connections:
 *  CS: IO5       - MAX3421   pin 1 to D5
 *  INT: IO17 (not used)  // trying to use from Dec8
 *  SCK: IO18                 pin 4 to D18
 *  MISO: IO19                pin 3 to D19
 *  MOSI: IO23                pin 2 to D23
 */
#define  ESP32
#include <usbh_midi.h>
#include <usbhub.h>
#include <SPI.h>

USB Usb;
USBH_MIDI  Midi(&Usb);

//static void UsbMidi_Poll();

uint16_t pid, vid;
bool usbPollLock = false;
static bool usbPollSuccess = false;

bool usbPollReadSuccess(){
  if(usbPollSuccess == true)
  {
    usbPollSuccess = false;
    return true;
  } else
    return false;
}

struct usbMidiMappingEntry_s
{
    void (*sendRaw)(uint8_t *buf);
    void (*shortMsg)(uint8_t *buf);
    void (*liveMsg)(uint8_t *buf);
    void (*sysEx)(uint8_t *buf, uint8_t len);

    uint8_t cableMask;
};

struct usbMidiMapping_s
{
    void (*usbMidiRxIndication)(uint8_t cable);
    void (*usbMidiTxIndication)(uint8_t cable);

    struct usbMidiMappingEntry_s *usbMidiMappingEntries;
    int usbMidiMappingEntriesCount;
};

extern struct usbMidiMapping_s usbMidiMapping; /* definition in z_config.ino */

void IRAM_ATTR Ext_INT1_ISR()
{
  Serial.println("interrupt");
}

void UsbMidi_Setup()
{
    pinMode(17, INPUT);
    attachInterrupt(17, Ext_INT1_ISR, CHANGE);
    vid = pid = 0;
    //Serial.begin(115200);
    Serial.println("Hello now we can start\n");

    if (Usb.Init() == -1)
    {
        Serial.println("Usb init failed!\n");
        while (1); //halt
    }//if (Usb.Init() == -1...
    delay(200);
    Serial.println("Usb init done!\n");
    Serial.println("USB address"+String(Midi.GetAddress()));
}

uint8_t lastState = 0xFF;


void UsbMidi_Loop()
{
    Usb.Task();
    if (lastState != Usb.getUsbTaskState())
    {
        lastState = Usb.getUsbTaskState();
        Serial.printf("state: %d\n",  Usb.getUsbTaskState());
        switch (Usb.getUsbTaskState())
        {
        case USB_STATE_DETACHED:
            Serial.printf("    USB_STATE_DETACHED\n");
            vid = pid = 0;
            break;

        case USB_DETACHED_SUBSTATE_INITIALIZE:
            Serial.printf("    USB_DETACHED_SUBSTATE_INITIALIZE\n");
            vid = pid = 0;
            break;

        case USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE:
            Serial.printf("    USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE\n");
            vid = pid = 0;
            break;

        case USB_DETACHED_SUBSTATE_ILLEGAL:
            Serial.printf("    USB_DETACHED_SUBSTATE_ILLEGAL\n");
            vid = pid = 0;
            break;

        case USB_ATTACHED_SUBSTATE_SETTLE:
            Serial.printf("    USB_ATTACHED_SUBSTATE_SETTLE\n");
            break;

        case USB_ATTACHED_SUBSTATE_RESET_DEVICE:
            Serial.printf("    USB_ATTACHED_SUBSTATE_RESET_DEVICE\n");
            break;

        case USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE:
            Serial.printf("    USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE\n");
            break;

        case USB_ATTACHED_SUBSTATE_WAIT_SOF:
            Serial.printf("    USB_ATTACHED_SUBSTATE_WAIT_SOF\n");
            break;

        case USB_ATTACHED_SUBSTATE_WAIT_RESET:
            Serial.printf("    USB_ATTACHED_SUBSTATE_WAIT_RESET\n");
            break;

        case USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE:
            Serial.printf("    USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE\n");
            break;

        case USB_STATE_CONFIGURING:
            Serial.printf("    USB_STATE_CONFIGURING\n");
            break;

        case USB_STATE_RUNNING:
            Serial.printf("    USB_STATE_RUNNING\n");
            break;

        case USB_STATE_ERROR:
            Serial.printf("    USB_STATE_ERROR\n");
            break;

        }
    }
    if (Midi)
    {
        UsbMidi_Poll();
    }
}

uint8_t msgQueue[128][3];

uint8_t msgQueueIn = 0;
uint8_t msgQueueOut = 0;

inline
void UsbMidi_HandleSysEx(uint8_t *buf, uint8_t len)
{
    Serial.printf("not supported yet\n");
}

inline
void UsbMidi_HandleLiveMsg(uint8_t msg)
{
    Serial.printf("live msg\n");
}

// The majority of MIDI communication consists of multi-byte packets beginning with a status byte followed by one or two data bytes. see https://cmtext.indiana.edu/MIDI/chapter3_midi_data_format.php
// status byte 1000nnnn data1 0kkkkkkk data2 0vvvvvvv - which - noteoff channel n, key k, velocity v -
// first 4 bits of status 1001 note 0n, 1010 Poly Key pressure, 1011 Controller, 1100 prog, 1101 channelPressure, 1110 PitchBend
inline
void UsbMidi_HandleShortMsg(uint8_t *data) 
{
 #ifndef DISPLAY_1306  //see  Midi_HandleShortMsg in the midi_interface library where I call decide how to handle certain messages
    Serial.printf("short: %02x %02x %02x\n", data[0], data[1], data[2]);  //show generic data here 
 #endif

    /* forward data to mapped function */
    for (int i = 0; i < usbMidiMapping.usbMidiMappingEntriesCount; i++)
    {
        if (((1 << 0) & usbMidiMapping.usbMidiMappingEntries[i].cableMask) > 0)
        {
            usbMidiMapping.usbMidiMappingEntries[i].shortMsg(data);
        }
    }
}

uint8_t MIDI_handleMsg(uint8_t *data, uint16_t len, uint8_t cable)
{
    /* sometimes we got just zeros from some hardware */
    int i = 0;
    while (data[i] == 0)
    {
        i++;
    }
    if (i > 0)
    {
        return i;
    }

    if ((data[0] & 0xF0) == 0xF0)
    {
        /* handle status msg */
        if (data[0] ==  0xF0)
        {
            for (int i = 2; i < len; i++)
            {
                if (data[i] == 0xF7)
                {
                    UsbMidi_HandleSysEx(data, i);
                    return i + 1;
                }
            }
        }
        else
        {
            UsbMidi_HandleLiveMsg(data[0]);
            return 1;
        }
    }
    else
    {
        memcpy(msgQueue[msgQueueIn], &data[0], 3);
        msgQueueIn++;
        if (msgQueueIn >= 128)
        {
            msgQueueIn = 0;
        }
        return 3;
    }

    Serial.printf("unhandled:\n");
    for (int i = 0; i < len; i++)
    {
        Serial.printf("0x%02x ", data[i]);
    }
    Serial.printf("\n");
}

/* process data now */
void UsbMidi_ProcessSync(void)
{
    
    while (msgQueueIn != msgQueueOut)
    {   
        UsbMidi_HandleShortMsg(msgQueue[msgQueueOut]);
        msgQueueOut ++;
        if (msgQueueOut >= 128)
        {
            msgQueueOut = 0;
        }
    }
}

static void UsbMidi_Poll()
{
    uint8_t bufMidi[MIDI_EVENT_PACKET_SIZE];
    uint16_t  rcvd;
    uint8_t    countReads = 0;
    usbPollLock = true;
    memset(bufMidi, 0xCC, sizeof(bufMidi));

    if (Midi.idVendor() != vid || Midi.idProduct() != pid)
    {
        vid = Midi.idVendor();
        pid = Midi.idProduct();
        Serial.printf("VID: %04x, PID: %04x\n", vid, pid);
    }
    //restart:
    while(countReads < 20){
      countReads++;
      if (Midi.RecvData(&rcvd,  bufMidi) == 0)
      {
         
         if (rcvd == 0)
         {
              /* Some devices sending a lot of empty messages */     
              return;
         } 
  
          uint8_t *midiData = &bufMidi[0];
          while (rcvd > 2)
          {
              usbPollSuccess = true;
              
              midiData = &midiData[1];
              int consumed = MIDI_handleMsg(midiData, rcvd, (bufMidi[0] >> 4) & 0xF);
              midiData = &midiData[consumed];
              if (consumed > rcvd)
              {
                  Serial.printf("overrun!\n");
                  return;
              }
              rcvd -= consumed;
          }
          
      }
     // __asm__ __volatile__ ("nop\n\t");
      //__asm__ __volatile__ ("nop\n\t");
      //__asm__ __volatile__ ("nop\n\t");
     // __asm__ __volatile__ ("nop\n\t");
      //delayMicroseconds(1);   //  this was workable as the shortest possible delay but I went for a microsecond length//  
   
   }
   usbPollLock = false;
}

void UsbMidi_SendRaw(uint8_t *buf, uint8_t cable)
{
    Midi.SendData(buf, cable);
}

inline
void UsbMidi_SendControlChange(uint8_t channel, uint8_t data1, uint8_t data2)
{
    uint8_t shortBuf[3] = {(uint8_t)(0xB0U + (channel & 0x0FU)), data1, data2};
    #ifdef MIDI_TX_PIN
      Serial2.write(shortBuf, 3);
    #endif
}

#endif /* MIDI_VIA_USB_ENABLED */
