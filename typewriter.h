//======================================================================================================================
//
//  typewriter.h - typewriter simulation
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

#ifndef TYPEWRITER_H_
#define TYPEWRITER_H_

#include "defines.h"

// Function declarations
void InitializeTypewriter(void);
void FinalizeTypewriter(void);
bool OpenTypewriter(void);
void CloseTypewriter(void);
char ReceiveTYChar(void);
void SendTYChar(char achar);
void UpdateTYPosition(char achar);
void ParseTabs(char *str);
void SetTabs(void);

#endif /* TYPEWRITER_H_ */
