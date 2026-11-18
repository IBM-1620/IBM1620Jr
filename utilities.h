//======================================================================================================================
//
//  utilities.h - utility functions
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

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include "defines.h"

// Function declarations
void PrintInformation(void);

void LoadMemory(int cnt, char *filenames[]);
void SaveMemory(char *filename);

int Min(int x, int y);
int Max(int x, int y);

#endif /* UTILITIES_H_ */
