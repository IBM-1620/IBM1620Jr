//======================================================================================================================
//
//  typewriter.c - typewriter simulation
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

#define _GNU_SOURCE 1
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

// Data
static int typeFile = -1;
static char *typeFilename = (char*)"/dev/ttyACM0";
static volatile bool expectACK = FALSE;
static pthread_mutex_t typeLock;


// Initialize console typewriter
void InitializeTypewriter(void) {

    // Open log file
    if (logOpt) {
        logFile = fopen(logFilename, "w");
        if (logFile != NULL) {
            (void)printf("Opened console typewriter log file \"%s\"\n", logFilename);
        } else {
            (void)printf("Failed to open log file: %s\n", strerror(errno));
            logOpt = FALSE;
        }
    }

    // Initialize typewriter mutex
    (void)pthread_mutex_init(&typeLock, NULL);

    // Open console typewriter
    (void)OpenTypewriter();
}

// Finalize console typewriter
void FinalizeTypewriter(void) {

    // Close console typewriter
    CloseTypewriter();

    // Destroy typewriter mutex
    (void)pthread_mutex_destroy(&typeLock);

    // Close log file
    if (logOpt) (void)fclose(logFile);
}

// Open console typewriter
bool OpenTypewriter(void) {
    int file;
    struct termios tio;

    // Lock typewriter open & close
    (void)pthread_mutex_lock(&typeLock);

    // Test if typewriter already open
    if (typeFile >= 0) {
        (void)pthread_mutex_unlock(&typeLock);
        return TRUE;
    }

    // Attempt to open typewriter
    file = open(typeFilename, O_RDWR | O_NOCTTY);
    if (file < 0) {
        (void)pthread_mutex_unlock(&typeLock);
        (void)usleep(100000);
        return FALSE;
    }

    // Set typewriter port characteristics
    bzero(&tio, sizeof(tio));
    tio.c_iflag = IGNBRK | IGNPAR | IGNCR;
    tio.c_oflag = ONLRET;
    tio.c_cflag = CS8 | CREAD | CLOCAL | B115200;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    tio.c_ispeed = B115200;
    tio.c_ospeed = B115200;
    (void)tcflush(file, TCIFLUSH);
    (void)tcsetattr(file, TCSANOW, &tio);

    typeFile = file;

    // Unlock typewriter open & close
    (void)pthread_mutex_unlock(&typeLock);

    return TRUE;
}

// Close console typewriter
void CloseTypewriter(void) {

    // Lock typewriter open & close
    (void)pthread_mutex_lock(&typeLock);

    // Close typewriter
    if (typeFile >= 0) {
        (void)close(typeFile);
        typeFile = -1;
    }

    // Unlock typewriter open & close
    (void)pthread_mutex_unlock(&typeLock);
}

// Receive input character from console typewriter
char ReceiveTYChar(void) {
    char achar;
    int offset;

    // Read character from console typewriter
    while (simulationRunning) {
        while (TRUE) {
            if (!simulationRunning) return 0;
            if ((typeFile < 0) && !OpenTypewriter()) continue;
            if (read(typeFile, &achar, 1) <= 0) {
                CloseTypewriter();
                continue;
            }
            if ((achar < 32) || (achar > 127)) continue;
            break;
        }

        offset = ((int)achar & 0x7f) - 32;

        // Echo and/or log character
        if (commandTYFlags[offset]) {
            if (protocolOpt) {
                (void)printf("{%s}", printTYStrings[offset]);
                if (logOpt) (void)fprintf(logFile, "{%s}", printTYStrings[offset]);
            }
        } else {
            if (echoOpt) (void)printf("%s", printTYStrings[offset]);
            if (logOpt) (void)fprintf(logFile, "%s", printTYStrings[offset]);
        }

        // Process command and special characters
        switch (achar & 0x7f) {

            case TY_COMMAND_MODE_0:
            case TY_COMMAND_MODE_1:
            case TY_COMMAND_MODE_2:
            case TY_COMMAND_MODE_3:
            case TY_COMMAND_SLASH:
            case TY_COMMAND_UNSLASH:
            case TY_COMMAND_SETUP:
            case TY_COMMAND_RESET:
                break;

            case TY_COMMAND_ACK:
                expectACK = FALSE;
                break;

            case TY_COMMAND_PING:
                SendTYChar(TY_COMMAND_ACK);
                break;

            case PROTOCOL_TAB:
            case PROTOCOL_RETURN:
            case PROTOCOL_BACKSPACE:
            case PROTOCOL_LEFT_ARROW:
            case PROTOCOL_RIGHT_ARROW:
                UpdateTYPosition(achar);
                break;

            case PROTOCOL_LEFT_MARGIN:
                leftMarginPosition = currentPosition;
                break;

            case PROTOCOL_RIGHT_MARGIN:
                rightMarginPosition = currentPosition;
                break;

            case PROTOCOL_CLEAR_TAB:
                tabs[currentPosition] = FALSE;
                break;

            case PROTOCOL_CLEAR_ALL_TABS:
                (void)memset((void *)tabs, FALSE, sizeof(tabs));
                break;

            case PROTOCOL_SET_TAB:
                tabs[currentPosition] = TRUE;
                break;

            case PROTOCOL_PAUSE:
                // TBD - implement pause
                break;

            case PROTOCOL_RESUME:
                // TBD - implement resume
                break;

            default:
                UpdateTYPosition(achar);
                return achar;
        }
    }

    return 0;
}

// Send output character to console typewriter
void SendTYChar(char achar) {
    int offset;

    // Write character to console typewriter
    offset = ((int)achar & 0x7f) - 32;
    if (offset < 0) return;
    while (TRUE) {
        if (!simulationRunning) return;
        if ((typeFile < 0) && !OpenTypewriter()) continue;
        if (write(typeFile, &achar, 1) <= 0) {
            CloseTypewriter();
            continue;
        }
        break;
    }

    // Update typewriter position
    UpdateTYPosition(achar);

    // Echo and/or log character
    if (commandTYFlags[offset]) {
        if (protocolOpt) {
            (void)printf("[%s]", printTYStrings[offset]);
            if (logOpt) (void)fprintf(logFile, "[%s]", printTYStrings[offset]);
        }
    } else {
        if (echoOpt) (void)printf("%s", printTYStrings[offset]);
        if (logOpt) (void)fprintf(logFile, "%s", printTYStrings[offset]);
    }

    // Process command characters
    if ((achar == TY_COMMAND_MODE_0)  || (achar == TY_COMMAND_MODE_1) || (achar == TY_COMMAND_MODE_2) ||
        (achar == TY_COMMAND_MODE_3)  || (achar == TY_COMMAND_PING)   || (achar == TY_COMMAND_SLASH)  ||
        (achar == TY_COMMAND_UNSLASH) || (achar == TY_COMMAND_SETUP)  || (achar == TY_COMMAND_RESET)) expectACK = TRUE;
}

// Update console typewriter line position due to input/output character
void UpdateTYPosition(char achar){
    int offset;

    // Determine position affect of input/output character
    offset = ((int)achar & 0x7f) - 32;
    if (offset < 0) return;

    // Update typewriter line position
    switch (charPosition[offset]) {

        case POSITION_NONE:
            break;

        case POSITION_LEFT:
            if (currentPosition > 1) --currentPosition;
            break;

        case POSITION_RIGHT:
            ++currentPosition;
            break;

        case POSITION_TAB:
            for (int i = currentPosition; i <= rightMarginPosition; ++i) {
                currentPosition = i;
                if (tabs[i]) break;
            }
            break;

        case POSITION_RETURN:
            currentPosition = leftMarginPosition;
            break;

        default:
            break;
    };

    if (currentPosition > rightMarginPosition) {
        WriteTYChar(PROTOCOL_RETURN);
    }
}

// Parse string and mark tabs
void ParseTabs(char *str) {
    char achar;
    int tab = 0;
    char *ptr = str;

    do {
        achar = *ptr++;
        if ((achar >= '0') && (achar <='9')) {
            tab = 10 * tab + (achar - '0');
        } else {
            if ((tab >= 1) && (tab <= NUM_TYPEWRITER_POS)) {
                tabs[tab] = TRUE;
            }
            tab = 0;
        }
    } while (achar != 0);
}

// Set typewriter tabs if needed
void SetTabs(void) {
    int last;

    for (last = rightMarginPosition; last >= 0; --last) {
        if (tabs[last]) break;
    }
    if (last == 0) return;

    WriteTYCommand(PROTOCOL_RETURN);
    WriteTYCommand(PROTOCOL_CLEAR_ALL_TABS);
    for (int i = 1; i <= last; ++i) {
        if (tabs[i]) WriteTYCommand(PROTOCOL_SET_TAB);
        if (i < last) WriteTYCommand(PROTOCOL_SPACE);
    }
    WriteTYCommand(PROTOCOL_RETURN);
}

