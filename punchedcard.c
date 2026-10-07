//======================================================================================================================
//
//  punchedcard.c - punched card simulation
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
#include "punchedcard.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

// Data
static int cardFile = -1;
static char *cardFilename = (char*)"/dev/ttyUSB0";
static pthread_mutex_t cardLock;


// Initialize punched card
void InitializePunchedCard(void) {

    // Initialize punched card mutex
    (void)pthread_mutex_init(&cardLock, NULL);

    // Open punched card
    (void)OpenPunchedCard();
}

// Finalize punched card
void FinalizePunchedCard(void) {

    // Close punched card
    ClosePunchedCard();

    // Destroy punched card mutex
    (void)pthread_mutex_destroy(&cardLock);
}

// Open punched card reader/punch
bool OpenPunchedCard(void) {
    int file;
    struct termios tio;

    // Lock punched card open & close
    (void)pthread_mutex_lock(&cardLock);

    // Test if punched card already open
    if (cardFile >= 0) {
        (void)pthread_mutex_unlock(&cardLock);
        return TRUE;
    }

    // Attempt to open punched card
    file = open(cardFilename, O_RDWR | O_NOCTTY);
    if (file < 0) {
        (void)pthread_mutex_unlock(&cardLock);
        (void)usleep(100000);
        return FALSE;
    }

    // Set punched card port characteristics
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

    cardFile = file;

    // Unlock punched card open & close
    (void)pthread_mutex_unlock(&cardLock);

    return TRUE;
}

// Close punched card reader/punch
void ClosePunchedCard(void) {

    // Lock punched card open & close
    (void)pthread_mutex_lock(&cardLock);

    // Close punched card
    if (cardFile >= 0) {
        (void)close(cardFile);
        cardFile = -1;
    }

    // Unlock punched card open & close
    (void)pthread_mutex_unlock(&cardLock);
}

// Receive punched card character
char ReceivePCChar(void) {
    char achar;
    int offset;

    while (simulationRunning) {
        while (TRUE) {
            if (!simulationRunning) return 0;
            if ((cardFile < 0) && !OpenPunchedCard()) continue;
            if (read(cardFile, &achar, 1) <= 0) {
                ClosePunchedCard();
                continue;
            }
            if ((achar < 32) || (achar > 127)) continue;
            break;
        }

        offset = ((int)achar & 0x7f) - 32;

        // Echo and/or log character
        if (commandCDFlags[offset]) {
            if (protocolOpt) {
                (void)printf("{%s}", printCDStrings[offset]);
                if (logOpt) (void)fprintf(logFile, "{%s}", printCDStrings[offset]);
            }
        } else {
            if (echoOpt) (void)printf("%s", printCDStrings[offset]);
            if (logOpt) (void)fprintf(logFile, "%s", printCDStrings[offset]);
        }

        // Process incoming character
        switch (protocolCDActions[offset]) {

            case ACTION_IGNORE:
                break;

            case ACTION_ACCEPT:
                return achar;

            case ACTION_READER_READY:
                readerReady = TRUE;
                readerBusy = FALSE;
                readerCheck = FALSE;
                readerLastCard = FALSE;
                break;

            case ACTION_READER_NOT_READY:
                readerReady = FALSE;
                readerBusy = TRUE;
                break;

            case ACTION_READER_BUSY:
                readerBusy = TRUE;
                break;

            case ACTION_READER_NOT_BUSY:
                readerBusy = FALSE;
                break;

            case ACTION_READER_CHECK:
                readerReady = FALSE;
                readerBusy = TRUE;
                readerCheck = TRUE;
                break;

            case ACTION_READER_LAST_CARD:
                readerLastCard = TRUE;
                break;

            case ACTION_READER_LOAD:
                readerLoad = TRUE;
                break;

            case ACTION_PUNCH_READY:
                punchReady = TRUE;
                punchBusy = FALSE;
                punchCheck = FALSE;
                break;

            case ACTION_PUNCH_NOT_READY:
                punchReady = FALSE;
                punchBusy = TRUE;
                break;

            case ACTION_PUNCH_BUSY:
                punchBusy = TRUE;
                break;

            case ACTION_PUNCH_NOT_BUSY:
                punchBusy = FALSE;
                break;

            case ACTION_PUNCH_CHECK:
                punchReady = FALSE;
                punchBusy = TRUE;
                punchCheck = TRUE;
                break;

            case ACTION_REQUEST_STATUS:
                SendPCStatus();
                break;

            case ACTION_ERROR:
                (void)printf("Invalid protocol action: %d\n", achar & 0x7f);
                break;

            default:
                break;
        }
    }

    return 0;
}

// Send punched card character
void SendPCChar(char achar) {
    int offset;

    // Write character to punched card
    offset = ((int)achar & 0x7f) - 32;
    if (offset < 0) return;
    while (TRUE) {
        if (!simulationRunning) return;
        if ((cardFile < 0) && !OpenPunchedCard()) continue;
        if (write(cardFile, &achar, 1) <= 0) {
            ClosePunchedCard();
            continue;
        }
        break;
    }

    // Echo and/or log character
    if (commandCDFlags[offset]) {
        if (protocolOpt) {
            (void)printf("[%s]", printCDStrings[offset]);
            if (logOpt) (void)fprintf(logFile, "[%s]", printCDStrings[offset]);
        }
    } else {
        if (echoOpt) (void)printf("%s", printCDStrings[offset]);
        if (logOpt) (void)fprintf(logFile, "%s", printCDStrings[offset]);
    }
}

// Send 1620 status to punched card reader/punch
void SendPCStatus(void) {

    // Send MANUAL or NOT_MANUAL status
    if ((runState == STOPPED_STATE) && (cycle == 1)) {
        SendPCChar(CD_COMMAND_MANUAL);
    } else {
        SendPCChar(CD_COMMAND_NOT_MANUAL);
    }

    // Send POWER_ON or POWER_OFF status
    if ((powerState == POWER_ON) || (powerState == POWER_READY)) {
        SendPCChar(CD_COMMAND_POWER_ON);
    } else {
        SendPCChar(CD_COMMAND_POWER_OFF);
    }
}
