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
#include "comunication-line.h"

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
    * TODO:
    **/
	require (barber != NULL, "barber argument required");
	require (num_seats_available_barber_bench(barber_bench(barber->shop)) > 0, "seat not available in barber shop");
	require (!seated_in_barber_bench(barber_bench(barber->shop), barber->id), "barber already seated in barber shop");

	mutex_lock(&barberBenchMutex);
	while (num_seats_available_barber_bench(&(barber->shop->barberBench)) == 0)
	{
	  cond_wait(&barberBenchNotFull, &barberBenchMutex);
	}

	random_sit_in_barber_bench(barber_bench(barber->shop), barber->id);

	mutex_unlock(&barberBenchMutex);

	log_barber(barber);
}

static void wait_for_client(Barber* barber)
{
   /**
    * 1: set the barber state to WAITING_CLIENTS
    * 2: get next client from client benches (if empty, wait) (also, it may be required to check for simulation termination)
    * 3: receive and greet client (receive its requested services, and give back the barber's id)
    * TODO:
    **/

	require (barber != NULL, "barber argument required");

	barber->state = WAITING_CLIENTS;

	// se clientes acabarem, conseguimos fechar a loja???
	while (next_client_in_benches(&(barber->shop->clientBenches)) == empty_item()){
	   cond_wait(&clientWaiting, &clientsBenchMutex);
    }

	RQItem requests = next_client_in_benches(&(barber->shop->clientBenches));
	barber->reqToDo = requests.request;
	barber->clientID = requests.clientID;

	receive_and_greet_client(barber->shop, barber->id, barber->clientID);

	log_barber(barber);  // (if necessary) more than one in proper places!!!
}

static int work_available(Barber* barber)
{
   /**
    * TODO:
    * 1: find a safe way to solve the problem of barber termination
    **
    **/

   require (barber != NULL, "barber argument required");

   if (!no_more_clients(&barber->shop->clientBenches)) {
	   return 1;
   }
   else{
	   // TODO: RESOLVER O PROBLEM DA TERMINACAO
	   return 0;
   }
}

static void rise_from_barber_bench(Barber* barber)
{
   /**
    * 1: rise from the seat of barber bench
    ** TODO:
    **/

   require (barber != NULL, "barber argument required");
   require (seated_in_barber_bench(barber_bench(barber->shop), barber->id), "barber not seated in barber shop");

   barber->benchPosition = -1;
   rise_barber_bench(&(barber->shop->barberBench), barber->benchPosition);

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

   //1: select the request to process (any order is acceptable)
	Service service;
	if ((barber->reqToDo & HAIRCUT_REQ) || (barber->reqToDo & SHAVE_REQ)) {
		//?2.1: set the barber (client???) state to a proper value
		barber->state = WAITING_BARBER_SEAT;

		//2.2: reserve a random empty chair
		int chairPos = reserve_random_empty_barber_chair(barber->shop, barber->id);
		BarberChair* chair = barber->shop->barberChair+chairPos;

		Service service;
		if (barber->reqToDo & HAIRCUT_REQ){
			set_barber_chair_service(&service, barber->id, barber->clientID, chairPos, HAIRCUT_REQ);
			inform_client_on_service(barber->shop, service);
			while(!barber_chair_with_a_client(chair));
			process_haircut_request(barber);
		}
		if (barber->reqToDo & SHAVE_REQ){
			set_barber_chair_service(&service, barber->id, barber->clientID, chairPos, SHAVE_REQ);
			inform_client_on_service(barber->shop, service);
			while(!barber_chair_with_a_client(chair));
			process_shave_request(barber);
		}

		barber_chair_service_finished(chair);
		cond_signal(&barberChairServiceFinished);
		while (barber_chair_with_a_client(chair));
		release_barber_chair(chair,barber->id);
		cond_signal(&barberChairAvailable);

	}

	if (barber->reqToDo & WASH_HAIR_REQ) {

		//?2.1: set the barber (client???) state to a proper value
		barber->state = WAITING_WASHBASIN;

		//2.2: reserve a random empty chair
		int basinPos = reserve_random_empty_washbasin(barber->shop, barber->id);
		Washbasin* basin = barber->shop->washbasin+basinPos;
		Service service;
		set_washbasin_service(&service, barber->id, barber->clientID, basinPos);
		inform_client_on_service(barber->shop, service);
		while(!washbasin_with_a_client(basin));
		process_wash_hair_request(barber);

		washbasin_service_finished(basin);
		cond_signal(&washbasinServiceFinished);
		while (washbasin_with_a_client(basin));
		release_washbasin(basin, barber->clientID);
		cond_signal(&washbasinAvailable);
	}

	// services are finished
	release_client(barber);

	//At the end the client must leave the barber shop

	log_barber(barber);  // (if necessary) more than one in proper places!!!
}


static void release_client(Barber* barber)
{
   /**
    * 1: notify client the all the services are done
    * TODO:
    **/

   require (barber != NULL, "barber argument required");

   Service service;
   inform_client_on_service(barber->shop, service);
   barber->clientID = -1;

   log_barber(barber);
}

static void done(Barber* barber)
{
   /**
    * 1: set the barber state to DONE
    * TODO:
    **/
   require (barber != NULL, "barber argument required");

   barber->state = DONE;

   log_barber(barber);
}

static void process_haircut_request(Barber* barber)
{
   /** 
    * ([incomplete] example code for task completion algorithm)
    ** TODO:
    **/
   require (barber != NULL, "barber argument required");
   require (barber->tools & SCISSOR_TOOL, "barber not holding a scissor");
   require (barber->tools & COMB_TOOL, "barber not holding a comb");

   //3: grab the necessary tools from the pot
   barber->state = REQ_SCISSOR;
   pick_scissor(&barber->shop->toolsPot);
   barber->state = REQ_COMB;
   pick_comb(&barber->shop->toolsPot);

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

   log_barber(barber);  // (if necessary) more than one in proper places!!!
}


static void process_shave_request(Barber* barber)
{
	/**
	* ([incomplete] example code for task completion algorithm)
	**TODO:
	**/
	require (barber != NULL, "barber argument required");
	require (barber->tools & SCISSOR_TOOL, "barber not holding a scissor");
	require (barber->tools & COMB_TOOL, "barber not holding a comb");

	//3: grab the necessary tools from the pot
	barber->state = REQ_RAZOR;
	pick_razor(&barber->shop->toolsPot);

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

	log_barber(barber);  // (if necessary) more than one in proper places!!!
}

static void process_wash_hair_request(Barber* barber)
{
   /**
    * ([incomplete] example code for task completion algorithm)
    **TODO:
    **/
	require (barber != NULL, "barber argument required");
	require (barber->tools & SCISSOR_TOOL, "barber not holding a scissor");
	require (barber->tools & COMB_TOOL, "barber not holding a comb");

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
	  set_completion_barber_chair(barber_chair(barber->shop, barber->chairPosition), complete);
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

