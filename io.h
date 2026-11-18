//======================================================================================================================
//
//  io.h - io threads
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

#ifndef IO_H_
#define IO_H_

#include "defines.h"

// Function declarations
void StartIO(void);
void StopIO(void);
void ShutdownIO(void);

void BeginIOInstruction(int);
void EndIOInstruction(void);

bool ReadTYAvail(void);
char ReadTYChar(void);
void WriteTYCommand(char);
void WriteTYChar(char);

bool ReadCDAvail(void);
char ReadCDChar(void);
void WriteCDCommand(char);
void WriteCDChar(char);
void WriteCDPowerStatus(char);
void WriteCDRunStatus(char);

#endif /* IO_H_ */
