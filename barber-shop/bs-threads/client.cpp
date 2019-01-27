#include <stdlib.h>
#include "dbc.h"
#include "global.h"
#include "utils.h"
#include "box.h"
#include "timer.h"
#include "logger.h"
#include "service.h"
#include "client.h"
#include "communication-line.h"

enum ClientState
{
	NONE = 0,                   // initial state
	WANDERING_OUTSIDE,          // client outside barshop doing (important) things
	WAITING_BARBERSHOP_VACANCY, // a place to sit in the barber shop clients benches
	SELECTING_REQUESTS,         // choosing barber shop services
	WAITING_ITS_TURN,           // waiting for a barber assignment
	WAITING_SERVICE,            // waiting service selection (includes seat id)
	WAITING_SERVICE_START,      // client already seated in chair/basin waiting service start
	HAVING_A_HAIRCUT,           // haircut in progress
	HAVING_A_SHAVE,             // shave in progress
	HAVING_A_HAIR_WASH,         // hair wash in progress
	DONE                        // final state
};

#define State_SIZE (DONE - NONE + 1)

static const char* stateText[State_SIZE] =
{
		"---------",
		"WANDERING",
		"W VACANCY",
		"REQUESTS ",
		"Wait TURN",
		"Wt SERVCE",
		"SERV STRT",
		"HAIRCUT  ",
		"SHAVE    ",
		"HAIR WASH",
		"DONE     ",
};

static const char* skel = 
		"@---+---+---@\n"
		"|C##|B##|###|\n"
		"+---+---+-+-+\n"
		"|#########|#|\n"
		"@---------+-@";
static int skel_length = num_lines_client()*(num_columns_client()+1)*4; // extra space for (pessimistic) utf8 encoding!

static void life(Client* client);

static void notify_client_birth(Client* client);
static void notify_client_death(Client* client);
static void wandering_outside(Client* client);
static int vacancy_in_barber_shop(Client* client);
static void select_requests(Client* client);
static void wait_its_turn(Client* client);
static void rise_from_client_benches(Client* client);
static void wait_all_services_done(Client* client);
static void update_client_with_service(Client* client, Service service);
static char* to_string_client(Client* client);

size_t sizeof_client()
{
	return sizeof(Client);
}

int num_lines_client()
{
	return string_num_lines((char*)skel);
}

int num_columns_client()
{
	return string_num_columns((char*)skel);
}

void init_client(Client* client, int id, BarberShop* shop, int num_trips_to_barber, int line, int column)
{
	require (client != NULL, "client argument required");
	require (id > 0, concat_3str("invalid id (", int2str(id), ")"));
	require (shop != NULL, "barber shop argument required");
	require (num_trips_to_barber > 0, concat_3str("invalid number of trips to barber (", int2str(num_trips_to_barber), ")"));
	require (line >= 0, concat_3str("Invalid line (", int2str(line), ")"));
	require (column >= 0, concat_3str("Invalid column (", int2str(column), ")"));

	client->id = id;
	client->state = NONE;
	client->shop = shop;
	client->barberID = 0;
	client->num_trips_to_barber = num_trips_to_barber;
	client->requests = 0;
	client->benchesPosition = -1;
	client->chairPosition = -1;
	client->basinPosition = -1;
	client->internal = NULL;
	client->logId = register_logger((char*)("Client:"), line ,column,
			num_lines_client(), num_columns_client(), NULL);
}

void term_client(Client* client)
{
	require (client != NULL, "client argument required");

	if (client->internal != NULL)
	{
		mem_free(client->internal);
		client->internal = NULL;
	}
}

void log_client(Client* client)
{
	require (client != NULL, "client argument required");

	spend(random_int(global->MIN_VITALITY_TIME_UNITS, global->MAX_VITALITY_TIME_UNITS));
	send_log(client->logId, to_string_client(client));
}

void* main_client(void* args)
{
	Client* client = (Client*)args;
	require (client != NULL, "client argument required");
	life(client);
	return NULL;
}

static void life(Client* client)
{
	require (client != NULL, "client argument required");

	int i = 0;
	notify_client_birth(client);
	while(i < client->num_trips_to_barber)
	{
		wandering_outside(client);
		if (vacancy_in_barber_shop(client))
		{
			select_requests(client);
			wait_its_turn(client);
			rise_from_client_benches(client);
			wait_all_services_done(client);
			i++;
		}
	}
	notify_client_death(client);

	// ALTERNATIVE LIFE -> check justification in select_requests() method
//	notify_client_birth(client);
//	while(i < client->num_trips_to_barber)
//	{
//		wandering_outside(client);
//		select_requests(client);
//		if (vacancy_in_barber_shop(client))
//		{
//			wait_its_turn(client);
//			rise_from_client_benches(client);
//			wait_all_services_done(client);
//		}
//		i++;
//	}
//	notify_client_death(client);
}

static void notify_client_birth(Client* client)
{
	/**
	 * 1: (if necessary) inform simulation that a new client begins its existence.
	 **/

	require (client != NULL, "client argument required");

	log_client(client);
}

static void notify_client_death(Client* client)
{
	/**
	 * 1: (if necessary) inform simulation that a new client ceases its existence.
	 **/

	require (client != NULL, "client argument required");

	log_client(client);
}

static void wandering_outside(Client* client)
{
	/**
	 * 1: set the client state to WANDERING_OUTSIDE
	 * 2. random a time interval [global->MIN_OUTSIDE_TIME_UNITS, global->MAX_OUTSIDE_TIME_UNITS]
	 **/

	require (client != NULL, "client argument required");

	client->state = WANDERING_OUTSIDE;

	spend(random_int(global->MIN_OUTSIDE_TIME_UNITS, global->MAX_OUTSIDE_TIME_UNITS));

	log_client(client);
}

static int vacancy_in_barber_shop(Client* client)
{
	/**
	 * 1: set the client state to WAITING_BARBERSHOP_VACANCY
	 * 2: check if there is an empty seat in the client benches (at this instant, later on it may fail)
	 **/

	require (client != NULL, "client argument required");

	client->state = WAITING_BARBERSHOP_VACANCY;
	log_client(client);

	int numSeats = num_available_benches_seats(client_benches(client->shop));

	return numSeats;
}

static void select_requests(Client* client)
{
	/**
	 * 1: set the client state to SELECTING_REQUESTS
	 * 2: choose a random combination of requests
	 **/

	require (client != NULL, "client argument required");

	client->state = SELECTING_REQUESTS;

	int res = 0;

	while (res == 0){
		int h = ( (int)((double)rand() / (double)RAND_MAX *100) < global->PROB_REQUEST_HAIRCUT) * HAIRCUT_REQ;
		int w = ( (int)((double)rand() / (double)RAND_MAX *100) < global->PROB_REQUEST_WASHHAIR) * WASH_HAIR_REQ;
		int s = ( (int)((double)rand() / (double)RAND_MAX *100) < global->PROB_REQUEST_SHAVE) * SHAVE_REQ;
		res = h+w+s;
	}

	// the client is forced to choose a service.
	// should he leave if no requests are selected? if so, this method should be
	// called immediately after wandering_outside(), and the count of
	// num_trips_to_barber after if statement, also the while statement above
	// would not be necessary.
	// This way, if the probability of all requests is set to zero in ./simulation
	// clients don't ever choose a request, but they will eventually die and the
	// program will terminate.
	// Without this, if probability 0 is selected when running ./simulation,
	// will result in all clients waiting forever.
	// - check Alternative Client Life above
	// *** Thanks to Filipe Vale (NMEC 85055) for the insight ***

	client->requests = res;

	log_client(client);
}

static void wait_its_turn(Client* client)
{
	/**
	 * 1: set the client state to WAITING_ITS_TURN
	 * 2: enter barbershop (if necessary waiting for an empty seat)
	 * 3. "handshake" with assigned barber (greet_barber)
	 **/

	require (client != NULL, "client argument required");

	client->state = WAITING_ITS_TURN;

	int clientSeat = enter_barber_shop(client->shop, client->id, client->requests);
	client->benchesPosition = clientSeat;

	int barberId = greet_barber(client->shop, client->id);
	client->barberID = barberId;

	log_client(client);
}

static void rise_from_client_benches(Client* client)
{
	/**
	 * 1: (exactly what the name says)
	 **/

	require (client != NULL, "client argument required");
	require (seated_in_client_benches(client_benches(client->shop), client->id), concat_3str("client ",int2str(client->id)," not seated in benches"));

	rise_client_benches(client_benches(client->shop) , client->benchesPosition, client->id);

	client->benchesPosition = -1;

	log_client(client);
}

static void wait_all_services_done(Client* client)
{
	/**
	 * Expect the realization of one request at a time, until all requests are fulfilled.
	 * For each request:
	 * 1: set the client state to WAITING_SERVICE
	 * 2: wait_service_from_barber from barbershop
	 * 3: set the client state to WAITING_SERVICE_START
	 * 4: sit in proper position in destination (chair/basin depending on the service selected)
	 * 5: set the client state to the active service
	 * 6: rise from destination
	 * At the end the client must leave the barber shop
	 **/

	require (client != NULL, "client argument required");

	while (client->state != DONE)
	{

		client->state = WAITING_SERVICE;
		Service service = wait_service_from_barber(client->shop, client->id);
		client->state = WAITING_SERVICE_START;

		/* Request to cut hair OR request to shave*/
		if (service.request == HAIRCUT_REQ)
		{
			update_client_with_service(client, service);

			BarberChair* chair = barber_chair(client->shop, service.pos);
			sit_in_barber_chair(chair, client->id);
			wait_for_barber_chair_service_completion(chair);

			if (!(client->requests & SHAVE_REQ)){
				rise_from_barber_chair(chair, client->id);
				client->chairPosition = -1;
			}

			log_client(client);
		}

		else if (service.request == SHAVE_REQ){

			update_client_with_service(client, service);

			BarberChair* chair =  barber_chair(client->shop, service.pos);
			if (!(client->requests & HAIRCUT_REQ)){
				sit_in_barber_chair(chair, client->id);
			}

			// wait_for_service_start() is only used in this shave requests,
			// because barber needs to reset barberChair completion. Before starting
			// shaving, otherwise client will verify completion twice and rise
			// immediately
			wait_for_service_start(client->shop, client->id);
			wait_for_barber_chair_service_completion(chair);

			rise_from_barber_chair(chair, client->id);
			client->chairPosition = -1;

			log_client(client);
		}

		else if (service.request == WASH_HAIR_REQ)
		{
			update_client_with_service(client, service);

			Washbasin* basin =  washbasin(client->shop, service.pos);
			sit_in_washbasin(basin, client->id);

			wait_for_washbasin_service_completion(basin);

			rise_from_washbasin(basin, client->id);
			client->basinPosition = -1;

			log_client(client);
		}

		else {
			update_client_with_service(client, service);
		}

	}

	leave_barber_shop(client->shop, client->id);

	log_client(client);
}

static void update_client_with_service(Client* client, Service service)
{
	require (client != NULL, "client argument required");
	require (!(service.barberChair && service.washbasin), "only one service must be active");

	client->chairPosition = -1;
	client->basinPosition = -1;
	client->state = NONE;

	if (service.barberChair){
		client->chairPosition = service.pos;
		if (service.request == HAIRCUT_REQ){
			client->state = HAVING_A_HAIRCUT;
		}
		else if(service.request == SHAVE_REQ){
			client->state = HAVING_A_SHAVE;
		}
	}
	else if (service.washbasin){
		client->basinPosition = service.pos;
		client->state = HAVING_A_HAIR_WASH;
	}
	else{
		client->barberID = 0;
		client->requests = 0;
		client->state = DONE;
	}
}


static char* to_string_client(Client* client)
{
	require (client != NULL, "client argument required");

	if (client->internal == NULL)
		client->internal = (char*)mem_alloc(skel_length + 1);

	char requests[4];
	requests[0] = (client->requests & HAIRCUT_REQ) ?   'H' : ':',
			requests[1] = (client->requests & WASH_HAIR_REQ) ? 'W' : ':',
					requests[2] = (client->requests & SHAVE_REQ) ?     'S' : ':',
							requests[3] = '\0';

	char* pos = (char*)"-";
	if (client->chairPosition >= 0)
		pos = int2nstr(client->chairPosition+1, 1);
	else if (client->basinPosition >= 0)
		pos = int2nstr(client->basinPosition+1, 1);

	return gen_boxes(client->internal, skel_length, skel,
			int2nstr(client->id, 2),
			client->barberID > 0 ? int2nstr(client->barberID, 2) : "--",
					requests, stateText[client->state], pos);
}

