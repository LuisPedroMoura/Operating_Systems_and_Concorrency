/**
 * \brief client benches representation
 *  
 * \author Miguel Oliveira e Silva - 2018
 */

#ifndef CLIENT_BENCHES_H
#define CLIENT_BENCHES_H

#include "global.h"
#include "client-queue.h"

typedef struct _ClientBenches_
{
	int closed;
	int numSeats;
	int numBenches;
	int id[MAX_CLIENT_BENCHES_SEATS];
	int order[MAX_CLIENT_BENCHES_SEATS];
	int request[MAX_CLIENT_BENCHES_SEATS];
	ClientQueue queue;
	int logId;
	char* internal;

	pthread_mutex_t clientBenchMutex;
	pthread_cond_t clientWaiting;
	pthread_cond_t clientReady;
	pthread_cond_t clientSeatAvailable;

} ClientBenches;

void init_client_benches(ClientBenches* benches, int num_seats, int num_benches, int line, int column);
void term_client_benches(ClientBenches* benches);
void log_client_benches(ClientBenches* benches);

int num_available_benches_seats(ClientBenches* benches);
int occupied_by_id_client_benches(ClientBenches* benches, int pos, int id);
void notify_barbers_all_clients_are_done(ClientBenches* benches);

// to use directly by clients:
void wait_for_available_seat(ClientBenches* benches);
int random_sit_in_client_benches(ClientBenches* benches, int id, int request);
void rise_client_benches(ClientBenches* benches, int pos, int id);
int seated_in_client_benches(ClientBenches* benches, int id);

// to use directly by barbers:
int no_more_clients(ClientBenches* benches);
RQItem next_client_in_benches(ClientBenches* benches);




#endif
