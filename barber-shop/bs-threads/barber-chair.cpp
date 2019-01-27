#include <stdlib.h>
#include "dbc.h"
#include "global.h"
#include "utils.h"
#include "box.h"
#include "logger.h"
#include "barber-chair.h"

static const char* skel = 
		"@-------@\n"
		"| ####  |\n"
		"@-+---+-@\n"
		"##|C##|##\n"
		"  +---+\n"
		"  |B##|\n"
		"  @---@";

static int skel_length = num_lines_barber_chair()*(num_columns_barber_chair()+1)*4; // extra space for (pessimistic) utf8 encoding!

static char* to_string_barber_chair(BarberChair* chair);

int num_lines_barber_chair()
{
	return string_num_lines((char*)skel);
}

int num_columns_barber_chair()
{
	return string_num_columns((char*)skel);
}

void init_barber_chair(BarberChair* chair, int id, int line, int column)
{
	require (chair != NULL, "chair argument required");
	require (id > 0, concat_3str("invalid id (", int2str(id), ")"));
	require (line >= 0, concat_3str("Invalid line (", int2str(line), ")"));
	require (column >= 0, concat_3str("Invalid column (", int2str(column), ")"));

	chair->id = id;
	chair->clientID = 0;
	chair->barberID = 0;
	chair->toolsHolded = 0;
	chair->completionPercentage = -1;
	chair->internal = NULL;
	char buf[31];
	gen_boxes(buf, 30, "Chair #.##: progress:", "#", int2nstr(chair->id, 2));
	static char* translations[] = {
		SCISSOR, (char*)"Scissor",
		COMB, (char*)"Comb",
		RAZOR, (char*)"Razor",
		NULL
	};

	chair->logId = register_logger(buf, line ,column , num_lines_barber_chair(), num_columns_barber_chair(), translations);

	chair->barberChairMutex = PTHREAD_MUTEX_INITIALIZER;
	chair->barberChairAvailable = PTHREAD_COND_INITIALIZER;
	chair->barberChairServiceFinished = PTHREAD_COND_INITIALIZER;
	chair->clientRoseFromBarberChair = PTHREAD_COND_INITIALIZER;
	chair->clientSatInBarberChair = PTHREAD_COND_INITIALIZER;
	chair->clientReadyForShave = PTHREAD_COND_INITIALIZER;
}

void term_barber_chair(BarberChair* chair)
{
	require (chair != NULL, "chair argument required");

	if (chair->internal != NULL)
	{
		mem_free(chair->internal);
		chair->internal = NULL;
	}
}

void log_barber_chair(BarberChair* chair)
{
	require (chair != NULL, "chair argument required");

	spend(random_int(global->MIN_VITALITY_TIME_UNITS, global->MAX_VITALITY_TIME_UNITS));
	send_log(chair->logId, to_string_barber_chair(chair));
}

static char* to_string_barber_chair(BarberChair* chair)
{
	if (chair->internal == NULL)
		chair->internal = (char*)mem_alloc(skel_length + 1);

	char t1[7];
	memset(t1, 0, 7);
	char t2[7];
	memset(t2, 0, 7);
	if (chair->toolsHolded & SCISSOR_TOOL)
	{
		strcpy(t1, SCISSOR);
		if (chair->toolsHolded & COMB_TOOL)
			strcpy(t2, COMB);
		else if (chair->toolsHolded & RAZOR_TOOL)
			strcpy(t2, RAZOR);

	}
	else if (chair->toolsHolded & COMB_TOOL)
	{
		strcpy(t1, COMB);
		if (chair->toolsHolded & RAZOR_TOOL)
			strcpy(t2, RAZOR);
	}
	else if (chair->toolsHolded & RAZOR_TOOL)
		strcpy(t1, RAZOR);
	return gen_boxes(chair->internal, skel_length, skel,
			chair->completionPercentage < 0 ? " ---" : perc2str(chair->completionPercentage),
					t1, barber_chair_with_a_client(chair) ? int2nstr(chair->clientID, 2) : "--", t2,
							barber_chair_with_a_barber(chair) ? int2nstr(chair->barberID, 2) : "--");
}

int empty_barber_chair(BarberChair* chair)
{
	require (chair != NULL, "chair argument required");

	mutex_lock(&chair->barberChairMutex);
	int res = _empty_barber_chair_(chair);
	mutex_unlock(&chair->barberChairMutex);

	return res;
}

int _empty_barber_chair_(BarberChair* chair)
{
	require (chair != NULL, "chair argument required");

	int res = chair->barberID == 0 && chair->clientID == 0;

	return res;
}

int complete_barber_chair(BarberChair* chair)
{
	require (chair != NULL, "chair argument required");

	int res = chair->barberID > 0 && chair->clientID > 0;
	return res;
}

int barber_chair_with_a_client(BarberChair* chair)
{
	require (chair != NULL, "chair argument required");

	int res = chair->clientID > 0;
	return res;
}

int barber_chair_with_a_barber(BarberChair* chair)
{
	require (chair != NULL, "chair argument required");

	int res = chair->barberID > 0;
	return res;
}

int client_seated_in_barber_chair(BarberChair* chair, int clientID)
{
	require (chair != NULL, "chair argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	int res = (clientID == chair->clientID);
	return res;
}

int barber_working_in_barber_chair(BarberChair* chair, int barberID)
{
	require (chair != NULL, "chair argument required");
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));

	int res = (barberID == chair->barberID);
	return res;
}

void reserve_barber_chair(BarberChair* chair, int barberID)
{
	require (chair != NULL, "chair argument required");
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));

	mutex_lock(&chair->barberChairMutex);
	require (_empty_barber_chair_(chair), "barber chair is already occupied");

	chair->barberID = barberID;
	chair->completionPercentage = 0;

	log_barber_chair(chair);

	mutex_unlock(&chair->barberChairMutex);
}

void release_barber_chair(BarberChair* chair, int barberID)
{
	require (chair != NULL, "chair argument required");
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));

	mutex_lock(&chair->barberChairMutex);
	require (barber_working_in_barber_chair(chair, barberID), concat_3str("barber ", int2str(barberID), " not working in chair"));

	while (barber_chair_with_a_client(chair)){
		cond_wait(&chair->clientRoseFromBarberChair, &chair->barberChairMutex);
	}
	require (!barber_chair_with_a_client(chair), "a client is still in chair");

	chair->barberID = 0;
	chair->clientID = 0;
	chair->toolsHolded = 0;
	chair->completionPercentage = -1;
	require (_empty_barber_chair_(chair), "release of barber chair was not successful");
	cond_broadcast(&chair->barberChairAvailable);

	log_barber_chair(chair);

	mutex_unlock(&chair->barberChairMutex);
}

void sit_in_barber_chair(BarberChair* chair, int clientID)
{
	require (chair != NULL, "chair argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	mutex_lock(&chair->barberChairMutex);

	if (client_seated_in_barber_chair(chair, clientID)){
		cond_broadcast(&chair->clientSatInBarberChair);
	}
	else{
		require (!barber_chair_with_a_client(chair), "chair occupied with a client");

		chair->clientID = clientID;
		cond_broadcast(&chair->clientSatInBarberChair);
	}

	log_barber_chair(chair);

	mutex_unlock(&chair->barberChairMutex);
}

void rise_from_barber_chair(BarberChair* chair, int clientID)
{
	require (chair != NULL, "chair argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	mutex_lock(&chair->barberChairMutex);
	require (client_seated_in_barber_chair(chair, clientID), concat_3str("client ", int2str(clientID), " not seated in barber chair"));
	//require (barber_chair_service_finished(chair), "barber chair service not complete");

	chair->clientID = 0;
	chair->completionPercentage = 0;
	cond_broadcast(&chair->clientRoseFromBarberChair);

	log_barber_chair(chair);

	mutex_unlock(&chair->barberChairMutex);
}

void set_tools_barber_chair(BarberChair* chair, int tools)
{
	require (chair != NULL, "chair argument required");
	require ((tools & 0x7) != 0, concat_3str("invalid tools mask (", int2str(tools), ")"));

	mutex_lock(&chair->barberChairMutex);

	while(!barber_chair_with_a_client(chair)){
		cond_wait(&chair->clientSatInBarberChair, &chair->barberChairMutex);
	}
	require (complete_barber_chair(chair), "barber chair is not complete");
	chair->toolsHolded = tools;
	chair->completionPercentage = 0;

	log_barber_chair(chair);

	mutex_unlock(&chair->barberChairMutex);
}

int barber_chair_service_finished(BarberChair* chair)
{
	require (chair != NULL, "chair argument required");

	int res;
	res = chair->completionPercentage == 100;
	return res;
}

void set_completion_barber_chair(BarberChair* chair, int completionPercentage)
{
	require (chair != NULL, "chair argument required");
	require (completionPercentage >= 0 && completionPercentage <= 100, concat_3str("invalid percentage (", int2str(completionPercentage), ")"));
	require ((chair->toolsHolded & 0x7) != 0, "barber not holding tools");

	mutex_lock(&chair->barberChairMutex);
	require (complete_barber_chair(chair), "barber chair is not complete");

	chair->completionPercentage = completionPercentage;
	log_barber_chair(chair);

	if(completionPercentage == 100){
		cond_broadcast(&chair->barberChairServiceFinished);
	}

	mutex_unlock(&chair->barberChairMutex);
}

void wait_for_barber_chair_service_completion(BarberChair* chair)
{
	require (chair != NULL, "chair argument required");

	mutex_lock(&chair->barberChairMutex);
	while(!barber_chair_service_finished(chair)){
		cond_wait(&chair->barberChairServiceFinished, &chair->barberChairMutex);
	}
	mutex_unlock(&chair->barberChairMutex);
}
