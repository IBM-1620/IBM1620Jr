//======================================================================================================================
//
//  punchcard.h - punched card simulation
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

#ifndef PUNCHEDCARD_H_
#define PUNCHEDCARD_H_

#include "defines.h"

// Function declarations
void InitializePunchedCard(void);
void FinalizePunchedCard(void);
bool OpenPunchedCard(void);
void ClosePunchedCard(void);
char ReceivePCChar(void);
void SendPCChar(char achar);
void SendPCStatus(void);

#endif /* PUNCHEDCARD_H_ */
