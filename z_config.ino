/*
 * z_config.ino
 *
 * Put all your project configuration here (no defines etc)
 * This file will be included at the and can access all
 * declarations and type definitions
 *
 *  Created on: 12.05.2021
 *      Author: Marcel Licence
 */

/*
 * adc to midi mapping
 */
struct adc_to_midi_s adcToMidiLookUp[ADC_TO_MIDI_LOOKUP_SIZE] =
{
    {0, 0x10},
    {1, 0x10},
    {2, 0x10},
    {3, 0x10},
    {4, 0x10},
    {5, 0x10},
    {6, 0x10},
    {7, 0x10},
};

/*
 * This is a new mapping for the Worlde Tuna midi controller
 * however it doesn't send value fader sizes like I did with my midi
 * it sends different messages from rotary type faders when they turn in an up or down direction/rotation
 * Eg the first knob sends B0 48 40 followed by B0 48 3F for left rotation and B0 48 40 and B0 48 41 for right turn - repeats as you turn "clicks" 
 * Noted as 30 48 (continuous controller)
 * struct midiControllerMapping
{
    uint8_t channel;
    uint8_t data1;
    const char *desc;
    void(*callback_mid)(uint8_t ch, uint8_t data1, uint8_t data2);
    void(*callback_val)(uint8_t userdata, float value);
    uint8_t user_data;
};
#define SYNTH_PARAM_DETUNE_1    8
#define SYNTH_PARAM_UNISON_2    9
#else
#define SYNTH_PARAM_WAVEFORM_1    8
#define SYNTH_PARAM_WAVEFORM_2    9
#endif
#define SYNTH_PARAM_MAIN_FILT_CUTOFF  10
#define SYNTH_PARAM_MAIN_FILT_RESO    11
#define SYNTH_PARAM_VOICE_FILT_RESO   12
#define SYNTH_PARAM_VOICE_NOISE_LEVEL 13
 */

struct midiControllerMapping tunaWorlde[] =
{
  #ifdef USE_UNISON
    { 0x0, 0x0a, "R1", NULL, Synth_SetParam, SYNTH_PARAM_DETUNE_1},
    { 0x0, 0x0b, "R2", NULL, Synth_SetParam, SYNTH_PARAM_UNISON_2},
#else
    { 0x0, 0x0a, "R1", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_ATTACK},  //0x0 - 0x7, 0x10 little tiger toy midi controller  Synth_SetParam, SYNTH_PARAM_WAVEFORM_1
    { 0x0, 0x0b, "R2", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_DECAY},  //Synth_SetParam, SYNTH_PARAM_WAVEFORM_2
#endif
    { 0x0, 0x0c, "R3", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_SUSTAIN},                      //Delay_SetLength, 2
    { 0x0, 0x0d, "R4", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_RELEASE},                       //Delay_SetLevel, 3    //Delay_SetFeedback, 4

    { 0x0, 0x0e, "R5", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_ATTACK},
    { 0x0, 0x0f, "R6", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_DECAY},
    { 0x0, 0x10, "R7", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_SUSTAIN},
    { 0x0, 0x11, "R8", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_RELEASE},

    { 0x0, 0x14, "R9", NULL, Delay_SetLength, 2},          //Delay_SetLevel, 3    //Delay_SetFeedback, 4
    { 0x0, 0x15, "R10", NULL, Delay_SetLevel, 3},
    { 0x0, 0x16, "R11", NULL, Delay_SetFeedback, 4},

    { 0x0, 0x17, "R12", NULL, Synth_SetParam, SYNTH_PARAM_MAIN_FILT_CUTOFF},
    { 0x0, 0x18, "R13", NULL, Synth_SetParam, SYNTH_PARAM_MAIN_FILT_RESO},
    { 0x0, 0x19, "R14", NULL, Synth_SetParam, SYNTH_PARAM_VOICE_FILT_RESO},
    { 0x0, 0x1a, "R15", NULL, Synth_SetParam, SYNTH_PARAM_WAVEFORM_1 },
    { 0x0, 0x1b, "R15", NULL, Synth_SetParam, SYNTH_PARAM_WAVEFORM_2},

    // Central slider */
    { 0x0, 0x13, "H1", NULL, NULL, 0},
};
/*
 * this mapping  originally used for the edirol pcr-800 - renamed to tiger when
 * I started to do mappings for my homebrew midicontroller
 * 
  */
  

struct midiControllerMapping tigerMapping[] =
{
    /*// transport buttons 
    { 0x8, 0x52, "back", NULL, NULL, 0},
    { 0xD, 0x52, "stop", NULL, NULL, 0},
    { 0xe, 0x52, "start", NULL, NULL, 0},
    { 0xe, 0x52, "start", NULL, NULL, 0},
    { 0xa, 0x52, "rec", NULL, NULL, 0},

    // upper row of buttons 
    { 0x0, 0x50, "A1", NULL, NULL, 0},
    { 0x1, 0x50, "A2", NULL, NULL, 1},
    { 0x2, 0x50, "A3", NULL, NULL, 2},
    { 0x3, 0x50, "A4", NULL, NULL, 3},

    { 0x4, 0x50, "A5", NULL, NULL, 0},
    { 0x5, 0x50, "A6", NULL, NULL, 1},
    { 0x6, 0x50, "A7", NULL, NULL, 2},
    { 0x7, 0x50, "A8", NULL, NULL, 3},

    { 0x0, 0x53, "A9", NULL, NULL, 0},

    // lower row of buttons 
    { 0x0, 0x51, "B1", NULL, NULL, 0},
    { 0x1, 0x51, "B2", NULL, NULL, 1},
    { 0x2, 0x51, "B3", NULL, NULL, 2},
    { 0x3, 0x51, "B4", NULL, NULL, 3},

    { 0x4, 0x51, "B5", NULL, NULL, 4},
    { 0x5, 0x51, "B6", NULL, NULL, 5},
    { 0x6, 0x51, "B7", NULL, NULL, 6},
    { 0x7, 0x51, "B8", NULL, NULL, 7},

    { 0x1, 0x53, "B9", NULL, NULL, 8},

    // pedal 
    { 0x0, 0x0b, "VolumePedal", NULL, NULL, 0},

    // slider 
    { 0x0, 0x11, "S1", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_ATTACK},
    { 0x1, 0x11, "S2", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_DECAY},
    { 0x2, 0x11, "S3", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_SUSTAIN},
    { 0x3, 0x11, "S4", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_RELEASE},

    { 0x4, 0x11, "S5", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_ATTACK},
    { 0x5, 0x11, "S6", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_DECAY},
    { 0x6, 0x11, "S7", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_SUSTAIN},
    { 0x7, 0x11, "S8", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_RELEASE},

    { 0x1, 0x12, "S9", NULL, Synth_SetParam, 8},

    // rotary */
#ifdef USE_UNISON
    { 0x0, 0x10, "R1", NULL, Synth_SetParam, SYNTH_PARAM_DETUNE_1},
    { 0x1, 0x10, "R2", NULL, Synth_SetParam, SYNTH_PARAM_UNISON_2},
#else
    { 0x0, 0x10, "R1", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_ATTACK},  //0x0 - 0x7, 0x10 little tiger toy midi controller  Synth_SetParam, SYNTH_PARAM_WAVEFORM_1
    { 0x1, 0x10, "R2", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_DECAY},  //Synth_SetParam, SYNTH_PARAM_WAVEFORM_2
#endif
    { 0x2, 0x10, "R3", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_SUSTAIN},                      //Delay_SetLength, 2
    { 0x3, 0x10, "R4", NULL, Synth_SetParam, SYNTH_PARAM_VEL_ENV_RELEASE},                       //Delay_SetLevel, 3    //Delay_SetFeedback, 4

    { 0x4, 0x10, "R5", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_ATTACK},
    { 0x5, 0x10, "R6", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_DECAY},
    { 0x6, 0x10, "R7", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_SUSTAIN},
    { 0x7, 0x10, "R8", NULL, Synth_SetParam, SYNTH_PARAM_FIL_ENV_RELEASE},

    { 0x0, 0x12, "R9", NULL, Delay_SetLength, 2},

    // Central slider */
    { 0x0, 0x13, "H1", NULL, NULL, 0},
};

struct midiMapping_s midiMapping =
{
    Synth_NoteOn,
    Synth_NoteOff,
    Synth_PitchBend,
    Synth_ModulationWheel,
    tunaWorlde,
    sizeof(tunaWorlde) / sizeof(tunaWorlde[0]),
};


struct usbMidiMappingEntry_s usbMidiMappingEntries[] =
{
    {
        NULL,
        Midi_SendShortMessage,
        NULL,
        NULL,
        0x01,
    },
};

struct usbMidiMapping_s usbMidiMapping =
{
    NULL,      //NULL
    NULL,      //NULL
    usbMidiMappingEntries,
    sizeof(usbMidiMappingEntries) / sizeof(usbMidiMappingEntries[0]),
};
