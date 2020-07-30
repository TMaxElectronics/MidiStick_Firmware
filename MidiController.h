#include <sys/attribs.h>
#include <stdint.h>
#include <xc.h>

//USB Config commands
#define USB_CMD_LIVE_PREV 0x10
#define USB_CMD_SET_ADSR_PREV 0x11

#define USB_CMD_GET_PROGRAMM            0x20
#define USB_CMD_SET_PROGRAMM            0x21
#define USB_CMD_GET_COIL_CONFIG         0x22
#define USB_CMD_SET_COIL_CONFIG         0x23
#define USB_CMD_SET_COIL_CONFIG         0x23
#define USB_CMD_GET_COILCONFIG_INDEX    0x24
#define USB_CMD_SET_COILCONFIG_INDEX    0x25
#define USB_CMD_CLEAR_ALL_PROGRAMMS     0x26
#define USB_CMD_CLEAR_ALL_COILS         0x27
#define USB_CMD_COILCONFIG_NOT_CHANGED  0x28

#define USB_CMD_GET_DEVCFG              0x30
#define USB_CMD_SET_DEVCFG              0x31
#define USB_CMD_BLINK                   0x32
#define USB_GET_CURR_NOTES              0x88
#define USB_CMD_PING                    0

#define USB_CMD_ENTER_PROGMODE          0x40
#define USB_CMD_EXIT_PROGMODE           0x41
#define USB_CMD_FWUPDATE_ERASE          0x42                 //erases the currently stored FW data at 0x9D020000, if there is any
#define USB_CMD_FWUPDATE_START_BULK_WRITE 0x43      //set up for bulk data
#define USB_CMD_FWUPDATE_BULK_WRITE_FINISHED 0x88      //set up for bulk data
#define USB_CMD_FWUPDATE_GET_CRC        0x44                
#define USB_CMD_FWUPDATE_COMMIT         0x45                 
#define USB_CMD_FWUPDATE_REBOOT         0x46 

#define USB_CMD_GET_SERIAL_NR           0x50
#define USB_CMD_GET_FWVERSION           0x51
#define USB_CMD_GET_BLVERSION           0x52

//Midi command macros
#define MIDI_CMD_NOTE_OFF               0x80
#define MIDI_CLK                        0xF8
#define MIDI_CMD_NOTE_ON                0x90
#define MIDI_CMD_KEY_PRESSURE           0xA0
#define MIDI_CMD_CONTROLLER_CHANGE      0xB0
#define MIDI_CC_ALL_SOUND_OFF           0x78
#define MIDI_CC_RESET_ALL_CONTROLLERS   0x79
#define MIDI_CC_ALL_NOTES_OFF           0x7B
#define MIDI_CC_VOLUME                  0x07
#define MIDI_CC_RPN_LSB                 0x64
#define MIDI_CC_RPN_MSB                 0x65
#define MIDI_CC_NRPN_LSB                0x62
#define MIDI_CC_SUSTAIN_PEDAL           0x40
#define MIDI_CC_DAMPER_PEDAL            0x43
#define MIDI_CC_NRPN_MSB                0x63
#define MIDI_CC_DATA_FINE               0x26
#define MIDI_CC_DATA_COARSE             0x06
#define MIDI_CC_RPN_MSB                 0x65
#define MIDI_CMD_PROGRAM_CHANGE         0xC0
#define MIDI_CMD_CHANNEL_PRESSURE       0xD0
#define MIDI_CMD_PITCH_BEND             0xE0
#define MIDI_CMD_SYSEX                  0xF0

#define MIDI_RPN_BENDRANGE              0x0000

//ADST states
#define NOTE_OFF    0
#define ATTAC       1
#define DECAY       2
#define SUSTAIN     3
#define RELEASE     4

#define LED_POWER          0
#define LED_DATA           1
#define LED_OUT_ON         2
#define LED_DUTY_LIMITER   3
#define LED_ADSR_PREVIEW   4

#define LED_OFF            0
#define LED_ON             1
#define LED_BLINK          2

#define AUX_AUDIO          0
#define AUX_E_STOP         1

//struct to contain all variables and parameters for the voices
typedef struct{
    uint8_t adsrState;
    uint8_t currNote;
    uint16_t currReqNoteOT;
    uint16_t lastReqNoteOT;
    uint16_t currNoteOT;
    float currOTMult;
    uint32_t noteAge;
    uint8_t currNoteOrigin;
} SynthVoice;

//Note frequency lookup table
const uint16_t Midi_NoteFreq[128] = {8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 5920, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544};

//Initialise midi stuff
void Midi_init();
void Midi_initTimers();

//Run the 1ms tick routines
void Midi_SOFHandler();

//Midi&Config CMD interpreter routines
void Midi_run();

//On time calculation functions
void Midi_setNoteOnTime(uint16_t onTimeUs, uint8_t ch);
//uint16_t Midi_getNoteOnTime();

//note config routines
void Midi_setNote(uint8_t voice, uint8_t note, uint8_t velocity, uint8_t channel);
void Midi_setNoteTPR(uint8_t voice, uint16_t freq);

//Calculates the ADSR Coefficients
void Midi_calculateADSR(uint8_t channel);

//Checks if any voices are still outputting data
unsigned Midi_isNotePlaing();               //returns: 1, when notes are playing, 0 otherwise

//Timer ISRs
inline void Midi_timerHandler(uint8_t voice);

//"HAL" for led on,off and invert
void Midi_LED(uint8_t type, uint8_t state);