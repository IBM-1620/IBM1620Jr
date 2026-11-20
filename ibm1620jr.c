//======================================================================================================================
//
//  ibm1620jr.c - main program
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
//   Versions:
//      1.0     06/09/2025     Initial version.
//      1.1     11/17/2026     Added floating point instructions.
//      1.2     11/19/2026     Added timing options.
//
//======================================================================================================================

#include "defines.h"
#include "data.h"
#include "frontpanel.h"
#include "io.h"
#include "simulator.h"
#include "typewriter.h"
#include "utilities.h"

#define _GNU_SOURCE 1
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Function declarations
static void InterruptHandler(int sig);
static void ComputeVersionBuild(void);
static void ParseCommandLine(int argc, char *argv[], char *defaultCmemFilename, char *defaultLogFilename);

// Data


// Main program
int main(int argc, char *argv[])
{
    ComputeVersionBuild();
    (void)printf("\nIBM 1620 Jr. (version %d.%d.%d)\n\n", versionMajor, versionMinor, versionBuild);

    ParseCommandLine(argc, argv, defaultCmemFilename, defaultLogFilename);
    if (informationOpt) PrintInformation();
    setbuf(stdout, NULL);

    (void)signal(SIGABRT, InterruptHandler);
    (void)signal(SIGHUP,  InterruptHandler);
    (void)signal(SIGINT,  InterruptHandler);
    (void)signal(SIGQUIT, InterruptHandler);
    (void)signal(SIGTERM, InterruptHandler);
    (void)signal(SIGUSR1, InterruptHandler);
    (void)signal(SIGUSR2, InterruptHandler);

    simulationRunning = TRUE;
    StartIO();
    StartFrontPanel();
    LoadMemory(cmemCount, cmemFiles);
    SetTabs();
    StartSimulator();

    pause();

    if (powerState == POWER_SHUTDOWN) {
        ShutdownIO();
    }
    simulationRunning = FALSE;
    StopSimulator();
    SaveMemory(defaultCmemFilename);
    StopFrontPanel();
    StopIO();

    (void)printf("\nEnd of IBM 1620 Jr.\n\n");

    if (powerState != POWER_SHUTDOWN) {
        exit(0);
    } else {
        exit(162);
    }
}

static void InterruptHandler(int sig) {
    // Do nothing
}

static void ComputeVersionBuild(void) {
    int month = 0;

    if      ((versionDate[0] == 'J') && (versionDate[1] == 'a') && (versionDate[2] == 'n')) month = 1;
    else if ((versionDate[0] == 'F') && (versionDate[1] == 'e') && (versionDate[2] == 'b')) month = 2;
    else if ((versionDate[0] == 'M') && (versionDate[1] == 'a') && (versionDate[2] == 'r')) month = 3;
    else if ((versionDate[0] == 'A') && (versionDate[1] == 'p') && (versionDate[2] == 'r')) month = 4;
    else if ((versionDate[0] == 'M') && (versionDate[1] == 'a') && (versionDate[2] == 'y')) month = 5;
    else if ((versionDate[0] == 'J') && (versionDate[1] == 'u') && (versionDate[2] == 'n')) month = 6;
    else if ((versionDate[0] == 'J') && (versionDate[1] == 'u') && (versionDate[2] == 'l')) month = 7;
    else if ((versionDate[0] == 'A') && (versionDate[1] == 'u') && (versionDate[2] == 'g')) month = 8;
    else if ((versionDate[0] == 'S') && (versionDate[1] == 'e') && (versionDate[2] == 'p')) month = 9;
    else if ((versionDate[0] == 'O') && (versionDate[1] == 'c') && (versionDate[2] == 't')) month = 10;
    else if ((versionDate[0] == 'N') && (versionDate[1] == 'o') && (versionDate[2] == 'v')) month = 11;
    else if ((versionDate[0] == 'D') && (versionDate[1] == 'e') && (versionDate[2] == 'c')) month = 12;
    int day = 10 * (int)(((versionDate[4] == ' ') ? '0' : versionDate[4]) - '0') + (int)(versionDate[5] - '0');
    int year = 10 * (int)(versionDate[9] - '0') + (int)(versionDate[10] - '0');
    versionBuild = 10000 * year + 100 * month + day;
}

static void ParseCommandLine(int argc, char *argv[], char *defaultCmemFilename, char *defaultLogFilename) {
    int opt;
    static struct option cmdOptions[] =
        { { "alphamemory",       no_argument,       NULL, 'a' },
          { "displaymar",        no_argument,       NULL, 'd' },
          { "echo",              no_argument,       NULL, 'e' },
          { "fast",              no_argument,       NULL, 'f' },
          { "information",       no_argument,       NULL, 'i' },
          { "log",               optional_argument, NULL, 'l' },
          { "model",             required_argument, NULL, 'm' },
          { "nodelay",           no_argument,       NULL, 'n' },
          { "protocol",          no_argument,       NULL, 'p' },
          { "stops",             required_argument, NULL, 's' },
          { "timestamp",         no_argument,       NULL, 't' },
          { "zeromemory",        no_argument,       NULL, 'z' },
          { 0, 0, 0, 0 } };

    // Process options
    while (TRUE) {
        int optIndex = 0;

        opt = getopt_long(argc, argv, "adefil::m:nps:tz", cmdOptions, &optIndex);
        if (opt == EOF) break;

        switch (opt) {

            case 'a':
                alphaOpt = TRUE;
                break;

            case 'd':
                displayMAROpt = TRUE;
                break;

            case 'e':
                echoOpt = TRUE;
                break;

            case 'f':
                fastOpt = TRUE;
                break;

            case 'i':
                informationOpt = TRUE;
                break;

            case 'l':
                logOpt = TRUE;
                if (optarg != NULL) {
                    logFilename = optarg;
                } else {
                    logFilename = defaultLogFilename;
                }
                break;

            case 'm':
                modelOpt = TRUE;
                if (*optarg == '1') {
                    cpuModel = 1;
                    cycleTime = CYCLE_TIME_MODEL_1;
                    powerOnWait = POWER_ON_WAIT_MODEL_1;
                    tyIOCycleWait = TY_IO_CYCLE_WAIT_MODEL_1;
                    cdIOCycleWait = CD_IO_CYCLE_WAIT_MODEL_1;
                } else if (*optarg == '2') {
                    cpuModel = 2;
                    cycleTime = CYCLE_TIME_MODEL_2;
                    powerOnWait = POWER_ON_WAIT_MODEL_2;
                    tyIOCycleWait = TY_IO_CYCLE_WAIT_MODEL_2;
                    cdIOCycleWait = CD_IO_CYCLE_WAIT_MODEL_2;
                } else {
                    printf("Invalid model number: %s\n", optarg);
                    cpuModel = 1;
                    cycleTime = CYCLE_TIME_MODEL_1;
                    powerOnWait = POWER_ON_WAIT_MODEL_1;
                    tyIOCycleWait = TY_IO_CYCLE_WAIT_MODEL_1;
                    cdIOCycleWait = CD_IO_CYCLE_WAIT_MODEL_1;
                }
                break;

            case 'n':
                nodelayOpt = TRUE;
                break;

            case 'p':
                protocolOpt = TRUE;
                break;

            case 's':
                stopsOpt = TRUE;
                ParseTabs(optarg);
                break;

            case 't':
                timestampOpt = TRUE;
                break;

            case 'z':
                zeroOpt = TRUE;
                break;

            case '?':
                (void)printf("IBM_1620_Jr command line options:\n");
                (void)printf("    -a    --alphamemory          include alpha interpretation in saved core memory file\n");
                (void)printf("    -d    --displaymar           allow display mar in any stopped state\n");
                (void)printf("    -e    --echo                 echo console typewriter input & output\n");
                (void)printf("    -f    --fast                 cycles run as fast as possible\n");
                (void)printf("    -i    --information          print simulator information\n");
                (void)printf("    -l    --log=filename         log console typewriter & card reader/punch input & output\n");
                (void)printf("    -m    --model=number         ibm 1620 model number\n");
                (void)printf("    -n    --nodelay              no delay in power on\n");
                (void)printf("    -p    --protocol             trace console typewriter & card reader/punch protocols\n");
                (void)printf("    -s    --stops=n,n,..         set console typewriter tab stops\n");
                (void)printf("    -t    --timestamp            print a timestamp for all run state changes\n");
                (void)printf("    -z    --zeromemory           start with zeroed memory\n");
                (void)printf("    filename(s)                  core memory file(s) to preload\n");
                break;

            default:
                break;
        }
    }

    // Process operands
    while (optind < argc) {
        cmemFiles[cmemCount++] = argv[optind++];
    }

    // Use default memory file if none are given
    if (!zeroOpt && (cmemCount == 0)) {
        cmemCount = 1;
        cmemFiles[0] = defaultCmemFilename;
    }
}
