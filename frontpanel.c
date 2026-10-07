//======================================================================================================================
//
//  frontpanel_rpi.c - front panel thread
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
#include "utilities.h"
#include "frontpanel.h"

#define _GNU_SOURCE 1
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

// Function declarations
void *FrontPanel(void *arg);
static void UpdateFrontPanel(void);
static int InitRegisters(int i2cFile, int addr);
static int FrontPanelIO(int i2cFile, struct lights *displayLights);
static int FrontPanelGPIO(int i2cFile, struct lights *displayLights);
static int SetPWMs(int i2cFile, int addr, char *pwms);
static int SetGetGPIOs(int i2cFile, int addr, char *out, char *in);
static int SetGPIOs(int i2cFile, int addr, char *out);

// Data
static int lastUpperSnakeLight1 = 0;
static int lastUpperSnakeLight2 = 0;
static int lastLowerSnakeLight = 0;
static int lastStatusSnakeLight = 0;
static int upperSnakeRow1 = 0;
static int upperSnakeColumn1 = 0;
static int upperSnakeRow2 = 0;
static int upperSnakeColumn2 = 0;
static int lowerSnakeIndex = 0;
static int lowerSnakeIncrement = 0;
static int statusSnakeIndex = 0;
static int statusSnakeIncrement = 0;
static int snakeDelayCounter = 0;
static struct lights *displayLights = NULL;
static const char *lightMap = NULL;
static int riseDelta = RISE_DELTA;
static int fallDelta = FALL_DELTA;
static struct timespec sleepTime = { 0, 0L };
static struct timespec scrDelayTime = { 0, SCR_DISPLAY_DELAY };
static int scrCounter = 0;
static bool scrDelay = FALSE;
static bool punchNoFeedScr = FALSE;
static bool readerNoFeedScr = FALSE;
static bool saveScr = FALSE;
static bool insertScr = FALSE;
static bool automaticScr = FALSE;
static bool manualScr = FALSE;
static bool checkStopScr = FALSE;

// Data
pthread_t frontPanelThread;


// Start front panel
void StartFrontPanel(void) {
    int rc = pthread_create(&frontPanelThread, NULL, FrontPanel, NULL);
    if (rc != 0) {
        (void)printf("Failed to create front panel thread: %s\n", strerror(rc));
        exit(-1);
    }
}

// Stop front panel
void StopFrontPanel(void) {
    (void)pthread_join(frontPanelThread, NULL);
}

// Front panel thread
void *FrontPanel(void *arg) {
    int i2cFile = 0;
    char *i2cFilename = (char*)"/dev/i2c-1";
    long long syncTime;
    struct timespec currentTime;
    pthread_t threadId = pthread_self();

    // Lock front panel thread on CPU 2
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(2, &cpus);
    if (pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpus) != 0) {
        (void)printf("FrontPanel CPU affinity failed: %s\n", strerror(errno));
        exit(-1);
    }

    // Set scheduler type and priority
    struct sched_param sparam = { 1 };
    if (pthread_setschedparam(threadId, SCHED_FIFO, &sparam) != 0) {
        (void)printf("FrontPanel SCHED_FIFO scheduler failed: %s\n", strerror(errno));
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

    // Open the I2C bus
    if ((i2cFile = open(i2cFilename, O_RDWR)) < 0) {
        (void)printf("Failed to open the i2c bus: %s\n", strerror(errno));
        exit(-1);
    }

    // Initialize the PCA9635 chips for the desired mode of operation
    for (int chip = 0; chip < NUM_9635; ++chip) {
        if (InitRegisters(i2cFile, BASE_9635 + chip)) {
            (void)printf("Failed to initialize PCA9635 registers: %s\n", strerror(errno));
            exit(-1);
        }
    }

    // Get sync time baseline
    (void)clock_gettime(CLOCK_MONOTONIC, &currentTime);
    syncTime = 1000000000LL * currentTime.tv_sec + currentTime.tv_nsec;

    while (simulationRunning) {

        // Update front panel lights
        UpdateFrontPanel();

        // Write lights and read switches
        if (FrontPanelIO(i2cFile, displayLights)) {
            (void)printf("Failed to read/write front panel: %s\n", strerror(errno));
            exit(-1);
        }

        // Simulate SCR operation
        if (scrDelay) {
            scrDelay = FALSE;
            simulatorLights.gpios[abs(PUNCH_NO_FEED_LIGHT)] = punchNoFeedScr;
            simulatorLights.gpios[abs(READER_NO_FEED_LIGHT)] = readerNoFeedScr;
            simulatorLights.gpios[abs(SAVE_LIGHT)] = saveScr;
            simulatorLights.gpios[abs(INSERT_LIGHT)] = insertScr;
            simulatorLights.gpios[abs(AUTOMATIC_LIGHT)] = automaticScr;
            simulatorLights.gpios[abs(MANUAL_LIGHT)] = manualScr;
            simulatorLights.gpios[abs(CHECK_STOP_LIGHT)] = checkStopScr;
            (void)clock_nanosleep(CLOCK_MONOTONIC, 0, &scrDelayTime, NULL);
            if (FrontPanelGPIO(i2cFile, displayLights)) {
                (void)printf("Failed to write front panel: %s\n", strerror(errno));
                exit(-1);
            }
        }

        // Sync to display interval time
        syncTime += DISPLAY_INTERVAL;
        sleepTime.tv_sec = syncTime / 1000000000LL;
        sleepTime.tv_nsec = syncTime % 1000000000LL;
        (void)clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepTime, NULL);
    }

    (void)FrontPanelIO(i2cFile, &powerOffLights);
    (void)close(i2cFile);

    // End front panel thread
    pthread_exit(NULL);
}

// Update front panel lights
static void UpdateFrontPanel(void) {
    int tRow;
    int tColumn;
    int tIndex;

    if (powerState == POWER_EMERGENCY_OFF) {

        // Reset SCR operation
        scrCounter = 0;
        scrDelay = FALSE;
        punchNoFeedScr = FALSE;
        readerNoFeedScr = FALSE;
        saveScr = FALSE;
        insertScr = FALSE;
        automaticScr = FALSE;
        manualScr = FALSE;
        checkStopScr = FALSE;

        testMode = NO_TEST;
        displayLights = &powerOffLights;

    } else if (powerState == POWER_OFF) {

        // Reset SCR operation
        scrCounter = 0;
        scrDelay = FALSE;
        punchNoFeedScr = FALSE;
        readerNoFeedScr = FALSE;
        saveScr = FALSE;
        insertScr = FALSE;
        automaticScr = FALSE;
        manualScr = FALSE;
        checkStopScr = FALSE;

        if (inputs[SPARE_1_SWITCH]) {

            if (inputs[PROGRAM_1_SWITCH]) {
                testMode = LAMP_TEST;
                displayLights = &lampTestLights;
                if (inputs[PARITY_STOP_SWITCH]) {
                    lampTestLights.pwms[MAR_CHK_LIGHT] = 0;
                } else {
                    lampTestLights.pwms[MAR_CHK_LIGHT] = 255;
                }
                if (inputs[IO_STOP_SWITCH]) {
                    lampTestLights.pwms[RD_CHK_LIGHT] = 0;
                    lampTestLights.pwms[WR_CHK_LIGHT] = 0;
                } else {
                    lampTestLights.pwms[RD_CHK_LIGHT] = 255;
                    lampTestLights.pwms[WR_CHK_LIGHT] = 255;
                }
                if (inputs[OFLOW_STOP_SWITCH]) {
                    lampTestLights.pwms[EXP_CHK_LIGHT] = 0;
                    lampTestLights.pwms[ARITH_CHK_LIGHT] = 0;
                } else {
                    lampTestLights.pwms[EXP_CHK_LIGHT] = 255;
                    lampTestLights.pwms[ARITH_CHK_LIGHT] = 255;
                }
                lampTestLights.gpios[-POWER_ON_LIGHT] = !inputs[RESET_SWITCH];
                lampTestLights.gpios[-POWER_READY_LIGHT] = !inputs[RESET_SWITCH];
                lampTestLights.gpios[-THERMAL_LIGHT] = !inputs[RESET_SWITCH];
                lampTestLights.gpios[-SAVE_LIGHT] = !inputs[SAVE_SWITCH];
                lampTestLights.gpios[-INSERT_LIGHT] = !inputs[INSERT_SWITCH];
                lampTestLights.gpios[-PUNCH_NO_FEED_LIGHT] = !inputs[RELEASE_SWITCH];
                lampTestLights.gpios[-READER_NO_FEED_LIGHT] = !inputs[RELEASE_SWITCH];
                lampTestLights.gpios[-AUTOMATIC_LIGHT] = !inputs[START_SWITCH];
                lampTestLights.gpios[-MANUAL_LIGHT] = !inputs[STOP_SIE_SWITCH];
                lampTestLights.gpios[-CHECK_STOP_LIGHT] = !inputs[INSTANT_STOP_SCE_SWITCH];
                if (inputs[DISPLAY_MAR_SWITCH]) {
                    lampTestLights.pwms[MAR_00100_LIGHT] = (marSelector & 1) ? 255 : 0;
                    lampTestLights.pwms[MAR_00200_LIGHT] = (marSelector & 2) ? 255 : 0;
                    lampTestLights.pwms[MAR_00400_LIGHT] = (marSelector & 4) ? 255 : 0;
                    lampTestLights.pwms[MAR_00800_LIGHT] = (marSelector & 8) ? 255 : 0;
                    lampTestLights.pwms[MAR_00C00_LIGHT] = 0;
                } else {
                    lampTestLights.pwms[MAR_00100_LIGHT] = 255;
                    lampTestLights.pwms[MAR_00200_LIGHT] = 255;
                    lampTestLights.pwms[MAR_00400_LIGHT] = 255;
                    lampTestLights.pwms[MAR_00800_LIGHT] = 255;
                    lampTestLights.pwms[MAR_00C00_LIGHT] = 255;
                }

            } else if (inputs[PROGRAM_2_SWITCH]) {
                testMode = BRIGHTNESS_TEST;
                displayLights = &brightnessTestLights;

            } else if (inputs[PROGRAM_3_SWITCH]) {
                if (testMode != SNAKE_TEST) {
                    testMode = SNAKE_TEST;
                    displayLights = &snakeTestLights;
                    upperSnakeRow1 = 1;
                    upperSnakeColumn1 = 1;
                    upperSnakeRow2 = 5;
                    upperSnakeColumn2 = 38;
                    lowerSnakeIndex = 1;
                    lowerSnakeIncrement = 1;
                    statusSnakeIndex = 1;
                    statusSnakeIncrement = 1;
                    snakeDelayCounter = 1;
                }
                if (--snakeDelayCounter == 0) {
                    if (lastUpperSnakeLight1 != 0) snakeTestLights.pwms[lastUpperSnakeLight1] = 0;
                    if (lastUpperSnakeLight2 != 0) snakeTestLights.pwms[lastUpperSnakeLight2] = 0;
                    if (lastLowerSnakeLight != 0) snakeTestLights.pwms[lastLowerSnakeLight] = 0;
                    if (lastStatusSnakeLight != 0) snakeTestLights.gpios[0 - lastStatusSnakeLight] = FALSE;

                    lastUpperSnakeLight1 = 0;
                    while (lastUpperSnakeLight1 == 0) {
                        int rnd = rand() & 0x7;
                        tRow = upperSnakeRow1 + snakeRowOffsets[rnd];
                        tColumn = upperSnakeColumn1 + snakeColumnOffsets[rnd];
                        lastUpperSnakeLight1 = snakeUpperLights[tRow][tColumn];
                    }
                    upperSnakeRow1 = tRow;
                    upperSnakeColumn1 = tColumn;
                    snakeTestLights.pwms[lastUpperSnakeLight1] = 255;

                    lastUpperSnakeLight2 = 0;
                    while (lastUpperSnakeLight2 == 0) {
                        int rnd = rand() & 0x7;
                        tRow = upperSnakeRow2 + snakeRowOffsets[rnd];
                        tColumn = upperSnakeColumn2 + snakeColumnOffsets[rnd];
                        lastUpperSnakeLight2 = snakeUpperLights[tRow][tColumn];
                    }
                    upperSnakeRow2 = tRow;
                    upperSnakeColumn2 = tColumn;
                    snakeTestLights.pwms[lastUpperSnakeLight2] = 255;

                    lastLowerSnakeLight = 0;
                    while (lastLowerSnakeLight == 0) {
                        tIndex = lowerSnakeIndex + lowerSnakeIncrement;
                        lastLowerSnakeLight = snakeLowerLights[tIndex];
                        if (lastLowerSnakeLight == 0) lowerSnakeIncrement *= -1;
                    }
                    lowerSnakeIndex = tIndex;
                    snakeTestLights.pwms[lastLowerSnakeLight] = 255;

                    lastStatusSnakeLight = 0;
                    while (lastStatusSnakeLight == 0) {
                        tIndex = statusSnakeIndex + statusSnakeIncrement;
                        lastStatusSnakeLight = snakeStatusLights[tIndex];
                        if (lastStatusSnakeLight == 0) statusSnakeIncrement *= -1;
                    }
                    statusSnakeIndex = tIndex;
                    snakeTestLights.gpios[0 - lastStatusSnakeLight] = TRUE;

                    snakeDelayCounter = (17 - marSelector) % 12 + 1;
                }

            } else if (inputs[PROGRAM_4_SWITCH]) {
                testMode = IBM_TEST;
                displayLights = &ibmTestLights;

            } else {
                testMode = NO_TEST;
                displayLights = &powerOffLights;
            }

        } else {
            testMode = NO_TEST;
            displayLights = &powerOffLights;
        }

    } else /* powerState == POWER_ON || POWER_READY */{

        testMode = NO_TEST;
        displayLights = &simulatorLights;
        if (lightMode == REALISTIC_LIGHTS) {
            lightMap = realisticMap;
            riseDelta = RISE_DELTA;
            fallDelta = FALL_DELTA;
        } else /* lightMode == BRIGHT_LIGHTS */ {
            lightMap = brightMap;
            riseDelta = 2 * RISE_DELTA;
            fallDelta = 2 * FALL_DELTA;
        }

        // Swap reading and writing counts arrays
        volatile struct cycleCounts *temp = reading;
        reading = writing;
        writing = temp;

        // Set pwms in simulatorLights
        char *pwm = simulatorLights.pwms;
        char *prev = previousLights;
        volatile unsigned int *count = reading->indicators;
        unsigned int total = Max(reading->total, 1);
        if (total == 1) return;
        for (int light = 0; light < NUM_PWMS; ++light) {
            int value = 0;
            int cnt = *count++;
            if (cnt > 0) value = lightMap[Min(255 * cnt / total, 255)];
            int pValue = *prev;
            if ((value - pValue) > riseDelta) value = pValue + riseDelta;
            if ((pValue - value) > fallDelta) value = pValue - fallDelta;
            *pwm++ = value;
            *prev++ = value;
        }

        // Set gpios in simulatorLights
        (void)memcpy((void *)simulatorLights.gpios, (void *)outputs, NUM_GPIOS);

        // Simulate SCR operation
        scrCounter = (scrCounter + 1) % 10;
        if (scrCounter == 0) {
            scrDelay = TRUE;
            punchNoFeedScr = simulatorLights.gpios[abs(PUNCH_NO_FEED_LIGHT)];
            simulatorLights.gpios[abs(PUNCH_NO_FEED_LIGHT)] = FALSE;
            readerNoFeedScr = simulatorLights.gpios[abs(READER_NO_FEED_LIGHT)];
            simulatorLights.gpios[abs(READER_NO_FEED_LIGHT)] = FALSE;
            saveScr = simulatorLights.gpios[abs(SAVE_LIGHT)];
            simulatorLights.gpios[abs(SAVE_LIGHT)] = FALSE;
            insertScr = simulatorLights.gpios[abs(INSERT_LIGHT)];
            simulatorLights.gpios[abs(INSERT_LIGHT)] = FALSE;
            automaticScr = simulatorLights.gpios[abs(AUTOMATIC_LIGHT)];
            simulatorLights.gpios[abs(AUTOMATIC_LIGHT)] = FALSE;
            manualScr = simulatorLights.gpios[abs(MANUAL_LIGHT)];
            simulatorLights.gpios[abs(MANUAL_LIGHT)] = FALSE;
            checkStopScr = simulatorLights.gpios[abs(CHECK_STOP_LIGHT)];
            simulatorLights.gpios[abs(CHECK_STOP_LIGHT)] = FALSE;
        } else {
            simulatorLights.gpios[abs(PUNCH_NO_FEED_LIGHT)] |= punchNoFeedScr;
            simulatorLights.gpios[abs(READER_NO_FEED_LIGHT)] |= readerNoFeedScr;
            simulatorLights.gpios[abs(SAVE_LIGHT)] |= saveScr;
            simulatorLights.gpios[abs(INSERT_LIGHT)] |= insertScr;
            simulatorLights.gpios[abs(AUTOMATIC_LIGHT)] |= automaticScr;
            simulatorLights.gpios[abs(MANUAL_LIGHT)] |= manualScr;
            simulatorLights.gpios[abs(CHECK_STOP_LIGHT)] |= checkStopScr;
        }

        // Clear reading counts array to be ready for next display cycle
        (void)memset((void*)reading, 0, sizeof(struct cycleCounts));
    }
}

// Initialize the PCA9635 chips to use only the individual PWM on each output
// and no group PWM, and set the initial PWM values to full OFF.
static int InitRegisters(int i2cFile, int addr) {
    static char buffer[] =
        { 0x80,                         // control register
          0x00, 0x01,                   // mode1, mode2
          0x00, 0x00, 0x00, 0x00,       // pwm0-3
          0x00, 0x00, 0x00, 0x00,       // pwm4-7
          0x00, 0x00, 0x00, 0x00,       // pwm8-11
          0x00, 0x00, 0x00, 0x00,       // pwm12-15
          0xFF, 0x00,                   // grppwm, grpfreq
          0xAA, 0xAA, 0xAA, 0xAA };     // ledout0-3
    static struct i2c_msg msgs[1] = { { 0, 0, sizeof(buffer), buffer } };
    static struct i2c_rdwr_ioctl_data rdwr = { msgs, 1 };

    msgs[0].addr = addr;
    if (ioctl(i2cFile, I2C_RDWR, &rdwr) < 0) {
        return errno;
    }

    return 0;
}

// Write the front panel lights (pwms & gpios) and read the switches (gpios).
static int FrontPanelIO(int i2cFile, struct lights *displayLights) {
    char *pwm = displayLights->pwms;
    bool *gpio = displayLights->gpios;
    volatile bool *input = inputs;
    const bool *inputFlag = inputFlags;
    int *debounce = debounces;
    const int *debounceMax = debounceMaxs;

    // Write pwm values for all variable intensity lights.
    for (int chip = 0; chip < NUM_9635; ++chip) {
        char pwmmsg[1 + NUM_BITS];
        char *p = &pwmmsg[1];

        pwmmsg[0] = 0x82;
        for (int bit = 0; bit < NUM_BITS; ++bit) {
            *p++ = *pwm++;
        }
        if (SetPWMs(i2cFile, BASE_9635 + chip, pwmmsg)) {
            return errno;
        }
    }

    // Perform a combined write/read operation to update the ON/OFF lights
    // and read back the switch values.
    for (int chip = 0; chip < NUM_9675; ++chip) {
        char outbuf[NUM_BYTES];
        char inbuf[NUM_BYTES];

        // First prepare the bits to be written, which is the inverse of
        // the state of the lights on output pins and 1 for all input pins.
        for (int byte = 0; byte < NUM_BYTES; ++byte) {
            char value = 0;
            for (int bit = 0; bit < 8; ++bit) {
                value >>= 1;
                if (!*gpio || *inputFlag) {
                    value |= 0x80;
                }
                ++gpio;
                ++inputFlag;
            }
            outbuf[byte] = value;
        }

        // Perform write/read.
        if (SetGetGPIOs(i2cFile, BASE_9675 + chip, outbuf, inbuf)) {
            return errno;
        }

        // From the data that was read, ignore the output pins and apply
        // debouncing to the input pins before saving the state.
        inputFlag -= NUM_BITS;
        for (int byte = 0; byte < NUM_BYTES; ++byte) {
            char value = inbuf[byte];
            for (int bit = 0; bit < 8; ++bit) {
                if (*inputFlag) {
                    if ((value & 1) == 1) {
                        // Switch reads as OFF, see if debounce hold elapsed
                        if (*debounce > 0)
                            --*debounce;
                        else
                            *input = FALSE;
                    } else {
                        // Switch reads as ON, set or keep state at TRUE
                        // and set or reset debounce count to full value
                        *input = TRUE;
                        *debounce = *debounceMax;
                        if (*debounce == 0)
                            // Zero means hardware debounce for SCE, SIE:
                            // NC contact ON transitions NO contact OFF
                            *(input - 2) = FALSE;
                    }
                }
                value >>= 1;
                ++input;
                ++inputFlag;
                ++debounce;
                ++debounceMax;
            }
        }

        // Determine MAR Selector value
        if (inputs[OR1_SELECTOR]) {
            marSelector = OR1_MAR;
        } else if (inputs[OR2_SELECTOR]) {
            marSelector = OR2_MAR;
        } else if (inputs[OR3_SELECTOR]) {
            marSelector = OR3_MAR;
        } else if (inputs[PR1_SELECTOR]) {
            marSelector = PR1_MAR;
        } else if (inputs[PR2_SELECTOR]) {
            marSelector = PR2_MAR;
        } else if (inputs[PR3_SELECTOR]) {
            marSelector = PR3_MAR;
        } else if (inputs[OR4_SELECTOR]) {
            marSelector = OR4_MAR;
        } else if (inputs[OR5_SELECTOR]) {
            marSelector = OR5_MAR;
        } else if (inputs[CR1_SELECTOR]) {
            marSelector = CR1_MAR;
        } else if (inputs[IR3_SELECTOR]) {
            marSelector = IR3_MAR;
        } else if (inputs[IR1_SELECTOR]) {
            marSelector = IR1_MAR;
        } else if (inputs[IR2_SELECTOR]) {
            marSelector = IR2_MAR;
        } else {
            marSelector = lastMarSelector;
        }
        lastMarSelector = marSelector;
    }

    return 0;
}

// Write the front panel lights (gpios).
static int FrontPanelGPIO(int i2cFile, struct lights *displayLights) {
    bool *gpio = displayLights->gpios;
    const bool *inputFlag = inputFlags;

    // Perform a write operation to update the ON/OFF lights.
    for (int chip = 0; chip < NUM_9675; ++chip) {
        char outbuf[NUM_BYTES];

        // Prepare the bits to be written, which is the inverse of the
        // state of the lights on output pins and 1 for all input pins.
        for (int byte = 0; byte < NUM_BYTES; ++byte) {
            char value = 0;
            for (int bit = 0; bit < 8; ++bit) {
                value >>= 1;
                if (!*gpio || *inputFlag) {
                    value |= 0x80;
                }
                ++gpio;
                ++inputFlag;
            }
            outbuf[byte] = value;
        }

        // Perform write.
        if (SetGPIOs(i2cFile, BASE_9675 + chip, outbuf)) {
            return errno;
        }
    }

    return 0;
}

// Update the PWM values for one PCA9635 chip.
static int SetPWMs(int i2cFile, int addr, char *pwms) {
    static struct i2c_msg msg = { 0, 0, 17, NULL };
    static struct i2c_rdwr_ioctl_data rdwr = { &msg, 1 };

    msg.addr = addr;
    msg.buf = pwms;
    if (ioctl(i2cFile, I2C_RDWR, &rdwr) < 0) {
        return errno;
    }

    return 0;
}

// Do a combined write/read operation on one PCA9675 chip to write the output
// pins and read back the input pins.
static int SetGetGPIOs(int i2cFile, int addr, char *out, char *in) {
    static struct i2c_msg msgs[2] = { { 0, 0, 2, NULL },
                                      { 0, I2C_M_RD, 2, NULL } };
    static struct i2c_rdwr_ioctl_data rdwr = { msgs, 2 };

    msgs[0].addr = addr;
    msgs[0].buf = out;
    msgs[1].addr = addr;
    msgs[1].buf = in;
    if (ioctl(i2cFile, I2C_RDWR, &rdwr) < 0) {
        return errno;
    }

    return 0;
}

// Do a write operation on one PCA9675 chip to write the output pins.
static int SetGPIOs(int i2cFile, int addr, char *out) {
    static struct i2c_msg msg[1] = { { 0, 0, 2, NULL } };
    static struct i2c_rdwr_ioctl_data wr = { msg, 1 };

    msg[0].addr = addr;
    msg[0].buf = out;
    if (ioctl(i2cFile, I2C_RDWR, &wr) < 0) {
        return errno;
    }

    return 0;
}
