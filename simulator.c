//======================================================================================================================
//
//  simulator.c - simulator thread
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
#include "io.h"
#include "typewriter.h"
#include "utilities.h"
#include "simulator.h"

#define _GNU_SOURCE 1
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#ifdef HARDWARE_TIMER
#include <sys/syscall.h>
#endif
#include <sys/types.h>

// Function declarations
void *Simulator(void *arg);
static        void InitializeSimulator(void);
static        void ResetSimulator(void);
static inline bool TestLiveSwitch(int sw);
static inline bool TestSwitch(int sw);
static inline void SetLight(int lite, bool offOn);
static        void UpdateDisplay(void);
static inline bool ValidAddress(int addr);
static inline bool ValidEvenAddress(int addr);
static inline bool ValidOddAddress(int addr);
static        bool ReadMemory(int marsId);
static        bool ReadPairMemory(int marsId, int align);
static        bool WriteMemory(int marsId);
static        bool WritePairMemory(int marsId, int value, int align);
static inline void ClearTriggers(void);
static inline void SetTrigger(int trig, bool offOn);
static inline bool TestTrigger(int trig);
static inline bool TestCycleTrigger(int trig);
static        bool TestBranch(int bc);
static inline void ClearRegisters(void);
static inline void ClearMAR(void);
static inline void CopyMAR(int dst, int src);
static inline void SetMAR(int marsId, int value);
static inline void LoadMAR(int marsId);
static inline void IncrementMAR(int marsId, int inc);
static inline void SaveMAR(int marsId);
static inline void ClearMARs(void);
static inline void ClearSingleMAR(int marsId);
static inline void ClearMBR(void);
static inline void ClearMDR(void);
static inline void ClearDR(void);
static inline void ClearOpR(void);
static inline void ClearMQR(void);
static inline void ClearSABR(void);
static inline void SetDigit(volatile struct digit *d, int value);
static inline void CopyDigit(volatile struct digit *dst, volatile struct digit *src);
static inline void SetMBR(int valueEven, int valueOdd);
static inline void SetMDR(int value);
static inline void SetDR(int value10, int value01);
static inline void SetOpR(int value10, int value01);
static inline void SetMQR(int value);
static inline void SetSABR(int value10, int value01);
static inline void SetMDRFlag(bool flag);
static inline void TimeLog(char *msg);
static        void PrintRegs(void);
static        void PrintLegend(void);

// Data
pthread_t simulatorThread;


// Start simulator
void StartSimulator(void) {
    int rc = pthread_create(&simulatorThread, NULL, Simulator, NULL);
    if (rc != 0) {
        (void)printf("Failed to create simulator thread: %s\n", strerror(rc));
        exit(-1);
    }
}

// Stop simulator
void StopSimulator(void) {
    (void)pthread_join(simulatorThread, NULL);
}

// Simulator thread
void *Simulator(void *arg) {
    int powerDelay = 0;
    long long syncTime = 0L;
#ifdef HARDWARE_TIMER
    int timerFile = 0;
    char *timerFilename = (char*)"/dev/mem";
    void *timerMmap = NULL;
    volatile long long *counter = NULL;
#else
    int cycleCounter = CYCLE_COUNT;
    struct timespec currentTime = { 0, 0L };
    struct timespec sleepTime = { 0, 0L };
#endif
    int temp = 0;
    int temp2 = 0;
    char chr = 0;
    int value = 0;
    pthread_t threadId = pthread_self();

    // Lock simulator thread on CPU 1
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(1, &cpus);
    if (pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpus) != 0) {
        (void)printf("Simulator CPU affinity failed: %s\n", strerror(errno));
        exit(-1);
    }
    // Set scheduler type and priority
    struct sched_param sparam = { 1 };
    if (pthread_setschedparam(threadId, SCHED_FIFO, &sparam) != 0) {
        (void)printf("Simulator SCHED_FIFO scheduler failed: %s\n", strerror(errno));
        exit(-1);
    }

    // Adjust signal mask for this thread
    sigset_t blockSignals;
    sigemptyset(&blockSignals);
    sigaddset(&blockSignals, SIGABRT);
    sigaddset(&blockSignals, SIGHUP);
    sigaddset(&blockSignals, SIGINT);
    sigaddset(&blockSignals, SIGQUIT);
    sigaddset(&blockSignals, SIGTERM);
    sigaddset(&blockSignals, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &blockSignals, NULL);

    // Initialize simulator
    powerState = POWER_OFF;
    InitializeSimulator();

#ifdef HARDWARE_TIMER
    // Open the system timer
    timerFile = open(timerFilename, O_RDONLY);
    if (timerFile < 0) {
        (void)printf("Failed to open system timer: %s\n", strerror(errno));
        exit(-1);
    }
    timerMmap = mmap(NULL, BCM2835_PAGE_SIZE, PROT_READ, MAP_SHARED, timerFile, BCM2835_SYSTEM_TIMER);
    if (timerMmap == MAP_FAILED) {
        (void)printf("Timer mmap failed: %s\n", strerror(errno));
        exit(-1);
    }
    counter = (long long*)(timerMmap + BCM2835_COUNTER_OFFSET);

// (void)printf("counter = %x\n", counter);
// volatile int *tptr = timerMmap;
// for (int i = 0; i < 7; ++i) {
//     (void)printf("  %x\n", *tptr++);
// }
// long long stt = *counter;
// for (int i = 0; i < 100; ++i) (void)syscall(SYS_gettid);
// long long stp = *counter;
// (void)printf("start = %lld\nstop  = %lld\n", stt, stp);

#endif

    // Get sync time baseline
#ifdef HARDWARE_TIMER
    syncTime = *counter;
#else
    (void)clock_gettime(CLOCK_MONOTONIC, &currentTime);
    syncTime = 1000000000LL * currentTime.tv_sec + currentTime.tv_nsec;
#endif

    while (simulationRunning) {

        lastRunState = runState;
        (void)memcpy((void *)&cycleTriggers, (void *)&triggers, sizeof(triggers));

        // Special built-in program load
        if ((powerState == POWER_OFF) && TestSwitch(RESET_SWITCH)) {
            temp = (TestSwitch(PROGRAM_1_SWITCH) ? 0x8 : 0) | (TestSwitch(PROGRAM_2_SWITCH) ? 0x4 : 0) |
                   (TestSwitch(PROGRAM_3_SWITCH) ? 0x2 : 0) | (TestSwitch(PROGRAM_4_SWITCH) ? 0x1 : 0);
            cmemCount = 1;
            cmemFiles[0] = programFiles[temp];
            if (cmemFiles[0] != NULL) {
                LoadMemory(cmemCount, cmemFiles);
                SetTabs();
            }
        }

        // Special display mar setting
        if ((powerState == POWER_OFF) && TestSwitch(DISPLAY_MAR_SWITCH)) {
            displayMAROpt = (displayMAROpt == FALSE) ? TRUE : FALSE;
        }

        // Process emergency off power state
        if (powerState == POWER_EMERGENCY_OFF) {
            if (!TestSwitch(EMERGENCY_OFF_SWITCH)) {
                if (TestSwitch(POWER_SWITCH)) {
                    InitializeSimulator();
                    powerState = POWER_ON;
                    powerDelay = (nodelayOpt ? 0 : POWER_ON_WAIT);
                    SetLight(POWER_ON_LIGHT, ON);
                    WriteCDPowerStatus(CD_COMMAND_POWER_ON);
                    TimeLog("Power On");
                } else {
                    powerState = POWER_OFF;
                    runState = STOPPED_STATE;
                    ioState = STOPPED_IO_STATE;
                    WriteCDPowerStatus(CD_COMMAND_POWER_OFF);
                    TimeLog("Power Off");
                }
            }
            goto End_Cycle;

        // Process power off state
        } else if (powerState == POWER_OFF) {
            if (TestSwitch(EMERGENCY_OFF_SWITCH)) {
                powerState = POWER_EMERGENCY_OFF;
                runState = STOPPED_STATE;
                ioState = STOPPED_IO_STATE;
                WriteCDPowerStatus(CD_COMMAND_POWER_OFF);
                TimeLog("Emergency Off");
            } else if (TestSwitch(POWER_SWITCH)) {
                InitializeSimulator();
                powerState = POWER_ON;
                powerDelay = (nodelayOpt ? 0 : POWER_ON_WAIT);
                SetLight(POWER_ON_LIGHT, ON);
                WriteCDPowerStatus(CD_COMMAND_POWER_ON);
                TimeLog("Power On");
            }
            goto End_Cycle;

        // Process power on state
        } else if (powerState == POWER_ON) {
            if (TestSwitch(EMERGENCY_OFF_SWITCH)) {
                powerState = POWER_EMERGENCY_OFF;
                runState = STOPPED_STATE;
                ioState = STOPPED_IO_STATE;
                WriteCDPowerStatus(CD_COMMAND_POWER_OFF);
                TimeLog("Emergency Off");
            } else if (TestSwitch(POWER_SWITCH)) {
                if (--powerDelay <= 0) {
                    powerState = POWER_READY;
                    SetLight(POWER_READY_LIGHT, ON);
                    WriteCDPowerStatus(CD_COMMAND_POWER_ON);
                    TimeLog("Power Ready");
                }
            } else {
                if (!TestLiveSwitch(RESET_SWITCH)) {
                    powerState = POWER_OFF;
                    runState = STOPPED_STATE;
                    ioState = STOPPED_IO_STATE;
                    WriteCDPowerStatus(CD_COMMAND_POWER_OFF);
                    TimeLog("Power Off");
                } else {
                    powerState = POWER_SHUTDOWN;
                    runState = STOPPED_STATE;
                    ioState = STOPPED_IO_STATE;
                    WriteCDPowerStatus(CD_COMMAND_POWER_OFF);
                    TimeLog("Shutdown");
                    kill(getpid(), SIGTERM);
                }
            }
            goto End_Cycle;

        // Process power ready state
        } else if (powerState == POWER_READY) {
            if (TestSwitch(EMERGENCY_OFF_SWITCH)) {
                powerState = POWER_EMERGENCY_OFF;
                runState = STOPPED_STATE;
                ioState = STOPPED_IO_STATE;
                WriteCDPowerStatus(CD_COMMAND_POWER_OFF);
                TimeLog("Emergency Off");
                goto End_Cycle;
            } else if (!TestSwitch(POWER_SWITCH)) {
                if (!TestLiveSwitch(RESET_SWITCH)) {
                    powerState = POWER_OFF;
                    runState = STOPPED_STATE;
                    ioState = STOPPED_IO_STATE;
                    WriteCDPowerStatus(CD_COMMAND_POWER_OFF);
                    TimeLog("Power Off");
                } else {
                    powerState = POWER_SHUTDOWN;
                    runState = STOPPED_STATE;
                    ioState = STOPPED_IO_STATE;
                    WriteCDPowerStatus(CD_COMMAND_POWER_OFF);
                    TimeLog("Shutdown");
                    kill(getpid(), SIGTERM);
                }
                goto End_Cycle;
            }

        // Process power shutdown state
        } else if (powerState == POWER_SHUTDOWN) {
            goto End_Cycle;
        }

        // Process LOAD operation and update INSERT light
        if (readerLoad && (runState == STOPPED_STATE)) {
            readerLoad = FALSE;
            SetTrigger(INSERT_TRIGGER, ON);
            SetTrigger(INSERT_2_TRIGGER, ON);
            ClearMAR();
            ClearSingleMAR(IR1_MAR);
            ClearSingleMAR(OR2_MAR);
            ClearSABR();
            runState = LOAD_STATE;
            BeginIOInstruction(LOAD_IO);
            opCode = RN_OPCODE;
            cycle = 31;
            SetTrigger(MEM_OFLOW_TRIGGER, OFF);
            SetTrigger(STOP_TRIGGER, OFF);
            SetTrigger(DISCN_GATE_TRIGGER, OFF);
            SetTrigger(RSP_GATE_TRIGGER, OFF);
            SetTrigger(RW_CALL_TRIGGER, ON);
            SetTrigger(IO_HOLD_TRIGGER, ON);
            SetTrigger(IO_FLAG_TRIGGER, OFF);
            SetTrigger(IO_SYNC_TRIGGER, OFF);
            SetTrigger(RDR_FEED_TRIGGER, ON);
            if (!readerReady) SetLight(READER_NO_FEED_LIGHT, ON);
            TimeLog("Load");
        }
        SetLight(INSERT_LIGHT, ((runState == INSERT_STATE) || (runState == LOAD_STATE)));

        // Process RESET switch
        if (TestSwitch(RESET_SWITCH) && (((runState == CHECK_STOP_STATE) || (runState == STOPPED_STATE)) ||
                                         TestLiveSwitch(RELEASE_SWITCH))) {
            ResetSimulator();
            TimeLog("Reset");
        }

        // Process DISPLAY MAR switch
        if (TestSwitch(DISPLAY_MAR_SWITCH) && (((runState == STOPPED_STATE) && (cycle == 1)) || displayMAROpt)) {
            LoadMAR(marSelector);
            switch (marSelector) {
                case OR1_MAR:    TimeLog("Display MAR (OR-1)");   break;
                case OR2_MAR:    TimeLog("Display MAR (OR-2)");   break;
                case OR3_MAR:    TimeLog("Display MAR (OR-3)");   break;
                case PR1_MAR:    TimeLog("Display MAR (PR-1)");   break;
                case PR2_MAR:    TimeLog("Display MAR (PR-2)");   break;
                case PR3_MAR:    TimeLog("Display MAR (PR-3)");   break;
                case OR4_MAR:    TimeLog("Display MAR (OR-4)");   break;
                case OR5_MAR:    TimeLog("Display MAR (OR-5)");   break;
                case CR1_MAR:    TimeLog("Display MAR (CR-1)");   break;
                case IR3_MAR:    TimeLog("Display MAR (IR-3)");   break;
                case IR1_MAR:    TimeLog("Display MAR (IR-1)");   break;
                case IR2_MAR:    TimeLog("Display MAR (IR-2)");   break;
                default:         TimeLog("Display MAR");          break;
            };
        }

        // Process SAVE switch and update SAVE light
        if (TestSwitch(SAVE_SWITCH) && ((runState == STOPPED_STATE) && (cycle == 1))) {
            LoadMAR(IR1_MAR);
            SaveMAR(PR1_MAR);
            SetTrigger(SAVE_CTRL_TRIGGER, ON);
            SetTrigger(SAVE_2_TRIGGER, ON);
            TimeLog("Save");
        }
        SetLight(SAVE_LIGHT, TestTrigger(SAVE_CTRL_TRIGGER));

        // Process INSERT switch and update INSERT light
        if (TestSwitch(INSERT_SWITCH) && ((runState == STOPPED_STATE) && (cycle == 1))) {
            SetTrigger(INSERT_TRIGGER, ON);
            SetTrigger(INSERT_2_TRIGGER, ON);
            ClearMAR();
            ClearSingleMAR(IR1_MAR);
            ClearSingleMAR(OR2_MAR);
            ClearSABR();
            runState = INSERT_STATE;
            BeginIOInstruction(INSERT_IO);
            opCode = RN_OPCODE;
            cycle = 31;
            SetTrigger(MEM_OFLOW_TRIGGER, OFF);
            SetTrigger(STOP_TRIGGER, OFF);
            SetTrigger(DISCN_GATE_TRIGGER, OFF);
            SetTrigger(RSP_GATE_TRIGGER, OFF);
            SetTrigger(RW_CALL_TRIGGER, ON);
            SetTrigger(IO_HOLD_TRIGGER, ON);
            SetTrigger(IO_FLAG_TRIGGER, OFF);
            SetTrigger(IO_SYNC_TRIGGER, OFF);
            TimeLog("Insert");
        }
        SetLight(INSERT_LIGHT, ((runState == INSERT_STATE) || (runState == LOAD_STATE)));

        // Process RELEASE switch
        if (TestSwitch(RELEASE_SWITCH)) {
            if (TestLiveSwitch(RESET_SWITCH)) {
                ResetSimulator();
            } else if (ioState == RUNNING_IO_STATE) {
                SetTrigger(INSERT_TRIGGER, OFF);
                SetTrigger(INSERT_2_TRIGGER, OFF);
                runState = SINGLE_INSTRUCTION_STATE;
                ioState = ABORTED_IO_STATE;
                SetTrigger(MEM_OFLOW_TRIGGER, ON);
                TimeLog("Release");
            }
        }

        // Process START switch
        if (TestSwitch(START_SWITCH) && ((runState == STOPPED_STATE) ||
                                         (runState == SINGLE_CYCLE_STATE) ||
                                         (runState == SINGLE_INSTRUCTION_STATE))) {
            runState = RUNNING_STATE;
            SetTrigger(START_CTRL_TRIGGER, ON);
            TimeLog("Start");
        }

        // Process STOP / SIE switch
        if (TestSwitch(STOP_SIE_SWITCH) && ((runState == STOPPED_STATE) ||
                                            (runState == SINGLE_CYCLE_STATE) ||
                                            (runState == SINGLE_INSTRUCTION_STATE) ||
                                            (runState == RUNNING_STATE))) {
            runState = SINGLE_INSTRUCTION_STATE;
            SetTrigger(START_CTRL_TRIGGER, OFF);
            if (lastRunState == RUNNING_STATE) {
                TimeLog("Stop");
            } else {
                TimeLog("Single Instruction");
            }
        }

        // Process INSTANT STOP / SCE switch
        if (TestSwitch(INSTANT_STOP_SCE_SWITCH) && ((runState == STOPPED_STATE) ||
                                                    (runState == SINGLE_CYCLE_STATE) ||
                                                    (runState == SINGLE_INSTRUCTION_STATE) ||
                                                    (runState == RUNNING_STATE))) {
            runState = SINGLE_CYCLE_STATE;
            SetTrigger(START_CTRL_TRIGGER, OFF);
            if (lastRunState == RUNNING_STATE) {
                TimeLog("Instant Stop");
            } else {
                TimeLog("Single Cycle");
            }
        }

        // Update PUNCH NO FEED and READER NO FEED lights
        if (punchReady) SetLight(PUNCH_NO_FEED_LIGHT, OFF);
        if (readerReady) SetLight(READER_NO_FEED_LIGHT, OFF);

        // Update AUTOMATIC / MANUAL / CHECK STOP lights
        if (runState == CHECK_STOP_STATE) {
            SetTrigger(RUN_TRIGGER, OFF);
            SetTrigger(START_CTRL_TRIGGER, OFF);
            SetLight(AUTOMATIC_LIGHT, OFF);
            SetLight(MANUAL_LIGHT, ON);
            SetLight(CHECK_STOP_LIGHT, ON);
            WriteCDRunStatus(CD_COMMAND_NOT_MANUAL);

        } else if (runState == STOPPED_STATE) {
            SetTrigger(RUN_TRIGGER, OFF);
            SetTrigger(START_CTRL_TRIGGER, OFF);
            if (cycle == 1) {    // special case for single cycle
                SetLight(AUTOMATIC_LIGHT, OFF);
                WriteCDRunStatus(CD_COMMAND_MANUAL);
            } else {
                SetLight(AUTOMATIC_LIGHT, ON);
                WriteCDRunStatus(CD_COMMAND_NOT_MANUAL);
            }
            SetLight(MANUAL_LIGHT, ON);
            SetLight(CHECK_STOP_LIGHT, OFF);

        } else if (runState == INSERT_STATE) {
            SetTrigger(RUN_TRIGGER, ON);
            SetLight(AUTOMATIC_LIGHT, ON);
            SetLight(MANUAL_LIGHT, OFF);
            SetLight(CHECK_STOP_LIGHT, OFF);
            WriteCDRunStatus(CD_COMMAND_NOT_MANUAL);

        } else if (runState == LOAD_STATE) {
            SetTrigger(RUN_TRIGGER, ON);
            SetLight(AUTOMATIC_LIGHT, ON);
            SetLight(MANUAL_LIGHT, OFF);
            SetLight(CHECK_STOP_LIGHT, OFF);
            WriteCDRunStatus(CD_COMMAND_NOT_MANUAL);

        } else if (runState == SINGLE_CYCLE_STATE) {
            SetTrigger(RUN_TRIGGER, ON);
            SetLight(AUTOMATIC_LIGHT, ON);
            SetLight(MANUAL_LIGHT, ON);
            SetLight(CHECK_STOP_LIGHT, OFF);
            WriteCDRunStatus(CD_COMMAND_NOT_MANUAL);

        } else if (runState == SINGLE_INSTRUCTION_STATE) {
            SetTrigger(RUN_TRIGGER, ON);
            SetLight(AUTOMATIC_LIGHT, ON);
            SetLight(MANUAL_LIGHT, OFF);
            SetLight(CHECK_STOP_LIGHT, OFF);
            WriteCDRunStatus(CD_COMMAND_NOT_MANUAL);

        } else /* runState == RUNNING_STATE */ {
            SetTrigger(RUN_TRIGGER, ON);
            SetLight(AUTOMATIC_LIGHT, ON);
            SetLight(MANUAL_LIGHT, OFF);
            SetLight(CHECK_STOP_LIGHT, OFF);
            WriteCDRunStatus(CD_COMMAND_NOT_MANUAL);
        }

        // Transition from E-cycle to I-cycle
        if ((cycle == 1) && (lastCycle != 1)) {
            SetTrigger(I_CYCLE_ENTRY_TRIGGER, ON);
            SetTrigger(I_TIME_TRIGGER, ON);
            SetTrigger(E_TIME_TRIGGER, OFF);
        }

        // Transition from I-cycle to E-cycle
        if ((cycle > 8) && (cycle != 56) && (lastCycle == 8)) {
            SetTrigger(E_CYCLE_ENTRY_TRIGGER, ON);
            SetTrigger(I_TIME_TRIGGER, OFF);
            SetTrigger(E_TIME_TRIGGER, ON);
        }

        // Process control instruction
        if ((cycle == 56) && (lastCycle == 8)) {
            if (sabr.value == 1) {
                SetTrigger(INCR_TRIGGER, ON);
                SetTrigger(DECR_TRIGGER, OFF);
                SetTrigger(PLUS_2_TRIGGER, OFF);
                SetTrigger(MEM_OFLOW_TRIGGER, OFF);
                SetTrigger(STOP_TRIGGER, OFF);
                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                SetTrigger(RSP_GATE_TRIGGER, OFF);
                SetTrigger(IO_FLAG_TRIGGER, OFF);
                SetTrigger(IO_SYNC_TRIGGER, OFF);
                temp = mars[OR1_MAR].digit00001.value;
                if (temp == 1) {
                    BeginIOInstruction(SPTY_IO);
                    SetTrigger(IO_HOLD_TRIGGER, ON);
                } else if (temp == 2) {
                    BeginIOInstruction(RCTY_IO);
                    SetTrigger(IO_HOLD_TRIGGER, ON);
                } else if (temp == 8) {
                    BeginIOInstruction(TBTY_IO);
                    SetTrigger(IO_HOLD_TRIGGER, ON);
                } else {
                    (void)printf("Undefined control code: %d\n", temp);
                    runState = CHECK_STOP_STATE;
                }
            } else {
                (void)printf("Unsupported I/O device: %d\n", sabr.value);
                runState = CHECK_STOP_STATE;
            }
        }

        // Save current cycle
        lastCycle = cycle;

        // Process stopped states
        if ((runState == CHECK_STOP_STATE) || (runState == STOPPED_STATE)) {
            goto End_Cycle;
        }

        // Process I-Cycle entry
        if (TestTrigger(I_CYCLE_ENTRY_TRIGGER)) {
            SetTrigger(I_CYCLE_ENTRY_TRIGGER, OFF);
            SetTrigger(DIV_1_CYCLE_TRIGGER, OFF);
            SetTrigger(DIV_ADD_TRIGGER, OFF);
            SetTrigger(DIV_LAST_CYC_TRIGGER, OFF);
            SetTrigger(RE_COMP_TRIGGER, OFF);
            SetTrigger(CARRY_IN_TRIGGER, OFF);
            SetTrigger(CARRY_OUT_TRIGGER, OFF);
            SetTrigger(REC_CTRL_TRIGGER, OFF);
            SetTrigger(IA_TRIGGER, OFF);
            SetTrigger(NEXT_IA_TRIGGER, OFF);
            SetTrigger(DIGIT_TRIGGER, OFF);
            SetTrigger(DIV_SIGN_TRIGGER, OFF);
            SetTrigger(FIRST_CYCLE_TRIGGER, OFF);
            SetTrigger(FIELD_MK_1_TRIGGER, OFF);
            SetTrigger(FIELD_MK_2_TRIGGER, OFF);
            SetTrigger(REC_MARK_TRIGGER, OFF);
            SetTrigger(DECR_TRIGGER, OFF);
            SetTrigger(INCR_TRIGGER, ON);
            SetTrigger(PLUS_2_TRIGGER, OFF);
            SetTrigger(BYPASS_TRIGGER, OFF);
            ClearMQR();
            ClearSABR();
        }

        // Process E-Cycle entry
        if (TestTrigger(E_CYCLE_ENTRY_TRIGGER)) {
            SetTrigger(E_CYCLE_ENTRY_TRIGGER, OFF);

            if ((opCode == A_OPCODE)  || (opCode == AM_OPCODE)  || (opCode == S_OPCODE)   || (opCode == SM_OPCODE) ||
                (opCode == C_OPCODE)  || (opCode == CM_OPCODE)  || (opCode == D_OPCODE)   || (opCode == DM_OPCODE) ||
                (opCode == TF_OPCODE) || (opCode == TFM_OPCODE) || (opCode == TNS_OPCODE) || (opCode == TNF_OPCODE)) {
                SetTrigger(FIRST_CYCLE_TRIGGER, ON);
            }

            if ((opCode == D_OPCODE) || (opCode == DM_OPCODE)) {
                SetTrigger(DIV_1_CYCLE_TRIGGER, ON);
            }

            if ((opCode == BT_OPCODE) || (opCode == BTM_OPCODE) || (opCode == A_OPCODE) || (opCode == AM_OPCODE) ||
                (opCode == S_OPCODE)  || (opCode == SM_OPCODE)  || (opCode == C_OPCODE) || (opCode == CM_OPCODE) ||
                (opCode == M_OPCODE)  || (opCode == MM_OPCODE)  || (opCode == D_OPCODE) || (opCode == DM_OPCODE)) {
                SetTrigger(INCR_TRIGGER, OFF);
                SetTrigger(DECR_TRIGGER, ON);
                SetTrigger(PLUS_2_TRIGGER, OFF);

            } else if ((opCode == DN_OPCODE) || (opCode == RN_OPCODE) || (opCode == RA_OPCODE) ||
                       (opCode == WN_OPCODE) || (opCode == WA_OPCODE)) {
                SetTrigger(INCR_TRIGGER, ON);
                SetTrigger(DECR_TRIGGER, OFF);
                SetTrigger(PLUS_2_TRIGGER, OFF);
            }

            if ((opCode == A_OPCODE) || (opCode == AM_OPCODE) || (opCode == S_OPCODE) || (opCode == SM_OPCODE) ||
                (opCode == C_OPCODE) || (opCode == CM_OPCODE) || (opCode == D_OPCODE) || (opCode == DM_OPCODE)) {
                SetTrigger(HP_TRIGGER, ON);
                SetTrigger(EZ_TRIGGER, ON);
            }

            if ((opCode == S_OPCODE) || (opCode == SM_OPCODE) || (opCode == C_OPCODE) || (opCode == CM_OPCODE)) {
                SetTrigger(TC_1_TRIGGER, OFF);
            }

            if ((opCode == DN_OPCODE) || (opCode == RN_OPCODE) || (opCode == RA_OPCODE) || (opCode == WN_OPCODE) ||
                (opCode == WA_OPCODE)) {
                SetTrigger(MEM_OFLOW_TRIGGER, OFF);
                SetTrigger(STOP_TRIGGER, OFF);
                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                SetTrigger(RSP_GATE_TRIGGER, OFF);
                SetTrigger(RW_CALL_TRIGGER, ON);
                SetTrigger(IO_FLAG_TRIGGER, OFF);
                SetTrigger(IO_SYNC_TRIGGER, OFF);

                if (sabr.value == 1) {
                    if (opCode == DN_OPCODE) {
                        BeginIOInstruction(DNTY_IO);
                    } else if (opCode == RN_OPCODE) {
                        BeginIOInstruction(RNTY_IO);
                        SetTrigger(IO_HOLD_TRIGGER, ON);
                    } else if (opCode == RA_OPCODE) {
                        BeginIOInstruction(RATY_IO);
                        SetTrigger(IO_HOLD_TRIGGER, ON);
                    } else if (opCode == WN_OPCODE) {
                        BeginIOInstruction(WNTY_IO);
                    } else /* opCode == WA_OPCODE */ {
                        BeginIOInstruction(WATY_IO);
                    }

                } else if (sabr.value == 4) {
                    if (opCode == DN_OPCODE) {
                        BeginIOInstruction(DNCD_IO);
                        SetTrigger(PCH_FEED_TRIGGER, ON);
                        if (!punchReady) SetLight(PUNCH_NO_FEED_LIGHT, ON);
                    } else if (opCode == WN_OPCODE) {
                        BeginIOInstruction(WNCD_IO);
                        SetTrigger(PCH_FEED_TRIGGER, ON);
                        if (!punchReady) SetLight(PUNCH_NO_FEED_LIGHT, ON);
                    } else if (opCode == WA_OPCODE) {
                        BeginIOInstruction(WACD_IO);
                        SetTrigger(PCH_FEED_TRIGGER, ON);
                        if (!punchReady) SetLight(PUNCH_NO_FEED_LIGHT, ON);
                    } else {
                        (void)printf("Unsupported I/O operation: %d\n", sabr.value);
                        runState = CHECK_STOP_STATE;
                    }

                } else if (sabr.value == 5) {
                    if (opCode == RN_OPCODE) {
                        BeginIOInstruction(RNCD_IO);
                        SetTrigger(IO_HOLD_TRIGGER, ON);
                        SetTrigger(RDR_FEED_TRIGGER, ON);
                        if (!readerReady) SetLight(READER_NO_FEED_LIGHT, ON);
                    } else if (opCode == RA_OPCODE) {
                        BeginIOInstruction(RACD_IO);
                        SetTrigger(IO_HOLD_TRIGGER, ON);
                        SetTrigger(RDR_FEED_TRIGGER, ON);
                        if (!readerReady) SetLight(READER_NO_FEED_LIGHT, ON);
                    } else {
                        (void)printf("Unsupported I/O operation: %d\n", sabr.value);
                        runState = CHECK_STOP_STATE;
                    }

                } else {
                    (void)printf("Unsupported I/O device: %d\n", sabr.value);
                    runState = CHECK_STOP_STATE;
                }
            }
        }

        // Process I/O wait and hold
        if (ioState != STOPPED_IO_STATE) {
            if ((runState != SINGLE_CYCLE_STATE) && (ioWait > 0) && (--ioWait > 0)) goto End_Cycle;
            if ((cycle == 30) && ((ioType == DNCD_IO) || (ioType == WNCD_IO) || (ioType == WACD_IO)) && punchBusy)
                goto End_Cycle;
            if (TestTrigger(IO_HOLD_TRIGGER)) {
                if ((ioState == RUNNING_IO_STATE) &&
                    ((ioType == INSERT_IO) || (ioType == RNTY_IO) || (ioType == RATY_IO)) && !ReadTYAvail()) {
                    goto End_Cycle;
                } else if (((ioType == LOAD_IO) || (ioType == RNCD_IO) || (ioType == RACD_IO)) && !ReadCDAvail()) {
                    goto End_Cycle;
                }
                SetTrigger(IO_HOLD_TRIGGER, OFF);
                if (cycle != 56) {
                    SetTrigger(RSP_GATE_TRIGGER, ON);
                    SetTrigger(IO_SYNC_TRIGGER, ON);
                    SetTrigger(RSP_GATE_TRIGGER, OFF);
                } else {
                    SetTrigger(DISCN_GATE_TRIGGER, ON);
                    SetTrigger(IO_EXIT_TRIGGER, ON);
                    SetTrigger(DISCN_GATE_TRIGGER, OFF);
                }
            }
        }

        // Toggle A-ADV trigger
        SetTrigger(A_ADV_TRIGGER, !TestTrigger(A_ADV_TRIGGER));

        // Execute one cycle
        switch (cycle) {

            case 1:    // I-1

                // Instructions must start on even address within MEMORY_SIZE
                if (!ValidEvenAddress(mars[IR1_MAR].value)) {
                    (void)printf("Invalid instruction address: %x%x%x%x%x\n", mars[IR1_MAR].digit10000.value,
                                 mars[IR1_MAR].digit01000.value, mars[IR1_MAR].digit00100.value,
                                 mars[IR1_MAR].digit00010.value, mars[IR1_MAR].digit00001.value);
                    runState = CHECK_STOP_STATE;
                    break;
                }

                // Read and process opcode (O0 & O1)
                if (!ReadPairMemory(IR1_MAR, EVEN)) break;
                IncrementMAR(IR1_MAR, 2);
                SetOpR(mbr.digitEven.value, mbr.digitOdd.value);
                opCode = opr.value;

                // Set auxiliary triggers
                SetTrigger(TC_1_TRIGGER, ON);
                SetTrigger(STOP_TRIGGER, OFF);
                SetTrigger(IO_EXIT_TRIGGER, OFF);

                // Clear registers
                ClearSingleMAR(OR1_MAR);
                ClearSingleMAR(OR2_MAR);
                ClearSingleMAR(OR3_MAR);

                // Set next cycle
                cycle = 2;
                break;

            case 2:    // I-2

                // Read and process P field (P2 & P3)
                if (!ReadPairMemory(IR1_MAR, EVEN)) break;
                IncrementMAR(IR1_MAR, 2);
                SetDR(mbr.digitEven.value, mbr.digitOdd.value);
                SetDigit(&mars[OR2_MAR].digit10000, dr.digit10.value);
                CopyDigit(&mars[OR3_MAR].digit10000, &mars[OR2_MAR].digit10000);
                SetDigit(&mars[OR2_MAR].digit01000, dr.digit01.value);
                CopyDigit(&mars[OR3_MAR].digit01000, &mars[OR2_MAR].digit01000);

                // Set auxiliary triggers
                SetTrigger(BRCH_TRIGGER, OFF);

                // Set next cycle
                cycle = 3;
                break;

            case 3:    // I-3

                // Read and process P field (P4 & P5)
                if (!ReadPairMemory(IR1_MAR, EVEN)) break;
                IncrementMAR(IR1_MAR, 2);
                SetDR(mbr.digitEven.value, mbr.digitOdd.value);
                SetDigit(&mars[OR2_MAR].digit00100, dr.digit10.value);
                CopyDigit(&mars[OR3_MAR].digit00100, &mars[OR2_MAR].digit00100);
                SetDigit(&mars[OR2_MAR].digit00010, dr.digit01.value);
                CopyDigit(&mars[OR3_MAR].digit00010, &mars[OR2_MAR].digit00010);

                // Set next cycle
                cycle = 4;
                break;

            case 4:    // I-4

                // Read and process P field (P6)
                if (!ReadMemory(IR1_MAR)) break;
                IncrementMAR(IR1_MAR, 1);
                SetTrigger(P_Q_TRIGGER, ON);
                if (mdr.digit.flag && opPIndirects[opCode]) {
                    SetTrigger(IA_TRIGGER, ON);
                    SetTrigger(I_TIME_TRIGGER, OFF);
                }
                SetDR(0, mdr.digit.value);
                SetDigit(&mars[OR2_MAR].digit00001, dr.digit01.value);
                CopyDigit(&mars[OR3_MAR].digit00001, &mars[OR2_MAR].digit00001);
                if ((mars[OR2_MAR].digit10000.value <= 9) && (mars[OR2_MAR].digit01000.value <= 9) &&
                    (mars[OR2_MAR].digit00100.value <= 9) && (mars[OR2_MAR].digit00010.value <= 9) &&
                    (mars[OR2_MAR].digit00001.value <= 9)) {
                    mars[OR2_MAR].value = 10000 * mars[OR2_MAR].digit10000.value +
                                           1000 * mars[OR2_MAR].digit01000.value +
                                            100 * mars[OR2_MAR].digit00100.value +
                                             10 * mars[OR2_MAR].digit00010.value +
                                                  mars[OR2_MAR].digit00001.value;
                    mars[OR3_MAR].value = mars[OR2_MAR].value;
                } else {
                    mars[OR2_MAR].value = -1;
                    mars[OR3_MAR].value = -1;
                }

                // Set next cycle
                if (!TestTrigger(IA_TRIGGER)) {
                    cycle = 5;
                } else {
                    cycle = 46;
                }
                break;

            case 5:    // I-5

                // Read and process Q field (Q7)
                if (!ReadMemory(IR1_MAR)) break;
                IncrementMAR(IR1_MAR, 2);
                SetDR(0, mdr.digit.value);
                if (opr.digit10.value != 1) {
                    SetDigit(&mars[OR1_MAR].digit10000, dr.digit01.value);
                    CopyDigit(&mars[PR1_MAR].digit10000, &mars[OR1_MAR].digit10000);
                }

                // Set next cycle
                cycle = 6;
                break;

            case 6:    // I-6

                // Read and process Q field (Q8 & Q9)
                if (!ReadPairMemory(IR1_MAR, ODD)) break;
                IncrementMAR(IR1_MAR, 2);
                SetDR(mbr.digitEven.value, mbr.digitOdd.value);
                if ((opCode == K_OPCODE)  || (opCode == DN_OPCODE) || (opCode == RN_OPCODE) || (opCode == RA_OPCODE) ||
                    (opCode == WN_OPCODE) || (opCode == WA_OPCODE) || (opCode == BI_OPCODE) || (opCode == BNI_OPCODE)) {
                    SetSABR(dr.digit10.value, dr.digit01.value);
                }
                if (opr.digit10.value != 1) {
                    SetDigit(&mars[OR1_MAR].digit01000, dr.digit10.value);
                    CopyDigit(&mars[PR1_MAR].digit01000, &mars[OR1_MAR].digit01000);
                    SetDigit(&mars[OR1_MAR].digit00100, dr.digit01.value);
                    CopyDigit(&mars[PR1_MAR].digit00100, &mars[OR1_MAR].digit00100);
                }

                // Set next cycle
                cycle = 7;
                break;

            case 7:    // I-7

                // Read and process Q field (Q10 & Q11)
                if (!ReadPairMemory(IR1_MAR, ODD)) break;
                IncrementMAR(IR1_MAR, 1);
                SetTrigger(P_Q_TRIGGER, OFF);
                if (mbr.digitOdd.flag && opQIndirects[opCode]) {
                    SetTrigger(IA_TRIGGER, ON);
                    SetTrigger(I_TIME_TRIGGER, OFF);
                }
                SetDR(mbr.digitEven.value, mbr.digitOdd.value);
                if (opr.digit10.value != 1) {
                    SetDigit(&mars[OR1_MAR].digit00010, dr.digit10.value);
                    CopyDigit(&mars[PR1_MAR].digit00010, &mars[OR1_MAR].digit00010);
                    SetDigit(&mars[OR1_MAR].digit00001, dr.digit01.value);
                    CopyDigit(&mars[PR1_MAR].digit00001, &mars[OR1_MAR].digit00001);
                    if ((mars[OR1_MAR].digit10000.value <= 9) && (mars[OR1_MAR].digit01000.value <= 9) &&
                        (mars[OR1_MAR].digit00100.value <= 9) && (mars[OR1_MAR].digit00010.value <= 9) &&
                        (mars[OR1_MAR].digit00001.value <= 9)) {
                        mars[OR1_MAR].value = 10000 * mars[OR1_MAR].digit10000.value +
                                               1000 * mars[OR1_MAR].digit01000.value +
                                                100 * mars[OR1_MAR].digit00100.value +
                                                 10 * mars[OR1_MAR].digit00010.value +
                                                      mars[OR1_MAR].digit00001.value;
                        mars[PR1_MAR].value = mars[OR1_MAR].value;
                    } else {
                        mars[OR1_MAR].value = -1;
                        mars[PR1_MAR].value = -1;
                    }
                }

                // Set auxiliary triggers
                if ((opCode == BI_OPCODE) || (opCode == BNI_OPCODE)) {
                    SetTrigger(BRCH_TRIGGER, TestBranch(sabr.value));
                }

                // Set next cycle
                if (!TestTrigger(IA_TRIGGER)) {
                    cycle = 8;
                } else {
                    cycle = 47;
                }
                break;

            case 8:    // I-8

                // Use address of Q11 digit for immediate instructions
                if (opr.digit10.value == 1) {
                    SaveMAR(OR1_MAR);
                    SaveMAR(PR1_MAR);
                }

                // Set next cycle
                if ((opr.value >= 0) && opCycles[opCode] != 0) {
                    if (((opCode == BI_OPCODE)  && !TestTrigger(BRCH_TRIGGER)) ||
                        ((opCode == BNI_OPCODE) && TestTrigger(BRCH_TRIGGER))) {
                        cycle = 1;
                    } else if (opCode == H_OPCODE) {
                        cycle = 1;
                        runState = STOPPED_STATE;
                        TimeLog("Halt");
                    } else {
                        cycle = opCycles[opCode];
                    }

                } else {
                    cycle = 1;
                    if (opCode != -1) {
                        (void)printf("Invalid opcode: %d\n", opCode);
                    } else {
                        (void)printf("Invalid opcode: 0x%X%X\n", opr.digit10.value, opr.digit01.value);
                    }
                    runState = CHECK_STOP_STATE;
                }
                break;

            case 9:    // T-1

                (void)printf("Bad cycle: T-1\n");
                runState = CHECK_STOP_STATE;
                break;

            case 10:    // T-2

                (void)printf("Bad cycle: T-2\n");
                runState = CHECK_STOP_STATE;
                break;

            case 11:    // E-11

                // Read a Q field digit
                if (!ReadMemory(OR1_MAR)) break;
                IncrementMAR(OR1_MAR, -1);
                SetDR(0, mdr.digit.value);

                // Process operation
                if ((opCode == A_OPCODE) || (opCode == AM_OPCODE) || (opCode == S_OPCODE) || (opCode == SM_OPCODE) ||
                    (opCode == C_OPCODE) || (opCode == CM_OPCODE)) {
                    SetTrigger(CARRY_OUT_TRIGGER, OFF);
                    if (mdr.digit.flag && !TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        SetTrigger(FIELD_MK_1_TRIGGER, ON);
                    }
                    if (mdr.digit.flag && TestTrigger(FIRST_CYCLE_TRIGGER) && !TestTrigger(RE_COMP_TRIGGER)) {
                        if ((opCode == A_OPCODE) || (opCode == AM_OPCODE)) {
                            SetTrigger(TC_1_TRIGGER, OFF);
                        } else if ((opCode == S_OPCODE) || (opCode == SM_OPCODE) ||
                               (opCode == C_OPCODE) || (opCode == CM_OPCODE)) {
                            SetTrigger(TC_1_TRIGGER, ON);
                        }
                    }
                    if ((mdr.digit.value != 0) && ((opCode == C_OPCODE) || (opCode == CM_OPCODE))) {
                        SetTrigger(DIGIT_TRIGGER, ON);
                    }

                } else if ((opCode == D_OPCODE) || (opCode == DM_OPCODE)) {
                    SetTrigger(CARRY_OUT_TRIGGER, OFF);
                    if (mdr.digit.flag && !TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        SetTrigger(FIELD_MK_1_TRIGGER, ON);
                    }
                    if (TestTrigger(DIV_ADD_TRIGGER)) {
                        SetTrigger(TC_1_TRIGGER, ON);
                        if (mdr.digit.flag && TestTrigger(FIRST_CYCLE_TRIGGER) && TestTrigger(DIV_LAST_CYC_TRIGGER)) {
                            SetTrigger(HP_TRIGGER, !TestTrigger(HP_TRIGGER));
                        }
                    } else {
                        SetTrigger(TC_1_TRIGGER, OFF);
                    }
                }
    
                // Set next cycle
                cycle = 12;
                break;

            case 12:    // E-12

                // Read a P field digit
                if (!ReadMemory(OR2_MAR)) break;
                SaveMAR(OR2_MAR);

                // Process operation
                if ((opCode == A_OPCODE) || (opCode == AM_OPCODE) || (opCode == S_OPCODE) || (opCode == SM_OPCODE) ||
                    (opCode == C_OPCODE) || (opCode == CM_OPCODE)) {
                    if (TestTrigger(RE_COMP_TRIGGER)) {
                        SetDR(0, mdr.digit.value);
                    }
                    if (mdr.digit.flag && !TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        SetTrigger(FIELD_MK_2_TRIGGER, ON);
                    }
                    if (mdr.digit.flag && TestTrigger(FIRST_CYCLE_TRIGGER) && !TestTrigger(RE_COMP_TRIGGER)) {
                        SetTrigger(TC_1_TRIGGER, !TestTrigger(TC_1_TRIGGER));
                        SetTrigger(HP_TRIGGER, OFF);
                    }
                    if (TestTrigger(FIRST_CYCLE_TRIGGER) &&
                        (!TestTrigger(TC_1_TRIGGER) || TestTrigger(RE_COMP_TRIGGER))) {
                        SetTrigger(CARRY_IN_TRIGGER, ON);
                    }
                    if (TestTrigger(FIRST_CYCLE_TRIGGER) && ((opCode == C_OPCODE) || (opCode == CM_OPCODE))) {
                        SetTrigger(CARRY_IN_TRIGGER, ON);
                    }
                    if ((mdr.digit.value != 0) && ((opCode == C_OPCODE) || (opCode == CM_OPCODE))) {
                        SetTrigger(DIGIT_TRIGGER, ON);
                    }

                } else if ((opCode == D_OPCODE) || (opCode == DM_OPCODE)) {
                    if (TestTrigger(DIV_ADD_TRIGGER)) {
                        if (TestTrigger(FIELD_MK_1_TRIGGER)) {
                            SetTrigger(FIELD_MK_2_TRIGGER, ON);
                        }
                    } else {
                        if (TestTrigger(FIRST_CYCLE_TRIGGER)) {
                            SetTrigger(CARRY_IN_TRIGGER, ON);
                            if (mdr.digit.flag && TestTrigger(DIV_LAST_CYC_TRIGGER)) {
                                SetTrigger(DIV_SIGN_TRIGGER, ON);
                                SetTrigger(HP_TRIGGER, OFF);
                            }
                        }
                    }
                }

                // Set next cycle
                if (((opCode == C_OPCODE) || (opCode == CM_OPCODE)) && !TestTrigger(FIRST_CYCLE_TRIGGER) &&
                    TestTrigger(DIGIT_TRIGGER) && TestTrigger(TC_1_TRIGGER)) {
                    SetTrigger(EZ_TRIGGER, OFF);
                    cycle = 1;

                } else {
                    cycle = 13;
                }
                break;

            case 13:    // E-13

                // Process operation
                if ((opCode == A_OPCODE) || (opCode == AM_OPCODE) || (opCode == S_OPCODE) || (opCode == SM_OPCODE) ||
                    (opCode == C_OPCODE) || (opCode == CM_OPCODE)) {
                    if (dr.digit01.value <= 9) {
                        if (TestTrigger(TC_1_TRIGGER)) {
                            temp = dr.digit01.value;
                        } else {
                            temp = 9 - dr.digit01.value;
                        }
                    } else if (dr.digit01.value == RECORD_MARK) {
                        if (TestTrigger(TC_1_TRIGGER)) {
                            temp = 0;
                        } else {
                            temp = 9;
                        }
                    } else {
                        SetTrigger(MAR_CHK_TRIGGER, ON);
                        break;
                    }
                    if (TestTrigger(RE_COMP_TRIGGER)) {
                        temp2 = 0;
                    } else {
                        if (mdr.digit.value <= 9) {
                            temp2 = mdr.digit.value;
                        } else {
                            SetTrigger(MAR_CHK_TRIGGER, ON);
                            break;
                        }
                    }
                    SetMAR(MAR_MAR, 300 + 10 * temp2 + (temp + TestTrigger(CARRY_IN_TRIGGER)) % 10);
                    if (!ReadMemory(MAR_MAR)) break;
                    if (mdr.digit.flag || ((temp == 9) && TestTrigger(CARRY_IN_TRIGGER))) {
                        SetTrigger(CARRY_OUT_TRIGGER, ON);
                    }
                    SetTrigger(CARRY_IN_TRIGGER, OFF);
                    if ((opCode == C_OPCODE) || (opCode == CM_OPCODE)) {
                        if (!TestTrigger(CARRY_OUT_TRIGGER) && TestTrigger(FIELD_MK_2_TRIGGER)) {
                            SetTrigger(HP_TRIGGER, !TestTrigger(HP_TRIGGER));
                        }
                    } else {
                        if (!TestTrigger(TC_1_TRIGGER)      && !TestTrigger(CARRY_OUT_TRIGGER) &&
                            TestTrigger(FIELD_MK_2_TRIGGER) && !TestTrigger(RE_COMP_TRIGGER)) {
                            SetTrigger(REC_CTRL_TRIGGER, ON);
                            SetTrigger(HP_TRIGGER, !TestTrigger(HP_TRIGGER));
                        }
                    }
                    if ((opCode != C_OPCODE) && (opCode != CM_OPCODE)) {
                        if (TestTrigger(TC_1_TRIGGER)       && TestTrigger(CARRY_OUT_TRIGGER) &&
                            TestTrigger(FIELD_MK_2_TRIGGER) && !TestTrigger(REC_CTRL_TRIGGER)) {
                            SetTrigger(ARITH_CHK_TRIGGER, ON);
                        }
                    }
                    if (!TestTrigger(FIELD_MK_1_TRIGGER) && TestTrigger(FIELD_MK_2_TRIGGER) &&
                        !TestTrigger(RE_COMP_TRIGGER)) {
                        SetTrigger(ARITH_CHK_TRIGGER, ON);
                    }
                    if (mdr.digit.value != 0) {
                        SetTrigger(EZ_TRIGGER, OFF);
                    }
                    ClearDR();

                } else if ((opCode == D_OPCODE) || (opCode == DM_OPCODE)) {
                    if ((dr.digit01.value > 9) || (mdr.digit.value > 9)) {
                        SetTrigger(MAR_CHK_TRIGGER, ON);
                        break;
                    }
                    if (TestTrigger(TC_1_TRIGGER)) {
                        temp = dr.digit01.value;
                    } else {
                        temp = 9 - dr.digit01.value;
                    }
                    SetMAR(MAR_MAR, 300 + 10 * mdr.digit.value + (temp + TestTrigger(CARRY_IN_TRIGGER)) % 10);
                    if (!ReadMemory(MAR_MAR)) break;
                    if (mdr.digit.flag || ((temp == 9) && TestTrigger(CARRY_IN_TRIGGER))) {
                        SetTrigger(CARRY_OUT_TRIGGER, ON);
                    }
                    SetTrigger(CARRY_IN_TRIGGER, OFF);
                    ClearDR();
                }

                // Set next cycle
                cycle = 14;
                break;

            case 14:    // E-14

                // Process operation
                if ((opCode == A_OPCODE) || (opCode == AM_OPCODE) || (opCode == S_OPCODE) || (opCode == SM_OPCODE) ||
                    (opCode == C_OPCODE) || (opCode == CM_OPCODE)) {

                    if ((opCode != C_OPCODE) && (opCode != CM_OPCODE)) {
                        if (TestTrigger(FIRST_CYCLE_TRIGGER) && !TestTrigger(HP_TRIGGER)) {
                            mdr.digit.flag = 1;
                        }
                        if (TestTrigger(FIRST_CYCLE_TRIGGER) && !TestTrigger(RE_COMP_TRIGGER) &&
                            TestTrigger(HP_TRIGGER)) {
                            mdr.digit.flag = 0;
                        }
                        if (TestTrigger(FIELD_MK_2_TRIGGER)) {
                            mdr.digit.flag = 1;
                        }
                        if (!TestTrigger(FIRST_CYCLE_TRIGGER) && !TestTrigger(RE_COMP_TRIGGER) &&
                            !TestTrigger(FIELD_MK_2_TRIGGER)) {
                            mdr.digit.flag = 0;
                        }
                        if (!((opCode == C_OPCODE) || (opCode == CM_OPCODE))) {
                            if (!WriteMemory(OR2_MAR)) break;
                        }
                    }

                    IncrementMAR(OR2_MAR, -1);
                    SetTrigger(FIRST_CYCLE_TRIGGER, OFF);
                    if (TestTrigger(CARRY_OUT_TRIGGER)) {
                        SetTrigger(CARRY_IN_TRIGGER, ON);
                    }
                    if (TestTrigger(FIELD_MK_2_TRIGGER) && TestTrigger(EZ_TRIGGER)) {
                        SetTrigger(HP_TRIGGER, OFF);
                    }

                } else if ((opCode == D_OPCODE) || (opCode == DM_OPCODE)) {
                    if (TestTrigger(CARRY_OUT_TRIGGER)) {
                        SetTrigger(CARRY_IN_TRIGGER, ON);
                    }
                    if (mdr.digit.flag) {
                        SetMDRFlag(FALSE);
                    }
                    if (TestTrigger(FIRST_CYCLE_TRIGGER) && TestTrigger(DIV_ADD_TRIGGER) &&
                        TestTrigger(DIV_LAST_CYC_TRIGGER) && TestTrigger(DIV_SIGN_TRIGGER)) {
                        SetMDRFlag(TRUE);
                    }
                    SetTrigger(FIRST_CYCLE_TRIGGER, OFF);
                    if (TestTrigger(DIV_ADD_TRIGGER) && TestTrigger(DIV_LAST_CYC_TRIGGER) &&
                        TestTrigger(FIELD_MK_2_TRIGGER)) {
                        SetMDRFlag(TRUE);
                    }
                    if (!WriteMemory(OR2_MAR)) break;
                    IncrementMAR(OR2_MAR, -1);
                    if (TestTrigger(FIELD_MK_2_TRIGGER) && !TestTrigger(DIV_ADD_TRIGGER) &&
                        TestTrigger(CARRY_OUT_TRIGGER)) {
                        SetMQR(mqr.value + 1);
                    }
                }

                // Set next cycle
                if ((opCode == A_OPCODE) || (opCode == AM_OPCODE) || (opCode == S_OPCODE) || (opCode == SM_OPCODE) ||
                    (opCode == C_OPCODE) || (opCode == CM_OPCODE)) {
                    if (!TestTrigger(REC_CTRL_TRIGGER) && !TestTrigger(FIELD_MK_1_TRIGGER) &&
                        !TestTrigger(FIELD_MK_2_TRIGGER)) {
                        cycle = 11;
                    } else if (!TestTrigger(REC_CTRL_TRIGGER) && TestTrigger(FIELD_MK_1_TRIGGER) &&
                               !TestTrigger(FIELD_MK_2_TRIGGER)) {
                        cycle = 21;
                    } else if (!TestTrigger(RE_COMP_TRIGGER) && TestTrigger(REC_CTRL_TRIGGER)) {
                        SetTrigger(FIRST_CYCLE_TRIGGER, ON);
                        cycle = 21;
                    } else if (TestTrigger(RE_COMP_TRIGGER) && !TestTrigger(FIELD_MK_2_TRIGGER)) {
                        cycle = 23;
                    } else {
                        cycle = 1;
                    }

                } else if ((opCode == D_OPCODE) || (opCode == DM_OPCODE)) {
                    if (!TestTrigger(FIELD_MK_1_TRIGGER)) {
                        cycle = 11;
                    } else if (TestTrigger(FIELD_MK_2_TRIGGER)) {
                        if (TestTrigger(DIV_ADD_TRIGGER)) {
                            cycle = 42;
                        } else {
                            cycle = 44;
                        }
                    } else {
                        cycle = 21;
                    }
                }
                break;

            case 15:    // E-15

                // Save return address
                LoadMAR(IR1_MAR);
                SaveMAR(IR2_MAR);

                // Set auxiliary triggers
                SetTrigger(FIRST_CYCLE_TRIGGER, ON);

                // Set next cycle
                cycle = 16;
                break;

            case 16:    // E-16

                // Branch to address
                LoadMAR(OR2_MAR);
                SaveMAR(IR1_MAR);

                // Update transmit address
                IncrementMAR(OR2_MAR, -1);

                // Set next cycle
                cycle = 26;
                break;

            case 17:    // E-17

                (void)printf("Unimplemented cycle: E-17\n");
                runState = CHECK_STOP_STATE;
                break;

            case 18:    // E-18

                // Branch to address
                if ((opCode == B_OPCODE)   || (opCode == BI_OPCODE)  || (opCode == BNI_OPCODE) ||
                    (opCode == BD_OPCODE)  || (opCode == BNF_OPCODE) || (opCode == BNR_OPCODE) ||
                    (opCode == BNG_OPCODE)) {
                    LoadMAR(OR2_MAR);

                } else if (opCode == BB_OPCODE) {
                    if (TestTrigger(SAVE_CTRL_TRIGGER)) {
                        SetTrigger(SAVE_CTRL_TRIGGER, OFF);
                        SetTrigger(SAVE_2_TRIGGER, OFF);
                        LoadMAR(PR1_MAR);
                    } else {
                        LoadMAR(IR2_MAR);
                        SetMAR(IR2_MAR, 0);
                    }
                }
                SaveMAR(IR1_MAR);

                // Set next cycle
                cycle = 19;
                break;

            case 19:    // E-19

                // Set next cycle
                if ((opCode == M_OPCODE) || (opCode == MM_OPCODE)) {
                    if (!TestTrigger(FIELD_MK_1_TRIGGER) && !TestTrigger(CARRY_OUT_TRIGGER)) {
                        cycle = 33;
                    } else if (TestTrigger(CARRY_OUT_TRIGGER)) {
                        cycle = 39;
                    } else {
                        cycle = 1;
                    }

                } else {
                    cycle = 1;
                }
                break;

            case 20:    // E-20

                (void)printf("Unimplemented cycle: E-20\n");
                runState = CHECK_STOP_STATE;
                break;

            case 21:    // E-21

                // Process operation
                if ((opCode == A_OPCODE) || (opCode == AM_OPCODE) || (opCode == S_OPCODE) || (opCode == SM_OPCODE) ||
                    (opCode == C_OPCODE) || (opCode == CM_OPCODE)) {
                    ClearDR();
                    SetTrigger(CARRY_OUT_TRIGGER, OFF);
                    if (TestTrigger(REC_CTRL_TRIGGER)) {
                        LoadMAR(OR3_MAR);
                        SaveMAR(OR2_MAR);
                        SetTrigger(FIELD_MK_1_TRIGGER, OFF);
                        SetTrigger(FIELD_MK_2_TRIGGER, OFF);
                        SetTrigger(RE_COMP_TRIGGER, ON);
                    }

                } else if ((opCode == D_OPCODE) || (opCode == DM_OPCODE)) {
                    SetTrigger(CARRY_OUT_TRIGGER, OFF);
                    SetTrigger(FIELD_MK_2_TRIGGER, ON);
                    ClearDR();
                }

                // Set next cycle
                cycle = 12;
                break;

            case 22:    // E-22

                (void)printf("Unimplemented cycle: E-22\n");
                runState = CHECK_STOP_STATE;
                break;

            case 23:    // E-23

                // Process operation
                SetTrigger(FIRST_CYCLE_TRIGGER, OFF);
                SetTrigger(CARRY_OUT_TRIGGER, OFF);

                // Set next cycle
                cycle = 12;
                break;

            case 24:    // E-24

                // Unused cycle
                (void)printf("Bad cycle: E-24\n");
                runState = CHECK_STOP_STATE;
                break;

            case 25:    // E-25

                // Unused cycle
                (void)printf("Bad cycle: E-25\n");
                runState = CHECK_STOP_STATE;
                break;

            case 26:    // E-26

                // Process operation
                if ((opCode == TF_OPCODE) || (opCode == TFM_OPCODE) || (opCode == BT_OPCODE) ||
                    (opCode == BTM_OPCODE)) {
                    if (!ReadMemory(OR1_MAR)) break;
                    IncrementMAR(OR1_MAR, -1);
                    if (mdr.digit.flag && !TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        SetTrigger(FIELD_MK_1_TRIGGER, ON);
                    }

                } else if (opCode == TR_OPCODE) {
                    if (!ReadMemory(OR1_MAR)) break;
                    IncrementMAR(OR1_MAR, 1);
                    if ((mdr.digit.value == RECORD_MARK) || (mdr.digit.value == RECORD_GROUP_MARK)) {
                        SetTrigger(REC_MARK_TRIGGER, ON);
                    }

                } else if ((opCode == TD_OPCODE) || (opCode == TDM_OPCODE)) {
                    if (!ReadMemory(OR1_MAR)) break;
                    SaveMAR(OR1_MAR);

                } else if ((opCode == LD_OPCODE) || (opCode == LDM_OPCODE)) {
                    if (!ReadMemory(OR1_MAR)) break;
                    IncrementMAR(OR1_MAR, -1);
                    if (mdr.digit.flag && !TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        SetTrigger(FIELD_MK_1_TRIGGER, ON);
                    }

                } else if (opCode == TNS_OPCODE) {
                    if (TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        if (TestTrigger(TC_1_TRIGGER)) {
                            SetTrigger(TC_1_TRIGGER, OFF);
                            if (!ReadMemory(OR1_MAR)) break;
                            SaveMAR(OR1_MAR);
                        } else {
                            SetTrigger(TC_1_TRIGGER, ON);
                            if (TestTrigger(FIELD_MK_2_TRIGGER)) {
                                SetMDRFlag(TRUE);
                            }
                            if (!WriteMemory(OR1_MAR)) break;
                            IncrementMAR(OR1_MAR, -1);
                        }
                    } else {
                        if (TestTrigger(TC_1_TRIGGER)) {
                            SetTrigger(TC_1_TRIGGER, OFF);
                            if (!ReadMemory(OR1_MAR)) break;
                            SaveMAR(OR1_MAR);
                            if (mdr.digit.flag) {
                                SetTrigger(FIELD_MK_1_TRIGGER, ON);
                            }
                        } else {
                            SetTrigger(TC_1_TRIGGER, ON);
                            if (TestTrigger(FIELD_MK_1_TRIGGER)) {
                                SetMDRFlag(TRUE);
                            }
                            if (!WriteMemory(OR1_MAR)) break;
                            IncrementMAR(OR1_MAR, -1);
                        }
                    }

                } else if (opCode == TNF_OPCODE) {
                    if (TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        if (TestTrigger(TC_1_TRIGGER)) {
                            SetTrigger(TC_1_TRIGGER, OFF);
                            if (!ReadMemory(OR1_MAR)) break;
                            SaveMAR(OR1_MAR);
                            if (mdr.digit.flag) {
                                SetTrigger(FIELD_MK_2_TRIGGER, ON);
                            }
                        } else {
                            SetTrigger(TC_1_TRIGGER, ON);
                            if (!ReadMemory(OR1_MAR)) break;
                            IncrementMAR(OR1_MAR, -1);
                        }
                    } else {
                        if (TestTrigger(TC_1_TRIGGER)) {
                            SetTrigger(TC_1_TRIGGER, OFF);
                            if (!ReadMemory(OR1_MAR)) break;
                            SaveMAR(OR1_MAR);
                            if (mdr.digit.flag) {
                                SetTrigger(FIELD_MK_1_TRIGGER, ON);
                            }
                        } else {
                            SetTrigger(TC_1_TRIGGER, ON);
                            if (!ReadMemory(OR1_MAR)) break;
                            IncrementMAR(OR1_MAR, -1);
                        }
                    }
                }

                // Set next cycle
                cycle = 27;
                break;

            case 27:    // E-27

                // Process operation
                if ((opCode == TF_OPCODE) || (opCode == TFM_OPCODE) || (opCode == BT_OPCODE) ||
                    (opCode == BTM_OPCODE)) {
                    if (!WriteMemory(OR2_MAR)) break;
                    IncrementMAR(OR2_MAR, -1);
                    SetTrigger(FIRST_CYCLE_TRIGGER, OFF);

                } else if (opCode == TR_OPCODE) {
                    if (!WriteMemory(OR2_MAR)) break;
                    IncrementMAR(OR2_MAR, 1);
                    SetTrigger(FIRST_CYCLE_TRIGGER, OFF);

                } else if ((opCode == TD_OPCODE) || (opCode == TDM_OPCODE)) {
                    if (!WriteMemory(OR2_MAR)) break;
                    SaveMAR(OR2_MAR);

                } else if ((opCode == LD_OPCODE) || (opCode == LDM_OPCODE)) {
                    if (mdr.digit.flag && TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        SetTrigger(DIV_SIGN_TRIGGER, ON);
                        SetMDRFlag(FALSE);
                    }
                    if (!WriteMemory(OR2_MAR)) break;
                    IncrementMAR(OR2_MAR, -1);
                    SetTrigger(FIRST_CYCLE_TRIGGER, OFF);
                    SetTrigger(CYCLE_CTRL_TRIGGER, OFF);

                } else if (opCode == TNS_OPCODE) {
                    if (TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        if (TestTrigger(TC_1_TRIGGER)) {
                            if (!ReadMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, -1);
                            SetTrigger(FIRST_CYCLE_TRIGGER, OFF);
                        } else {
                            if (!ReadPairMemory(OR2_MAR, ODD)) break;
                            IncrementMAR(OR2_MAR, -1);
                            if ((mbr.digitEven.value == 5) || (mbr.digitEven.value == 2) ||
                                (mbr.digitEven.value == 1)) {
                                SetTrigger(FIELD_MK_2_TRIGGER, ON);
                            }
                        }
                    } else {
                        if (TestTrigger(TC_1_TRIGGER)) {
                            if (!ReadMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, -1);
                        } else {
                            if (!ReadPairMemory(OR2_MAR, ODD)) break;
                            IncrementMAR(OR2_MAR, -1);
                        }
                    }

                } else if (opCode == TNF_OPCODE) {
                    if (TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        if (TestTrigger(TC_1_TRIGGER)) {
                            SetTrigger(FIRST_CYCLE_TRIGGER, OFF);
                            if (TestTrigger(FIELD_MK_2_TRIGGER)) {
                                SetMDR(5);
                            } else {
                                SetMDR(7);
                            }
                            if (!WriteMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, -1);
                        } else {
                            SetMDRFlag(FALSE);
                            if (!WriteMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, -1);
                        }
                    } else {
                        if (TestTrigger(TC_1_TRIGGER)) {
                            SetMDR(7);
                            if (!WriteMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, -1);
                        } else {
                            SetMDRFlag(FALSE);
                            if (!WriteMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, -1);
                        }
                    }
                }

                // Set next cycle
                if ((opCode == TD_OPCODE) || (opCode == TDM_OPCODE)) {
                    cycle = 1;

                } else if ((opCode == TF_OPCODE) || (opCode == TFM_OPCODE) || (opCode == BT_OPCODE) ||
                           (opCode == BTM_OPCODE)) {
                    if (TestTrigger(FIELD_MK_1_TRIGGER)) {
                        cycle = 1;
                    } else {
                        cycle = 26;
                    }

                } else if (opCode == TR_OPCODE) {
                    if (TestTrigger(REC_MARK_TRIGGER)) {
                        cycle = 1;
                    } else {
                        cycle = 26;
                    }

                } else if ((opCode == LD_OPCODE) || (opCode == LDM_OPCODE)) {
                    if (TestTrigger(FIELD_MK_1_TRIGGER)) {
                        cycle = 28;
                    } else {
                        cycle = 26;
                    }

                } else if ((opCode == TNS_OPCODE) || (opCode == TNF_OPCODE)) {
                    if (TestTrigger(FIELD_MK_1_TRIGGER) && TestTrigger(TC_1_TRIGGER)) {
                        cycle = 1;
                    } else {
                        cycle = 26;
                    }
                }
                break;

            case 28:    // E-28

                // Process operation
                if ((opCode == SF_OPCODE) || (opCode == CF_OPCODE)) {
                    if (!ReadMemory(OR2_MAR)) break;
                    SaveMAR(OR2_MAR);

                } else if (opCode == BD_OPCODE) {
                    if (!ReadMemory(OR1_MAR)) break;
                    SaveMAR(OR1_MAR);
                    if (mdr.digit.value != 0) {
                        SetTrigger(DIGIT_TRIGGER, ON);
                    }

                } else if (opCode == BNF_OPCODE) {
                    if (!ReadMemory(OR1_MAR)) break;
                    SaveMAR(OR1_MAR);
                    if (mdr.digit.flag) {
                        SetTrigger(FIELD_MK_1_TRIGGER, ON);
                    }

                } else if (opCode == BNR_OPCODE) {
                    if (!ReadMemory(OR1_MAR)) break;
                    SaveMAR(OR1_MAR);
                    if ((mdr.digit.value == RECORD_MARK) || (mdr.digit.value == RECORD_GROUP_MARK) ||
                        (mdr.digit.value == GROUP_MARK)) {
                        SetTrigger(REC_MARK_TRIGGER, ON);
                    }

                } else if (opCode == BNG_OPCODE) {
                    if (!ReadMemory(OR1_MAR)) break;
                    SaveMAR(OR1_MAR);
                    if ((mdr.digit.value == GROUP_MARK) || (mdr.digit.value == RECORD_GROUP_MARK)) {
                        SetTrigger(REC_MARK_TRIGGER, ON);
                    }

                } else if ((opCode == LD_OPCODE) || (opCode == LDM_OPCODE)) {
                    if (!ReadMemory(PR1_MAR)) break;
                    SaveMAR(PR1_MAR);
                    SaveMAR(OR2_MAR);

                } else if (opCode == MF_OPCODE) {
                    if (TestTrigger(TC_1_TRIGGER)) {
                        SetTrigger(TC_1_TRIGGER, OFF);
                        if (!ReadMemory(OR1_MAR)) break;
                        SaveMAR(OR1_MAR);
                        if (mdr.digit.flag) {
                            SetTrigger(FIELD_MK_1_TRIGGER, ON);
                        }
                    } else {
                        SetTrigger(TC_1_TRIGGER, ON);
                        if (!ReadMemory(OR2_MAR)) break;
                        SaveMAR(OR2_MAR);
                    }
                }

                // Set next cycle
                cycle = 29;
                break;

            case 29:    // E-29

                // Process operation
                if (opCode == SF_OPCODE) {
                    mdr.digit.flag = 1;
                    if (!WriteMemory(OR2_MAR)) break;
                    SaveMAR(OR2_MAR);

                } else if (opCode == CF_OPCODE) {
                    mdr.digit.flag = 0;
                    if (!WriteMemory(OR2_MAR)) break;
                    SaveMAR(OR2_MAR);

                } else if ((opCode == LD_OPCODE) || (opCode == LDM_OPCODE)) {
                    SetMDRFlag(TestTrigger(DIV_SIGN_TRIGGER));
                    if (!WriteMemory(OR2_MAR)) break;
                    SaveMAR(OR2_MAR);

                } else if (opCode == MF_OPCODE) {
                    if (TestTrigger(TC_1_TRIGGER)) {
                        if (TestTrigger(FIELD_MK_1_TRIGGER)) {
                            SetMDRFlag(TRUE);
                        } else {
                            SetMDRFlag(FALSE);
                        }
                        if (!WriteMemory(OR2_MAR)) break;
                        SaveMAR(OR2_MAR);
                    } else {
                        SetMDRFlag(FALSE);
                        if (!WriteMemory(OR1_MAR)) break;
                        SaveMAR(OR1_MAR);
                    }
                }

                // Set next cycle
                if ((opCode == SF_OPCODE) || (opCode == CF_OPCODE) || (opCode == LD_OPCODE) || (opCode == LDM_OPCODE)) {
                    cycle = 1;

                } else if (opCode == BD_OPCODE) {
                    if (TestTrigger(DIGIT_TRIGGER)) {
                        cycle = 18;
                    } else {
                        cycle = 1;
                    }

                } else if (opCode == BNF_OPCODE) {
                    if (TestTrigger(FIELD_MK_1_TRIGGER)) {
                        cycle = 1;
                    } else {
                        cycle = 18;
                    }

                } else if ((opCode == BNR_OPCODE) || (opCode == BNG_OPCODE)) {
                    if (TestTrigger(REC_MARK_TRIGGER)) {
                        cycle = 1;
                    } else {
                        cycle = 18;
                    }

                } else if (opCode == MF_OPCODE) {
                    if (TestTrigger(TC_1_TRIGGER)) {
                        cycle = 1;
                    } else {
                        cycle = 28;
                    }
                }
                break;

            case 30:    // E-30

                // Process operation
                if ((ioState == RUNNING_IO_STATE) ||
                    ((ioState == ABORTED_IO_STATE) && ((ioType == LOAD_IO) || (ioType == DNCD_IO) ||
                                                       (ioType == RNCD_IO) || (ioType == RACD_IO) ||
                                                       (ioType == WNCD_IO) || (ioType == WACD_IO)))) {

                    switch (ioType) {
    
                        case INSERT_IO:
                            chr = ReadTYChar();
                            if (chr < 32) break;
                            value = numericTYDigits[((int)chr & 0x7f) - 32];
                            if (value == -1) {
                                (void)printf("Invalid numeric character: 0x%02x\n", chr);
                                SetTrigger(RD_CHK_TRIGGER, ON);
                                value = 0;
                            } else if (value == -2) {
                                EndIOInstruction();
                                runState = RUNNING_STATE;
                                SetTrigger(INSERT_TRIGGER, OFF);
                                SetTrigger(INSERT_2_TRIGGER, OFF);
                                SetTrigger(STOP_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                                break;
                            } else if (value == -3) {
                                break;
                            }
                            SetTrigger(IO_FLAG_TRIGGER, (value & FLAG_MASK) ? ON : OFF);
                            SetMDR(value);
                            if (!WriteMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, 1);
                            if (mars[OR2_MAR].value == 100) {
                                EndIOInstruction();
                                runState = SINGLE_INSTRUCTION_STATE;
                                SetTrigger(INSERT_TRIGGER, OFF);
                                SetTrigger(INSERT_2_TRIGGER, OFF);
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                            }
                            break;
    
                        case LOAD_IO:
                            SetTrigger(RDR_FEED_TRIGGER, OFF);
                            chr = ReadCDChar();
                            if (chr < 32) break;
                            value = numericCDDigits[((int)chr & 0x7f) - 32];
                            if (value < 0) {
                                (void)printf("Invalid numeric character: 0x%02x\n", chr);
                                SetTrigger(RD_CHK_TRIGGER, ON);
                                value = 0;
                            }
                            SetMDR(value);
                            if (!WriteMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, 1);
                            if (++ioCounter == NUM_CARD_POS) {
                                EndIOInstruction();
                                runState = RUNNING_STATE;
                                SetTrigger(INSERT_TRIGGER, OFF);
                                SetTrigger(INSERT_2_TRIGGER, OFF);
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                                if (readerLastCard) {
                                    SetTrigger(LAST_CARD_TRIGGER, ON);
                                }
                            }
                            break;
    
                        case DNTY_IO:
                            if (!ReadMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, 1);
                            temp = (mdr.digit.flag << 4) | mdr.digit.value;
                            chr = dumpTYChars[temp];
                            if (chr == 0) {
                                WriteTYChar(PROTOCOL_INVALID_CHARACTER);
                                SetTrigger(WR_CHK_TRIGGER, ON);
                            } else {
                                WriteTYChar(chr);
                            }
                            if ((mars[OR2_MAR].value % 20000) == 0) {
                                EndIOInstruction();
                                SetTrigger(MEM_OFLOW_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                            }
                            break;
    
                        case RNTY_IO:
                            chr = ReadTYChar();
                            if (chr < 32) break;
                            value = numericTYDigits[((int)chr & 0x7f) - 32];
                            if (value == -1) {
                                (void)printf("Invalid numeric character: 0x%02x\n", chr);
                                SetTrigger(RD_CHK_TRIGGER, ON);
                                value = 0;
                            } else if (value == -2) {
                                EndIOInstruction();
                                runState = RUNNING_STATE;
                                SetTrigger(STOP_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                                break;
                            } else if (value == -3) {
                                break;
                            }
                            SetTrigger(IO_FLAG_TRIGGER, (value & FLAG_MASK) ? ON : OFF);
                            SetMDR(value);
                            if (!WriteMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, 1);
                            break;
    
                        case RATY_IO:
                            chr = ReadTYChar();
                            if (chr < 32) break;
                            value = alphamericTYDigits[((int)chr & 0x7f) - 32];
                            if (value == -1) {
                                (void)printf("Invalid alphameric character: 0x%02x\n", chr);
                                SetTrigger(RD_CHK_TRIGGER, ON);
                                value = 0;
                            } else if (value == -2) {
                                EndIOInstruction();
                                runState = RUNNING_STATE;
                                SetTrigger(STOP_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                                break;
                            } else if (value == -3) {
                                break;
                            }
                            if (!WritePairMemory(OR2_MAR, value, ODD)) break;
                            IncrementMAR(OR2_MAR, 2);
                            break;
    
                        case WNTY_IO:
                            if (!ReadMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, 1);
                            temp = (mdr.digit.flag << 4) | mdr.digit.value;
                            chr = numericTYChars[temp];
                            if (chr == '#') {
                                EndIOInstruction();
                                SetTrigger(REC_MARK_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                                break;
                            } else if (chr == 0) {
                                WriteTYChar(PROTOCOL_INVALID_CHARACTER);
                                SetTrigger(WR_CHK_TRIGGER, ON);
                            } else {
                                WriteTYChar(chr);
                            }
                            break;
    
                        case WATY_IO:
                            if (!ReadPairMemory(OR2_MAR, ODD)) break;
                            IncrementMAR(OR2_MAR, 2);
                            temp = (mbr.digitEven.value << 4) | mbr.digitOdd.value;
                            chr = alphamericTYChars[temp];
                            if (chr == '#') {
                                EndIOInstruction();
                                SetTrigger(REC_MARK_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                                break;
                            } else if (chr == 0) {
                                WriteTYChar(PROTOCOL_INVALID_CHARACTER);
                                SetTrigger(WR_CHK_TRIGGER, ON);
                            } else {
                                WriteTYChar(chr);
                            }
                            break;
    
                        case DNCD_IO:
                            if (!ReadMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, 1);
                            SetTrigger(PCH_FEED_TRIGGER, OFF);
                            temp = (mdr.digit.flag << 4) | mdr.digit.value;
                            chr = dumpCDChars[temp];
                            if (chr == 0) {
                                WriteCDChar(PROTOCOL_INVALID_CHARACTER);
                                SetTrigger(WR_CHK_TRIGGER, ON);
                            } else {
                                WriteCDChar(chr);
                            }
                            if ((mars[OR2_MAR].value % 20000) == 0) {
                                SetTrigger(MEM_OFLOW_TRIGGER, ON);
                            }
                            if (++ioCounter == NUM_CARD_POS) {
                                EndIOInstruction();
                                if (!TestTrigger(MEM_OFLOW_TRIGGER)) {
                                    BeginIOInstruction(DNCD_IO);
                                    if (!punchReady) SetLight(PUNCH_NO_FEED_LIGHT, ON);
                                } else {
                                    SetTrigger(DISCN_GATE_TRIGGER, ON);
                                    SetTrigger(IO_EXIT_TRIGGER, ON);
                                    SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                    SetTrigger(IO_HOLD_TRIGGER, OFF);
                                }
                            }
                            break;
    
                        case RNCD_IO:
                            SetTrigger(RDR_FEED_TRIGGER, OFF);
                            chr = ReadCDChar();
                            if (chr < 32) break;
                            value = numericCDDigits[((int)chr & 0x7f) - 32];
                            if (value < 0) {
                                (void)printf("Invalid numeric character: 0x%02x\n", chr);
                                SetTrigger(RD_CHK_TRIGGER, ON);
                                value = 0;
                            }
                            SetMDR(value);
                            if (!WriteMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, 1);
                            if (++ioCounter == NUM_CARD_POS) {
                                EndIOInstruction();
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                                if (readerLastCard) {
                                    SetTrigger(LAST_CARD_TRIGGER, ON);
                                }
                            }
                            break;
    
                        case RACD_IO:
                            SetTrigger(RDR_FEED_TRIGGER, OFF);
                            chr = ReadCDChar();
                            if (chr < 32) break;
                            value = alphamericCDDigits[((int)chr & 0x7f) - 32];
                            if (value < 0) {
                                (void)printf("Invalid alphameric character: 0x%02x\n", chr);
                                SetTrigger(RD_CHK_TRIGGER, ON);
                                value = 0;
                            }
                            if (!WritePairMemory(OR2_MAR, value, ODD)) break;
                            IncrementMAR(OR2_MAR, 2);
                            if (++ioCounter == NUM_CARD_POS) {
                                EndIOInstruction();
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                                if (readerLastCard) {
                                    SetTrigger(LAST_CARD_TRIGGER, ON);
                                }
                            }
                            break;
    
                        case WNCD_IO:
                            if (!ReadMemory(OR2_MAR)) break;
                            IncrementMAR(OR2_MAR, 1);
                            SetTrigger(PCH_FEED_TRIGGER, OFF);
                            temp = (mdr.digit.flag << 4) | mdr.digit.value;
                            chr = numericCDChars[temp];
                            if (chr == 0) {
                                WriteCDChar(PROTOCOL_INVALID_CHARACTER);
                                SetTrigger(WR_CHK_TRIGGER, ON);
                            } else {
                                WriteCDChar(chr);
                            }
                            if (++ioCounter == NUM_CARD_POS) {
                                EndIOInstruction();
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                            }
                            break;
    
                        case WACD_IO:
                            if (!ReadPairMemory(OR2_MAR, ODD)) break;
                            IncrementMAR(OR2_MAR, 2);
                            SetTrigger(PCH_FEED_TRIGGER, OFF);
                            temp = (mbr.digitEven.value << 4) | mbr.digitOdd.value;
                            chr = alphamericCDChars[temp];
                            if (chr == 0) {
                                WriteCDChar(PROTOCOL_INVALID_CHARACTER);
                                SetTrigger(WR_CHK_TRIGGER, ON);
                            } else {
                                WriteCDChar(chr);
                            }
                            if (++ioCounter == NUM_CARD_POS) {
                                EndIOInstruction();
                                SetTrigger(DISCN_GATE_TRIGGER, ON);
                                SetTrigger(IO_EXIT_TRIGGER, ON);
                                SetTrigger(DISCN_GATE_TRIGGER, OFF);
                                SetTrigger(IO_HOLD_TRIGGER, OFF);
                            }
                            break;
                    }

                } else if (ioState == ABORTED_IO_STATE) {
                    EndIOInstruction();
                    SetTrigger(INSERT_TRIGGER, OFF);
                    SetTrigger(INSERT_2_TRIGGER, OFF);
                    SetTrigger(DISCN_GATE_TRIGGER, ON);
                    SetTrigger(IO_EXIT_TRIGGER, ON);
                    SetTrigger(DISCN_GATE_TRIGGER, OFF);
                    SetTrigger(IO_HOLD_TRIGGER, OFF);
                }

                // Set next cycle
                if ((ioState == RUNNING_IO_STATE) ||
                    ((ioState == ABORTED_IO_STATE) && ((ioType == LOAD_IO) || (ioType == DNCD_IO) ||
                                                       (ioType == RNCD_IO) || (ioType == RACD_IO) ||
                                                       (ioType == WNCD_IO) || (ioType == WACD_IO)))) {
                    SetTrigger(RSP_GATE_TRIGGER, OFF);
                    SetTrigger(IO_HOLD_TRIGGER, ON);
                    SetTrigger(IO_SYNC_TRIGGER, OFF);
                    if ((ioType == DNCD_IO) || (ioType == WNCD_IO) || (ioType == WACD_IO)) {
                        cycle = 30;
                    } else {
                        cycle = 31;
                    }
                } else {
                    SetTrigger(MEM_OFLOW_TRIGGER, OFF);
                    SetTrigger(DISCN_GATE_TRIGGER, OFF);
                    SetTrigger(RSP_GATE_TRIGGER, OFF);
                    SetTrigger(RW_CALL_TRIGGER, OFF);
                    SetTrigger(IO_EXIT_TRIGGER, ON);
                    SetTrigger(IO_FLAG_TRIGGER, OFF);
                    SetTrigger(IO_SYNC_TRIGGER, OFF);
                    cycle = 1;
                }
                break;

            case 31:    // E-31

                // Read memory per OR2
                if (!ReadMemory(OR2_MAR)) break;
                SaveMAR(OR2_MAR);

                // Set next cycle
                cycle = 30;
                break;

            case 32:    // E-32

                // Initialize on first entry
                if (!TestTrigger(FIRST_CYCLE_TRIGGER)) {
                    SetTrigger(FIRST_CYCLE_TRIGGER, ON);
                    SetTrigger(HP_TRIGGER, ON);
                    if ((opCode == M_OPCODE) || (opCode == MM_OPCODE)) SetTrigger(EZ_TRIGGER, ON);
                    SetMAR(PR1_MAR, 80);
                }

                // Clear memory locations 00080 - 0099
                if (!ReadPairMemory(PR1_MAR, EVEN)) break;
                if (!TestTrigger(CYCLE_CTRL_TRIGGER)) {
                    IncrementMAR(PR1_MAR, 2);
                } else {
                    IncrementMAR(PR1_MAR, 1);
                }
                ClearMBR();
                if (!WritePairMemory(MAR_MAR, 0, EVEN)) break;
                if (mars[PR1_MAR].value == 98) {
                    SetTrigger(CYCLE_CTRL_TRIGGER, ON);
                }

                // Set next cycle
                if (mars[PR1_MAR].value < 99) {
                    cycle = 32;

                } else if ((opCode == M_OPCODE) || (opCode == MM_OPCODE)) {
                    cycle = 33;

                } else /* opCode == LD_OPCODE || opCode == LDM_OPCODE */{
                    cycle = 26;
                }
                break;

            case 33:    // E-33

                // Read multiplier digit
                if (!ReadMemory(OR1_MAR)) break;
                IncrementMAR(OR1_MAR, -1);
                SetMQR(mdr.digit.value);
                mqr.value = mqr.digit.value;

                // Process operation
                if (mdr.digit.flag) {
                    if (TestTrigger(FIRST_CYCLE_TRIGGER)) {
                        SetTrigger(HP_TRIGGER, OFF);
                    } else {
                        SetTrigger(FIELD_MK_1_TRIGGER, ON);
                        SetTrigger(CYCLE_CTRL_TRIGGER, ON);
                    }
                }
                if (TestTrigger(FIRST_CYCLE_TRIGGER)) {
                    SetTrigger(INCR_TRIGGER, OFF);
                    SetTrigger(DECR_TRIGGER, ON);
                    SetTrigger(PLUS_2_TRIGGER, OFF);
                }
                SetTrigger(FIELD_MK_2_TRIGGER, OFF);
                SetTrigger(MULTIPLY_UNITS_TRIGGER, ON);

                // Set next cycle
                cycle = 34;
                break;

            case 34:    // E-34

                // Read multiplicand digit
                if (TestTrigger(MULTIPLY_UNITS_TRIGGER)) {
                    if (!ReadMemory(OR2_MAR)) break;
                    SaveMAR(OR2_MAR);
                    SaveMAR(OR3_MAR);
                    IncrementMAR(OR3_MAR, -1);
                } else {
                    if (!ReadMemory(OR3_MAR)) break;
                    IncrementMAR(OR3_MAR, -1);
                }

                // Process operation
                if (mdr.digit.flag) {
                    if (TestTrigger(MULTIPLY_UNITS_TRIGGER)) {
                        if (TestTrigger(FIRST_CYCLE_TRIGGER)) {
                            SetTrigger(HP_TRIGGER, !TestTrigger(HP_TRIGGER));
                        }
                    } else {
                        SetTrigger(FIELD_MK_2_TRIGGER, ON);
                    }
                }

                // Set next cycle
                cycle = 35;
                break;

            case 35:    // E-35

                // Form multiply table lookup address
                if ((mqr.digit.value > 9) || (mdr.digit.value > 9)) {
                    SetTrigger(MAR_CHK_TRIGGER, ON);
                    break;
                }
                temp = 2 * mqr.digit.value;
                SetMAR(MAR_MAR, 100 + 100 * (temp / 10) + 10 * mdr.digit.value + (temp % 10));

                // Read product digits
                if (!ReadPairMemory(MAR_MAR, EVEN)) break;
                SetDR(mbr.digitOdd.value, mbr.digitEven.value);

                // Set auxiliary triggers
                SetTrigger(CARRY_IN_TRIGGER, OFF);
                SetTrigger(CARRY_OUT_TRIGGER, OFF);

                // Set next cycle
                cycle = 36;
                break;

            case 36:    // E-36

                // Read product area 1st digit
                if (TestTrigger(MULTIPLY_UNITS_TRIGGER)) {
                    if (!ReadMemory(PR1_MAR)) break;
                    IncrementMAR(PR1_MAR, -1);
                    SaveMAR(PR2_MAR);
                    SetTrigger(MULTIPLY_UNITS_TRIGGER, OFF);
                } else {
                    if (!ReadMemory(PR2_MAR)) break;
                    SaveMAR(PR2_MAR);
                }

                // Set next cycle
                cycle = 37;
                break;

            case 37:    // E-37

                // Form add table lookup address
                if ((mdr.digit.value > 9) || (dr.digit01.value > 9)) {
                    SetTrigger(MAR_CHK_TRIGGER, ON);
                    break;
                }
                SetMAR(MAR_MAR, 300 + 10 * mdr.digit.value + dr.digit01.value);

                // Read sum digit
                if (!ReadMemory(MAR_MAR)) break;
                SetDR(0, dr.digit10.value);

                // Set auxiliary triggers
                if (mdr.digit.flag) {
                    SetTrigger(CARRY_OUT_TRIGGER, ON);
                }
                if (mdr.digit.value != 0) {
                    SetTrigger(EZ_TRIGGER, OFF);
                }

                // Set next cycle
                cycle = 38;
                break;

            case 38:    // E-38

                // Set product sign
                if (TestTrigger(FIRST_CYCLE_TRIGGER) && !TestTrigger(HP_TRIGGER)) {
                    mdr.digit.flag = 1;
                } else {
                    mdr.digit.flag = 0;
                }

                // Store 1st sum digit in product area
                if (!WriteMemory(PR2_MAR)) break;
                IncrementMAR(PR2_MAR, -1);
                LoadMAR(PR2_MAR);
                SaveMAR(PR3_MAR);

                // Set auxiliary triggers
                if (TestTrigger(CARRY_OUT_TRIGGER)) {
                    SetTrigger(CARRY_IN_TRIGGER, ON);
                }

                // Set next cycle
                cycle = 39;
                break;

            case 39:    // E-39

                // Read product area 2nd, 3rd, ... digit
                if (!ReadMemory(PR3_MAR)) break;
                SaveMAR(PR3_MAR);

                // Set auxiliary triggers
                SetTrigger(FIRST_CYCLE_TRIGGER, OFF);
                SetTrigger(CARRY_OUT_TRIGGER, OFF);
                SetTrigger(CYCLE_CTRL_TRIGGER, OFF);

                // Set next cycle
                cycle = 40;
                break;

            case 40:    // E-40

                // Form add table lookup address
                if ((mdr.digit.value <= 9) && (dr.digit01.value <= 9)) {
                    SetMAR(MAR_MAR, 300 + 10 * mdr.digit.value +
                           (dr.digit01.value + TestTrigger(CARRY_IN_TRIGGER)) % 10);
                } else {
                    runState = CHECK_STOP_STATE;
                    break;
                }

                // Read sum digit
                if (!ReadMemory(MAR_MAR)) break;

                // Set auxiliary triggers
                if (mdr.digit.flag || ((dr.digit01.value == 9) && TestTrigger(CARRY_IN_TRIGGER))) {
                    SetTrigger(CARRY_OUT_TRIGGER, ON);
                }
                if (mdr.digit.value != 0) {
                    SetTrigger(EZ_TRIGGER, OFF);
                }
                SetTrigger(CARRY_IN_TRIGGER, OFF);
                SetDR(dr.digit10.value, 0);

                // Set next cycle
                cycle = 41;
                break;

            case 41:    // E-41

                // Set end of product field mark
                if (TestTrigger(FIELD_MK_1_TRIGGER) && TestTrigger(FIELD_MK_2_TRIGGER)) {
                    mdr.digit.flag = 1;
                } else {
                    mdr.digit.flag = 0;
                }

                // Store 2nd, 3rd, ... sum digit in product area
                if (!WriteMemory(PR3_MAR)) break;
                IncrementMAR(PR3_MAR, -1);

                // Set auxiliary triggers
                if (TestTrigger(CARRY_OUT_TRIGGER)) {
                    SetTrigger(CARRY_IN_TRIGGER, ON);
                }

                // Set next cycle
                if (TestTrigger(CARRY_OUT_TRIGGER) || TestTrigger(FIELD_MK_2_TRIGGER)) {
                    cycle = 19;
                } else {
                    cycle = 34;
                }
                break;

            case 42:    // E-42

                // Process operation
                SetMDR(mqr.value);
                if (TestTrigger(DIV_1_CYCLE_TRIGGER)) {
                    SetMDRFlag(TRUE);
                }
                if (TestTrigger(DIV_LAST_CYC_TRIGGER) && !TestTrigger(HP_TRIGGER)) {
                    SetMDRFlag(TRUE);
                }
                if (!WriteMemory(OR2_MAR)) break;
                SaveMAR(OR2_MAR);
                if (mqr.value != 0) {
                    SetTrigger(EZ_TRIGGER, OFF);
                }

                // Set next cycle
                cycle = 43;
                break;

            case 43:    // E-43

                // Process operation
                LoadMAR(OR3_MAR);
                IncrementMAR(OR3_MAR, 1);
                ClearMQR();
                SetTrigger(DIV_1_CYCLE_TRIGGER, OFF);

                // Set next cycle
                if (TestTrigger(DIV_LAST_CYC_TRIGGER)) {
                    cycle = 1;
                } else {
                    cycle = 44;
                }
                break;

            case 44:    // E-44

                // Process operation
                LoadMAR(OR3_MAR);
                SaveMAR(OR2_MAR);
                // if (mar.value == 99) {
                if ((mar.digit10000.value == 0) &&
                    (mar.digit00010.value == 9) &&
                    (mar.digit00001.value == 9)) {
                    SetTrigger(DIV_LAST_CYC_TRIGGER, ON);
                }
                SetTrigger(INCR_TRIGGER, OFF);
                SetTrigger(DECR_TRIGGER, ON);
                SetTrigger(PLUS_2_TRIGGER, OFF);
                SetTrigger(BYPASS_TRIGGER, OFF);
                SetTrigger(DIV_ADD_TRIGGER, !TestTrigger(CARRY_OUT_TRIGGER));

                // Set next cycle
                cycle = 45;
                break;

            case 45:    // E-45

                // Process operation
                LoadMAR(PR1_MAR);
                SaveMAR(OR1_MAR);
                SetTrigger(FIELD_MK_1_TRIGGER, OFF);
                SetTrigger(FIELD_MK_2_TRIGGER, OFF);
                SetTrigger(FIRST_CYCLE_TRIGGER, ON);
                if (mqr.digit.value == 10) {
                    SetTrigger(ARITH_CHK_TRIGGER, ON);
                }

                // Set next cycle
                if (mqr.digit.value == 10) {
                    cycle = 1;
                } else {
                    cycle = 11;
                }
                break;

            case 46:    // E-46

                // Process operation
                if (!TestTrigger(P_Q_TRIGGER) && !TestTrigger(NEXT_IA_TRIGGER)) {
                    SetTrigger(IA_TRIGGER, OFF);
                    SetTrigger(I_TIME_TRIGGER, ON);
                }

                // Set next cycle
                if (!TestTrigger(P_Q_TRIGGER) && !TestTrigger(IA_TRIGGER)) {
                    cycle = 8;
                } else {
                    cycle = 47;
                }
                break;

            case 47:    // E-47

                // Process operation
                if (TestTrigger(P_Q_TRIGGER)) {
                    if (!ReadPairMemory(OR2_MAR, ANY)) break;
                    SaveMAR(PR3_MAR);
                    IncrementMAR(PR3_MAR, -2);
                    if (mars[OR2_MAR].value & 0x1) {
                        SetDigit(&mars[OR2_MAR].digit00001, mbr.digitOdd.value);
                        CopyDigit(&mars[OR3_MAR].digit00001, &mars[OR2_MAR].digit00001);
                        SetDigit(&mars[OR2_MAR].digit00010, mbr.digitEven.value);
                        CopyDigit(&mars[OR3_MAR].digit00010, &mars[OR2_MAR].digit00010);
                        SetTrigger(NEXT_IA_TRIGGER, mbr.digitOdd.flag);
                    } else {
                        SetDigit(&mars[OR2_MAR].digit00001, mbr.digitEven.value);
                        CopyDigit(&mars[OR3_MAR].digit00001, &mars[OR2_MAR].digit00001);
                        SetTrigger(NEXT_IA_TRIGGER, mbr.digitEven.flag);
                    }
                } else {
                    if (!ReadPairMemory(OR1_MAR, ANY)) break;
                    SaveMAR(PR3_MAR);
                    IncrementMAR(PR3_MAR, -2);
                    if (mars[OR1_MAR].value & 0x1) {
                        SetDigit(&mars[OR1_MAR].digit00001, mbr.digitOdd.value);
                        CopyDigit(&mars[PR1_MAR].digit00001, &mars[OR1_MAR].digit00001);
                        SetDigit(&mars[OR1_MAR].digit00010, mbr.digitEven.value);
                        CopyDigit(&mars[PR1_MAR].digit00010, &mars[OR1_MAR].digit00010);
                        SetTrigger(NEXT_IA_TRIGGER, mbr.digitOdd.flag);
                    } else {
                        SetDigit(&mars[OR1_MAR].digit00001, mbr.digitEven.value);
                        CopyDigit(&mars[PR1_MAR].digit00001, &mars[OR1_MAR].digit00001);
                        SetTrigger(NEXT_IA_TRIGGER, mbr.digitEven.flag);
                    }
                }

                // Set next cycle
                cycle = 48;
                break;

            case 48:    // E-48

                // Process operation
                if (!ReadPairMemory(PR3_MAR, ANY)) break;
                IncrementMAR(PR3_MAR, -2);
                if (TestTrigger(P_Q_TRIGGER)) {
                    if (mars[PR3_MAR].value & 0x1) {
                        SetDigit(&mars[OR2_MAR].digit00100, mbr.digitOdd.value);
                        CopyDigit(&mars[OR3_MAR].digit00100, &mars[OR2_MAR].digit00100);
                        SetDigit(&mars[OR2_MAR].digit01000, mbr.digitEven.value);
                        CopyDigit(&mars[OR3_MAR].digit01000, &mars[OR2_MAR].digit01000);
                    } else {
                        SetDigit(&mars[OR2_MAR].digit00100, mbr.digitEven.value);
                        CopyDigit(&mars[OR3_MAR].digit00100, &mars[OR2_MAR].digit00100);
                        SetDigit(&mars[OR2_MAR].digit00010, mbr.digitOdd.value);
                        CopyDigit(&mars[OR3_MAR].digit00010, &mars[OR2_MAR].digit00010);
                    }
                } else {
                    if (mars[PR3_MAR].value & 0x1) {
                        SetDigit(&mars[OR1_MAR].digit00100, mbr.digitOdd.value);
                        CopyDigit(&mars[PR1_MAR].digit00100, &mars[OR1_MAR].digit00100);
                        SetDigit(&mars[OR1_MAR].digit01000, mbr.digitEven.value);
                        CopyDigit(&mars[PR1_MAR].digit01000, &mars[OR1_MAR].digit01000);
                    } else {
                        SetDigit(&mars[OR1_MAR].digit00100, mbr.digitEven.value);
                        CopyDigit(&mars[PR1_MAR].digit00100, &mars[OR1_MAR].digit00100);
                        SetDigit(&mars[OR1_MAR].digit00010, mbr.digitOdd.value);
                        CopyDigit(&mars[PR1_MAR].digit00010, &mars[OR1_MAR].digit00010);
                    }
                }

                // Set next cycle
                cycle = 49;
                break;

            case 49:    // E-49

                // Process operation
                if (!ReadPairMemory(PR3_MAR, ANY)) break;
                SaveMAR(PR3_MAR);
                if (TestTrigger(P_Q_TRIGGER)) {
                    if (mars[PR3_MAR].value & 0x1) {
                        SetDigit(&mars[OR2_MAR].digit10000, mbr.digitOdd.value);
                        CopyDigit(&mars[OR3_MAR].digit10000, &mars[OR2_MAR].digit10000);
                    } else {
                        SetDigit(&mars[OR2_MAR].digit10000, mbr.digitEven.value);
                        CopyDigit(&mars[OR3_MAR].digit10000, &mars[OR2_MAR].digit10000);
                        SetDigit(&mars[OR2_MAR].digit01000, mbr.digitOdd.value);
                        CopyDigit(&mars[OR3_MAR].digit01000, &mars[OR2_MAR].digit01000);
                    }
                    if ((mars[OR2_MAR].digit10000.value <= 9) && (mars[OR2_MAR].digit01000.value <= 9) &&
                        (mars[OR2_MAR].digit00100.value <= 9) && (mars[OR2_MAR].digit00010.value <= 9) &&
                        (mars[OR2_MAR].digit00001.value <= 9)) {
                        mars[OR2_MAR].value = 10000 * mars[OR2_MAR].digit10000.value +
                                               1000 * mars[OR2_MAR].digit01000.value +
                                                100 * mars[OR2_MAR].digit00100.value +
                                                 10 * mars[OR2_MAR].digit00010.value +
                                                      mars[OR2_MAR].digit00001.value;
                        mars[OR3_MAR].value = mars[OR2_MAR].value;
                    } else {
                        mars[OR2_MAR].value = -1;
                        mars[OR3_MAR].value = -1;
                    }
                    if (!TestTrigger(NEXT_IA_TRIGGER)) {
                        SetTrigger(IA_TRIGGER, OFF);
                        SetTrigger(I_TIME_TRIGGER, ON);
                    }
                } else {
                    if (mars[PR3_MAR].value & 0x1) {
                        SetDigit(&mars[OR1_MAR].digit10000, mbr.digitOdd.value);
                        CopyDigit(&mars[PR1_MAR].digit10000, &mars[OR1_MAR].digit10000);
                    } else {
                        SetDigit(&mars[OR1_MAR].digit10000, mbr.digitEven.value);
                        CopyDigit(&mars[PR1_MAR].digit10000, &mars[OR1_MAR].digit10000);
                        SetDigit(&mars[OR1_MAR].digit01000, mbr.digitOdd.value);
                        CopyDigit(&mars[PR1_MAR].digit01000, &mars[OR1_MAR].digit01000);
                    }
                    if ((mars[OR1_MAR].digit10000.value <= 9) && (mars[OR1_MAR].digit01000.value <= 9) &&
                        (mars[OR1_MAR].digit00100.value <= 9) && (mars[OR1_MAR].digit00010.value <= 9) &&
                        (mars[OR1_MAR].digit00001.value <= 9)) {
                        mars[OR1_MAR].value = 10000 * mars[OR1_MAR].digit10000.value +
                                               1000 * mars[OR1_MAR].digit01000.value +
                                                100 * mars[OR1_MAR].digit00100.value +
                                                 10 * mars[OR1_MAR].digit00010.value +
                                                      mars[OR1_MAR].digit00001.value;
                        mars[PR1_MAR].value = mars[OR1_MAR].value;
                    } else {
                        mars[OR1_MAR].value = -1;
                        mars[PR1_MAR].value = -1;
                    }
                }

                // Set next cycle
                if (TestTrigger(P_Q_TRIGGER) && !TestTrigger(IA_TRIGGER)) {
                    cycle = 5;
                } else {
                    cycle = 46;
                }
                break;

            case 50:    // E-50

                (void)printf("Unimplemented cycle: E-50\n");
                runState = CHECK_STOP_STATE;
                break;

            case 51:    // E-51

                (void)printf("Unimplemented cycle: E-51\n");
                runState = CHECK_STOP_STATE;
                break;

            case 52:    // E-52

                (void)printf("Unimplemented cycle: E-52\n");
                runState = CHECK_STOP_STATE;
                break;

            case 53:    // E-53

                (void)printf("Unimplemented cycle: E-53\n");
                runState = CHECK_STOP_STATE;
                break;

            case 54:    // E-54

                (void)printf("Unimplemented cycle: E-54\n");
                runState = CHECK_STOP_STATE;
                break;

            case 55:    // E-55

                (void)printf("Unimplemented cycle: E-55\n");
                runState = CHECK_STOP_STATE;
                break;

            case 56:    // I-1 (I/O wait)

                // End control instruction
                EndIOInstruction();

                // Set next cycle
                cycle = 1;
                
                break;

            default:

                // Bad cycle
                (void)printf("Bad cycle: %d\n", cycle);
                runState = CHECK_STOP_STATE;
                break;

        }

    End_Cycle:

        // Update all display lights for this cycle
        if ((powerState == POWER_ON) || (powerState == POWER_READY)) {
            UpdateDisplay();
        }

        // Update runState as needed
        if (powerState == POWER_READY) {
            if ((runState != CHECK_STOP_STATE) && TestTrigger(MAR_CHK_TRIGGER)) {
                runState = CHECK_STOP_STATE;
                TimeLog("MAR Check Stop");
            } else if ((runState == CHECK_STOP_STATE) && (lastRunState != CHECK_STOP_STATE)) {
                TimeLog("Check Stop");
            } else if (runState == SINGLE_CYCLE_STATE) {
                runState = STOPPED_STATE;
            } else if ((runState == SINGLE_INSTRUCTION_STATE) && (cycle == 1)) {
                runState = STOPPED_STATE;
            } else if ((runState == RUNNING_STATE) && (cycle == 1)) {
                if (TestSwitch(IO_STOP_SWITCH) && (TestTrigger(RD_CHK_TRIGGER) || TestTrigger(WR_CHK_TRIGGER))) {
                    runState = STOPPED_STATE;
                    TimeLog("I/O Stop");
                }
                if (TestSwitch(OFLOW_STOP_SWITCH) && (TestTrigger(EXP_CHK_TRIGGER) || TestTrigger(ARITH_CHK_TRIGGER))) {
                    runState = STOPPED_STATE;
                    TimeLog("O'Flow Stop");
                }
            }
        }

        // Sync to cycle interval time
#ifdef HARDWARE_TIMER
        syncTime += CYCLE_TIME;
        while (*counter < syncTime) {
            (void)syscall(SYS_gettid);    // ~ 0.25usec (RPi3) system call
        }
#else
        if (--cycleCounter == 0) {    // cannot sync every 20usec machine cycle due to 54usec (RPi3) overhead of
                                      // clock_nanosleep()
            cycleCounter = CYCLE_COUNT;
            syncTime += CYCLE_COUNT * CYCLE_INTERVAL;
            sleepTime.tv_sec = syncTime / 1000000000LL;
            sleepTime.tv_nsec = syncTime % 1000000000LL;
            (void)clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepTime, NULL);
        }
#endif
    }

#ifdef HARDWARE_TIMER
    (void)munmap(timerMmap, BCM2835_PAGE_SIZE);
    (void)close(timerFile);
#endif

    // End simulator thread
    pthread_exit(NULL);
}

static void InitializeSimulator(void) {
    runState = STOPPED_STATE;
    ioState = STOPPED_IO_STATE;
    ioType = NO_IO;
    cycle = 1;
    lastCycle = 0;
    ClearRegisters();
    ClearTriggers();
    (void)memset((void *)lastSwitches, FALSE, sizeof(lastSwitches));
    (void)memset((void *)previousLights, 0, sizeof(previousLights));
    (void)memset((void *)outputs, 0, sizeof(outputs));
}

static void ResetSimulator(void) {
    runState = STOPPED_STATE;
    cycle = 1;
    lastCycle = 0;
    ClearTriggers();
    SetLight(PUNCH_NO_FEED_LIGHT, OFF);
    SetLight(READER_NO_FEED_LIGHT, OFF);
    SetLight(SAVE_LIGHT, OFF);
    SetLight(INSERT_LIGHT, OFF);
    SetLight(CHECK_STOP_LIGHT, OFF);
    if (ioState != STOPPED_IO_STATE) EndIOInstruction();
    WriteCDCommand(CD_COMMAND_RESET);
}

static inline bool TestLiveSwitch(int sw) {
    // Special handling for Emergency Off switch
    if (sw == EMERGENCY_OFF_SWITCH) {
        return !inputs[sw];
    }

    return inputs[sw];
}

static inline bool TestSwitch(int sw) {
    // Special handling for momentary switches
    if ((sw == RESET_SWITCH)   || (sw == DISPLAY_MAR_SWITCH) || (sw == SAVE_SWITCH)     || (sw == INSERT_SWITCH) ||
        (sw == RELEASE_SWITCH) || (sw == START_SWITCH)       || (sw == STOP_SIE_SWITCH) ||
        (sw == INSTANT_STOP_SCE_SWITCH)) {
        if (inputs[sw]) {
            if (!lastSwitches[sw]) {
                lastSwitches[sw] = TRUE;
                return TRUE;
            } else {
                return FALSE;
            }
        } else {
            lastSwitches[sw] = FALSE;
            return FALSE;
        }
    }

    // Special handling for Emergency Off switch
    if (sw == EMERGENCY_OFF_SWITCH) {
        return !inputs[sw];
    }

    // Toggle switches & MAR Selector
    return inputs[sw];
}

static inline void SetLight(int lite, bool offOn) {
    if (lite > 0) {
        if (offOn) {
            ++writing->indicators[lite];    // pwm light
        }
    } else {
        outputs[0 - lite] = offOn;    // gpio light
    }
}

static void UpdateDisplay(void) {
    int temp;

    // Bump the total cycle count
    ++writing->total;

    // Count the current cycle
    temp = cycleLights[cycle];
    if (temp >= 0) {
        SetLight(temp, ON);
    }

    // Update cycle count for all of the displayed triggers that have been on in this cycle
    for (int trigger = 0; trigger < NUM_TRIGGER_LIGHTS; ++trigger) {
        if (TestCycleTrigger(trigger)) {
            SetLight(triggerLights[trigger], ON);
        }
    }

    // Update OPERATION REGISTER cycle counts
    SetLight(OR_C0_LIGHT, opr.digit10.check);
    SetLight(OR_80_LIGHT, opr.digit10.bit8);
    SetLight(OR_40_LIGHT, opr.digit10.bit4);
    SetLight(OR_20_LIGHT, opr.digit10.bit2);
    SetLight(OR_10_LIGHT, opr.digit10.bit1);
    SetLight(OR_0C_LIGHT, opr.digit01.check);
    SetLight(OR_08_LIGHT, opr.digit01.bit8);
    SetLight(OR_04_LIGHT, opr.digit01.bit4);
    SetLight(OR_02_LIGHT, opr.digit01.bit2);
    SetLight(OR_01_LIGHT, opr.digit01.bit1);

    // Update MULTIPLIER / QUOTIENT cycle counts
    SetLight(MQ_C_LIGHT, mqr.digit.check);
    SetLight(MQ_8_LIGHT, mqr.digit.bit8);
    SetLight(MQ_4_LIGHT, mqr.digit.bit4);
    SetLight(MQ_2_LIGHT, mqr.digit.bit2);
    SetLight(MQ_1_LIGHT, mqr.digit.bit1);

    // Update SENSE AND BRANCH cycle counts
    SetLight(SAB_C0_LIGHT, sabr.digit10.check);
    SetLight(SAB_80_LIGHT, sabr.digit10.bit8);
    SetLight(SAB_40_LIGHT, sabr.digit10.bit4);
    SetLight(SAB_20_LIGHT, sabr.digit10.bit2);
    SetLight(SAB_10_LIGHT, sabr.digit10.bit1);
    SetLight(SAB_0C_LIGHT, sabr.digit01.check);
    SetLight(SAB_08_LIGHT, sabr.digit01.bit8);
    SetLight(SAB_04_LIGHT, sabr.digit01.bit4);
    SetLight(SAB_02_LIGHT, sabr.digit01.bit2);
    SetLight(SAB_01_LIGHT, sabr.digit01.bit1);

    // Update MEMORY BUFFER REGISTER cycle counts
    SetLight(MBR_EC_LIGHT, mbr.digitEven.check);
    SetLight(MBR_EF_LIGHT, mbr.digitEven.flag);
    SetLight(MBR_E8_LIGHT, mbr.digitEven.bit8);
    SetLight(MBR_E4_LIGHT, mbr.digitEven.bit4);
    SetLight(MBR_E2_LIGHT, mbr.digitEven.bit2);
    SetLight(MBR_E1_LIGHT, mbr.digitEven.bit1);
    SetLight(MBR_OC_LIGHT, mbr.digitOdd.check);
    SetLight(MBR_OF_LIGHT, mbr.digitOdd.flag);
    SetLight(MBR_O8_LIGHT, mbr.digitOdd.bit8);
    SetLight(MBR_O4_LIGHT, mbr.digitOdd.bit4);
    SetLight(MBR_O2_LIGHT, mbr.digitOdd.bit2);
    SetLight(MBR_O1_LIGHT, mbr.digitOdd.bit1);

    // Update MEMORY DATA REGISTER cycle counts
    SetLight(MDR_C_LIGHT, mdr.digit.check);
    SetLight(MDR_F_LIGHT, mdr.digit.flag);
    SetLight(MDR_8_LIGHT, mdr.digit.bit8);
    SetLight(MDR_4_LIGHT, mdr.digit.bit4);
    SetLight(MDR_2_LIGHT, mdr.digit.bit2);
    SetLight(MDR_1_LIGHT, mdr.digit.bit1);

    // Update DIGIT REGISTER cycle counts
    SetLight(DR_C0_LIGHT, dr.digit10.check);
    SetLight(DR_80_LIGHT, dr.digit10.bit8);
    SetLight(DR_40_LIGHT, dr.digit10.bit4);
    SetLight(DR_20_LIGHT, dr.digit10.bit2);
    SetLight(DR_10_LIGHT, dr.digit10.bit1);
    SetLight(DR_0C_LIGHT, dr.digit01.check);
    SetLight(DR_08_LIGHT, dr.digit01.bit8);
    SetLight(DR_04_LIGHT, dr.digit01.bit4);
    SetLight(DR_02_LIGHT, dr.digit01.bit2);
    SetLight(DR_01_LIGHT, dr.digit01.bit1);

    // Update MEMORY ADDRESS REGISTER cycle counts
    SetLight(MAR_C0000_LIGHT, mar.digit10000.check);
    SetLight(MAR_80000_LIGHT, mar.digit10000.bit8);
    SetLight(MAR_40000_LIGHT, mar.digit10000.bit4);
    SetLight(MAR_20000_LIGHT, mar.digit10000.bit2);
    SetLight(MAR_10000_LIGHT, mar.digit10000.bit1);
    SetLight(MAR_0C000_LIGHT, mar.digit01000.check);
    SetLight(MAR_08000_LIGHT, mar.digit01000.bit8);
    SetLight(MAR_04000_LIGHT, mar.digit01000.bit4);
    SetLight(MAR_02000_LIGHT, mar.digit01000.bit2);
    SetLight(MAR_01000_LIGHT, mar.digit01000.bit1);
    SetLight(MAR_00C00_LIGHT, mar.digit00100.check);
    SetLight(MAR_00800_LIGHT, mar.digit00100.bit8);
    SetLight(MAR_00400_LIGHT, mar.digit00100.bit4);
    SetLight(MAR_00200_LIGHT, mar.digit00100.bit2);
    SetLight(MAR_00100_LIGHT, mar.digit00100.bit1);
    SetLight(MAR_000C0_LIGHT, mar.digit00010.check);
    SetLight(MAR_00080_LIGHT, mar.digit00010.bit8);
    SetLight(MAR_00040_LIGHT, mar.digit00010.bit4);
    SetLight(MAR_00020_LIGHT, mar.digit00010.bit2);
    SetLight(MAR_00010_LIGHT, mar.digit00010.bit1);
    SetLight(MAR_0000C_LIGHT, mar.digit00001.check);
    SetLight(MAR_00008_LIGHT, mar.digit00001.bit8);
    SetLight(MAR_00004_LIGHT, mar.digit00001.bit4);
    SetLight(MAR_00002_LIGHT, mar.digit00001.bit2);
    SetLight(MAR_00001_LIGHT, mar.digit00001.bit1);
    SetLight(ODD_LIGHT, mar.digit00001.bit1);    // special case for ODD light
}

static inline bool ValidAddress(int addr) {
    if ((addr >= 0) && (addr < MEMORY_SIZE)) return TRUE;
    SetTrigger(MAR_CHK_TRIGGER, ON);
    return FALSE;
}

static inline bool ValidEvenAddress(int addr) {
    if ((addr >= 0) && (addr < MEMORY_SIZE) && !(addr & 0x1)) return TRUE;
    SetTrigger(MAR_CHK_TRIGGER, ON);
    return FALSE;
}

static inline bool ValidOddAddress(int addr) {
    if ((addr >= 0) && (addr < MEMORY_SIZE) && (addr & 0x1)) return TRUE;
    SetTrigger(MAR_CHK_TRIGGER, ON);
    return FALSE;
}

static bool ReadMemory(int marsId) {
    if (marsId != MAR_MAR) {
        LoadMAR(marsId);
    }
    if (!ValidAddress(mar.value)) return FALSE;
    SetTrigger(GR_50_TRIGGER, ((mar.value / 100) >= 50));
    SetMBR(memory[mar.value & 0xFFFFFFFE], memory[mar.value | 0x00000001]);
    if (mar.value & 0x1) {
        SetMDR((mbr.digitOdd.flag ? FLAG_MASK : 0) | mbr.digitOdd.value);
    } else {
        SetMDR((mbr.digitEven.flag ? FLAG_MASK : 0) | mbr.digitEven.value);
    }
    return TRUE;
}

static bool ReadPairMemory(int marsId, int align) {
    if (marsId != MAR_MAR) {
        LoadMAR(marsId);
    }
    if (align == EVEN) {
        if (!ValidEvenAddress(mar.value)) return FALSE;
    } else if (align == ODD) {
        if (!ValidOddAddress(mar.value)) return FALSE;
    } else /* align == ANY */ {
        if (!ValidAddress(mar.value)) return FALSE;
    }
    SetTrigger(GR_50_TRIGGER, ((mar.value / 100) >= 50));
    SetMBR(memory[mar.value & 0xFFFFFFFE], memory[mar.value | 0x00000001]);
    SetMDR((mbr.digitOdd.flag ? FLAG_MASK : 0) | mbr.digitOdd.value);
    return TRUE;
}

static bool WriteMemory(int marsId) {
    if (marsId != MAR_MAR) {
        LoadMAR(marsId);
    }
    if (!ValidAddress(mar.value)) return FALSE;
    SetTrigger(GR_50_TRIGGER, ((mar.value / 100) >= 50));
    if (mar.value & 0x1) {
        SetMBR(memory[mar.value & 0xFFFFFFFE], (mdr.digit.flag ? FLAG_MASK : 0) | mdr.digit.value);
        memory[mar.value] = (mbr.digitOdd.flag << 4) | (mbr.digitOdd.bit8 << 3) | (mbr.digitOdd.bit4 << 2) |
                            (mbr.digitOdd.bit2 << 1) |  mbr.digitOdd.bit1;
    } else {
        SetMBR((mdr.digit.flag ? FLAG_MASK : 0) | mdr.digit.value, memory[mar.value | 0x00000001]);
        memory[mar.value] = (mbr.digitEven.flag << 4) | (mbr.digitEven.bit8 << 3) | (mbr.digitEven.bit4 << 2) |
                            (mbr.digitEven.bit2 << 1) |  mbr.digitEven.bit1;
    }
    return TRUE;
}

static bool WritePairMemory(int marsId, int value, int align) {
    if (marsId != MAR_MAR) {
        LoadMAR(marsId);
    }
    if (align == EVEN) {
        if (!ValidEvenAddress(mar.value)) return FALSE;
    } else if (align == ODD) {
        if (!ValidOddAddress(mar.value)) return FALSE;
    } else /* align == ANY */ {
        if (!ValidAddress(mar.value)) return FALSE;
    }
    SetTrigger(GR_50_TRIGGER, ((mar.value / 100) >= 50));
    SetMBR(((memory[mar.value & 0xFFFFFFFE] & FLAG_MASK) | ((value >> 8) & DIGIT_MASK)),
           ((memory[mar.value | 0x00000001] & FLAG_MASK) | (value & DIGIT_MASK)));
    SetMDR((mbr.digitOdd.flag ? FLAG_MASK : 0) | mbr.digitOdd.value);
    memory[mar.value & 0xFFFFFFFE] = (mbr.digitEven.flag << 4) | (mbr.digitEven.bit8 << 3) | (mbr.digitEven.bit4 << 2) |
                                     (mbr.digitEven.bit2 << 1) | mbr.digitEven.bit1;
    memory[mar.value | 0x00000001] = (mbr.digitOdd.flag << 4) | (mbr.digitOdd.bit8 << 3) | (mbr.digitOdd.bit4 << 2) |
                                     (mbr.digitOdd.bit2 << 1) | mbr.digitOdd.bit1;
    return TRUE;
}

static inline void ClearTriggers(void) {
    (void)memset((void *)triggers, FALSE, sizeof(triggers));
    SetTrigger(I_TIME_TRIGGER, ON);
    SetTrigger(INCR_TRIGGER, ON);
}

static inline void SetTrigger(int trig, bool offOn) {
    triggers[trig] = offOn;
    if (offOn) cycleTriggers[trig] = ON;
}

static inline bool TestTrigger(int trig) {
    return triggers[trig];
}

static inline bool TestCycleTrigger(int trig) {
    return cycleTriggers[trig];
}

static bool TestBranch(int bc) {

    switch (bc) {

        case PROGRAM_SWITCH_1_INDICATOR:
            if (TestSwitch(PROGRAM_1_SWITCH)) return TRUE;
            break;

        case PROGRAM_SWITCH_2_INDICATOR:
            if (TestSwitch(PROGRAM_2_SWITCH)) return TRUE;
            break;

        case PROGRAM_SWITCH_3_INDICATOR:
            if (TestSwitch(PROGRAM_3_SWITCH)) return TRUE;
            break;

        case PROGRAM_SWITCH_4_INDICATOR:
            if (TestSwitch(PROGRAM_4_SWITCH)) return TRUE;
            break;

        case READ_CHECK_INDICATOR:
            if (TestTrigger(RD_CHK_TRIGGER)) {
                SetTrigger(RD_CHK_TRIGGER, OFF);
                return TRUE;
            }
            break;

        case WRITE_CHECK_INDICATOR:
            if (TestTrigger(WR_CHK_TRIGGER)) {
                SetTrigger(WR_CHK_TRIGGER, OFF);
                return TRUE;
            }
            break;

        case MAR_CHECK_INDICATOR:
            if (TestTrigger(MAR_CHK_TRIGGER)) return TRUE;
            break;

        case LAST_CARD_INDICATOR:
            if (TestTrigger(LAST_CARD_TRIGGER)) {
                SetTrigger(LAST_CARD_TRIGGER, OFF);
                SetLight(LAST_CARD_LIGHT, OFF);
                return TRUE;
            }
            break;

        case HIGH_POSITIVE_INDICATOR:
            if (TestTrigger(HP_TRIGGER)) return TRUE;
            break;

        case EQUAL_ZERO_INDICATOR:
            if (TestTrigger(EZ_TRIGGER)) return TRUE;
            break;

        case HP_OR_EQ_INDICATOR:
            if (TestTrigger(HP_TRIGGER) || TestTrigger(EZ_TRIGGER)) return TRUE;
            break;

        case ARITHMETIC_OVERFLOW_INDICATOR:
            if (TestTrigger(ARITH_CHK_TRIGGER)) {
                SetTrigger(ARITH_CHK_TRIGGER, OFF);
                return TRUE;
            }
            break;

        case EXPONENT_INDICATOR:
            if (TestTrigger(EXP_CHK_TRIGGER)) {
                SetTrigger(EXP_CHK_TRIGGER, OFF);
                return TRUE;
            }
            break;

        case MBR_EVEN_CHECK_INDICATOR:
            // Memory parity errors are not simulated
            break;

        case MBR_ODD_CHECK_INDICATOR:
            // Memory parity errors are not simulated
            break;

        case ANY_DATA_INDICATOR:
            if (TestTrigger(RD_CHK_TRIGGER)  || TestTrigger(WR_CHK_TRIGGER)) return TRUE;
            break;

        default:
            (void)printf("Undefined branch id: %d\n", bc);
            runState = CHECK_STOP_STATE;
    }

    return FALSE;
}

static inline void ClearRegisters(void) {
    ClearMAR();
    ClearMARs();
    ClearMBR();
    ClearMDR();
    ClearOpR();
    ClearDR();
    ClearMQR();
    ClearSABR();
}

static inline void ClearMAR(void) {
    (void)memcpy((void *)&mar, (void *)&initMAR, sizeof(struct maRegister));
}

static inline void CopyMAR(int dst, int src) {
    (void)memcpy((void *)&mars[dst], (void *)&mars[src], sizeof(struct maRegister));
}

static inline void SetMAR(int marsId, int value) {
    if (marsId == MAR_MAR) {
        SetDigit(&mar.digit10000, value / 10000 % 10);
        SetDigit(&mar.digit01000, value / 1000 % 10);
        SetDigit(&mar.digit00100, value / 100 % 10);
        SetDigit(&mar.digit00010, value / 10 % 10);
        SetDigit(&mar.digit00001, value % 10);
        mar.value = value;
    } else {
        SetDigit(&mars[marsId].digit10000, value / 10000 % 10);
        SetDigit(&mars[marsId].digit01000, value / 1000 % 10);
        SetDigit(&mars[marsId].digit00100, value / 100 % 10);
        SetDigit(&mars[marsId].digit00010, value / 10 % 10);
        SetDigit(&mars[marsId].digit00001, value % 10);
        mars[marsId].value = value;
    }
}

static inline void LoadMAR(int marsId) {
    if (marsId != MAR_MAR) {
        (void)memcpy((void *)&mar, (void *)&mars[marsId], sizeof(struct maRegister));
    }
}

static inline void IncrementMAR(int marsId, int inc) {
    int value;

    if (inc == 1) {
        SetTrigger(INCR_TRIGGER, ON);
        SetTrigger(DECR_TRIGGER, OFF);
        SetTrigger(PLUS_2_TRIGGER, OFF);
        SetTrigger(BYPASS_TRIGGER, OFF);
    } else if (inc == 2) {
        SetTrigger(INCR_TRIGGER, ON);
        SetTrigger(DECR_TRIGGER, OFF);
        SetTrigger(PLUS_2_TRIGGER, ON);
        SetTrigger(BYPASS_TRIGGER, OFF);
    } else if (inc < 0) {
        SetTrigger(INCR_TRIGGER, OFF);
        SetTrigger(DECR_TRIGGER, ON);
        SetTrigger(PLUS_2_TRIGGER, OFF);
        SetTrigger(BYPASS_TRIGGER, OFF);
    }

    if (marsId == MAR_MAR) {
        if (mar.value >= 0) {
            value = mar.value + inc;
            if (value < 0) value += MEMORY_SIZE;
            else if (value >= MEMORY_SIZE) value -= MEMORY_SIZE;
            SetMAR(marsId, value);
        } else {
            runState = CHECK_STOP_STATE;
        }
    } else {
        if (mars[marsId].value >= 0) {
            value = mars[marsId].value + inc;
            if (value < 0) value += MEMORY_SIZE;
            else if (value >= MEMORY_SIZE) value -= MEMORY_SIZE;
            SetMAR(marsId, value);
        } else {
            runState = CHECK_STOP_STATE;
        }
    }
}

static inline void SaveMAR(int marsId) {
    if (marsId != MAR_MAR) {
        (void)memcpy((void *)&mars[marsId], (void *)&mar, sizeof(struct maRegister));
        SetTrigger(BYPASS_TRIGGER, ON);
    }
}

static inline void ClearMARs(void) {
    (void)memcpy((void *)&mars, (void *)&initMARs, NUM_MARS * sizeof(struct maRegister));
}

static inline void ClearSingleMAR(int marsId) {
    if (marsId == MAR_MAR) {
        (void)memcpy((void *)&mar, (void *)&initMAR, sizeof(struct maRegister));
    } else {
        (void)memcpy((void *)&mars[marsId], (void *)&initMAR, sizeof(struct maRegister));
    }
}

static inline void ClearMBR(void) {
    (void)memcpy((void *)&mbr, (void *)&initMBR, sizeof(struct mbRegister));
}

static inline void ClearMDR(void) {
    (void)memcpy((void *)&mdr, (void *)&initMDR, sizeof(struct mdRegister));
}

static inline void ClearDR(void) {
    (void)memcpy((void *)&dr, (void *)&initDR, sizeof(struct dRegister));
}

static inline void ClearOpR(void) {
    (void)memcpy((void *)&opr, (void *)&initOpR, sizeof(struct opRegister));
}

static inline void ClearMQR(void) {
    (void)memcpy((void *)&mqr, (void *)&initMQR, sizeof(struct mqRegister));
}

static inline void ClearSABR(void) {
    (void)memcpy((void *)&sabr, (void *)&initSABR, sizeof(struct sabRegister));
}

static inline void SetDigit(volatile struct digit *d, int value) {
    d->check = parity[value & FLAG_DIGIT_MASK];
    d->flag = (value & FLAG_MASK) >> 4;
    d->bit8 = (value & 0x8) >> 3;
    d->bit4 = (value & 0x4) >> 2;
    d->bit2 = (value & 0x2) >> 1;
    d->bit1 = (value & 0x1);
    d->value = value & DIGIT_MASK;
}

static inline void CopyDigit(volatile struct digit *dst, volatile struct digit *src) {
    (void)memcpy((void *)dst, (void *)src, sizeof(struct digit));
}

static inline void SetMBR(int valueEven, int valueOdd) {
    SetDigit(&mbr.digitEven, valueEven);
    SetDigit(&mbr.digitOdd, valueOdd);
    if ((mbr.digitEven.value <= 9) && (mbr.digitOdd.value <= 9)) {
        mbr.value = 10 * mbr.digitEven.value + mbr.digitOdd.value;
    } else {
        mbr.value = -1;
    }
}

static inline void SetMDR(int value) {
    SetDigit(&mdr.digit, value);
    if (mdr.digit.value <= 9) {
        mdr.value = mdr.digit.value;
    } else {
        mdr.value = -1;
    }
}

static inline void SetDR(int value10, int value01) {
    SetDigit(&dr.digit10, value10 & DIGIT_MASK);
    SetDigit(&dr.digit01, value01 & DIGIT_MASK);
    if ((dr.digit10.value <= 9) && (dr.digit01.value <= 9)) {
        dr.value = 10 * dr.digit10.value + dr.digit01.value;
    } else {
        dr.value = -1;
    }
}

static inline void SetOpR(int value10, int value01) {
    SetDigit(&opr.digit10, value10 & DIGIT_MASK);
    SetDigit(&opr.digit01, value01 & DIGIT_MASK);
    if ((opr.digit10.value <= 9) && (opr.digit01.value <= 9)) {
        opr.value = 10 * opr.digit10.value + opr.digit01.value;
    } else {
        opr.value = -1;
    }
}

static inline void SetMQR(int value) {
    SetDigit(&mqr.digit, value & DIGIT_MASK);
    if (mqr.digit.value <= 9) {
        mqr.value = mqr.digit.value;
    } else {
        mqr.value = -1;
    }
}

static inline void SetSABR(int value10, int value01) {
    SetDigit(&sabr.digit10, value10 & DIGIT_MASK);
    SetDigit(&sabr.digit01, value01 & DIGIT_MASK);
    if ((sabr.digit10.value <= 9) && (sabr.digit01.value <= 9)) {
        sabr.value = 10 * sabr.digit10.value + sabr.digit01.value;
    } else {
        sabr.value = -1;
    }
}

static inline void SetMDRFlag(bool flag) {
    mdr.digit.check = parity[mdr.digit.value | (flag << 4)];
    mdr.digit.flag = flag;
}

static inline void TimeLog(char *msg) {
    if (timestampOpt) {
        time_t t = time(NULL);
        (void)printf("==> %s @ %s", msg, ctime(&t));
    }
}

static void PrintLegend(void) {  // debug
    (void)printf("   c c t h e o 1 f f r r             m      sa\n");
    (void)printf("cy i o c p z v c 1 2 c e op   mar mb d dr * br   or1   or2   or3   or4   or5   pr1   pr2   pr3   "
                 "cr1   ir1   ir2\n");
}

static void PrintRegs(void) {  // debug
    (void)printf("%2d %1d %1d %1d %1d %1d %1d %1d %1d %1d %1d %1d %02d %05d %02d %1d %02d %1d %02d %05d %05d %05d "
                 "%05d %05d %05d %05d %05d %05d %05d %05d\n",
                 cycle,
                 TestTrigger(CARRY_IN_TRIGGER), TestTrigger(CARRY_OUT_TRIGGER), TestTrigger(TC_1_TRIGGER),
                 TestTrigger(HP_TRIGGER), TestTrigger(EZ_TRIGGER), TestTrigger(ARITH_CHK_TRIGGER),
                 TestTrigger(FIRST_CYCLE_TRIGGER), TestTrigger(FIELD_MK_1_TRIGGER), TestTrigger(FIELD_MK_2_TRIGGER),
                 TestTrigger(REC_CTRL_TRIGGER), TestTrigger(RE_COMP_TRIGGER),
                 opr.value, mar.value, mbr.value, mdr.value, dr.value, mqr.value, sabr.value,
                 mars[OR1_MAR].value, mars[OR2_MAR].value, mars[OR3_MAR].value, mars[OR4_MAR].value,
                 mars[OR5_MAR].value, mars[PR1_MAR].value, mars[PR2_MAR].value, mars[PR3_MAR].value,
                 mars[CR1_MAR].value, mars[IR1_MAR].value, mars[IR2_MAR].value);
}
