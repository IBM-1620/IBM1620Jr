//======================================================================================================================
//
//  utilities.c - utility functions
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
#include "typewriter.h"
#include "utilities.h"

#define _GNU_SOURCE 1
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>


void PrintInformation(void) {

    (void)printf("Simulator information:\n");
    (void)printf("  Simulator version: %d.%d.%d\n", versionMajor, versionMinor, versionBuild);
    (void)printf("  OS:  Raspian\n");
    (void)printf("  GCC version: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
}

void LoadMemory(int cnt, char *filenames[]) {
    FILE *memoryFile = NULL;
    char line[MAX_INPUT_LINE];
    int nextAddr = 0;
    bool invalid = FALSE;

    // Clear memory
    (void)memset((void *)memory, 0, sizeof(memory));

    // Process all of the memory files
    for (int i = 0; i < cnt; ++i) {

        // Open a memory file
        memoryFile = fopen(filenames[i], "r");
        if (memoryFile == NULL) {
            (void)printf("Failed to open core memory file \"%s\": %s\n", filenames[i], strerror(errno));
            continue;
        }
        invalid = FALSE;

        // Read and process the memory file
        while (fgets(line, MAX_INPUT_LINE, memoryFile) != NULL) {
            int idx = 0;
            int addr = 0;
            int data = 0;
            bool seen = FALSE;
            bool addrSeen = FALSE;
            bool cont = TRUE;

            // Process options
            if (line[0] == '*') {
                if (strcasestr(line, "invalid") != NULL) invalid = TRUE;
                if (strcasestr(line, "tabs") != NULL) ParseTabs(line);
                continue;
            }

            // Process the address
            while ((line[idx] >= '0') && (line[idx] <= '9')) {
                addr = 10 * addr + (line[idx++] - '0');
                addrSeen = TRUE;
            }
            if (line[idx] != ':') continue;
            ++idx;
            if (!addrSeen) addr = nextAddr;
            while (line[idx] == ' ') ++idx;

            // Process the data
            while (cont) {
                switch (line[idx]) {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        data = (data << 4) + (line[idx++] - '0');
                        seen = TRUE;
                        break;
                    case 'a':
                    case 'b':
                    case 'c':
                    case 'f':
                        data = (data << 4) + 10 + (line[idx++] - 'a');
                        seen = TRUE;
                        break;
                    case 'd':
                    case 'e':
                        if (invalid) {
                            data = (data << 4) + 10 + (line[idx] - 'a');
                        } else {
                            (void)printf("Bad value @ %d: %c\n", addr, line[idx]);
                        }
                        seen = TRUE;
                        ++idx;
                        break;
                    case 'A':
                    case 'B':
                    case 'C':
                    case 'F':
                        data = (data << 4) + 10 + (line[idx++] - 'A');
                        seen = TRUE;
                        break;
                    case 'D':
                    case 'E':
                        if (invalid) {
                            data = (data << 4) + 10 + (line[idx] - 'A');
                        } else {
                            (void)printf("Bad value @ %d: %c\n", addr, line[idx]);
                        }
                        seen = TRUE;
                        ++idx;
                        break;
                    case '/':
                        cont = FALSE;
                        break;
                    default:
                        if (seen) {
                            if (addr < MEMORY_SIZE) {
                                if (data <= 0x1F) {
                                    memory[addr++] = data;
                                    nextAddr = addr;
                                    data = 0;
                                    seen = FALSE;
                                    while (line[idx] == ' ') ++idx;
                                } else {
                                    (void)printf("Bad value @ %d: %x\n", addr, data);
                                }
                            } else {
                                (void)printf("Bad address - %d\n", addr);
                                cont = FALSE;
                            }
                        } else {
                            cont = FALSE;
                        }
                        break;
                }
            }
        }

        (void)fclose(memoryFile);

        (void)printf("Loaded core memory file \"%s\"\n", filenames[i]);
    }
}

void SaveMemory(char *filename) {
    FILE *memoryFile = NULL;

    // Open core memory file
    memoryFile = fopen(filename, "w");
    if (memoryFile == NULL) {
        (void)printf("Failed to open core memory file \"%s\": %s\n", filename, strerror(errno));
        return;
    }

    // Write header
    time_t t = time(NULL);
    (void)fprintf(memoryFile, "// Core memory saved @ %s", ctime(&t));

    // Write invalid option if needed
    for (int addr = 0; addr < MEMORY_SIZE; ++addr) {
        char digit = memory[addr] & DIGIT_MASK;
        if ((digit == INVALID_13) || (digit == INVALID_14)) {
            (void)fprintf(memoryFile, "* invalid\n");
            break;
        }
    }

    // Write tab setting option if needed
    int last;
    for (last = rightMarginPosition; last >= 0; --last) {
        if (tabs[last]) break;
    }
    if (last > 0) {
        (void)fprintf(memoryFile, "* tabs ");
        for (int i = 1; i <= last; ++i) {
            if (tabs[i]) {
                (void)fprintf(memoryFile, "%d", i);
                if (i < last) (void)fprintf(memoryFile, ",");
            }
        }
        (void)fprintf(memoryFile, "\n");
    }

    // Write non-zero memory contents
    for (int addr = 0; addr < MEMORY_SIZE; addr += DIGITS_PER_LINE) {
        bool nonZero = FALSE;
        for (int i = 0; i < DIGITS_PER_LINE; ++i) {
            if (memory[addr + i] != 0) {
                nonZero = TRUE;
                break;
            }
        }
        if (nonZero) {
            (void)fprintf(memoryFile, "%05d:", addr);
            for (int i = 0; i < DIGITS_PER_LINE; ++i) {
                if (i == (DIGITS_PER_LINE / 2)) (void)fprintf(memoryFile, " ");
                (void)fprintf(memoryFile, " %02X", (memory[addr + i] & FLAG_DIGIT_MASK));
            }
            if (alphaOpt) {
                (void)fprintf(memoryFile, "  // \"");
                for (int i = 0; i < DIGITS_PER_LINE; i += 2) {
                    char c = alphamericTYChars[((memory[addr + i] & DIGIT_MASK) << 4) +
                                                (memory[addr + i + 1] & DIGIT_MASK)];
                    if (c == 0) c = '?';
                    if (i == (DIGITS_PER_LINE / 2)) (void)fprintf(memoryFile, " ");
                    (void)fprintf(memoryFile, "%c", c);
                }
                (void)fprintf(memoryFile, "\"");
            }
            (void)fprintf(memoryFile, "\n");
        }
    }

    (void)fprintf(memoryFile, "// End\n");
    (void)fclose(memoryFile);

    (void)printf("Saved core memory file \"%s\"\n", filename);
}

int Min(int x, int y) {

    if (x <= y) return x;
    return y;
}

int Max(int x, int y) {

    if (x >= y) return x;
    return y;
}
