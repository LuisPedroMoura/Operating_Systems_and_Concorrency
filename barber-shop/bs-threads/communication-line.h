#ifndef COMMUNICATION_LINE_H
#define COMMUNICATION_LINE_H

#include "global.h"

typedef struct _CommunicationLine_
{
	int commLine[global->NUM_CLIENTS];
}CommunicationLine;

int read_communication(int clientID);
int write_communication(int clientID, int message);

#endif
