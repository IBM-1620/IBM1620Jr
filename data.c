//======================================================================================================================
//
//  data.c - global data
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

#include "defines.h"
#include "data.h"

#define _GNU_SOURCE 1
#include <limits.h>


// =================================  Program Data  =================================

// Simulator version
int versionMajor = SIM_VERSION_MAJOR;
int versionMinor = SIM_VERSION_MINOR;
int versionBuild = 0;
char *versionDate = (char*)__DATE__;

// Command line options
bool alphaOpt = FALSE;
bool displayMAROpt = FALSE;
bool echoOpt = FALSE;
bool flowOpt = FALSE;
char *flowFilename = NULL;
char *defaultFlowFilename = (char*)"execution_trace.txt";
FILE *flowFile = NULL;
bool informationOpt = FALSE;
bool logOpt = FALSE;
char *logFilename = NULL;
char *defaultLogFilename = (char*)"io_log.txt";
FILE *logFile = NULL;
bool nodelayOpt = FALSE;
bool protocolOpt = FALSE;
bool stopsOpt = FALSE;
bool timestampOpt = FALSE;
bool unalignedOpt = FALSE;
bool zeroOpt = FALSE;
int cmemCount = 0;
char *cmemFiles[NUM_CMEM_FILES] = {0};
char *defaultCmemFilename = (char*)"core_memory.cmem";
char *programFiles[16] = {"PROGRAMS/APP_Power_Of_2.cmem",
                          "PROGRAMS/CU00_Console.cmem",
                          "PROGRAMS/CU01_General_Op_Codes.cmem",
                          "PROGRAMS/CU05_Special_Instructions.cmem",
                          "PROGRAMS/FP01_Floating_Point_Instructions.cmem",
                          "PROGRAMS/CU03_Indirect_Addressing.cmem",
                          "PROGRAMS/CS01_Check_Stops.cmem",
                          "PROGRAMS/DX05L_Core_Storage_L_20K.cmem",
                          "PROGRAMS/DX05H_Core_Storage_H_20K.cmem",
                          "PROGRAMS/CU04_Additional_Core.cmem",
                          "PROGRAMS/DX03_Console_Typewriter.cmem",
                          "PROGRAMS/IO02_Card_IO.cmem",
                          "PROGRAMS/IO03_Card_IO_Reliability.cmem",
                          NULL,
                          NULL,
                          NULL};


// =================================  Shared Data  =================================

volatile bool simulationRunning = FALSE;

volatile int powerState = POWER_OFF;
volatile int runState = STOPPED_STATE;
volatile int lastRunState = STOPPED_STATE;
volatile int ioState = STOPPED_IO_STATE;
volatile int ioType = NO_IO;
volatile int ioWait = 0;
volatile int ioCounter = 0;

volatile int lightMode = BRIGHT_LIGHTS;
volatile int testMode = NO_TEST;

volatile struct cycleCounts countsA = {0, {0}};
volatile struct cycleCounts countsB = {0, {0}};

volatile struct cycleCounts *writing = &countsA;
volatile struct cycleCounts *reading = &countsB;

volatile int marSelector = OR1_MAR;
volatile bool inputs[NUM_GPIOS] =
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
     FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE, FALSE, FALSE,
     FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
const bool inputFlags[NUM_GPIOS] =
    {TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
     TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
     TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE};
volatile bool outputs[NUM_GPIOS] =
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
     FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
     FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};

volatile memDigit memory[MEMORY_SIZE] = {0};

const char parity[NUM_PARITY] =
    {1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
     0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};

volatile struct maRegister mar =
    {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
     {1, 0, 0, 0, 0, 0, 0}, 0};
volatile struct maRegister mars[NUM_MARS] =
    {{{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0},
      {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
       {1, 0, 0, 0, 0, 0, 0}, 0}};
volatile struct mbRegister mbr =
    {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, 0};
volatile struct mdRegister mdr =
    {{1, 0, 0, 0, 0, 0, 0}, 0};
volatile struct opRegister opr =
    {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, 0};
volatile struct dRegister dr =
    {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, 0};
volatile struct mqRegister mqr =
    {{1, 0, 0, 0, 0, 0, 0}, 0};
volatile struct sabRegister sabr =
    {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, 0};

const struct maRegister initMAR =
    {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
     {1, 0, 0, 0, 0, 0, 0}, 0};
const struct maRegister initMARs[NUM_MARS] =
    {{{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0},
     {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0}, 0}};
const struct mbRegister initMBR =
    {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, 0};
const struct mdRegister initMDR =
    {{1, 0, 0, 0, 0, 0, 0}, 0};
const struct opRegister initOpR =
    {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, 0};
const struct dRegister initDR =
    {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, 0};
const struct mqRegister initMQR =
    {{1, 0, 0, 0, 0, 0, 0}, 0};
const struct sabRegister initSABR =
    {{1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0}, 0};


// =================================  Simulator Data  =================================

int cycle = 1;
int lastCycle = 0;
const int cycleLights[NUM_CYCLES] =
    {       -1,  I1_LIGHT,  I2_LIGHT,  I3_LIGHT,  I4_LIGHT,  I5_LIGHT,  I6_LIGHT, I7_LIGHT,  I8_LIGHT,  T1_LIGHT,
      T2_LIGHT, E11_LIGHT, E12_LIGHT, E13_LIGHT, E14_LIGHT, E15_LIGHT, E16_LIGHT, E17_LIGHT, E18_LIGHT, E19_LIGHT,
     E20_LIGHT, E21_LIGHT, E22_LIGHT, E23_LIGHT,        -1,        -1, E26_LIGHT, E27_LIGHT, E28_LIGHT, E29_LIGHT,
     E30_LIGHT, E31_LIGHT, E32_LIGHT, E33_LIGHT, E34_LIGHT, E35_LIGHT, E36_LIGHT, E37_LIGHT, E38_LIGHT, E39_LIGHT,
     E40_LIGHT, E41_LIGHT, E42_LIGHT, E43_LIGHT, E44_LIGHT, E45_LIGHT, E46_LIGHT, E47_LIGHT, E48_LIGHT, E49_LIGHT,
     E50_LIGHT, E51_LIGHT, E52_LIGHT, E53_LIGHT, E54_LIGHT, E55_LIGHT, I1_LIGHT};

int opCode = 0;
const int opCycles[NUM_OPCYCLES] =
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   //    -, FADD, FSUB, FMUL,    -,  FSL,  TFL, BTFL,  FSR, FDIV
     0, 11, 11, 32, 11, 26, 26, 15, 32, 11,   //    -,   AM,   SM,   MM,   CM,  TDM,  TFM,  BTM,  LDM,   DM
     0, 11, 11, 32, 11, 26, 26, 15, 32, 11,   //    -,    A,    S,    M,    C,   TD,   TF,   BT,   LD,    D
     0, 26, 28, 28, 56, 30, 31, 31, 30, 30,   //    -,   TR,   SF,   CF,    K,   DN,   RN,   RA,   WN,   WA
     0,  1, 18, 28, 28, 28, 18, 18,  1, 18,   //    -,  NOP,   BB,   BD,  BNF,  BNR,   BI,  BNI,    H,    B
     0,  0,  0,  0,  0, 28,  0,  0,  0,  0,   //    -,    -,    -,    -,    -,  BNG,    -,    -,    -,    -
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   //    -,    -,    -,    -,    -,    -,    -,    -,    -,    -
     0, 28, 26, 26,  0,  0,  0,  0,  0,  0,   //    -,   MF,  TNS,  TNF,    -,    -,    -,    -,    -,    -
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   //    -,    -,    -,    -,    -,    -,    -,    -,    -,    -
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0};  //    -,    -,    -,    -,    -,    -,    -,    -,    -,    -
const bool opPIndirects[NUM_OPCYCLES] =
    {FALSE,  TRUE,  TRUE,  TRUE, FALSE,  //     -, FADD, FSUB, FMUL,    -
      TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  //   FSL,  TFL, BTFL,  FSR, FDIV
     FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  //     -,   AM,   SM,   MM,   CM
      TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  //   TDM,  TFM,  BTM,  LDM,   DM
     FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  //     -,    A,    S,    M,    C
      TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  //    TD,   TF,   BT,   LD,    D
     FALSE,  TRUE,  TRUE,  TRUE, FALSE,  //     -,   TR,   SF,   CF,    K
      TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  //    DN,   RN,   RA,   WN,   WA
     FALSE, FALSE, FALSE,  TRUE,  TRUE,  //     -,  NOP,   BB,   BD,  BNF
      TRUE,  TRUE,  TRUE, FALSE,  TRUE,  //   BNR,   BI,  BNI,    H,    B
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
      TRUE, FALSE, FALSE, FALSE, FALSE,  //   BNG,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE,  TRUE,  TRUE,  TRUE, FALSE,  //     -,   MF,  TNS,  TNF,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE}; //     -,    -,    -,    -,    -
const bool opQIndirects[NUM_OPCYCLES] =
    {FALSE,  TRUE,  TRUE,  TRUE, FALSE,  //     -, FADD, FSUB, FMUL,    -
      TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  //   FSL,  TFL, BTFL,  FSR, FDIV
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,   AM,   SM,   MM,   CM
     FALSE, FALSE, FALSE, FALSE, FALSE,  //   TDM,  TFM,  BTM,  LDM,   DM
     FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  //     -,    A,    S,    M,    C
      TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  //    TD,   TF,   BT,   LD,    D
     FALSE,  TRUE, FALSE, FALSE, FALSE,  //     -,   TR,   SF,   CF,    K
     FALSE, FALSE, FALSE, FALSE, FALSE,  //    DN,   RN,   RA,   WN,   WA
     FALSE, FALSE, FALSE,  TRUE,  TRUE,  //     -,  NOP,   BB,   BD,  BNF
      TRUE, FALSE, FALSE, FALSE, FALSE,  //   BNR,   BI,  BNI,    H,    B
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
      TRUE, FALSE, FALSE, FALSE, FALSE,  //   BNG,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE,  TRUE,  TRUE,  TRUE, FALSE,  //     -,   MF,  TNS,  TNF,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE,  //     -,    -,    -,    -,    -
     FALSE, FALSE, FALSE, FALSE, FALSE}; //     -,    -,    -,    -,    -

bool triggers[NUM_TRIGGERS] = {FALSE};
bool cycleTriggers[NUM_TRIGGERS] = {FALSE};
const int triggerLights[NUM_TRIGGER_LIGHTS] =
    {       RUN_LIGHT,   START_CTRL_LIGHT,   INSERT_2_LIGHT, DISP_ADR_3_LIGHT,      SAVE_2_LIGHT, DIV_1_CYCLE_LIGHT,
        DIV_ADD_LIGHT, DIV_LAST_CYC_LIGHT, OFLO_UNFLO_LIGHT,  OFLO_CORR_LIGHT,     RE_COMP_LIGHT,    CARRY_IN_LIGHT,
      CARRY_OUT_LIGHT,   CYCLE_CTRL_LIGHT,      GR_50_LIGHT,   REC_CTRL_LIGHT,        TC_1_LIGHT,          IA_LIGHT,
          DIGIT_LIGHT,     DIV_SIGN_LIGHT, UNFLO_ZERO_LIGHT,    FP_EXIT_LIGHT, FIRST_CYCLE_LIGHT,  FIELD_MK_1_LIGHT,
     FIELD_MK_2_LIGHT,     REC_MARK_LIGHT,       BRCH_LIGHT,      A_ADV_LIGHT,        DECR_LIGHT,        INCR_LIGHT,
         PLUS_2_LIGHT,       BYPASS_LIGHT,        ODD_LIGHT,     I_TIME_LIGHT,      E_TIME_LIGHT,          HP_LIGHT,
             EZ_LIGHT,    MEM_OFLOW_LIGHT,       STOP_LIGHT,  LAST_CARD_LIGHT,  DISCN_GATE_LIGHT,    RSP_GATE_LIGHT,
        RW_CALL_LIGHT,      IO_EXIT_LIGHT,    IO_HOLD_LIGHT,   PCH_FEED_LIGHT,     IO_FLAG_LIGHT,     IO_SYNC_LIGHT,
       RDR_FEED_LIGHT,      MAR_CHK_LIGHT,     RD_CHK_LIGHT,     WR_CHK_LIGHT,     EXP_CHK_LIGHT,   ARITH_CHK_LIGHT};

bool lastSwitches[NUM_GPIOS] = {FALSE};

const char dumpTYChars[NUM_NUMERIC_CHARS] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '|', '|', '@',   0,   0, '}',
     'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', '!', '!', '~',   0,   0, '"'};

const char numericTYChars[NUM_NUMERIC_CHARS] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '#', '#', '@',   0,   0, '#',
     'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', '#', '#', '~',   0,   0, '#'};

const char alphamericTYChars[NUM_ALPHAMERIC_CHARS] =
    {' ',   0,   0, '.', ')',   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
     '+',   0,   0, '$', '*',   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
     '-', '/',   0, ',', '(',   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
       0,   0,   0, '=', '@',   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
       0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', '#', '#',   0,   0,   0, '#',
     '-', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', '#', '#',   0,   0,   0, '#',
       0,   0, 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '#', '#',   0,   0,   0, '#',
     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '#', '#',   0,   0,   0, '#',
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#',
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '#', '#',   0,   0,   0, '#'};

const char dumpCDChars[NUM_NUMERIC_CHARS] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '|', '=', ' ',   0,   0, '}',
     ']', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', '!', '$', '-',   0,   0, '"'};

const char numericCDChars[NUM_NUMERIC_CHARS] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '|', '=', ' ',   0,   0, '}',
     ']', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', '!', '$', '-',   0,   0, '"'};

const char alphamericCDChars[NUM_ALPHAMERIC_CHARS] =
    {' ',   0,   0, '.', ')',   0,   0,   0,   0,   0, '|',   0,   0,   0,   0, '}',
     '+',   0,   0, '$', '*',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     '-', '/',   0, ',', '(',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0, '=', '@',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',   0,   0,   0,   0,   0,   0,
     ']', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', '!',   0,   0,   0,   0, '"',
       0,   0, 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',   0,   0,   0,   0,   0,   0,
     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};


// =================================  Front Panel Data  =================================

int lastMarSelector = OR1_MAR;

struct lights simulatorLights = {{0}, {FALSE}};

char previousLights[NUM_PWMS] = {0};

struct lights powerOffLights =
    {{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
     {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}};

struct lights powerOnLights =
    {{  0,   0,   0, 255,   0,   0,   0, 255,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0, 255,   0, 255,   0, 255,   0,   0,   0, 255,   0,   0, 255,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
     {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE,  TRUE, FALSE,  TRUE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}};

struct lights lampTestLights =
    {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
     {FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,
      FALSE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,
      FALSE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,
      FALSE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}};

struct lights brightnessTestLights =
    {{  0,  14,  16,  21,  18,  21,  16,  14,  23,  14,  23,  23,  18,  18,  16,  18,
       14,  21,  23,  18,  23,  21,  21,  53,  63,  88,  49,  49,  58,  74,  74,  95,
       81,  53,  40, 101,  36,  69,  36,  58,  36,  95,  29,  40,  36,  33,  29,  36,
       74,  95,  49,  63,  29,  26,   0,  26,   0,   0,   0,   0,   0,   0,   0,   0,
        1,  44, 255,  40, 255, 101, 101,  49, 109,  88, 255, 138,  88, 255,  95,  49,
      138,  58,  44, 109, 101, 138,  74, 109,  88, 138,  88, 138,  81,  58,  81, 109,
       53, 109,  81, 255,  63,  95,  44,  81,  53,  74,  40,  69, 101,  58,  69,  44,
       69,  26,  33,  44,  69,  33,  53,  10,  40,  10,  29,  10,  29,  12,  33,  12,
        1,   2,   4,   2,   2,   5,   3,   5,   1,   1,   1,   4,   0,   5,   2,   1,
        3,   1,   0,   5,   3,   0,   7,   3,   6,   1,   3,   3,   3,   2,   0,   3,
        4,   1,   1,   4,   2,   1,   2,   3,   0,   5,   7,   4,  10,   7,   9,   6,
       12,   2,  10,   7,   9,   6,   9,   1,  12,   6,  12,   3,   9,   6,   9,   7},
     {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}};

struct lights snakeTestLights =
    {{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
     {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}};

struct lights ibmTestLights =
    {{  0,   0,   0, 255,   0, 255,   0, 255,   0,   0,   0,   0,   0,   0, 255,   0,
        0, 255,   0, 255,   0, 255, 255,   0, 255,   0,   0, 255, 255,   0, 255,   0,
      255, 255, 255, 255, 255, 255,   0, 255, 255, 255, 255, 255, 255,   0, 255,   0,
      255, 255, 255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0, 255,   0, 255, 255,   0,   0,   0,   0,   0,   0,   0, 255, 255, 255,
      255,   0,   0,   0,   0, 255,   0,   0,   0,   0,   0,   0,   0,   0, 255, 255,
        0, 255,   0,   0,   0,   0, 255,   0, 255, 255, 255, 255,   0, 255, 255, 255,
        0,   0,   0, 255,   0,   0,   0,   0,   0,   0, 255,   0, 255, 255,   0, 255,
        0, 255,   0,   0,   0, 255,   0, 255,   0,   0, 255,   0,   0, 255, 255,   0,
        0,   0,   0, 255,   0,   0,   0,   0, 255,   0,   0, 255,   0, 255,   0,   0,
        0,   0, 255,   0, 255, 255, 255,   0,   0, 255, 255,   0,   0,   0, 255, 255,
      255, 255,   0, 255,   0,   0, 255, 255, 255,   0, 255, 255,   0, 255,   0, 255},
     {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}};

const int snakeUpperLights[7][40] = 
    {{              0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0},
     {              0,         I1_LIGHT,          I6_LIGHT,          E11_LIGHT,        E16_LIGHT,
            E21_LIGHT,        E26_LIGHT,         E31_LIGHT,          E36_LIGHT,        E41_LIGHT,
            E46_LIGHT,        E51_LIGHT,         RUN_LIGHT,  DIV_1_CYCLE_LIGHT,    RE_COMP_LIGHT,
       REC_CTRL_LIGHT,   DIV_SIGN_LIGHT, FIRST_CYCLE_LIGHT,        A_ADV_LIGHT,        ODD_LIGHT,
      MEM_OFLOW_LIGHT, DISCN_GATE_LIGHT,                 0,        OR_C0_LIGHT,      OR_80_LIGHT,
          OR_40_LIGHT,      OR_20_LIGHT,       OR_10_LIGHT,       MBR_EC_LIGHT,     MBR_EF_LIGHT,
         MBR_E8_LIGHT,     MBR_E4_LIGHT,      MBR_E2_LIGHT,       MBR_E1_LIGHT,  MAR_C0000_LIGHT,
      MAR_80000_LIGHT,  MAR_40000_LIGHT,   MAR_20000_LIGHT,    MAR_10000_LIGHT,                0},
     {              0,         I2_LIGHT,          I7_LIGHT,          E12_LIGHT,        E17_LIGHT,
            E22_LIGHT,        E27_LIGHT,         E32_LIGHT,          E37_LIGHT,        E42_LIGHT,
            E47_LIGHT,        E52_LIGHT,  START_CTRL_LIGHT,      DIV_ADD_LIGHT,   CARRY_IN_LIGHT,
           TC_1_LIGHT, UNFLO_ZERO_LIGHT,  FIELD_MK_1_LIGHT,         DECR_LIGHT,     I_TIME_LIGHT,
           STOP_LIGHT,   RSP_GATE_LIGHT,    PCH_FEED_LIGHT,        OR_0C_LIGHT,      OR_08_LIGHT,
          OR_04_LIGHT,      OR_02_LIGHT,       OR_01_LIGHT,       MBR_OC_LIGHT,     MBR_OF_LIGHT,
         MBR_O8_LIGHT,     MBR_O4_LIGHT,      MBR_O2_LIGHT,       MBR_O1_LIGHT,  MAR_0C000_LIGHT,
      MAR_08000_LIGHT,  MAR_04000_LIGHT,   MAR_02000_LIGHT,    MAR_01000_LIGHT,                0},
     {              0,         I3_LIGHT,          I8_LIGHT,          E13_LIGHT,        E18_LIGHT,
            E23_LIGHT,        E28_LIGHT,         E33_LIGHT,          E38_LIGHT,        E43_LIGHT,
            E48_LIGHT,        E53_LIGHT,    INSERT_2_LIGHT, DIV_LAST_CYC_LIGHT,  CARRY_OUT_LIGHT,
             IA_LIGHT,    FP_EXIT_LIGHT,  FIELD_MK_2_LIGHT,         INCR_LIGHT,     E_TIME_LIGHT,
                    0,    RW_CALL_LIGHT,     IO_FLAG_LIGHT,         MQ_C_LIGHT,       MQ_8_LIGHT,
           MQ_4_LIGHT,       MQ_2_LIGHT,        MQ_1_LIGHT,        MDR_C_LIGHT,      MDR_F_LIGHT,
          MDR_8_LIGHT,      MDR_4_LIGHT,       MDR_2_LIGHT,        MDR_1_LIGHT,  MAR_00C00_LIGHT,
      MAR_00800_LIGHT,  MAR_00400_LIGHT,   MAR_00200_LIGHT,    MAR_00100_LIGHT,                0},
     {              0,         I4_LIGHT,          T1_LIGHT,          E14_LIGHT,        E19_LIGHT,
                    0,        E29_LIGHT,         E34_LIGHT,          E39_LIGHT,        E44_LIGHT,
            E49_LIGHT,        E54_LIGHT,  DISP_ADR_3_LIGHT,   OFLO_UNFLO_LIGHT, CYCLE_CTRL_LIGHT,
                    0,                0,    REC_MARK_LIGHT,       PLUS_2_LIGHT,         HP_LIGHT,
      LAST_CARD_LIGHT,    IO_EXIT_LIGHT,     IO_SYNC_LIGHT,       SAB_C0_LIGHT,     SAB_80_LIGHT,
         SAB_40_LIGHT,     SAB_20_LIGHT,      SAB_10_LIGHT,        DR_C0_LIGHT,                0,
          DR_80_LIGHT,      DR_40_LIGHT,       DR_20_LIGHT,        DR_10_LIGHT,  MAR_000C0_LIGHT,
      MAR_00080_LIGHT,  MAR_00040_LIGHT,   MAR_00020_LIGHT,    MAR_00010_LIGHT,                0},
     {              0,         I5_LIGHT,          T2_LIGHT,          E15_LIGHT,        E20_LIGHT,
                    0,        E30_LIGHT,         E35_LIGHT,          E40_LIGHT,        E45_LIGHT,
            E50_LIGHT,        E55_LIGHT,      SAVE_2_LIGHT,    OFLO_CORR_LIGHT,      GR_50_LIGHT,
          DIGIT_LIGHT,                0,        BRCH_LIGHT,       BYPASS_LIGHT,         EZ_LIGHT,
                    0,    IO_HOLD_LIGHT,    RDR_FEED_LIGHT,       SAB_0C_LIGHT,     SAB_08_LIGHT,
         SAB_04_LIGHT,     SAB_02_LIGHT,      SAB_01_LIGHT,        DR_0C_LIGHT,                0,
          DR_08_LIGHT,      DR_04_LIGHT,       DR_02_LIGHT,        DR_01_LIGHT,  MAR_0000C_LIGHT,
      MAR_00008_LIGHT,  MAR_00004_LIGHT,   MAR_00002_LIGHT,    MAR_00001_LIGHT,                0},
     {              0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0,
                    0,                0,                 0,                  0,                0}};

const int snakeLowerLights[19] = 
    {              0, MAR_CHK_LIGHT, 56, 62, WR_CHK_LIGHT, RD_CHK_LIGHT, 61, 60, EXP_CHK_LIGHT,
     ARITH_CHK_LIGHT, EXP_CHK_LIGHT, 60, 61, RD_CHK_LIGHT, WR_CHK_LIGHT, 62, 56, MAR_CHK_LIGHT, 0};
const int snakeStatusLights[21] = 
    {                   0,      POWER_ON_LIGHT, POWER_READY_LIGHT,     THERMAL_LIGHT, PUNCH_NO_FEED_LIGHT,
     READER_NO_FEED_LIGHT,          SAVE_LIGHT,      INSERT_LIGHT,   AUTOMATIC_LIGHT,        MANUAL_LIGHT,
         CHECK_STOP_LIGHT,        MANUAL_LIGHT,   AUTOMATIC_LIGHT,      INSERT_LIGHT,          SAVE_LIGHT,
     READER_NO_FEED_LIGHT, PUNCH_NO_FEED_LIGHT,     THERMAL_LIGHT, POWER_READY_LIGHT,      POWER_ON_LIGHT,
                        0};

const int snakeRowOffsets[8] =    {-1, -1, 0, 1, 1,  1,  0, -1};
const int snakeColumnOffsets[8] = { 0,  1, 1, 1, 0, -1, -1, -1};

const char realisticMap[256] =
    {  0,   0,   0,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   3,   3,   3,
       3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,   6,   6,   6,
       6,   7,   7,   7,   7,   7,   8,   8,   8,   8,   8,   9,   9,   9,   9,   9,
      10,  10,  10,  10,  10,  11,  11,  11,  11,  11,  12,  12,  12,  12,  12,  13,
      13,  13,  13,  13,  14,  14,  14,  14,  14,  15,  15,  15,  15,  15,  16,  16,
      16,  16,  16,  17,  17,  17,  17,  17,  18,  18,  18,  18,  18,  19,  19,  19,
      19,  19,  20,  20,  20,  20,  20,  21,  21,  21,  21,  21,  22,  22,  22,  22,
      22,  23,  23,  23,  23,  23,  24,  24,  24,  24,  24,  25,  25,  25,  25,  25,
      28,  28,  29,  29,  30,  30,  31,  31,  32,  32,  33,  33,  34,  34,  35,  35,
      36,  36,  37,  38,  38,  39,  39,  40,  40,  41,  42,  42,  43,  43,  44,  45,
      45,  46,  47,  47,  48,  49,  49,  50,  51,  51,  52,  53,  53,  54,  55,  56,
      56,  57,  58,  59,  59,  60,  61,  62,  63,  63,  64,  65,  66,  67,  67,  68,
      69,  70,  71,  72,  73,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,
      84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,
     100, 101, 102, 103, 104, 106, 107, 108, 109, 110, 111, 113, 114, 115, 116, 117,
     118, 120, 121, 122, 123, 125, 126, 127, 128, 130, 131, 132, 134, 135, 136, 140};

const char brightMap[256] =
    {  0,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,
       7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,   8,   8,
       8,   8,   8,   9,   9,   9,   9,   9,   9,   9,  10,  10,  10,  10,  10,  10,
      11,  11,  11,  11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  14,  14,
      14,  14,  15,  15,  15,  15,  16,  16,  16,  16,  17,  17,  17,  18,  18,  18,
      19,  19,  19,  20,  20,  20,  21,  21,  21,  22,  22,  22,  23,  23,  24,  24,
      24,  25,  25,  26,  26,  26,  27,  27,  28,  28,  29,  29,  30,  30,  31,  31,
      32,  32,  33,  33,  34,  34,  35,  35,  36,  36,  37,  38,  38,  39,  39,  40,
      41,  41,  42,  42,  43,  44,  44,  45,  46,  46,  47,  48,  48,  49,  50,  51,
      51,  52,  53,  53,  54,  55,  56,  57,  57,  58,  59,  60,  61,  61,  62,  63,
      64,  65,  66,  67,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,
      79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,
      95,  96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
     111, 112, 113, 114, 116, 117, 118, 120, 121, 122, 124, 125, 127, 128, 130, 131,
     132, 134, 135, 137, 138, 140, 141, 143, 145, 146, 148, 149, 151, 152, 154, 156,
     157, 159, 161, 162, 164, 166, 167, 169, 171, 173, 174, 176, 178, 180, 182, 255};

int debounces[NUM_GPIOS] = {0};
const int debounceMaxs[NUM_GPIOS] =
    { DB,  DB,  DB,  DB,  DB,  DB,  DB,  DB,
     INF,  DB,   0,  DB, INF,  DB,   0,  DB,
      DB,  DB,  DB,  DB,  DB,  DB,  DB,  DB,
      DB,  DB,  DB,  DB,  DB,  DB,  DB,  DB,
      DB,  DB,  DB,  DB,  DB,  DB,  DB,  DB,
      DB,  DB,  DB,  DB,  DB,  DB,  DB,  DB};


// =================================  Console Typewriter Data  =================================

int currentPosition = 1;
int leftMarginPosition = 1;
int rightMarginPosition = NUM_TYPEWRITER_POS;
bool tabs[NUM_TYPEWRITER_POS + 1] = {FALSE};

const char *printTYStrings[NUM_IO_CHARS] =
    {// Characters
     //        ' '          '!'          '"'                 '#'                 '$'               '%'              '&'               '''
     //        '('          ')'          '*'                 '+'                 ','               '-'              '.'               '/'
     //        '0'          '1'          '2'                 '3'                 '4'               '5'              '6'               '7'
     //        '8'          '9'          ':'                 ';'                 '<'               '='              '>'               '?'
     //        '@'          'A'          'B'                 'C'                 'D'               'E'              'F'               'G'
     //        'H'          'I'          'J'                 'K'                 'L'               'M'              'N'               'O'
     //        'P'          'Q'          'R'                 'S'                 'T'               'U'              'V'               'W'
     //        'X'          'Y'          'Z'                 '['                 '\'               ']'              '^'               '_'
     //        '`'          'a'          'b'                 'c'                 'd'               'e'              'f'               'g'
     //        'h'          'i'          'j'                 'k'                 'l'               'm'              'n'               '9'
     //        'p'          'q'          'r'                 's'                 't'               'u'              'v'               'w'
     //        'x'          'y'          'z'                 '{'                 '|'               '}'              '~'             '<DEL>'

     // Print strings
               " ", "<flag-rm>", "<flag-gm>",  "<release-start>",                "$",        "<pause>",      "<resume>",      "<set-tab>",
               "(",         ")",         "*",                "+",                ",",              "-",             ".",              "/",
               "0",         "1",         "2",                "3",                "4",              "5",             "6",              "7",
               "8",         "9",     "<tab>",       "<return>\n",     "<left-arrow>",              "=", "<right-arrow>", "<invalid-char>",
               "@",         "A",         "B",                "C",                "D",              "E",             "F",              "G",
               "H",         "I",         "J",                "K",                "L",              "M",             "N",              "O",
               "P",         "Q",         "R",                "S",                "T",              "U",             "V",              "W",
               "X",         "Y",         "Z",    "<left-margin>", "<clear-all-tabs>", "<right-margin>",              "",    "<backspace>",
     "<clear-tab>",    "mode-0",    "mode-1",           "mode-2",           "mode-3",               "",              "",           "ping",
             "ack",  "<flag-0>",  "<flag-1>",         "<flag-2>",         "<flag-3>",       "<flag-4>",      "<flag-5>",       "<flag-6>",
        "<flag-7>",  "<flag-8>",  "<flag-9>",            "slash",                 "",        "unslash",              "",               "",
                "",          "",     "reset", "<margin-release>",    "<record-mark>",   "<group-mark>",              "",               ""};

const bool commandTYFlags[NUM_IO_CHARS] =
    {// Characters
     // ' '    '!'    '"'    '#'    '$'    '%'    '&'    '''    '('    ')'    '*'    '+'    ','    '-'    '.'    '/'
     // '0'    '1'    '2'    '3'    '4'    '5'    '6'    '7'    '8'    '9'    ':'    ';'    '<'    '='    '>'    '?'
     // '@'    'A'    'B'    'C'    'D'    'E'    'F'    'G'    'H'    'I'    'J'    'K'    'L'    'M'    'N'    'O'
     // 'P'    'Q'    'R'    'S'    'T'    'U'    'V'    'W'    'X'    'Y'    'Z'    '['    '\'    ']'    '^'    '_'
     // '`'    'a'    'b'    'c'    'd'    'e'    'f'    'g'    'h'    'i'    'j'    'k'    'l'    'm'    'n'    'o'
     // 'p'    'q'    'r'    's'    't'    'u'    'v'    'w'    'x'    'y'    'z'    '{'    '|'    '}'    '~'  '<DEL>'

     // Command flags
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE,  TRUE,  TRUE,  TRUE,  TRUE, FALSE, FALSE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE,  TRUE, FALSE,  TRUE, FALSE, FALSE, FALSE, FALSE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE};

const int numericTYDigits[NUM_IO_CHARS] =
    {// Characters
     // ' '    '!'    '"'    '#'    '$'    '%'    '&'    '''    '('    ')'    '*'    '+'    ','    '-'    '.'    '/'
     // '0'    '1'    '2'    '3'    '4'    '5'    '6'    '7'    '8'    '9'    ':'    ';'    '<'    '='    '>'    '?'
     // '@'    'A'    'B'    'C'    'D'    'E'    'F'    'G'    'H'    'I'    'J'    'K'    'L'    'M'    'N'    'O'
     // 'P'    'Q'    'R'    'S'    'T'    'U'    'V'    'W'    'X'    'Y'    'Z'    '['    '\'    ']'    '^'    '_'
     // '`'    'a'    'b'    'c'    'd'    'e'    'f'    'g'    'h'    'i'    'j'    'k'    'l'    'm'    'n'    'o'
     // 'p'    'q'    'r'    's'    't'    'u'    'v'    'w'    'x'    'y'    'z'    '{'    '|'    '}'    '~'  '<DEL>'

     // Numeric digits
         -1,  0x1a,  0x1f,    -2,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       0x00,  0x01,  0x02,  0x03,  0x04,  0x05,  0x06,  0x07,  0x08,  0x09,    -1,    -1,    -3,    -1,    -3,    -1,
       0x0c,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
         -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -3,    -1,
         -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  0x10,  0x11,  0x12,  0x13,  0x14,  0x15,  0x16,
       0x17,  0x18,  0x19,    -1,    -1,    -1,    -3,    -1,    -1,    -1,    -1,    -1,  0x0a,  0x0f,  0x1c,    -1};

const int alphamericTYDigits[NUM_IO_CHARS] =
    {// Characters
     // ' '     '!'     '"'     '#'     '$'     '%'     '&'     '''     '('     ')'     '*'     '+'     ','     '-'     '.'     '/'
     // '0'     '1'     '2'     '3'     '4'     '5'     '6'     '7'     '8'     '9'     ':'     ';'     '<'     '='     '>'     '?'
     // '@'     'A'     'B'     'C'     'D'     'E'     'F'     'G'     'H'     'I'     'J'     'K'     'L'     'M'     'N'     'O'
     // 'P'     'Q'     'R'     'S'     'T'     'U'     'V'     'W'     'X'     'Y'     'Z'     '['     '\'     ']'     '^'     '_'
     // '`'     'a'     'b'     'c'     'd'     'e'     'f'     'g'     'h'     'i'     'j'     'k'     'l'     'm'     'n'     'o'
     // 'p'     'q'     'r'     's'     't'     'u'     'v'     'w'     'x'     'y'     'z'     '{'     '|'     '}'     '~'   '<DEL>'

     // Alphameric digits
     0x0000, 0x000a, 0x000f,     -2, 0x0103,     -1,     -1,     -1, 0x0204, 0x0004, 0x0104, 0x0100, 0x0203, 0x0200, 0x0003, 0x0201,
     0x0700, 0x0701, 0x0702, 0x0703, 0x0704, 0x0705, 0x0706, 0x0707, 0x0708, 0x0709,     -1,     -1,     -3, 0x0303,     -3,     -1,
     0x0304, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0506,
     0x0507, 0x0508, 0x0509, 0x0602, 0x0603, 0x0604, 0x0605, 0x0606, 0x0607, 0x0608, 0x0609,     -1,     -1, 0x0500,     -3,     -1,
         -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1, 0x0500,     -1,     -1,     -1,     -1,     -1,     -1,
         -1,     -1,     -1,     -1,     -1,     -1,     -3,     -1,     -1,     -1,     -1,     -1, 0x000a, 0x000f, 0x0304,     -1};

const int charPosition[NUM_IO_CHARS] =
    {// Characters
     //                     ' '                      '!'                      '"'                       '#'
     //                     '$'                      '%'                      '&'                       '''
     //                     '('                      ')'                      '*'                       '+'
     //                     ','                      '-'                      '.'                       '/'
     //                     '0'                      '1'                      '2'                       '3'
     //                     '4'                      '5'                      '6'                       '7'
     //                     '8'                      '9'                      ':'                       ';'
     //                     '<'                      '='                      '>'                       '?'
     //                     '@'                      'A'                      'B'                       'C'
     //                     'D'                      'E'                      'F'                       'G'
     //                     'H'                      'I'                      'J'                       'K'
     //                     'L'                      'M'                      'N'                       'O'
     //                     'P'                      'Q'                      'R'                       'S'
     //                     'T'                      'U'                      'V'                       'W'
     //                     'X'                      'Y'                      'Z'                       '['
     //                     '\'                      ']'                      '^'                       '_'
     //                     '`'                      'a'                      'b'                       'c'
     //                     'd'                      'e'                      'f'                       'g'
     //                     'h'                      'i'                      'j'                       'k'
     //                     'l'                      'm'                      'n'                       '9'
     //                     'p'                      'q'                      'r'                       's'
     //                     't'                      'u'                      'v'                       'w'
     //                     'x'                      'y'                      'z'                       '{'
     //                     '|'                      '}'                      '~'                   '<DEL>'

     // Position actions
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,           POSITION_NONE,           POSITION_NONE,            POSITION_NONE,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,            POSITION_TAB,          POSITION_RETURN,
                  POSITION_LEFT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,            POSITION_NONE,
                  POSITION_NONE,           POSITION_NONE,           POSITION_NONE,            POSITION_LEFT,
                  POSITION_NONE,           POSITION_NONE,           POSITION_NONE,            POSITION_NONE,
                  POSITION_NONE,           POSITION_LEFT,           POSITION_NONE,            POSITION_NONE,
                  POSITION_NONE,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,           POSITION_RIGHT,
                 POSITION_RIGHT,          POSITION_RIGHT,          POSITION_RIGHT,            POSITION_NONE,
                  POSITION_NONE,           POSITION_NONE,           POSITION_NONE,            POSITION_NONE,
                  POSITION_NONE,           POSITION_NONE,           POSITION_NONE,            POSITION_NONE,
                 POSITION_RIGHT,          POSITION_RIGHT,           POSITION_NONE,            POSITION_NONE};

const int charWait[NUM_IO_CHARS] =
    {// Characters
     // ' '    '!'     '"'    '#'    '$'    '%'    '&'    '''    '('    ')'    '*'    '+'    ','    '-'    '.'    '/'
     // '0'    '1'     '2'    '3'    '4'    '5'    '6'    '7'    '8'    '9'    ':'    ';'    '<'    '='    '>'    '?'
     // '@'    'A'     'B'    'C'    'D'    'E'    'F'    'G'    'H'    'I'    'J'    'K'    'L'    'M'    'N'    'O'
     // 'P'    'Q'     'R'    'S'    'T'    'U'    'V'    'W'    'X'    'Y'    'Z'    '['    '\'    ']'    '^'    '_'
     // '`'    'a'     'b'    'c'    'd'    'e'    'f'    'g'    'h'    'i'    'j'    'k'    'l'    'm'    'n'    'o'
     // 'p'    'q'     'r'    's'    't'    'u'    'v'    'w'    'x'    'y'    'z'    '{'    '|'    '}'    '~'  '<DEL>'

     // Character wait time
       5000, 42250, 104250, 46250,  5000,     0,     0,     0,  5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000,
      21250,  5000,   5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000, 12500,  5000,  5000,  5000,  5000, 19750,
       5000,  5000,   5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000,
       5000,  5000,   5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000,  5000,     0,     0,     0, 10000,  5000,
          0,     0,      0,     0,     0,     0,     0,     0,     0, 61750, 34750, 34750, 34750, 34750, 34750, 34750,
      34750, 34750,  34750,     0,     0,     0, 10000,     0,     0,     0,     0,     0, 17250, 75250, 23250,     0};


// =================================  Punched Card Data  =================================

volatile bool readerReady = FALSE;
volatile bool readerBusy = FALSE;
volatile bool readerCheck = FALSE;
volatile bool readerLastCard = FALSE;
volatile bool readerLoad = FALSE;
volatile bool punchReady = FALSE;
volatile bool punchBusy = FALSE;
volatile bool punchCheck = FALSE;

const char *printCDStrings[NUM_IO_CHARS] =
    {// Characters
     //        ' '             '!'                 '"'                '#'                '$'                 '%'                '&'               '''
     //        '('             ')'                 '*'                '+'                ','                 '-'                '.'               '/'
     //        '0'             '1'                 '2'                '3'                '4'                 '5'                '6'               '7'
     //        '8'             '9'                 ':'                ';'                '<'                 '='                '>'               '?'
     //        '@'             'A'                 'B'                'C'                'D'                 'E'                'F'               'G'
     //        'H'             'I'                 'J'                'K'                'L'                 'M'                'N'               'O'
     //        'P'             'Q'                 'R'                'S'                'T'                 'U'                'V'               'W'
     //        'X'             'Y'                 'Z'                '['                '\'                 ']'                '^'               '_'
     //        '`'             'a'                 'b'                'c'                'd'                 'e'                'f'               'g'
     //        'h'             'i'                 'j'                'k'                'l'                 'm'                'n'               '9'
     //        'p'             'q'                 'r'                's'                't'                 'u'                'v'               'w'
     //        'x'             'y'                 'z'                '{'                '|'                 '}'                '~'             '<DEL>'

     // Print strings
               " ",    "<flag-rm>",        "<flag-gm>",                "",               "$",    "<special-rgm>",      "<flag-rgm>",               "",
               "(",            ")",                "*",               "+",               ",",                "-",               ".",              "/",
               "0",            "1",                "2",               "3",               "4",                "5",               "6",              "7",
               "8",            "9",     "READER-CHECK",     "PUNCH_CHECK",                "",                "=",                "", "<invalid-char>",
               "@",            "A",                "B",               "C",               "D",                "E",               "F",              "G",
               "H",            "I",                "J",               "K",               "L",                "M",               "N",              "O",
               "P",            "Q",                "R",               "S",               "T",                "U",               "V",              "W",
               "X",            "Y",                "Z", "READER-NOT-BUSY",     "READER-BUSY",                "]",      "PUNCH-BUSY",       "SHUTDOWN",
                "", "READER-READY", "READER-NOT-READY",     "PUNCH-READY", "PUNCH-NOT-READY", "READER-LAST-CARD",     "READER-LOAD",               "",
                "",     "<flag-0>",         "<flag-1>",        "<flag-2>",        "<flag-3>",         "<flag-4>",        "<flag-5>",       "<flag-6>",
        "<flag-7>",     "<flag-8>",         "<flag-9>",          "MANUAL",      "NOT-MANUAL",     "READ-NUMERIC", "READ-ALPHAMERIC",  "WRITE-NUMERIC",
"WRITE-ALPHAMERIC", "DUMP-NUMERIC",            "RESET",  "PUNCH-NOT-BUSY",   "<record-mark>",     "<group-mark>",       "<flag-nb>",               ""};

const bool commandCDFlags[NUM_IO_CHARS] =
    {// Characters
     // ' '    '!'    '"'    '#'    '$'    '%'    '&'    '''    '('    ')'    '*'    '+'    ','    '-'    '.'    '/'
     // '0'    '1'    '2'    '3'    '4'    '5'    '6'    '7'    '8'    '9'    ':'    ';'    '<'    '='    '>'    '?'
     // '@'    'A'    'B'    'C'    'D'    'E'    'F'    'G'    'H'    'I'    'J'    'K'    'L'    'M'    'N'    'O'
     // 'P'    'Q'    'R'    'S'    'T'    'U'    'V'    'W'    'X'    'Y'    'Z'    '['    '\'    ']'    '^'    '_'
     // '`'    'a'    'b'    'c'    'd'    'e'    'f'    'g'    'h'    'i'    'j'    'k'    'l'    'm'    'n'    'o'
     // 'p'    'q'    'r'    's'    't'    'u'    'v'    'w'    'x'    'y'    'z'    '{'    '|'    '}'    '~'  '<DEL>'

     // Command flags
      FALSE, FALSE, FALSE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,
      FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
      FALSE, FALSE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE};

const int numericCDDigits[NUM_IO_CHARS] =
    {// Characters
     // ' '    '!'    '"'    '#'    '$'    '%'    '&'    '''    '('    ')'    '*'    '+'    ','    '-'    '.'    '/'
     // '0'    '1'    '2'    '3'    '4'    '5'    '6'    '7'    '8'    '9'    ':'    ';'    '<'    '='    '>'    '?'
     // '@'    'A'    'B'    'C'    'D'    'E'    'F'    'G'    'H'    'I'    'J'    'K'    'L'    'M'    'N'    'O'
     // 'P'    'Q'    'R'    'S'    'T'    'U'    'V'    'W'    'X'    'Y'    'Z'    '['    '\'    ']'    '^'    '_'
     // '`'    'a'    'b'    'c'    'd'    'e'    'f'    'g'    'h'    'i'    'j'    'k'    'l'    'm'    'n'    'o'
     // 'p'    'q'    'r'    's'    't'    'u'    'v'    'w'    'x'    'y'    'z'    '{'    '|'    '}'    '~'  '<DEL>'

     // Numeric digits
       0x00,  0x1a,  0x1f,    -1,  0x1b,    -1,    -1,    -1,  0x0c,  0x0c,  0x1c,  0x00,  0x0b,  0x10,  0x0b,  0x01,
       0x00,  0x01,  0x02,  0x03,  0x04,  0x05,  0x06,  0x07,  0x08,  0x09,    -1,    -1,    -1,  0x0b,    -1,    -1,
       0x0c,  0x01,  0x02,  0x03,  0x04,  0x05,  0x06,  0x07,  0x08,  0x09,  0x11,  0x12,  0x13,  0x14,  0x15,  0x16,
       0x17,  0x18,  0x19,  0x02,  0x03,  0x04,  0x05,  0x06,  0x07,  0x08,  0x09,    -1,    -1,  0x10,    -1,    -1,
         -1,  0x01,  0x02,  0x03,  0x04,  0x05,  0x06,  0x07,  0x08,  0x09,  0x11,  0x12,  0x13,  0x14,  0x15,  0x16,
       0x17,  0x18,  0x19,  0x02,  0x03,  0x04,  0x05,  0x06,  0x07,  0x08,  0x09,    -1,  0x0a,  0x0f,  0x1c,    -1};

const int alphamericCDDigits[NUM_IO_CHARS] =
    {// Characters
     // ' '     '!'     '"'     '#'     '$'     '%'     '&'     '''     '('     ')'     '*'     '+'     ','     '-'     '.'     '/'
     // '0'     '1'     '2'     '3'     '4'     '5'     '6'     '7'     '8'     '9'     ':'     ';'     '<'     '='     '>'     '?'
     // '@'     'A'     'B'     'C'     'D'     'E'     'F'     'G'     'H'     'I'     'J'     'K'     'L'     'M'     'N'     'O'
     // 'P'     'Q'     'R'     'S'     'T'     'U'     'V'     'W'     'X'     'Y'     'Z'     '['     '\'     ']'     '^'     '_'
     // '`'     'a'     'b'     'c'     'd'     'e'     'f'     'g'     'h'     'i'     'j'     'k'     'l'     'm'     'n'     'o'
     // 'p'     'q'     'r'     's'     't'     'u'     'v'     'w'     'x'     'y'     'z'     '{'     '|'     '}'     '~'   '<DEL>'

     // Alphameric digits
     0x0000, 0x000a, 0x000f,     -1, 0x0103,     -1,     -1,     -1, 0x0204, 0x0004, 0x0104, 0x0100, 0x0203, 0x0200, 0x0003, 0x0201,
     0x0700, 0x0701, 0x0702, 0x0703, 0x0704, 0x0705, 0x0706, 0x0707, 0x0708, 0x0709,     -1,     -1,     -1, 0x0303,     -1,     -1,
     0x0304, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0506,
     0x0507, 0x0508, 0x0509, 0x0602, 0x0603, 0x0604, 0x0605, 0x0606, 0x0607, 0x0608, 0x0609,     -1,     -1, 0x0500,     -1,     -1,
         -1, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0506,
     0x0507, 0x0508, 0x0509, 0x0602, 0x0603, 0x0604, 0x0605, 0x0606, 0x0607, 0x0608, 0x0609,     -1, 0x000a, 0x000f, 0x0000,     -1};

const int protocolCDActions[NUM_IO_CHARS] =
    {// Characters
     //                 ' '                      '!'                      '"'                     '#'
     //                 '$'                      '%'                      '&'                     '''
     //                 '('                      ')'                      '*'                     '+'
     //                 ','                      '-'                      '.'                     '/'
     //                 '0'                      '1'                      '2'                     '3'
     //                 '4'                      '5'                      '6'                     '7'
     //                 '8'                      '9'                      ':'                     ';'
     //                 '<'                      '='                      '>'                     '?'
     //                 '@'                      'A'                      'B'                     'C'
     //                 'D'                      'E'                      'F'                     'G'
     //                 'H'                      'I'                      'J'                     'K'
     //                 'L'                      'M'                      'N'                     'O'
     //                 'P'                      'Q'                      'R'                     'S'
     //                 'T'                      'U'                      'V'                     'W'
     //                 'X'                      'Y'                      'Z'                     '['
     //                 '\'                      ']'                      '^'                     '_'
     //                 '`'                      'a'                      'b'                     'c'
     //                 'd'                      'e'                      'f'                     'g'
     //                 'h'                      'i'                      'j'                     'k'
     //                 'l'                      'm'                      'n'                     'o'
     //                 'p'                      'q'                      'r'                     's'
     //                 't'                      'u'                      'v'                     'w'
     //                 'x'                      'y'                      'z'                     '{'
     //                 '|'                      '}'                      '~'                 '<DEL>'

     // Protocol actions
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,  ACTION_REQUEST_STATUS,
              ACTION_ACCEPT,            ACTION_ERROR,            ACTION_ERROR,           ACTION_ERROR,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,     ACTION_READER_CHECK,     ACTION_PUNCH_CHECK,
               ACTION_ERROR,           ACTION_ACCEPT,            ACTION_ERROR,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT, ACTION_READER_NOT_BUSY,
         ACTION_READER_BUSY,           ACTION_ACCEPT,       ACTION_PUNCH_BUSY,           ACTION_ERROR,
               ACTION_ERROR,     ACTION_READER_READY, ACTION_READER_NOT_READY,     ACTION_PUNCH_READY,
     ACTION_PUNCH_NOT_READY, ACTION_READER_LAST_CARD,      ACTION_READER_LOAD,           ACTION_ERROR,
               ACTION_ERROR,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_ACCEPT,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ERROR,
               ACTION_ERROR,            ACTION_ERROR,            ACTION_ERROR,           ACTION_ERROR,
               ACTION_ERROR,            ACTION_ERROR,            ACTION_ERROR,  ACTION_PUNCH_NOT_BUSY,
              ACTION_ACCEPT,           ACTION_ACCEPT,           ACTION_ACCEPT,          ACTION_IGNORE};
