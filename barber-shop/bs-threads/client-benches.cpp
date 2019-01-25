#include <stdlib.h>
#include "dbc.h"
#include "global.h"
#include "utils.h"
#include "box.h"
#include "logger.h"
#include "client-benches.h"

static const int skel_length = (MAX_CLIENT_BENCHES_SEATS*12*3+6)*4;

static int random_empty_seat_position_client_benches(ClientBenches* benches);
static char* to_string_client_benches(ClientBenches* benches);
static int _num_available_benches_seats_(ClientBenches* benches);


void init_client_benches(ClientBenches* benches, int num_seats, int num_benches, int line, int column)
{
	require (benches != NULL, "benches argument required");
	require (num_seats > 0 && num_seats <= MAX_CLIENT_BENCHES_SEATS, concat_5str("invalid number of seats (", int2str(num_seats), " not in [1,", int2str(MAX_CLIENT_BENCHES_SEATS), "])"));
	require (num_benches > 0 && num_benches <= num_seats, concat_5str("invalid number of benches (", int2str(num_benches), " not in [1,", int2str(num_seats), "])"));
	require (line >= 0, concat_3str("Invalid line (", int2str(line), ")"));
	require (column >= 0, concat_3str("Invalid column (", int2str(column), ")"));

	benches->closed = 0;
	benches->numSeats = num_seats;
	benches->numBenches = num_benches;
	for(int i = 0; i < num_seats; i++)
	{  // empty
		benches->id[i] = 0;
		benches->order[i] = 0;
		benches->request[i] = 0;
	}
	init_client_queue(&benches->queue);
	benches->internal = (char*)mem_alloc(skel_length + 1);
	benches->logId = register_logger((char*)"Client benches:", line, column, 7 ,num_seats*4+1 ,NULL);

	benches->clientBenchMutex = PTHREAD_MUTEX_INITIALIZER;

	benches->clientWaiting = PTHREAD_COND_INITIALIZER;
	benches->clientReady = PTHREAD_COND_INITIALIZER;
	benches->clientSeatAvailable = PTHREAD_COND_INITIALIZER;
}

void term_client_benches(ClientBenches* benches)
{
	require (benches != NULL, "benches argument required");

	term_client_queue(&benches->queue);
	mem_free(benches->internal);
}

void log_client_benches(ClientBenches* benches)
{
	require (benches != NULL, "benches argument required");

	spend(random_int(global->MIN_VITALITY_TIME_UNITS, global->MAX_VITALITY_TIME_UNITS));
	send_log(benches->logId, to_string_client_benches(benches));
}

int num_available_benches_seats(ClientBenches* benches)
{
	require (benches != NULL, "benches argument required");

	int res;
	mutex_lock(&benches->clientBenchMutex);
	res = _num_available_benches_seats_(benches);
	mutex_unlock(&benches->clientBenchMutex);

	return res;
}

static int _num_available_benches_seats_(ClientBenches* benches)
{
	int res = 0;
	for(int pos = 0; pos < benches->numSeats ; pos++)
		if (benches->id[pos] == 0)
			res++;

	return res;
}

int occupied_by_id_client_benches(ClientBenches* benches, int pos, int id)
{
	require (benches != NULL, "benches argument required");
	require (pos >= 0 && pos < benches->numSeats, concat_5str("invalid seat position (", int2str(pos), " not in [0,", int2str(benches->numSeats), "[)"));
	require (id > 0, concat_3str("invalid id (", int2str(id), ")"));

	return benches->id[pos] == id;
}

void wait_for_available_seat(ClientBenches* benches)
{
	require (benches != NULL, "benches argument required");

	while(_num_available_benches_seats_(benches) == 0){
		cond_wait(&benches->clientSeatAvailable, &benches->clientBenchMutex);
	}
}

// returns position
int random_sit_in_client_benches(ClientBenches* benches, int id, int request)
{
	require (benches != NULL, "benches argument required");
	require (id > 0, concat_3str("invalid id (", int2str(id), ")"));
	require (request > 0, concat_3str("invalid request (", int2str(request), ")"));

	mutex_lock(&benches->clientBenchMutex);

	wait_for_available_seat(benches);
	require (_num_available_benches_seats_(benches) > 0, "empty seat not available in client benches");

	int res = random_empty_seat_position_client_benches(benches);
	RQItem item = {id, res, request, 0};
	benches->id[res] = id;
	benches->request[res] = request;
	benches->order[res] = in_client_queue(&benches->queue, item);

	cond_signal(&benches->clientWaiting);
	log_client_benches(benches);

	mutex_unlock(&benches->clientBenchMutex);

	return res;
}

int no_more_clients(ClientBenches* benches)
{
	require (benches != NULL, "benches argument required");

	int res = empty_client_queue(&benches->queue);

	return res;
}

RQItem next_client_in_benches(ClientBenches* benches)
{
	require (benches != NULL, "benches argument required");

	RQItem res;
	mutex_lock(&benches->clientBenchMutex);

	while (no_more_clients(benches) && !benches->closed){
		cond_wait(&benches->clientWaiting, &benches->clientBenchMutex);
	}

	if (!empty_client_queue(&benches->queue))
	{
		res = out_client_queue(&benches->queue);
	}
	else
		res = empty_item();
	printf("next client in benches - returns: %d\n", res.benchPos);

	mutex_unlock(&benches->clientBenchMutex);

	return res;
}

void rise_client_benches(ClientBenches* benches, int pos, int id)
{
	require (benches != NULL, "benches argument required");
	require (pos >= 0 && pos < benches->numSeats, concat_5str("invalid seat position (", int2str(pos), " not in [0,", int2str(benches->numSeats), "[)"));
	require (id > 0, concat_3str("invalid id (", int2str(id), ")"));

	mutex_lock(&benches->clientBenchMutex);
	require (occupied_by_id_client_benches(benches, pos, id), concat_2str("client not seated in ",int2str(pos)));

	benches->id[pos] = 0;
	benches->order[pos] = 0;
	benches->request[pos] = 0;
	cond_broadcast(&benches->clientSeatAvailable);

	log_client_benches(benches);

	mutex_unlock(&benches->clientBenchMutex);
}

int seated_in_client_benches(ClientBenches* benches, int id)
{
	require (benches != NULL, "benches argument required");
	require (id > 0, concat_3str("invalid id (", int2str(id), ")"));

	int res = 0;
	mutex_lock(&benches->clientBenchMutex);
	for(int i = 0; !res && i < benches->numSeats ; i++)
		res = (benches->id[i] == id);
	mutex_unlock(&benches->clientBenchMutex);

	return res;
}

static int random_empty_seat_position_client_benches(ClientBenches* benches)
{
	require (benches != NULL, "benches argument required");

	int r = random_int(1, _num_available_benches_seats_(benches));
	int res;
	for(res = 0; r > 0 && res < benches->numSeats ; res++)
		if (benches->id[res] == 0)
			r--;
	res--;

	ensure (res >= 0 && res < benches->numSeats, "");

	return res;
}

void notify_barbers_all_clients_are_done(ClientBenches* benches)
{
	require (benches != NULL, "benches argument required");

	mutex_lock(&benches->clientBenchMutex);
	benches->closed = 1;
	cond_broadcast(&benches->clientWaiting);
	mutex_unlock(&benches->clientBenchMutex);
}

static char* to_string_client_benches(ClientBenches* benches)
{
	char s[skel_length+1];
	gen_matrix(s, skel_length, benches->numBenches, (benches->numSeats+benches->numBenches-1)/benches->numBenches, 3, 13, 1);
	int i;
	for(i = 0; s[i] != '|'; i++)
		;
	//printf("[to_string_client_benches] i:%d\n", i);
	i++;
	for(int pos = 0; pos < benches->numSeats ; pos++)
	{
		if (benches->id[pos] > 0)
		{
			//assert (s[i] == ' ' && s[i+1] == ' ' && s[i+2] == ' ');
			char* buf = int2nstr(benches->id[pos], 2);
			s[i] = 'C';
			strcpy(s+i+1, buf);
			s[i+3] = '/';
			buf = int2nstr(benches->order[pos], 3);
			strcpy(s+i+4, buf);
			s[i+7] = '/';
			s[i+8] =   (benches->request[pos] & HAIRCUT_REQ) ?  'H' : ':';
			s[i+9] = (benches->request[pos] & WASH_HAIR_REQ) ?  'W' : ':';
			s[i+10] =    (benches->request[pos] & SHAVE_REQ) ?  'S' : ':';
		}
		i += 12;
		if (s[i] == '\n')
		{
			for(; s[i] && s[i] != '|'; i++)
				;
			if (s[i])
				i++;
		}
	}
	//printf("[to_string_client_benches]\n%s\n", s);

	return gen_boxes(benches->internal, skel_length, s);
}

