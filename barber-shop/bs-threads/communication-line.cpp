#include "communication-line.h"

#include "dbc.h"
#include "global.h"
#include "service.h"

#define nClients global->NUM_CLIENTS

Message write_message(Service service)
{
	Message message;
	message.service = service;
	message.notRead = 1;
	return message;
}
int read_message(CommunicationLine* commLine, Message message)
{
	require (message != NULL, "service argument required");
	require (message.service.clientID < nClients, "Invalid clientID");

	message.notRead = 0;
	return commLine->commArray[message.service.clientID];
}

void send_message(CommunicationLine* commLine, Message message)
{
	require (message != NULL, "clientID argument required");
	require (message.service.clientID < nClients, "Invalid clientID");

	commLine->commArray[message.service.clientID] = message;
}
