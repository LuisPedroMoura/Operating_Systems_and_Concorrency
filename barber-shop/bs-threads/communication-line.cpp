#include "communication-line.h"

#include "dbc.h"
#include "global.h"
#include "service.h"

#define nClients global->NUM_CLIENTS

Message write_message(Service service)
{
	Message message;
	message.service = service;
	message.newMessage = 1;
	return message;
}

Message read_message(CommunicationLine* commLine, int clientID)
{
	require (clientID != NULL, "clientID argument required");
	require (clientID >= 0 && clientID < nClients, "Invalid clientID");

	commLine->commArray[clientID].newMessage = 0;
	return commLine->commArray[clientID];
}

Message read_message_with_barberID(CommunicationLine* commLine, int barberID)
{
	require (barberID != NULL, "clientID argument required");
	require (barberID >= 0 && barberID < nClients, "Invalid clientID");
	int size = sizeof(commLine->commArray) / sizeof(Service);
	for (int i = 0; i < size; i++){
		if (commLine->commArray[i].service.barberID == barberID){
			return commLine->commArray[i];
		}
	}
	return NULL;
}

void send_message(CommunicationLine* commLine, Message message)
{
	require (message != NULL, "message argument required");
	require (message.service.clientID < nClients, "Invalid clientID in message argument");

	commLine->commArray[message.service.clientID] = message;
}

void delete_message(CommunicationLine* commLine, int clientID)
{
	require (clientID != NULL, "clientID argument required");
	require (clientID >= 0 && clientID < nClients, "Invalid clientID");

	commLine->commArray[clientID] = NULL;
}

int new_message_available(CommunicationLine* commLine, int clientID)
{
	require (clientID != NULL, "clientID argument required");
	require (clientID >= 0 && clientID < nClients, "Invalid clientID");

	return commLine->commArray[clientID].newMessage;
}

int no_message_available(CommunicationLine* commLine, int clientID)
{
	require (clientID != NULL, "clientID argument required");
	require (clientID >= 0 && clientID < nClients, "Invalid clientID");

	return !new_message_available(commLine, clientID);
}
