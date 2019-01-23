#include "dbc.h"
#include "global.h"
#include "communication-line.h"
#include "service.h"

#define nClients global->NUM_CLIENTS

static Service emptyService = {0,0,0,0,0,0};
static Message empty = {emptyService,-1};

Message empty_message(int clientID)
{
	Message message = empty;
	message.service.clientID = clientID;
	message.newMessage = 1;
	return message;
}

int is_empty(Message message)
{
	return message.newMessage == -1;
}

void init_communication_line(CommunicationLine* commLine, int numClients){

	commLine->commArray = new Message[numClients];
	for (int i = 0; i < numClients; i++){
		commLine->commArray[i] = empty;
	}
}

Message write_message(Service service)
{
	Message message;
	message.service = service;
	message.newMessage = 1;
	return message;
}

Message read_message(CommunicationLine* commLine, int clientID)
{
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	commLine->commArray[clientID].newMessage = 0;
	return commLine->commArray[clientID];
}

void send_message(CommunicationLine* commLine, Message message)
{
	require (message.service.clientID > 0, "Invalid clientID in message argument");

	commLine->commArray[message.service.clientID] = message;

}

void delete_message(CommunicationLine* commLine, int clientID)
{
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	commLine->commArray[clientID] = empty;
}

int no_message_available(CommunicationLine* commLine, int clientID)
{
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	return !commLine->commArray[clientID].newMessage || is_empty(commLine->commArray[clientID]);
}
