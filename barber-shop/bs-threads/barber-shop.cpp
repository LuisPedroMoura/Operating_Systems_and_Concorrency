#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "include/thread.h"
#include "dbc.h"
#include "utils.h"
#include "box.h"
#include "logger.h"
#include "global.h"
#include "barber-shop.h"
#include "communication-line.h"


/* TODO: take a careful look to all the non static (public) functions, to check
 * if a proper synchronization is needed.
 */


/* TODO: change here this file to your needs */



static const int skel_length = 10000;
static char skel[skel_length];

static char* to_string_barber_shop(BarberShop* shop);

int num_lines_barber_shop(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	return 1+3+num_lines_barber_chair()+num_lines_tools_pot()+3*shop->numClientBenches+1;
}

int num_columns_barber_shop(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	return w.ws_col == 0 ? 80 : w.ws_col;
}

void init_barber_shop(BarberShop* shop, int num_barbers, int num_chairs,
		int num_scissors, int num_combs, int num_razors, int num_basins,
		int num_client_benches_seats, int num_client_benches)
{
	require (shop != NULL, "shop argument required");
	require (num_barbers > 0 && num_barbers <= MAX_BARBERS, concat_5str("invalid number of barbers (", int2str(num_barbers), " not in [1,", int2str(MAX_BARBERS), "])"));
	require (num_chairs > 0 && num_chairs <= MAX_BARBER_CHAIRS, concat_5str("invalid number of chairs (", int2str(num_chairs), " not in [1,", int2str(MAX_BARBER_CHAIRS), "])"));
	require (num_scissors > 0 && num_scissors <= MAX_NUM_TOOLS, concat_5str("invalid number of scissors (", int2str(num_scissors), " not in [1,", int2str(MAX_NUM_TOOLS), "])"));
	require (num_combs > 0 && num_combs <= MAX_NUM_TOOLS, concat_5str("invalid number of combs (", int2str(num_combs), " not in [1,", int2str(MAX_NUM_TOOLS), "])"));
	require (num_razors > 0 && num_razors <= MAX_NUM_TOOLS, concat_5str("invalid number of razors (", int2str(num_razors), " not in [1,", int2str(MAX_NUM_TOOLS), "])"));
	require (num_basins > 0 && num_basins <= MAX_WASHBASINS, concat_5str("invalid number of washbasins (", int2str(num_basins), " not in [1,", int2str(MAX_WASHBASINS), "])"));
	require (num_client_benches_seats > 0 && num_client_benches_seats <= MAX_CLIENT_BENCHES_SEATS, concat_5str("invalid number of client benches seats (", int2str(num_client_benches_seats), " not in [1,", int2str(MAX_CLIENT_BENCHES_SEATS), "])"));
	require (num_client_benches > 0 && num_client_benches <= num_client_benches_seats, concat_5str("invalid number of client benches (", int2str(num_client_benches), " not in [1,", int2str(num_client_benches_seats), "])"));

	shop->numBarbers = num_barbers;
	shop->numChairs = num_chairs;
	shop->numScissors = num_scissors;
	shop->numCombs = num_combs;
	shop->numRazors = num_razors;
	shop->numWashbasins = num_basins;
	shop->numClientBenchesSeats = num_client_benches_seats;
	shop->numClientBenches = num_client_benches;
	shop->numClientsInside = 0;
	shop->clientsAlive = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
		shop->clientsInside[i] = 0;
	shop->opened = 1;

	gen_rect(skel, skel_length, num_lines_barber_shop(shop), num_columns_barber_shop(shop), 0xF, 1);
	gen_overlap_boxes(skel, 0, skel,
			(char*)" BARBER SHOP ", 0, 2,
			(char*)" Idle Barbers:", 2, 1,
			(char*)"Barber Chairs:", 2+3, 1,
			(char*)"Washbasins:", 3+3+num_lines_barber_chair(), num_columns_tools_pot()+3,
			(char*)" Waiting Room:", 2+3+num_lines_barber_chair()+num_lines_tools_pot(), 1,
			(char*)"+          +", num_lines_barber_shop(shop)-1, num_columns_barber_shop(shop)-15, NULL);

	shop->internal = (char*)mem_alloc(skel_length + 1);

	shop->logId = register_logger((char*)"Barber Shop:", 0, 0, num_lines_barber_shop(shop), num_columns_barber_shop(shop), NULL);

	// init components:
	init_barber_bench(&shop->barberBench, num_barbers, 0, 1, 16);
	for (int i = 0; i < num_chairs; i++)
		init_barber_chair(shop->barberChair+i, i+1, 1+3, 16+i*(num_columns_barber_chair()+2));
	init_tools_pot(&shop->toolsPot, num_scissors, num_combs, num_razors, 1+3+num_lines_barber_chair(), 1);
	for (int i = 0; i < num_basins; i++)
		init_washbasin(shop->washbasin+i, i+1, 1+3+num_lines_barber_chair(), num_columns_tools_pot()+3+11+1+i*(num_columns_washbasin()+2));
	init_client_benches(&shop->clientBenches, num_client_benches_seats, num_client_benches, 1+3+num_lines_barber_chair()+num_lines_tools_pot(), 16);
	init_communication_line(&shop->commLine);

	/* Barber chair semaphore init */
	require(sem_init(&shop->availableBarberChair, 0, global->NUM_BARBER_CHAIRS) == 0, "Barber Chair semaphore init error");
	require(sem_init(&shop->accessBarberChair, 0, 1) == 0, "Barber Chair semaphore init error");

	/* Washbasin semaphore init */
	require (sem_init(&shop->availableWashbasin, 0, global->NUM_WASHBASINS) == 0, "Washbasin semaphore error");
	require (sem_init(&shop->accessWashbasin, 0, 1) == 0, "Washbasin semaphore error");

	shop->shopFloorMutex = PTHREAD_MUTEX_INITIALIZER;
}

void term_barber_shop(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	// term components:
	term_client_benches(&shop->clientBenches);
	for (int i = 0; i < shop->numWashbasins; i++)
		term_washbasin(shop->washbasin+i);
	term_tools_pot(&shop->toolsPot);
	for (int i = 0; i < shop->numChairs; i++)
		term_barber_chair(shop->barberChair+i);
	term_barber_bench(&shop->barberBench);

	mem_free(shop->internal);
}

void show_barber_shop(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	log_barber_shop(shop);
	log_barber_bench(&shop->barberBench);
	for (int i = 0; i < shop->numChairs; i++)
		log_barber_chair(shop->barberChair+i);
	log_tools_pot(&shop->toolsPot);
	for (int i = 0; i < shop->numWashbasins; i++)
		log_washbasin(shop->washbasin+i);
	log_client_benches(&shop->clientBenches);
}

void log_barber_shop(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	spend(random_int(global->MIN_VITALITY_TIME_UNITS, global->MAX_VITALITY_TIME_UNITS));
	send_log(shop->logId, to_string_barber_shop(shop));
}

int valid_barber_chair_pos(BarberShop* shop, int pos)
{
	require (shop != NULL, "shop argument required");

	return pos >= 0 && pos < shop->numChairs;
}

BarberChair* barber_chair(BarberShop* shop, int pos)
{
	require (shop != NULL, "shop argument required");
	require (valid_barber_chair_pos(shop, pos), concat_5str("invalid chair position (", int2str(pos), " not in [0,", int2str(shop->numChairs), "[)"));

	return shop->barberChair + pos;
}

ToolsPot* tools_pot(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	return &shop->toolsPot;
}

int valid_washbasin_pos(BarberShop* shop, int pos)
{
	require (shop != NULL, "shop argument required");

	return pos >= 0 && pos < shop->numWashbasins;
}

Washbasin* washbasin(BarberShop* shop, int pos)
{
	require (shop != NULL, "shop argument required");
	require (valid_washbasin_pos(shop, pos), concat_5str("invalid basin position (", int2str(pos), " not in [0,", int2str(shop->numWashbasins), "[)"));

	return shop->washbasin + pos;
}

BarberBench* barber_bench(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	return &shop->barberBench;
}

ClientBenches* client_benches(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	return &shop->clientBenches;
}

int reserve_empty_barber_chair(BarberShop* shop, int barberID)
{
	require (shop != NULL, "shop argument required");
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));

	sem_down(&shop->availableBarberChair);
	sem_down(&shop->accessBarberChair);
	int chairPos = reserve_random_empty_barber_chair(shop, barberID);

	return chairPos;
}

int reserve_random_empty_barber_chair(BarberShop* shop, int barberID)
{
	/**
	 * function called from a barber, when reserving a empty barber chair
	 **/

	require (shop != NULL, "shop argument required");
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));
	require (num_available_barber_chairs(shop) > 0, "barber chair not available");

	int r = random_int(1, num_available_barber_chairs(shop));
	int res;
	for(res = 0; r > 0 && res < shop->numChairs ; res++){
		if (empty_barber_chair(shop->barberChair+res)){
			r--;
		}
	}
	res--;

	reserve_barber_chair(shop->barberChair+res, barberID);
	sem_up(&shop->accessBarberChair);
	ensure (res >= 0 && res < shop->numChairs, "");

	return res;
}

int num_available_barber_chairs(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	int res = 0;
	for(int pos = 0; pos < shop->numChairs ; pos++)
		if (empty_barber_chair(shop->barberChair+pos))
			res++;

	return res;
}

void release_barber_barberchair(BarberShop* shop, int barberID, int barberPos)
{
	require (shop != NULL, "shop argument required");
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));

	release_barber_chair(&shop->barberChair[barberPos], barberID);
	require ( num_available_barber_chairs(shop) >= 0, "available barber chairs went down to zero");
	sem_up(&shop->availableBarberChair);
}

int reserve_empty_washbasin(BarberShop* shop, int barberID)
{
	require (shop != NULL, "shop argument required");
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));

	sem_down(&shop->availableWashbasin);
	sem_down(&shop->accessWashbasin);
	int chairPos = reserve_random_empty_washbasin(shop, barberID);

	return chairPos;
}

int num_available_washbasin(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	int res = 0;
	for(int pos = 0; pos < shop->numWashbasins ; pos++)
		if (empty_washbasin(shop->washbasin+pos))
			res++;

	return res;
}

int reserve_random_empty_washbasin(BarberShop* shop, int barberID)
{
	/**
	 * function called from a barber, when reserving a empty washbasin
	 **/

	require (shop != NULL, "shop argument required");
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));
	require (num_available_washbasin(shop) > 0, "washbasin not available");

	int r = random_int(1, num_available_washbasin(shop));
	int res;
	for(res = 0; r > 0 && res < shop->numWashbasins ; res++){
		if (empty_washbasin(shop->washbasin+res)){
			r--;
		}
	}
	res--;
	reserve_washbasin(shop->washbasin+res, barberID);
	sem_up(&shop->accessWashbasin);
	ensure (res >= 0 && res < shop->numWashbasins, "");

	return res;
}

void release_barber_washbasin(BarberShop* shop, int barberID, int barberPos)
{
	require (shop != NULL, "shop argument required");
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));

	release_washbasin(&shop->washbasin[barberPos], barberID);
	sem_up(&shop->availableWashbasin);
}

int is_client_inside(BarberShop* shop, int clientID)
{
	require (shop != NULL, "shop argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	int res = 0;

	for(int i = 0; !res && i < shop->numClientsInside; i++)
		res = (clientID == shop->clientsInside[i]);

	return res;
}

Service wait_service_from_barber(BarberShop* shop, int clientID)
{
	/**
	 * function called from a client, expecting to be informed of the next Service to be provided by a barber
	 **/

	require (shop != NULL, "shop argument required");
	require (clientID > 0, concat_3str("invalid barber id (", int2str(clientID), ")"));

	Message message = read_message(&shop->commLine, clientID);
	Service service = message.service;

	return service;
}

void inform_client_on_service(BarberShop* shop, Service service)
{
	/**
	 * function called from a barber, expecting to inform a client of its next service
	 **/

	require (shop != NULL, "shop argument required");
	require (service.barberChair || service.washbasin, "only one request per service can be active");

	Message message = write_message(service);
	message.newMessage = 1;
	send_message(&shop->commLine, message);
}

void inform_client_on_service_start(BarberShop* shop, Service service)
{
	/**
	 * function called from a barber, expecting to inform a client of its next service
	 **/

	require (shop != NULL, "shop argument required");
	require (service.barberChair || service.washbasin, "only one request per service can be active");

	inform_client_on_service(shop,service);
}

void wait_for_service_start(BarberShop* shop, int clientID)
{
	/**
	 * function called from a client, expecting to receive its barber's ID
	 **/

	require (shop != NULL, "shop argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	read_message(&(shop->commLine), clientID);
}

void client_done(BarberShop* shop, int clientID)
{
	/**
	 * function called from a barber, notifying a client that all its services are done
	 **/

	require (shop != NULL, "shop argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	Message message = empty_message(clientID);
	send_message(&shop->commLine, message);
}

void wait_for_client_to_sit_in_washbasin(BarberShop* shop, int basinPos)
{
	require (shop != NULL, "shop argument required");

	wait_client_to_sit_in_washbasin(&shop->washbasin[basinPos]);
}

int enter_barber_shop(BarberShop* shop, int clientID, int request)
{
	/**
	 * Function called from a client when entering the barbershop
	 **/

	require (shop != NULL, "shop argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));
	require (request > 0 && request < 8, concat_3str("invalid request (", int2str(request), ")"));

	// client inside the shop, seat might not be available, but random_sit_in_client_benches assures the client waits
	//require (num_available_benches_seats(client_benches(shop)) > 0, "empty seat not available in client benches");

	mutex_lock(&shop->shopFloorMutex);
	require (!is_client_inside(shop, clientID), concat_3str("client ", int2str(clientID), " already inside barber shop"));
	mutex_unlock(&shop->shopFloorMutex);

	int res = random_sit_in_client_benches(&shop->clientBenches, clientID, request);

	mutex_lock(&shop->shopFloorMutex);
	shop->clientsInside[shop->numClientsInside++] = clientID;
	mutex_unlock(&shop->shopFloorMutex);

	return res;
}

void leave_barber_shop(BarberShop* shop, int clientID)
{
	/**
	 * Function called from a client when leaving the barbershop
	 **/

	require (shop != NULL, "shop argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	mutex_lock(&shop->shopFloorMutex);
	require (is_client_inside(shop, clientID), concat_3str("client ", int2str(clientID), " already inside barber shop"));

	int i;
	for(i = 0; shop->clientsInside[i] != clientID; i++)
		;
	shop->numClientsInside--;
	check (shop->clientsInside[i] == clientID, "");
	for(; i < shop->numClientsInside; i++)
		shop->clientsInside[i] = shop->clientsInside[i+1];

	mutex_unlock(&shop->shopFloorMutex);
}

void receive_and_greet_client(BarberShop* shop, int barberID, int clientID)
{
	/**
	 * function called from a barber, when receiving a new client
	 * it must send the barber ID to the client
	 **/

	require (shop != NULL, "shop argument required");
	require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	Service service;
	service.barberID = barberID;
	service.clientID = clientID;
	Message message = write_message(service);
	send_message(&(shop->commLine), message);
}

int greet_barber(BarberShop* shop, int clientID)
{
	/**
	 * function called from a client, expecting to receive its barber's ID
	 **/

	require (shop != NULL, "shop argument required");
	require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

	Message message = read_message(&(shop->commLine), clientID);
	int barberID = message.service.barberID;

	return barberID;
}

int shop_opened(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");

	return shop->opened;
}

void close_shop(BarberShop* shop)
{
	require (shop != NULL, "shop argument required");
	require (shop_opened(shop), "barber shop already closed");

	notify_barbers_all_clients_are_done(&shop->clientBenches);
}

void sem_up(sem_t* semaphore){
	sem_post(semaphore);
}
void sem_down(sem_t* semaphore){
	sem_wait(semaphore);
}

static char* to_string_barber_shop(BarberShop* shop)
{
	return gen_boxes(shop->internal, skel_length, skel);
}
