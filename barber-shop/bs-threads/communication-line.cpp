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

Message read_message(CommunicationLine* commLine, int clientID, pthread_mutex_t* mutex, pthread_cond_t* messageAvailable)
{
	require (commLine != NULL , "commLine argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));
	require (mutex != NULL, "mutex argument required");
	require (messageAvailable != NULL, "vcond argument required");

	mutex_lock(mutex);
	
	while(no_message_available(commLine, clientID)){
		cond_wait(messageAvailable, mutex);
	}

	commLine->commArray[clientID].newMessage = 0;
	
	mutex_unlock(mutex);
	
	return commLine->commArray[clientID];
}

void send_message(CommunicationLine* commLine, Message message, pthread_mutex_t* mutex, pthread_cond_t* messageAvailable)
{
	require (commLine != NULL , "commLine argument required");
//	require (message != NULL , "message argument required");
	require (message.service.clientID > 0, "Invalid clientID in message argument");
	require (mutex != NULL, "mutex argument required");

	mutex_lock(mutex);
	
	commLine->commArray[message.service.clientID] = message;
	cond_broadcast(messageAvailable);

	mutex_lock(mutex);
}

void delete_message(CommunicationLine* commLine, int clientID, pthread_mutex_t* mutex)
{
	require (commLine != NULL , "commLine argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));
	require (mutex != NULL, "mutex argument required");

	mutex_lock(mutex);
	
	commLine->commArray[clientID] = nullMessage;
	
	mutex_lock(mutex);
}

int no_message_available(CommunicationLine* commLine, int clientID)
{
	require (commLine != NULL , "commLine argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	return !commLine->commArray[clientID].newMessage || is_empty(commLine->commArray[clientID]);
}
