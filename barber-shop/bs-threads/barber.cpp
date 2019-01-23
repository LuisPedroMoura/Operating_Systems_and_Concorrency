#include <stdlib.h>
#include "./include/thread.h"
#include "dbc.h"
#include "global.h"
#include "utils.h"
#include "box.h"
#include "timer.h"
#include "logger.h"
#include "barber-shop.h"
#include "barber.h"
#include "communication-line.h"

enum State
{
	NONE = 0,
	CUTTING,
	SHAVING,
	WASHING,
	WAITING_CLIENTS,
	WAITING_BARBER_SEAT,
	WAITING_WASHBASIN,
	REQ_SCISSOR,
	REQ_COMB,
	REQ_RAZOR,
	DONE
};

#define State_SIZE (DONE - NONE + 1)

static const char* stateText[State_SIZE] =
{
		"---------",
		"CUTTING  ",
		"SHAVING  ",
		"WASHING  ",
		"W CLIENT ", // Waiting for client
		"W SEAT   ", // Waiting for barber seat
		"W BASIN  ", // Waiting for washbasin
		"R SCISSOR", // Request a scissor
		"R COMB   ", // Request a comb
		"R RAZOR  ", // Request a razor
		"DONE     ",
};

static const char* skel = 
		"@---+---+---@\n"
		"|B##|C##|###|\n"
		"+---+---+-+-+\n"
		"|#########|#|\n"
		"@---------+-@";
static int skel_length = num_lines_barber()*(num_columns_barber()+1)*4; // extra space for (pessimistic) utf8 encoding!

static void life(Barber* barber);

static void sit_in_barber_bench(Barber* barber);
static void wait_for_client(Barber* barber);
static int work_available(Barber* barber);
static void rise_from_barber_bench(Barber* barber);
static void process_resquests_from_client(Barber* barber);
static void release_client(Barber* barber);
static void done(Barber* barber);
static void process_haircut_request(Barber* barber);
static void process_wash_hair_request(Barber* barber);
static void process_shave_request(Barber* barber);

static char* to_string_barber(Barber* barber);



size_t sizeof_barber()
{
	return sizeof(Barber);
}

int num_lines_barber()
{
	return string_num_lines((char*)skel);
}

int num_columns_barber()
{
	return string_num_columns((char*)skel);
}

void init_barber(Barber* barber, int id, BarberShop* shop, int line, int column)
{
	require (barber != NULL, "barber argument required");
	require (id > 0, concat_3str("invalid id (", int2str(id), ")"));
	require (shop != NULL, "barber shop argument required");
	require (line >= 0, concat_3str("Invalid line (", int2str(line), ")"));
	require (column >= 0, concat_3str("Invalid column (", int2str(column), ")"));

	barber->id = id;
	barber->state = NONE;
	barber->shop = shop;
	barber->clientID = 0;
	barber->reqToDo = 0;
	barber->benchPosition = -1;
	barber->chairPosition = -1;
	barber->basinPosition = -1;
	barber->tools = 0;
	barber->internal = NULL;
	barber->logId = register_logger((char*)("Barber:"), line ,column,
			num_lines_barber(), num_columns_barber(), NULL);
}

void term_barber(Barber* barber)
{
	require (barber != NULL, "barber argument required");

	if (barber->internal != NULL)
	{
		mem_free(barber->internal);
		barber->internal = NULL;
	}
}

void log_barber(Barber* barber)
{
	require (barber != NULL, "barber argument required");

	spend(random_int(global->MIN_VITALITY_TIME_UNITS, global->MAX_VITALITY_TIME_UNITS));
	send_log(barber->logId, to_string_barber(barber));
}

void* main_barber(void* args)
{
	Barber* barber = (Barber*)args;
	require (barber != NULL, "barber argument required");
	life(barber);
	return NULL;
}

static void life(Barber* barber)
{
	require (barber != NULL, "barber argument required");

	sit_in_barber_bench(barber);
	wait_for_client(barber);
	while(work_available(barber)) // no more possible clients and closes barbershop
	{
		rise_from_barber_bench(barber);
		process_resquests_from_client(barber);
		release_client(barber);
		sit_in_barber_bench(barber);
		wait_for_client(barber);
	}
	done(barber);
}

static void sit_in_barber_bench(Barber* barber)
{
	/**
	 * 1: sit in a random empty seat in barber bench (always available)
	 ** TODO:
	 **/
	require (barber != NULL, "barber argument required");
	require (num_seats_available_barber_bench(barber_bench(barber->shop)) > 0, "seat not available in barber shop");
	require (!seated_in_barber_bench(barber_bench(barber->shop), barber->id), "barber already seated in barber shop");

	//mutex_lock(&barber->shop->barberBenchMutex);

	int seatPos = random_sit_in_barber_bench(barber_bench(barber->shop), barber->id);
	barber->benchPosition = seatPos;

	//mutex_unlock(&barber->shop->barberBenchMutex);
	//printf("--------------------------------------------BARBER LIFE - SIT IN BARBER BENCH\n");
	log_barber(barber);
}

static void wait_for_client(Barber* barber)
{
	/**
	 * 1: set the barber state to WAITING_CLIENTS
	 * 2: get next client from client benches (if empty, wait) (also, it may be required to check for simulation termination)
	 * 3: receive and greet client (receive its requested services, and give back the barber's id)
	 ** TODO:
	 **/

	require (barber != NULL, "barber argument required");

	barber->state = WAITING_CLIENTS;
	//printf("--------------------------------------------BARBER LIFE - BEGIN WAIT FOR CLIENT\n");

	//mutex_lock(&barber->shop->barberShopMutex);

	if (barber->shop->opened){

		mutex_lock(&barber->shop->clientsBenchMutex);

		while (no_more_clients(&(barber->shop->clientBenches)) ){
			//printf("--------------------------------------------BARBER - waiting for client\n");
			cond_wait(&barber->shop->clientWaiting, &barber->shop->clientsBenchMutex);
		}

		RQItem requests = next_client_in_benches(&(barber->shop->clientBenches));
		barber->reqToDo = requests.request;
		barber->clientID = requests.clientID;
		
		mutex_unlock(&barber->shop->clientsBenchMutex);
		
		receive_and_greet_client(barber->shop, barber->id, barber->clientID);
	}

	//mutex_unlock(&barber->shop->barberShopMutex);

	//printf("--------------------------------------------BARBER LIFE - WAIT FOR CLIENT\n");
	log_barber(barber);  // (if necessary) more than one in proper places!!!
}

static int work_available(Barber* barber)
{
	/**
	 * TODO:
	 * 1: find a safe way to solve the problem of barber termination
	 **/

	require (barber != NULL, "barber argument required");
	//printf("--------------------------------------------BARBER LIFE - WORK AVAILABLE\n");

	if (!(barber->shop->opened)){
		rise_barber_bench(&barber->shop->barberBench, barber->benchPosition);
		barber->benchPosition = -1;
		return 0;
	}

	if (barber->clientID > 0){
		return 1;
	}

	return 0;
}

static void rise_from_barber_bench(Barber* barber)
{
	/**
	 * 1: rise from the seat of barber bench
	 ** TODO:
	 **/

	require (barber != NULL, "barber argument required");
	require (seated_in_barber_bench(barber_bench(barber->shop), barber->id), "barber not seated in barber shop");
	
	rise_barber_bench(&(barber->shop->barberBench), barber->benchPosition);
	barber->benchPosition = -1;
	//printf("--------------------------------------------BARBER LIFE - RISE FROM BARBER BENCH\n");
	log_barber(barber);
}


static void process_resquests_from_client(Barber* barber)
{
	/**
	 * Process one client request at a time, until all requests are fulfilled.
	 * For each request:
	 * 1: select the request to process (any order is acceptable)
	 * 2: reserve the chair/basin for the service (setting the barber's state accordingly)
	 *    2.1: set the client state to a proper value
	 *    2.2: reserve a random empty chair/basin
	 *    2.2: inform client on the service to be performed
	 * 3: depending on the service, grab the necessary tools from the pot (if any)
	 * 4: process the service (see [incomplete] process_haircut_request as an example)
	 *  4.1: return the used tools to the pot (if any)
	 *
	 * At the end the client must leave the barber shop
	 **TODO:
	 **/

	require (barber != NULL, "barber argument required");
	//printf("--------------------------barber - going to process requests\n");

	//1: select the request to process (any order is acceptable)
	Service service;
	if ((barber->reqToDo & HAIRCUT_REQ) || (barber->reqToDo & SHAVE_REQ)) {

		//reserve a random empty chair
		mutex_lock(&barber->shop->barberChairMutex);
		barber->state = WAITING_BARBER_SEAT;
		int chairPos = reserve_random_empty_barber_chair(barber->shop, barber->id);
		BarberChair* chair = barber->shop->barberChair+chairPos;
		barber->chairPosition = chairPos;
		mutex_unlock(&barber->shop->barberChairMutex);

		if (barber->reqToDo & HAIRCUT_REQ){

			set_barber_chair_service(&service, barber->id, barber->clientID, chairPos, HAIRCUT_REQ);
			inform_client_on_service(barber->shop, service);

			pick_haircut_tools(barber);

			mutex_lock(&barber->shop->barberChairMutex);
			while(!barber_chair_with_a_client(chair)){
				cond_wait(&barber->shop->clientSatInBarberChair, &barber->shop->barberShopMutex);
			}

			process_haircut_request(barber);
			cond_broadcast(&barber->shop->barberChairServiceFinished);
			mutex_unlock(&barber->shop->barberChairMutex);

			return_haircut_tools(baber);
		}

		if (barber->reqToDo & SHAVE_REQ){
			set_barber_chair_service(&service, barber->id, barber->clientID, chairPos, SHAVE_REQ);
			inform_client_on_service(barber->shop, service);

			//printf("--------------------------barber - informed client on shave\n");

			if(barber_chair_with_a_client(chair)){
				cond_wait(&barber->shop->clientReadyForShave, &barber->shop->barberShopMutex);
				//printf("--------------------------barber - client ready for shave\n");
			}
			while(!barber_chair_with_a_client(chair)){
				cond_wait(&barber->shop->clientSatInBarberChair, &barber->shop->barberShopMutex);
			}

			process_shave_request(barber);
			//printf("--------------------------barber - shaved client\n");
			cond_signal(&barber->shop->barberChairServiceFinished);

		}

		while (barber_chair_with_a_client(chair)){
			cond_wait(&barber->shop->clientRoseFromBarberChair, &barber->shop->barberShopMutex);
		}
		//printf("--------------------------barber - client got up\n");
		release_barber_chair(chair,barber->id);
		cond_signal(&barber->shop->barberChairAvailable);

	}

	if (barber->reqToDo & WASH_HAIR_REQ) {

		//?2.1: set the barber (client???) state to a proper value
		barber->state = WAITING_WASHBASIN;

		//2.2: reserve a random empty chair
		int basinPos = reserve_random_empty_washbasin(barber->shop, barber->id);
		Washbasin* basin = barber->shop->washbasin+basinPos;
		barber->basinPosition = basinPos;
		set_washbasin_service(&service, barber->id, barber->clientID, basinPos);
		//printf("--------------------------barber - inform wash\n");
		inform_client_on_service(barber->shop, service);

		while(!washbasin_with_a_client(basin)){
			cond_wait(&barber->shop->clientSatInWashbasin, &barber->shop->barberShopMutex);
		}
		process_wash_hair_request(barber);
		cond_signal(&barber->shop->washbasinServiceFinished);

		while (washbasin_with_a_client(basin)){
			cond_wait(&barber->shop->clientRoseFromWashbasin, &barber->shop->barberShopMutex);
		}
		release_washbasin(basin, barber->id);
		cond_signal(&barber->shop->washbasinAvailable);
	}

	barber->state = NONE;
	// services are finished
	//	if (barber->shop->opened == 0){
	//		//printf("--------------------------barber - shop closed, barber going to die\n");
	//		term_barber(barber);
	//	}

	//printf("--------------------------------------------BARBER LIFE - PROCESS REQUESTS FROM CLIENT\n");
	mutex_unlock(&barber->shop->barberShopMutex);

	//At the end the client must leave the barber shop

	log_barber(barber);  // (if necessary) more than one in proper places!!!
}


static void release_client(Barber* barber)
{
	/**
	 * 1: notify client the all the services are done
	 ** TODO:
	 **/

	require (barber != NULL, "barber argument required");

	mutex_lock(&barber->shop->barberShopMutex);

	//printf("--------------------------------------------BARBER LIFE - RELEASE CLIENT %d\n", barber->clientID);
	client_done(barber->shop, barber->clientID);
	cond_signal(&barber->shop->messageAvailable);
	while (is_client_inside(barber->shop, barber->clientID)){
		//printf("--------------------------barber - waiting for client to signal he is leaving\n");
		cond_wait(&barber->shop->clientLeft, &barber->shop->barberShopMutex);
	}
	barber->clientID = -1;

	//printf("--------------------------------------------BARBER LIFE - RELEASE CLIENT\n");

	mutex_unlock(&barber->shop->barberShopMutex);
	log_barber(barber);
}

static void done(Barber* barber)
{
	/**
	 * 1: set the barber state to DONE
	 ** TODO:
	 **/
	require (barber != NULL, "barber argument required");

	barber->state = DONE;

	//printf("--------------------------------------------BARBER LIFE - EXITING - DONE\n");

	log_barber(barber);
}

static void pick_haircut_tools(Barber* barber)
{
	require (barber != NULL, "barber argument required");

	mutex_lock(&barber->shop->toolsPotMutex);

	barber->state = REQ_SCISSOR;
	pick_scissor(&barber->shop->toolsPot);
	barber->tools += SCISSOR_TOOL;

	barber->state = REQ_COMB;
	pick_comb(&barber->shop->toolsPot);
	barber->tools += COMB_TOOL;

	mutex_unlock(&barber->shop->toolsPotMutex);

	mutex_lock(&barber->shop->barberChairMutex);

	BarberChair* chair = &barber->shop->barberChair[barber->chairPosition];
	set_tools_barber_chair(chair, barber->tools);

	mutex_unlock(&barber->shop->barberChairMutex);
}

static void return_haircut_tools(Barber* barber)
{

}

static void process_haircut_request(Barber* barber)
{
	/**
	 * ([incomplete] example code for task completion algorithm)
	 ** TODO:
	 **/
	require (barber != NULL, "barber argument required");
	//   require (barber->tools & SCISSOR_TOOL, "barber not holding a scissor");
	//   require (barber->tools & COMB_TOOL, "barber not holding a comb");

	//3: grab the necessary tools from the pot




	//4: process the service
	barber->state = CUTTING;

	int steps = random_int(5,20);
	int slice = (global->MAX_WORK_TIME_UNITS-global->MIN_WORK_TIME_UNITS+steps)/steps;
	int complete = 0;
	while(complete < 100)
	{
		spend(slice);
		complete += 100/steps;
		if (complete > 100)
			complete = 100;
		set_completion_barber_chair(barber_chair(barber->shop, barber->chairPosition), complete);
	}

	return_scissor(&barber->shop->toolsPot);
	return_comb(&barber->shop->toolsPot);
	barber->tools = NO_TOOLS;

	log_barber(barber);  // (if necessary) more than one in proper places!!!
}


static void process_shave_request(Barber* barber)
{
	/**
	 * ([incomplete] example code for task completion algorithm)
	 **TODO:
	 **/
	require (barber != NULL, "barber argument required");

	//3: grab the necessary tools from the pot
	BarberChair* chair = &barber->shop->barberChair[barber->chairPosition];

	barber->state = REQ_RAZOR;
	pick_razor(&barber->shop->toolsPot);
	barber->tools = RAZOR_TOOL;

	set_tools_barber_chair(chair, barber->tools);
	//4: process the service
	barber->state = SHAVING;

	int steps = random_int(5,20);
	int slice = (global->MAX_WORK_TIME_UNITS-global->MIN_WORK_TIME_UNITS+steps)/steps;
	int complete = 0;
	while(complete < 100)
	{
		spend(slice);
		complete += 100/steps;
		if (complete > 100)
			complete = 100;
		set_completion_barber_chair(barber_chair(barber->shop, barber->chairPosition), complete);
	}

	return_razor(&barber->shop->toolsPot);
	barber->tools = NO_TOOLS;

	log_barber(barber);  // (if necessary) more than one in proper places!!!
}

static void process_wash_hair_request(Barber* barber)
{
	/**
	 * ([incomplete] example code for task completion algorithm)
	 **TODO:
	 **/
	require (barber != NULL, "barber argument required");

	//4: process the service
	barber->state = WASHING;

	int steps = random_int(5,20);
	int slice = (global->MAX_WORK_TIME_UNITS-global->MIN_WORK_TIME_UNITS+steps)/steps;
	int complete = 0;
	while(complete < 100)
	{
		spend(slice);
		complete += 100/steps;
		if (complete > 100)
			complete = 100;
		set_completion_washbasin(washbasin(barber->shop, barber->basinPosition), complete);
	}

	log_barber(barber);  // (if necessary) more than one in proper places!!!
}


static char* to_string_barber(Barber* barber)
{
	require (barber != NULL, "barber argument required");

	if (barber->internal == NULL)
		barber->internal = (char*)mem_alloc(skel_length + 1);

	char tools[4];
	tools[0] = (barber->tools & SCISSOR_TOOL) ? 'S' : '-',
			tools[1] = (barber->tools & COMB_TOOL) ?    'C' : '-',
					tools[2] = (barber->tools & RAZOR_TOOL) ?   'R' : '-',
							tools[3] = '\0';

	char* pos = (char*)"-";
	if (barber->chairPosition >= 0)
		pos = int2nstr(barber->chairPosition+1, 1);
	else if (barber->basinPosition >= 0)
		pos = int2nstr(barber->basinPosition+1, 1);

	return gen_boxes(barber->internal, skel_length, skel,
			int2nstr(barber->id, 2),
			barber->clientID > 0 ? int2nstr(barber->clientID, 2) : "--",
					tools, stateText[barber->state], pos);
}

