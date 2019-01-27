#include "dbc.h"
#include "global.h"
#include "communication-line.h"
#include "service.h"

static Service nullService = {0,0,0,0,0,0};
static Message nullMessage = {nullService,0};

Message empty_message(int clientID)
{
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	Message message = nullMessage;
	message.service.clientID = clientID;
	message.newMessage = 1;

	return message;
}

int is_empty(Message message)
{
//	require (message != NULL , "message argument required");
	return message.newMessage == 0;
}

void init_communication_line(CommunicationLine* commLine)
{
	require (commLine != NULL , "commLine argument required");
	for (int i = 0; i < MAX_CLIENTS; i++){
		commLine->commArray[i] = nullMessage;
		commLine->messageMutex[i] = PTHREAD_MUTEX_INITIALIZER;
		commLine->messageAvailable[i] = PTHREAD_COND_INITIALIZER;
	}
}


Message write_message(Service service)
{
//	require (service != NULL , "service argument required");
		
	Message message;
	message.service = service;
	message.newMessage = 1;
	
	return message;
}

Message read_message(CommunicationLine* commLine, int clientID)
{
	require (commLine != NULL , "commLine argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));
	
	mutex_lock(&commLine->messageMutex[clientID]);
	while(no_message_available(commLine, clientID)){
		cond_wait(&commLine->messageAvailable[clientID], &commLine->messageMutex[clientID]);
	}

	commLine->commArray[clientID].newMessage = 0;	
	
	Message message = commLine->commArray[clientID];
	mutex_unlock(&commLine->messageMutex[clientID]);
	
	return message;
}

void send_message(CommunicationLine* commLine, Message message)
{
	require (commLine != NULL , "commLine argument required");
	require (message.service.clientID > 0, "Invalid clientID in message argument");

	int clientID = message.service.clientID;
	mutex_lock(&commLine->messageMutex[clientID]);
	commLine->commArray[clientID] = message;
	cond_broadcast(&commLine->messageAvailable[clientID]);
	mutex_unlock(&commLine->messageMutex[clientID]);	
}

void delete_message(CommunicationLine* commLine, int clientID)
{
	require (commLine != NULL , "commLine argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	mutex_lock(&commLine->messageMutex[clientID]);
	commLine->commArray[clientID] = nullMessage;
	mutex_unlock(&commLine->messageMutex[clientID]);
}

int no_message_available(CommunicationLine* commLine, int clientID)
{
	require (commLine != NULL , "commLine argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	return !commLine->commArray[clientID].newMessage || is_empty(commLine->commArray[clientID]);
}
