#include "communication-line.h"

#include "dbc.h"
#include "global.h"

#define nClients global->NUM_CLIENTS


int read_communication(CommunicationLine* commLine, int clientID)
{
	require (clientID != NULL, "clientID argument required");
	require (clientID >= 0 && clientID < nClients, "Invalid clientID argument");

	return commLine[clientID];
}

void write_communication(CommunicationLine* commLine, int clientID, int message)
{
	require (clientID != NULL, "clientID argument required");
	require (clientID >= 0 && clientID < nClients, "Invalid clientID argument");
	require (message != NULL, "message argument required");

	commLine[clientID] = message;
}
