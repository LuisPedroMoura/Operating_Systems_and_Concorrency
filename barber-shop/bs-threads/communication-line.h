#ifndef COMMUNICATION_LINE_H
#define COMMUNICATION_LINE_H

#include "global.h"
#include "service.h"

typedef struct _Message_
{
	Service service;
	int newMessage = 0;
}Message;

typedef struct _CommunicationLine_
{
	Message commArray[MAX_CLIENTS];
	pthread_mutex_t messageMutex[MAX_CLIENTS];
	pthread_cond_t messageAvailable[MAX_CLIENTS];
}CommunicationLine;

Message empty_message(int clientID);
int is_empty(Message message);
void init_communication_line(CommunicationLine* commLine);
Message write_message(Service service);
Message read_message(CommunicationLine* commLine, int clientID);
void send_message(CommunicationLine* commLine, Message message);
int no_message_available(CommunicationLine* commLine, int clientID);
void delete_message(CommunicationLine* commLine, int clientID);

#endif
