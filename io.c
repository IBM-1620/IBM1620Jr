//======================================================================================================================
//
//  io.c - io threads
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
#include "punchedcard.h"

#define _GNU_SOURCE 1
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

// Function declarations
void *CTIn(void *arg);
void *CTOut(void *arg);

void *PCIn(void *arg);
void *PCOut(void *arg);

static inline void SetIOWait(int wait);

// Data
bool ctStarted = FALSE;
pthread_t ctInThread;
pthread_t ctOutThread;
int ctInPipe[2];
int ctOutPipe[2];

bool pcStarted = FALSE;
pthread_t pcInThread;
pthread_t pcOutThread;
int pcInPipe[2];
int pcOutPipe[2];

char lastPowerStatus = 0;
char lastRunStatus = 0;


// StartIO
void StartIO(void) {
    int rc;

    // Create typewriter I/O pipes
    rc = pipe(ctInPipe);
    if (rc < 0) {
        (void)printf("Failed to create console typewriter input pipe: %s\n", strerror(rc));
        exit(-1);
    }

    rc = pipe(ctOutPipe);
    if (rc < 0) {
        (void)printf("Failed to create console typewriter output pipe: %s\n", strerror(rc));
        exit(-1);
    }

    // Create typewriter I/O threads
    rc = pthread_create(&ctInThread, NULL, CTIn, NULL);
    if (rc != 0) {
        (void)printf("Failed to create console typewriter input thread: %s\n", strerror(rc));
        exit(-1);
    }

    rc = pthread_create(&ctOutThread, NULL, CTOut, NULL);
    if (rc != 0) {
        (void)printf("Failed to create console typewriter output thread: %s\n", strerror(rc));
        exit(-1);
    }

    // Reset console typewriter
    WriteTYCommand(TY_COMMAND_RESET);

    // Create punched card I/O pipes
    rc = pipe(pcInPipe);
    if (rc < 0) {
        (void)printf("Failed to create punched card input pipe: %s\n", strerror(rc));
        exit(-1);
    }

    rc = pipe(pcOutPipe);
    if (rc < 0) {
        (void)printf("Failed to create punched card output pipe: %s\n", strerror(rc));
        exit(-1);
    }

    // Create punched card I/O threads
    rc = pthread_create(&pcInThread, NULL, PCIn, NULL);
    if (rc != 0) {
        (void)printf("Failed to create punched card input thread: %s\n", strerror(rc));
        exit(-1);
    }

    rc = pthread_create(&pcOutThread, NULL, PCOut, NULL);
    if (rc != 0) {
        (void)printf("Failed to create punched card output thread: %s\n", strerror(rc));
        exit(-1);
    }

    // Reset punched card reader/punch
    WriteCDCommand(CD_COMMAND_RESET);

    // Request current punched card reader/punch status
    WriteCDCommand(CD_COMMAND_REQUEST_STATUS);
}

// StopIO
void StopIO(void) {

    CloseTypewriter();
    ClosePunchedCard();

    (void)close(ctInPipe[0]);
    (void)close(ctInPipe[1]);
    (void)close(ctOutPipe[0]);
    (void)close(ctOutPipe[1]);

    (void)close(pcInPipe[0]);
    (void)close(pcInPipe[1]);
    (void)close(pcOutPipe[0]);
    (void)close(pcOutPipe[1]);

    (void)pthread_kill(ctInThread, SIGUSR1);
    (void)pthread_kill(ctOutThread, SIGUSR1);

    (void)pthread_kill(pcInThread, SIGUSR1);
    (void)pthread_kill(pcOutThread, SIGUSR1);

    (void)pthread_join(ctInThread, NULL);
    (void)pthread_join(ctOutThread, NULL);

    (void)pthread_join(pcInThread, NULL);
    (void)pthread_join(pcOutThread, NULL);
}

// ShutdownIO
void ShutdownIO(void) {
    SendTYChar(TY_COMMAND_RESET);
    SendPCChar(CD_COMMAND_SHUTDOWN);
}

// BeginIOInstruction
void BeginIOInstruction(int inst) {

    ioState = RUNNING_IO_STATE;
    ioType = inst;
    ioWait = 0;
    ioCounter = 0;

    switch (inst) {

        case NO_IO:
            break;

        case SPTY_IO:
            WriteTYChar(PROTOCOL_SPACE);
            break;

        case RCTY_IO:
            WriteTYChar(PROTOCOL_RETURN);
            break;

        case TBTY_IO:
            WriteTYChar(PROTOCOL_TAB);
            break;

        case DNTY_IO:
            WriteTYCommand(TY_COMMAND_MODE_3);
            break;

        case INSERT_IO:
        case RNTY_IO:
            WriteTYCommand(TY_COMMAND_MODE_1);
            break;

        case RATY_IO:
            WriteTYCommand(TY_COMMAND_MODE_2);
            break;

        case WNTY_IO:
            WriteTYCommand(TY_COMMAND_MODE_3);
            break;

        case WATY_IO:
            WriteTYCommand(TY_COMMAND_MODE_3);
            break;

        case LOAD_IO:
        case RNCD_IO:
            readerBusy = TRUE;
            WriteCDCommand(CD_COMMAND_READ_NUMERIC);
            break;

        case RACD_IO:
            readerBusy = TRUE;
            WriteCDCommand(CD_COMMAND_READ_ALPHAMERIC);
            break;

        case DNCD_IO:
        case WNCD_IO:
        case WACD_IO:
            break;

        default:
            (void)printf("Invalid I/O instruction: %d\n", inst);
    }
}

// EndIOInstruction
void EndIOInstruction(void) {

    switch (ioType) {

        case NO_IO:
        case SPTY_IO:
        case RCTY_IO:
        case TBTY_IO:
            break;

        case INSERT_IO:
        case DNTY_IO:
        case RNTY_IO:
        case RATY_IO:
        case WNTY_IO:
        case WATY_IO:
            WriteTYCommand(TY_COMMAND_MODE_0);
            break;

        case LOAD_IO:
        case RNCD_IO:
        case RACD_IO:
            break;

        case DNCD_IO:
        case WNCD_IO:
            punchBusy = TRUE;
            WriteCDCommand(CD_COMMAND_WRITE_NUMERIC);
            break;

        case WACD_IO:
            punchBusy = TRUE;
            WriteCDCommand(CD_COMMAND_WRITE_ALPHAMERIC);
            break;

        default:
            (void)printf("Invalid I/O instruction: %d\n", ioType);
    }

    ioState = STOPPED_STATE;
    ioType = NO_IO;
}

// Character available to read
bool ReadTYAvail(void) {
    int nchar = 0;
    (void)ioctl(ctInPipe[0], FIONREAD, &nchar);
    return (nchar > 0);
}

// Read input character
char ReadTYChar(void) {
    char achar = 0;
    (void)read(ctInPipe[0], &achar, 1);
    SetIOWait(0);
    return achar;
}

// Write output command
void WriteTYCommand(char achar) {
    (void)write(ctOutPipe[1], &achar, 1);
}

// Write output character
void WriteTYChar(char achar) {
    (void)write(ctOutPipe[1], &achar, 1);
    int wait = charWait[((int)achar & 0x7f) - 32];
    if (achar == PROTOCOL_RETURN) {
        wait += (TIMING_CARRIAGE_RETURN * currentPosition) / 80;
    }
    SetIOWait(wait);
}

// Console typewriter input thread
void *CTIn(void *arg) {
    pthread_t threadId = pthread_self();

    // Lock CTIn thread on CPU 3
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(3, &cpus);
    if (pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpus) != 0) {
        (void)printf("CTIn CPU affinity failed: %s\n", strerror(errno));
        exit(-1);
    }
    // Set scheduler type and priority
    struct sched_param sparam = { 1 };
    if (pthread_setschedparam(threadId, SCHED_FIFO, &sparam) != 0) {
        (void)printf("CTIn SCHED_FIFO scheduler failed: %s\n", strerror(errno));
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

    // Open typewriter connection
    InitializeTypewriter();
    ctStarted = TRUE;

    // Receive typewriter input
    while (simulationRunning) {
        char achar = ReceiveTYChar();
        if (achar == 0) break;
        (void)write(ctInPipe[1], &achar, 1);
    }

    // Close typewriter connection
    ctStarted = FALSE;
    FinalizeTypewriter();

    // End CTIn thread
    pthread_exit(NULL);
}

// Console typewriter output thread
void *CTOut(void *arg) {
    pthread_t threadId = pthread_self();

    // Lock CTOut thread on CPU 3
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(3, &cpus);
    if (pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpus) != 0) {
        (void)printf("CTOut CPU affinity failed: %s\n", strerror(errno));
        exit(-1);
    }
    // Set scheduler type and priority
    struct sched_param sparam = { 2 };
    if (pthread_setschedparam(threadId, SCHED_FIFO, &sparam) != 0) {
        (void)printf("CTOut SCHED_FIFO scheduler failed: %s\n", strerror(errno));
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

    // Wait until console typewriter is started
    while (!ctStarted) {
        (void)usleep(100);
    }

    // Send typewriter output
    while (simulationRunning) {
        char achar = 0;
        (void)read(ctOutPipe[0], &achar, 1);
        if (achar == 0) continue;
        SendTYChar(achar);
    }

    // End CTOut thread
    pthread_exit(NULL);
}

// Character avaiable to read
bool ReadCDAvail(void) {
    int nchar = 0;
    (void)ioctl(pcInPipe[0], FIONREAD, &nchar);
    return (nchar > 0);
}

// Read input character
char ReadCDChar(void) {
    char achar = 0;
    (void)read(pcInPipe[0], &achar, 1);
    SetIOWait(CD_IO_CYCLE_WAIT);
    return achar;
}

// Write output command
void WriteCDCommand(char achar) {
    (void)write(pcOutPipe[1], &achar, 1);
}

// Write output character
void WriteCDChar(char achar) {
    (void)write(pcOutPipe[1], &achar, 1);
    SetIOWait(CD_IO_CYCLE_WAIT);
}

// Write 1620 power status when changed
void WriteCDPowerStatus(char achar) {
    if (achar != lastPowerStatus) {
        lastPowerStatus = achar;
        (void)write(pcOutPipe[1], &achar, 1);
    }
}

// Write 1620 run status when changed
void WriteCDRunStatus(char achar) {
    if (achar != lastRunStatus) {
        lastRunStatus = achar;
        (void)write(pcOutPipe[1], &achar, 1);
    }
}

// Punched card input thread
void *PCIn(void *arg) {
    pthread_t threadId = pthread_self();

    // Lock PCIn thread on CPU 3
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(3, &cpus);
    if (pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpus) != 0) {
        (void)printf("PCIn CPU affinity failed: %s\n", strerror(errno));
        exit(-1);
    }
    // Set scheduler type and priority
    struct sched_param sparam = { 1 };
    if (pthread_setschedparam(threadId, SCHED_FIFO, &sparam) != 0) {
        (void)printf("PCIn SCHED_FIFO scheduler failed: %s\n", strerror(errno));
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

    // Open punched card connection
    InitializePunchedCard();
    pcStarted = TRUE;

    // Receive punched card input
    while (simulationRunning) {
        char achar = ReceivePCChar();
        if (achar == 0) break;
        (void)write(pcInPipe[1], &achar, 1);
    }

    // Close punched card connection
    pcStarted = FALSE;
    FinalizePunchedCard();

    // End PCIn thread
    pthread_exit(NULL);
}

// Punched card output thread
void *PCOut(void *arg) {
    pthread_t threadId = pthread_self();

    // Lock PCOut thread on CPU 3
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(3, &cpus);
    if (pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpus) != 0) {
        (void)printf("PCOut CPU affinity failed: %s\n", strerror(errno));
        exit(-1);
    }
    // Set scheduler type and priority
    struct sched_param sparam = { 2 };
    if (pthread_setschedparam(threadId, SCHED_FIFO, &sparam) != 0) {
        (void)printf("PCOut SCHED_FIFO scheduler failed: %s\n", strerror(errno));
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

    // Wait until punched card is started
    while (!pcStarted) {
        (void)usleep(100);
    }

    // Send punched card output
    while (simulationRunning) {
        char achar = 0;
        (void)read(pcOutPipe[0], &achar, 1);
        if (achar == 0) continue;
        SendPCChar(achar);
    }

    // End PCOut thread
    pthread_exit(NULL);
}

static inline void SetIOWait(int wait) {
    ioWait = wait;
    triggers[IO_HOLD_TRIGGER] = ON;
    cycleTriggers[IO_HOLD_TRIGGER] = ON;
}
