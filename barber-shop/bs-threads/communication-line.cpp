#include "dbc.h"
#include "global.h"
#include "communication-line.h"
#include "service.h"

#define nClients global->NUM_CLIENTS

static Message empty = {0,-1};

Message empty_message()
{
   return empty;
}

int is_empty(Message message)
{
	return message.newMessage == -1;
}

void init_communication_line(CommunicationLine* commLine, int numClients){

	commLine->commArray = new Message[numClients];
	for (int i = 0; i < numClients; i++){
		commLine->commArray[i] = empty_message();
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

Message read_message_with_barberID(CommunicationLine* commLine, int barberID)
{
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));

	int size = sizeof(commLine->commArray) / sizeof(Service);
	for (int i = 0; i < size; i++){
		if (commLine->commArray[i].service.barberID == barberID){
			return commLine->commArray[i];
		}
	}
	return empty_message();
}

void send_message(CommunicationLine* commLine, Message message)
{
	require (!is_empty(message), "message argument required");
	require (message.service.clientID > 0, "Invalid clientID in message argument");

	commLine->commArray[message.service.clientID] = message;

}

void delete_message(CommunicationLine* commLine, int clientID)
{
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	commLine->commArray[clientID] = empty_message();
}

int no_message_available(CommunicationLine* commLine, int clientID)
{
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	return !commLine->commArray[clientID].newMessage || is_empty(commLine->commArray[clientID]);
}
