//======================================================================================================================
//
//  data.h - global data
//
//  IBM 1620 Jr Project, Computer History Museum, 2017-2026
//
//  To recreate the experience (visual, auditory, tactile, visceral) of running historic software on a 1960s-era
//  computer.
//
//    Dave Babcock  - project lead, software, website
//   David Brock    - CHM sponsor
//   Steve Casner   - hardware, software
//     Joe Fredrick - hardware, firmware
//     Len Shustek  - CHM advisor
//     Dag Spicer   - CHM advisor
//   David Wise     - IBM 1620 expert
//
//======================================================================================================================

#ifndef DATA_H_
#define DATA_H_

#include "defines.h"

#define _GNU_SOURCE 1
#include "stdio.h"


// =================================  Common Types  =================================

struct lights {
    unsigned char pwms[NUM_PWMS];
    bool gpios[NUM_GPIOS];
};

struct cycleCounts {
    unsigned int total;
    unsigned int indicators[NUM_PWMS];
};

struct digit {
    unsigned char check;  // parity bit not stored in simulated memory only set in registers
    unsigned char flag;
    unsigned char bit8;
    unsigned char bit4;
    unsigned char bit2;
    unsigned char bit1;
    unsigned char value;
};

struct maRegister {
    struct digit digit10000;
    struct digit digit01000;
    struct digit digit00100;
    struct digit digit00010;
    struct digit digit00001;
    int value;
};

struct mbRegister {
    struct digit digitEven;
    struct digit digitOdd;
    int value;
};

struct mdRegister {
    struct digit digit;
    int value;
};

struct dRegister {
    struct digit digit10;
    struct digit digit01;
    int value;
};

struct opRegister {
    struct digit digit10;
    struct digit digit01;
    int value;
};

struct mqRegister {
    struct digit digit;
    int value;
};

struct sabRegister {
    struct digit digit10;
    struct digit digit01;
    int value;
};


// =================================  Program Data  =================================

// Simulator version
extern int versionMajor;
extern int versionMinor;
extern int versionBuild;
extern char *versionDate;

// Command line options
extern bool alphaOpt;
extern bool displayMAROpt;
extern bool echoOpt;
extern bool fastOpt;
extern bool informationOpt;
extern bool logOpt;
extern char *logFilename;
extern char *defaultLogFilename;
extern FILE *logFile;
extern bool modelOpt;
extern bool nodelayOpt;
extern bool protocolOpt;
extern bool stopsOpt;
extern bool timestampOpt;
extern bool zeroOpt;
extern int cmemCount;
extern char *cmemFiles[NUM_CMEM_FILES];
extern char *defaultCmemFilename;
extern char *programFiles[16];


// =================================  Shared Data  =================================

// CPU Model
extern volatile int cpuModel;

// Simulation state
extern volatile bool simulationRunning;

// Machine state
extern volatile int powerState;
extern volatile int runState;
extern volatile int lastRunState;
extern volatile int fpMode;
extern volatile int ioState;
extern volatile int ioType;
extern volatile int ioWait;
extern volatile int ioCounter;

// Display mode
extern volatile int lightMode;
extern volatile int testMode;

// Cycle counts buffers, one is written by the simulator while the other is read and
// then cleared by the front panel driver before swapping.
extern volatile struct cycleCounts countsA;
extern volatile struct cycleCounts countsB;
extern volatile struct cycleCounts *writing;
extern volatile struct cycleCounts *reading;

// Front panel switches and status lights
extern volatile int marSelector;
extern volatile bool inputs[NUM_GPIOS];
extern const bool inputFlags[NUM_GPIOS];
extern volatile bool outputs[NUM_GPIOS];

// Memory
extern volatile memDigit memory[MEMORY_SIZE];

// Parity check
extern const char parity[NUM_PARITY];

// Registers
extern volatile struct maRegister mar;
extern volatile struct maRegister mars[NUM_MARS];
extern volatile struct mbRegister mbr;
extern volatile struct mdRegister mdr;
extern volatile struct opRegister opr;
extern volatile struct dRegister dr;
extern volatile struct mqRegister mqr;
extern volatile struct sabRegister sabr;

extern const struct maRegister initMAR;
extern const struct maRegister initMARs[NUM_MARS];
extern const struct mbRegister initMBR;
extern const struct mdRegister initMDR;
extern const struct opRegister initOpR;
extern const struct dRegister initDR;
extern const struct mqRegister initMQR;
extern const struct sabRegister initSABR;


// =================================  Simulator Data  =================================

extern int cycleTime;
extern int powerOnWait;
extern int tyIOCycleWait;
extern int cdIOCycleWait;

extern int cycle;
extern int lastCycle;
extern const int cycleLights[NUM_CYCLES];

extern int opCode;
extern const int opCycles[NUM_OPCYCLES];
extern const bool opPIndirects[NUM_OPCYCLES];
extern const bool opQIndirects[NUM_OPCYCLES];

extern bool triggers[NUM_TRIGGERS];
extern bool cycleTriggers[NUM_TRIGGERS];
extern const int triggerLights[NUM_TRIGGER_LIGHTS];

extern bool lastSwitches[NUM_GPIOS];

extern const char dumpTYChars[NUM_NUMERIC_CHARS];
extern const char numericTYChars[NUM_NUMERIC_CHARS];
extern const char alphamericTYChars[NUM_ALPHAMERIC_CHARS];
extern const char dumpCDChars[NUM_NUMERIC_CHARS];
extern const char numericCDChars[NUM_NUMERIC_CHARS];
extern const char alphamericCDChars[NUM_ALPHAMERIC_CHARS];


// =================================  Front Panel Data  =================================

extern int lastMarSelector;

extern struct lights simulatorLights;
extern char previousLights[NUM_PWMS];

extern struct lights powerOffLights;
extern struct lights powerOnLights;
extern struct lights lampTestLights;
extern struct lights brightnessTestLights;
extern struct lights snakeTestLights;
extern struct lights ibmTestLights;

extern const int snakeUpperLights[7][40];
extern const int snakeLowerLights[19];
extern const int snakeStatusLights[21];
extern const int snakeRowOffsets[8];
extern const int snakeColumnOffsets[8];

extern const char realisticMap[256];
extern const char brightMap[256];

extern int debounces[NUM_GPIOS];
extern const int debounceMaxs[NUM_GPIOS];


// =================================  Console Typewriter Data  =================================

extern int currentPosition;
extern int leftMarginPosition;
extern int rightMarginPosition;
extern bool tabs[NUM_TYPEWRITER_POS + 1];

extern const char *printTYStrings[NUM_IO_CHARS];
extern const bool commandTYFlags[NUM_IO_CHARS];
extern const int numericTYDigits[NUM_IO_CHARS];
extern const int alphamericTYDigits[NUM_IO_CHARS];
extern const int charPosition[NUM_IO_CHARS];
extern const int charWait[NUM_IO_CHARS];


// =================================  Punched Card Data  =================================

extern volatile bool readerReady;
extern volatile bool readerBusy;
extern volatile bool readerCheck;
extern volatile bool readerLastCard;
extern volatile bool readerLoad;
extern volatile bool punchReady;
extern volatile bool punchBusy;
extern volatile bool punchCheck;

extern const char *printCDStrings[NUM_IO_CHARS];
extern const bool commandCDFlags[NUM_IO_CHARS];
extern const int numericCDDigits[NUM_IO_CHARS];
extern const int alphamericCDDigits[NUM_IO_CHARS];
extern const int protocolCDActions[NUM_IO_CHARS];


#endif /* DATA_H_ */
