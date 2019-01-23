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
	printf("++++++++++++++++++++++++++++++CLIENT IS BORN, comes %d\n", client->num_trips_to_barber);
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
	printf("+++++++++++++++++++++++++++++CLIENT DIES! MUAHAHAH\n");
	notify_client_death(client);
}

static void notify_client_birth(Client* client)
{
	require (client != NULL, "client argument required");

	/** TODO:
	 * 1: (if necessary) inform simulation that a new client begins its existence.
	 **/



	log_client(client);
}

static void notify_client_death(Client* client)
{
	/** TODO:
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
	 *  TODO:
	 **/

	require (client != NULL, "client argument required");

	client->state = WANDERING_OUTSIDE;

	spend(random_int(global->MIN_OUTSIDE_TIME_UNITS, global->MAX_OUTSIDE_TIME_UNITS));
	printf("--------------------------------------------CLIENT LIFE - WANDERING OUTSIDE\n");
	log_client(client);
}

static int vacancy_in_barber_shop(Client* client)
{
	/**
	 * 1: set the client state to WAITING_BARBERSHOP_VACANCY
	 * 2: check if there is an empty seat in the client benches (at this instant, later on it may fail)
	 * TODO:
	 **/

	require (client != NULL, "client argument required");

	client->state = WAITING_BARBERSHOP_VACANCY;

	int res = 0;
	res = num_available_benches_seats(&(client->shop->clientBenches)); // any value greater than 0 is True

	log_client(client);
	printf("--------------------------------------------CLIENT LIFE - VACANCY IN BARBER SHOP\n");
	return res;
}

static void select_requests(Client* client)
{
	/**
	 * 1: set the client state to SELECTING_REQUESTS
	 * 2: choose a random combination of requests
	 * TODO:
	 **/

	require (client != NULL, "client argument required");

	client->state = SELECTING_REQUESTS;

	int res = 0;
	while (res == 0) {
		printf("%d\n", client->id);
		int h = ( ((rand() / (RAND_MAX))*100) <= global->PROB_REQUEST_HAIRCUT) * HAIRCUT_REQ;
		int w = ( ((rand() / (RAND_MAX))*100) <= global->PROB_REQUEST_WASHHAIR) * WASH_HAIR_REQ;
		int s = ( ((rand() / (RAND_MAX))*100) <= global->PROB_REQUEST_SHAVE) * SHAVE_REQ;
		res = h+w+s;
	}
	client->requests = res;
	printf("--------------------------------------------CLIENT LIFE - SELECT REQUESTS\n");
	log_client(client);
}

static void wait_its_turn(Client* client)
{
	/**
	 * 1: set the client state to WAITING_ITS_TURN
	 * 2: enter barbershop (if necessary waiting for an empty seat)
	 * 3. "handshake" with assigned barber (greet_barber)
	 * TODO:
	 **/

	require (client != NULL, "client argument required");

	//* 1: set the client state to WAITING_ITS_TURN
	client->state = WAITING_ITS_TURN;

	mutex_lock(&client->shop->barberShopMutex);

	//* 2: enter barbershop (if necessary waiting for an empty seat)
	while(num_available_benches_seats(&(client->shop->clientBenches)) == 0){
		cond_wait(&client->shop->clientSeatAvailable, &client->shop->barberShopMutex);
	}

	int clientSeat = enter_barber_shop(client->shop, client->id, client->requests);
	client->benchesPosition = clientSeat;

	cond_broadcast(&client->shop->clientWaiting);

	//* 3. "handshake" with assigned barber (greet_barber)
	while(no_message_available(&(client->shop->commLine), client->id)){
		cond_wait(&client->shop->messageAvailable, &client->shop->barberShopMutex);
	}

	int barberId;
	barberId = greet_barber(client->shop, client->id);
	client->barberID = barberId;

	mutex_unlock(&client->shop->barberShopMutex);
	printf("--------------------------------------------CLIENT LIFE - WAIT ITS TURN\n");
	log_client(client);
}

static void rise_from_client_benches(Client* client)
{
	/**
	 * 1: (exactly what the name says)
	 * TODO:
	 **/

	require (client != NULL, "client argument required");
	require (seated_in_client_benches(client_benches(client->shop), client->id), concat_3str("client ",int2str(client->id)," not seated in benches"));

	// Client stands on his/her feet
	rise_client_benches(&(client->shop->clientBenches) , client->benchesPosition, client->id);
	client->benchesPosition = -1;
	client->chairPosition = -1;
	client->basinPosition = -1;
	cond_signal(&client->shop->clientSeatAvailable);
	printf("--------------------------------------------CLIENT LIFE - RISE FROM CLIENT BENCH\n");
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
	 * TODO
	 * At the end the client must leave the barber shop
	 **/

	require (client != NULL, "client argument required");

	mutex_lock(&client->shop->barberShopMutex);

	while (client->state != DONE)
	{

		/* Waiting for service */
		printf("--------------------------client - in waiting service\n");
		client->state = WAITING_SERVICE;
		while(no_message_available(&client->shop->commLine, client->id)){
			cond_wait(&client->shop->messageAvailable, &client->shop->barberShopMutex);
		}
		Service service = wait_service_from_barber(client->shop, client->id);
		printf("service %d\n", service.request);
		client->state = WAITING_SERVICE_START;

		/* Request to cut hair OR Resquest to shave*/
		if (service.request == HAIRCUT_REQ)
		{
			BarberChair* chair = client->shop->barberChair+service.pos;
			sit_in_barber_chair(chair, client->id);
			update_client_with_service(client, service);
			cond_signal(&client->shop->clientSatInBarberChair);

			while(!barber_chair_service_finished(chair)){
				cond_wait(&client->shop->barberChairServiceFinished, &client->shop->barberShopMutex);
			}

			if (!(client->requests & SHAVE_REQ)){
				rise_from_barber_chair(chair, client->id);
				cond_signal(&client->shop->clientRoseFromBarberChair);
			}

			client->shop->barberChair[client->chairPosition].completionPercentage = 0;
			printf("--------------------------client - had haircut\n");
			log_client(client);
		}

		else if (service.request == SHAVE_REQ){
			printf("--------------------------client - gonna have a shave\n");
			BarberChair* chair;
			if (client->requests & HAIRCUT_REQ){
				chair = client->shop->barberChair+service.pos;
				update_client_with_service(client, service);
				cond_signal(&client->shop->clientReadyForShave);
			}
			else{
				chair = client->shop->barberChair+service.pos;
				sit_in_barber_chair(chair, client->id);
				update_client_with_service(client, service);
				cond_signal(&client->shop->clientSatInBarberChair);
			}

			while(!barber_chair_service_finished(chair)){
				cond_wait(&client->shop->barberChairServiceFinished, &client->shop->barberShopMutex);
			}

			rise_from_barber_chair(chair, client->id);
			client->chairPosition = -1;
			cond_signal(&client->shop->clientRoseFromBarberChair);
			printf("--------------------------client - had a shave\n");
			log_client(client);
		}

		else if (service.request == WASH_HAIR_REQ)
		{
			printf("--------------------------client - gonna have a wash\n");
			Washbasin* basin = client->shop->washbasin+service.pos;
			sit_in_washbasin(basin, client->id);
			update_client_with_service(client, service);
			cond_signal(&client->shop->clientSatInWashbasin);
			while (!washbasin_service_finished(basin)){
				cond_wait(&client->shop->washbasinServiceFinished, &client->shop->barberShopMutex);
			}
			rise_from_washbasin(basin, client->id);
			cond_signal(&client->shop->clientRoseFromWashbasin);
			printf("--------------------------client - had a wash\n");
			log_client(client);
		}

		else{
			update_client_with_service(client, service);
		}

	}

	leave_barber_shop(client->shop, client->id);
	// 'E necessário repôr os valores originais do cliente???

	printf("--------------------------------------------CLIENT LIFE - WAIT ALL SERVICES DONE\n");

	mutex_unlock(&client->shop->barberShopMutex);

	log_client(client); // more than one in proper places!!!
}

static void update_client_with_service(Client* client, Service service)
{
	require (client != NULL, "client argument required");
	require (!(service.barberChair && service.washbasin), "only one service must be active");

	client->chairPosition = -1;
	client->basinPosition = -1;
	if (service.barberChair){
		client->chairPosition = service.pos;
	}
	else if (service.washbasin){
		client->basinPosition = service.pos;
	}
	else{
		client->state = DONE;
	}

	client->state = -1;
	if (service.request == HAIRCUT_REQ){
		client->state = HAVING_A_HAIRCUT;
	}
	else if(service.request == SHAVE_REQ){
		client->state = HAVING_A_SHAVE;
	}
	else if(service.request == WASH_HAIR_REQ){
		client->state = HAVING_A_HAIR_WASH;
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

