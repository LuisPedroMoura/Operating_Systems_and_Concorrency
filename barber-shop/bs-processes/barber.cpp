#include <stdlib.h>
#include "dbc.h"
#include "global.h"
#include "utils.h"
#include "box.h"
#include "timer.h"
#include "logger.h"
#include "barber-shop.h"
#include "barber.h"

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
   DONE,
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
static void process_hairwash_request(Barber* barber);
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

   barber->shop->opened = 1;
   sit_in_barber_bench(barber);
   wait_for_client(barber);
   while(work_available(barber)) // no more possible clients and closes barbershop
   {
      rise_from_barber_bench(barber);
      process_resquests_from_client(barber);
      bci_set_barberFirstTrip(barber->id);
      //release_client(barber);
      sit_in_barber_bench(barber);
      wait_for_client(barber);
   }
   done(barber);
}

static void sit_in_barber_bench(Barber* barber)
{
   /** TODO:
    * 1: sit in a random empty seat in barber bench (always available)
    **/

   require (barber != NULL, "barber argument required");
   require (num_seats_available_barber_bench(barber_bench(barber->shop)) > 0, "seat not available in barber shop");
   require (!seated_in_barber_bench(barber_bench(barber->shop), barber->id), "barber already seated in barber shop");
  
   bci_wait_semBarberBench();

   bci_get_syncBBBench(barber_bench(barber->shop));

   barber->benchPosition = random_sit_in_barber_bench(barber_bench(barber->shop),barber->id); 
   log_barber(barber);

   BarberBench* tmp_bbbenches = barber_bench(barber->shop);
   bci_set_syncBBBench(*tmp_bbbenches);

   bci_post_semBarberBench();

   log_barber(barber);
}

static void wait_for_client(Barber* barber)
{
   /** TODO:
    * 1: set the barber state to WAITING_CLIENTS
    * 2: get next client from client benches (if empty, wait) (also, it may be required to check for simulation termination)
    * 3: receive and greet client (receive its requested services, and give back the barber's id)
    **/ 

   require (barber != NULL, "barber argument required");

   //if(bci_get_barberFirstTrip(barber->id) == 0) {
   //  return;
   //}

   if(bci_get_shop_status() == 0) {
     barber->state = DONE;
     log_barber(barber);
     return;
   }

   barber->state = WAITING_CLIENTS;
   log_barber(barber);

   bci_get_syncBenches(client_benches(barber->shop));

   while(bci_get_num_clients_in_bench() == 0);

   bci_wait_semClientBench();

   bci_get_syncBenches(client_benches(barber->shop));

   RQItem queue_item = next_client_in_benches(client_benches(barber->shop));
   RQItem* tmp_qitem = &(queue_item);

   //printf("\n\n\n\n CLIENT ID: %d\n\n\n\n",tmp_qitem->clientID);

   bci_set_syncBenches(*client_benches(barber->shop));

   receive_and_greet_client(barber->shop,barber->id,tmp_qitem->clientID);
 
   barber->clientID = tmp_qitem->clientID;
 
   bci_post_semClientAccess(barber->clientID);
   //bci_grant_client_access(barber->clientID);
 
   log_barber(barber);  // (if necessary) more than one in proper places!!!
}

static int work_available(Barber* barber)
{
   /** TODO:
    * 1: find a safe way to solve the problem of barber termination
    **/

   require (barber != NULL, "barber argument required");

   bci_get_syncBenches(client_benches(barber->shop));

   //printf("\n\n\n\n NUM CLIENTS IN BENCH: %d \n\n\n\n",bci_get_num_clients_in_bench());
   //printf("\n\n\n\n NUM CLIENTS IN BENCH: %d \n\n\n\n",bci_get_num_clients_in_bench());
   //printf("\n\n\n\n NUM CLIENTS IN BENCH: %d \n\n\n\n",bci_get_num_clients_in_bench());

   log_barber(barber);

   //if(bci_get_num_clients_in_bench() != 0) {
   if(bci_get_barberFirstTrip(barber->id) == 1 or barber->clientID > 0) {
     return 1;
   }

   return 0;
}

static void rise_from_barber_bench(Barber* barber)
{
   /** TODO:
    * 1: rise from the seat of barber bench
    **/

   require (barber != NULL, "barber argument required");
   require (seated_in_barber_bench(barber_bench(barber->shop), barber->id), "barber not seated in barber shop");

   bci_wait_semBarberBench();

   bci_get_syncBBBench(barber_bench(barber->shop));

   rise_barber_bench(barber_bench(barber->shop),barber->benchPosition);
   barber->benchPosition = -1;

   BarberBench* tmp_bbbenches = barber_bench(barber->shop);
   bci_set_syncBBBench(*tmp_bbbenches);

   bci_post_semBarberBench(); 

   log_barber(barber);
}

static void process_resquests_from_client(Barber* barber)
{
   /** TODO:
    * Process one client request at a time, until all requests are fulfilled.
    * For each request:
    * 1: select the request to process (any order is acceptable)
    * 2: reserve the chair/basin for the service (setting the barber's state accordingly) 
    *    2.1: set the client state to a proper value
    *    2.2: reserve a random empty chair/basin 
    *    2.3: inform client on the service to be performed
    * 3: depending on the service, grab the necessary tools from the pot (if any)
    * 4: process the service (see [incomplete] process_haircut_request as an example)
    * 5: return the used tools to the pot (if any)
    *
    *
    * At the end the client must leave the barber shop
    **/

   require (barber != NULL, "barber argument required");

   //printf("\n\n\n\n BEFORE EVEN STARTING: %d \n\n\n\n",bci_get_request(barber->clientID));

   while(bci_get_request(barber->clientID) > 0) {

     //printf("\n\n\n\n THIS ITERATION: %d \n\n\n\n",bci_get_request(barber->clientID));

     barber->reqToDo = bci_get_next_request(barber->clientID);

     bci_wait_semChairBasin();

     bci_get_syncBBChairAll(barber->shop->barberChair);
     bci_get_syncWashbasinAll(barber->shop->washbasin);

     if(barber->reqToDo == 1) {
       while(num_available_barber_chairs(barber->shop) == 0);
       barber->chairPosition = reserve_random_empty_barber_chair(barber->shop,barber->id);
       BarberChair* tmp_bbc1 = barber_chair(barber->shop,barber->chairPosition); 
       bci_set_syncBBChair(*tmp_bbc1,barber->id);
     }
     else if(barber->reqToDo == 2) {
       while(num_available_washbasin(barber->shop) == 0);
       barber->basinPosition = reserve_random_empty_washbasin(barber->shop,barber->id);
       Washbasin* tmp_wsh = washbasin(barber->shop,barber->basinPosition);
       bci_set_syncWashbasin(*tmp_wsh,barber->id);
     }
     else {
       while(num_available_barber_chairs(barber->shop) == 0);
       barber->chairPosition = reserve_random_empty_barber_chair(barber->shop,barber->id);
       BarberChair* tmp_bbc2 = barber_chair(barber->shop,barber->chairPosition);
       bci_set_syncBBChair(*tmp_bbc2,barber->id);
     }
 
     log_barber(barber); 
     
     bci_post_semReserved(barber->id); 

     bci_set_syncBBChairAll(barber->shop->barberChair);
     bci_set_syncWashbasinAll(barber->shop->washbasin);

     bci_post_semChairBasin();

     Service service_to_send; 
     if(barber->reqToDo == 1 || barber->reqToDo == 4)
       set_barber_chair_service(&service_to_send,barber->id,barber->clientID,barber->chairPosition,barber->reqToDo);
     else if(barber->reqToDo == 2)
       set_washbasin_service(&service_to_send,barber->id,barber->clientID,barber->basinPosition);
     else
       break; 
      
     inform_client_on_service(barber->shop,service_to_send);
     
     bci_post_semServiceInfoAvailable(barber->id); 

     //bci_wait_semToolPot();

     if(barber->reqToDo == 1) {
       barber->state = REQ_SCISSOR;
       log_barber(barber);
 
       //while((tools_pot(barber->shop))->availScissors == 0);
       bci_wait_semScissor(); 

       bci_wait_semToolPot();

       bci_get_syncBBChairAll(barber->shop->barberChair);

       bci_get_syncToolsPot(tools_pot(barber->shop));
       pick_scissor(tools_pot(barber->shop));
       barber->tools += 1;
       ToolsPot* tmp_toolset1 = tools_pot(barber->shop);
       bci_set_syncToolsPot(*tmp_toolset1);

       bci_get_syncBBChair(barber_chair(barber->shop,barber->chairPosition),barber->id);
       BarberChair* bbchair1 = barber_chair(barber->shop,barber->chairPosition);
       bbchair1->toolsHolded += 1;
       bci_set_syncBBChair(*bbchair1,barber->id);

       bci_set_syncBBChairAll(barber->shop->barberChair);

       bci_post_semToolPot();

       barber->state = REQ_COMB;
       log_barber(barber);
       
       //while((tools_pot(barber->shop))->availCombs == 0);
       bci_wait_semComb();

       bci_wait_semToolPot();

       bci_get_syncBBChairAll(barber->shop->barberChair);

       bci_get_syncToolsPot(tools_pot(barber->shop));
       pick_comb(tools_pot(barber->shop));
       barber->tools += 2;
       ToolsPot* tmp_toolset2 = tools_pot(barber->shop);
       bci_set_syncToolsPot(*tmp_toolset2);

       bci_get_syncBBChair(barber_chair(barber->shop,barber->chairPosition),barber->id);
       BarberChair* bbchair2 = barber_chair(barber->shop,barber->chairPosition);
       bbchair2->toolsHolded += 2;
       bci_set_syncBBChair(*bbchair2,barber->id);

       bci_set_syncBBChairAll(barber->shop->barberChair);

       bci_post_semToolPot();
     }
     else if(barber->reqToDo == 4) {
       barber->state = REQ_RAZOR;
       log_barber(barber);

       //while((tools_pot(barber->shop))->availRazors == 0);
       bci_wait_semRazor();

       bci_wait_semToolPot();

       bci_get_syncBBChairAll(barber->shop->barberChair);

       bci_get_syncToolsPot(tools_pot(barber->shop));
       pick_razor(tools_pot(barber->shop));
       barber->tools += 4;
       ToolsPot* tmp_toolset = tools_pot(barber->shop);
       bci_set_syncToolsPot(*tmp_toolset);

       bci_get_syncBBChair(barber_chair(barber->shop,barber->chairPosition),barber->id);
       BarberChair* bbchair3 = barber_chair(barber->shop,barber->chairPosition);
       bbchair3->toolsHolded += 4;
       bci_set_syncBBChair(*bbchair3,barber->id);
 
       bci_set_syncBBChairAll(barber->shop->barberChair);
 
       bci_post_semToolPot();
     }

     //bci_post_semToolPot();
	 
     log_barber(barber);

     bci_post_semWaitingOnClientSit(barber->id);

     bci_wait_semClientSeated(barber->id);

     bci_post_semProcessing(barber->id);

     if(barber->reqToDo == 1) {
       barber->state = CUTTING;
       log_barber(barber);

       //sem for barberbasin?

       bci_get_syncBBChairAll(barber->shop->barberChair);
       bci_get_syncBBChair(barber_chair(barber->shop,barber->chairPosition),barber->id);
       BarberChair* bbchaircut= barber_chair(barber->shop,barber->chairPosition);
       process_haircut_request(barber);
       bci_set_syncBBChair(*bbchaircut,barber->id);
       bci_set_syncBBChairAll(barber->shop->barberChair);
     }
     else if(barber->reqToDo == 2) {
       barber->state = WASHING;
       log_barber(barber);
       bci_get_syncWashbasinAll(barber->shop->washbasin);
       bci_get_syncWashbasin(washbasin(barber->shop,barber->basinPosition),barber->id);
       Washbasin* wshbasin = washbasin(barber->shop,barber->basinPosition);
       process_hairwash_request(barber);
       bci_set_syncWashbasin(*wshbasin,barber->id);
       bci_set_syncWashbasinAll(barber->shop->washbasin);
     }
     else {
       barber->state = SHAVING;
       log_barber(barber);
       bci_get_syncBBChairAll(barber->shop->barberChair);
       bci_get_syncBBChair(barber_chair(barber->shop,barber->chairPosition),barber->id);
       BarberChair* bbchairshave= barber_chair(barber->shop,barber->chairPosition);
       process_shave_request(barber);
       bci_set_syncBBChair(*bbchairshave,barber->id);
       bci_set_syncBBChairAll(barber->shop->barberChair);
     }

     bci_post_semWaitingOnRise(barber->id);

     bci_wait_semClientRisen(barber->id);
	 
     //printf("\n\n\n\n REQUEST JUST DONE: %d \n\n\n\n",bci_get_next_request(barber->clientID));

     bci_did_request(barber->clientID);
     //barber->state = DONE;
    
     //printf("\n\n\n\n NEXT REQUEST: %d \n\n\n\n",bci_get_next_request(barber->clientID));

     //log_barber(barber);

     bci_wait_semToolPot();

     bci_get_syncToolsPot(tools_pot(barber->shop));

     if(barber->reqToDo == 1) {
       bci_get_syncBBChairAll(barber->shop->barberChair);
       return_scissor(tools_pot(barber->shop));
       barber->tools -= 1;
       bci_get_syncBBChair(barber_chair(barber->shop,barber->chairPosition),barber->id);
       BarberChair* bbchair4 = barber_chair(barber->shop,barber->chairPosition);
       bbchair4->toolsHolded -= 1;
       bci_set_syncBBChair(*bbchair4,barber->id);
       
       bci_post_semScissor();

       return_comb(tools_pot(barber->shop));
       barber->tools -= 2;
       bci_get_syncBBChair(barber_chair(barber->shop,barber->chairPosition),barber->id);
       BarberChair* bbchair5 = barber_chair(barber->shop,barber->chairPosition);
       bbchair5->toolsHolded -= 2;
       bci_set_syncBBChair(*bbchair5,barber->id);

       bci_post_semComb();
       bci_get_syncBBChairAll(barber->shop->barberChair);
     }
     else if(barber->reqToDo == 4) {
       bci_get_syncWashbasinAll(barber->shop->washbasin);
       return_razor(tools_pot(barber->shop));
       barber->tools -= 4;
       bci_get_syncBBChair(barber_chair(barber->shop,barber->chairPosition),barber->id);
       BarberChair* bbchair6 = barber_chair(barber->shop,barber->chairPosition);
       bbchair6->toolsHolded -= 4;
       bci_set_syncBBChair(*bbchair6,barber->id);
       bci_post_semRazor();
       bci_get_syncWashbasinAll(barber->shop->washbasin);
     }

     ToolsPot* tmp_toolyp = tools_pot(barber->shop);
     bci_set_syncToolsPot(*tmp_toolyp);

     bci_post_semToolPot();
 
     bci_wait_semChairBasin();

     bci_get_syncBBChairAll(barber->shop->barberChair);
     bci_get_syncWashbasinAll(barber->shop->washbasin);

     if(barber->reqToDo == 1 or barber->reqToDo == 4) {
       bci_get_syncBBChair(barber_chair(barber->shop,barber->chairPosition),barber->id);
       release_barber_chair(barber_chair(barber->shop,barber->chairPosition), barber->id);
       BarberChair* bbchair7 = barber_chair(barber->shop,barber->chairPosition);
       bci_set_syncBBChair(*bbchair7,barber->id);
       barber->chairPosition = -1;
     }
     else {
       bci_get_syncWashbasin(washbasin(barber->shop,barber->basinPosition),barber->id);
       release_washbasin(washbasin(barber->shop,barber->basinPosition),barber->id);
       Washbasin* tmp_wsh2 = washbasin(barber->shop,barber->basinPosition);
       bci_set_syncWashbasin(*tmp_wsh2,barber->id);
       barber->basinPosition = -1;
     }

     bci_set_syncBBChairAll(barber->shop->barberChair);
     bci_set_syncWashbasinAll(barber->shop->washbasin);

     bci_post_semChairBasin();

     log_barber(barber);

     if(bci_get_request(barber->clientID) == 0) {
       barber->state = DONE;
       bci_post_semAllProcessesDone(barber->id);
       release_client(barber);
     }

     //printf("\n\n\n\n BARBER: REQUEST IS??? %d \n\n\n\n",bci_get_request(barber->clientID));;

     bci_post_semProcessDone(barber->id);

   }

   bci_unset_barberFirstTrip(barber->id);
 
   log_barber(barber);
}

static void release_client(Barber* barber)
{
   /** TODO:
    * 1: notify client that all the services are done
    **/

   require (barber != NULL, "barber argument required");

   barber->clientID = 0;
   
   bci_unset_clientID(barber->id);

   log_barber(barber);
}

static void done(Barber* barber)
{
   /** TODO:
    * 1: set the client state to DONE
    **/
   require (barber != NULL, "barber argument required");

   barber->state = DONE;

   log_barber(barber);
}

static void process_haircut_request(Barber* barber)
{
   /** TODO:
    * ([incomplete] example code for task completion algorithm)
    **/
   require (barber != NULL, "barber argument required");
   require (barber->tools & SCISSOR_TOOL, "barber not holding a scissor");
   require (barber->tools & COMB_TOOL, "barber not holding a comb");

   int steps = random_int(5,20);
   int slice = (global->MAX_WORK_TIME_UNITS-global->MIN_WORK_TIME_UNITS+steps)/steps;
   int complete = 0;
   while(complete < 100)
   {
     spend(slice);
     complete += 100/steps;
     if (complete > 100) {
       complete = 100;
     }
     set_completion_barber_chair(barber_chair(barber->shop, barber->chairPosition), complete);
   }
}

static void process_hairwash_request(Barber* barber)
{
   require (barber != NULL, "barber argument required");

   int steps = random_int(5,20);
   int wash = (global->MAX_WORK_TIME_UNITS-global->MIN_WORK_TIME_UNITS+steps)/steps;
   int complete = 0;
   while(complete < 100)
   {
     spend(wash);
     complete += 100/steps;
     if (complete > 100) {
       complete = 100;
     }
     set_completion_washbasin(washbasin(barber->shop, barber->basinPosition), complete);
   }
}

static void process_shave_request(Barber* barber)
{
   require (barber != NULL, "barber argument required");
   require (barber->tools & RAZOR_TOOL, "barber not holding a razor");

   int steps = random_int(5,20);
   int slice = (global->MAX_WORK_TIME_UNITS-global->MIN_WORK_TIME_UNITS+steps)/steps;
   int complete = 0;
   while(complete < 100)
   {
     spend(slice);
     complete += 100/steps;
     if (complete > 100) {
       complete = 100;
     }
     set_completion_barber_chair(barber_chair(barber->shop, barber->chairPosition), complete);
   }
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

