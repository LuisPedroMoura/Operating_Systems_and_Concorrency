#include <stdlib.h>
#include "dbc.h"
#include "global.h"
#include "utils.h"
#include "box.h"
#include "timer.h"
#include "logger.h"
#include "service.h"
#include "client.h"

enum BCState
{
   NO_BARBER_GREET,           //barber has yet to receive and greet the client
   GREET_AVAILABLE,	          //client can get barberID
   WAITING_ON_RESERVE,        //client waiting until the barber has reserved the seat for the process
   WAITING_ON_PROCESS_START,  //client waiting until the process starts (barber has all the needed tools)
   PROCESSING,                //process running
   PROCESS_DONE,              //process has finished
   ALL_PROCESSES_DONE         //all processes done   
};

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
   bci_connect();
   life(client);
   return NULL;
}

static void life(Client* client)
{
   require (client != NULL, "client argument required");

   int i = 0;
   notify_client_birth(client);
   //while(i < client->num_trips_to_barber)
   //{
      wandering_outside(client);
      if (vacancy_in_barber_shop(client))
      {
         select_requests(client);
         wait_its_turn(client);
         //rise_from_client_benches(client);
         //wait_all_services_done(client);
         i++;
      }
   //}
   notify_client_death(client);
}

static void notify_client_birth(Client* client)
{
   require (client != NULL, "client argument required");

   /** TODO:
    * 1: (if necessary) inform simulation that a new client begins its existence.
    **/

   if(client->state == NONE)
      log_client(client);
}

static void notify_client_death(Client* client)
{
   /** TODO:
    * 1: (if necessary) inform simulation that a new client ceases its existence.
    **/

   require (client != NULL, "client argument required");

   if(client->state == DONE)
      log_client(client);
}

static void wandering_outside(Client* client)
{
   /** TODO:
    * 1: set the client state to WANDERING_OUTSIDE
    * 2. random a time interval [global->MIN_OUTSIDE_TIME_UNITS, global->MAX_OUTSIDE_TIME_UNITS]
    **/

   require (client != NULL, "client argument required");
   
   client->state = WANDERING_OUTSIDE;
   log_client(client);
   spend(random_int(global->MIN_OUTSIDE_TIME_UNITS, global->MAX_OUTSIDE_TIME_UNITS));
}

static int vacancy_in_barber_shop(Client* client)
{
   /** TODO:
    * 1: set the client state to WAITING_BARBERSHOP_VACANCY
    * 2: check if there is an empty seat in the client benches (at this instante, later on it may fail)
    **/

   require (client != NULL, "client argument required");

   int res = 0;

   client->state = WAITING_BARBERSHOP_VACANCY;
   log_client(client);

   for(int i = 0; i < (client_benches(client->shop))->numSeats; i++) 
     if((client_benches(client->shop))->id[i] == 0) {
       res = 1;
       break;
     }

   return res;
}

static void select_requests(Client* client)
{
   /** TODO:
    * 1: set the client state to SELECTING_REQUESTS
    * 2: choose a random combination of requests
    **/

   require (client != NULL, "client argument required");

   client->state = SELECTING_REQUESTS;
   log_client(client);
   client->requests = random_int(1,7);
   log_client(client);
}

static void wait_its_turn(Client* client)
{
   /** TODO:
    * 1: set the client state to WAITING_ITS_TURN
    * 2: enter barbershop (if necessary waiting for an empty seat)
    * 3. "handshake" with assigned barber (greet_barber)
    **/

   require (client != NULL, "client argument required");

   client->state = WAITING_ITS_TURN;
   log_client(client);

   while(bci_get_num_clients_in_bench() == client_benches(client->shop)->numSeats);
   client->benchesPosition = enter_barber_shop(client->shop,client->id,client->requests);
   bci_client_in();
   
   log_client(client);
   
   //WARNING
   //while(tmp_inter->currentState == NO_BARBER_GREET);
   while(bci_get_client_access(client->id) == 0);
   
   client->barberID = greet_barber(client->shop,client->id);
   log_client(client);
   
   //WARNING
   //tmp_inter->currentState = WAITING_ON_RESERVE;
   bci_set_state(client->barberID,WAITING_ON_RESERVE);
}

static void rise_from_client_benches(Client* client)
{
   /** TODO:
    * 1: (exactly what the name says)
    **/

   require (client != NULL, "client argument required");
   require (client != NULL, "client argument required");
   require (seated_in_client_benches(client_benches(client->shop), client->id), concat_3str("client ",int2str(client->id)," not seated in benches"));

   rise_client_benches(client_benches(client->shop),client->benchesPosition,client->id);
   client->benchesPosition = -1;
   bci_client_out();

   log_client(client);
}

static void wait_all_services_done(Client* client)
{
   /** TODO:
    * Expect the realization of one request at a time, until all requests are fulfilled.
    * For each request:
    * 1: set the client state to WAITING_SERVICE
    * 2: wait_service_from_barber from barbershop
    * 3: set the client state to WAITING_SERVICE_START
    * 4: sit in proper position in destination (chair/basin depending on the service selected)
    * 5: set the client state to the active service
    * 6: rise from destination
    *
    * At the end the client must leave the barber shop
    **/

   require (client != NULL, "client argument required");

   while(client->state != DONE) {
     client->state = WAITING_SERVICE;
     log_client(client);

     Service given_service = wait_service_from_barber(client->shop,client->barberID);
     Service* tmp_service = &(given_service);

     client->state = WAITING_SERVICE_START;
     log_client(client);  

     if(is_barber_chair_service(tmp_service)) {
       client->chairPosition = service_position(tmp_service);
       sit_in_barber_chair(barber_chair(client->shop,client->chairPosition),client->id);
     }
     else {
       client->basinPosition = service_position(tmp_service);
       sit_in_washbasin(washbasin(client->shop,client->basinPosition),client->id);
     }

     log_client(client);
     
     //WARNING
     //tmp_inter->currentState = WAITING_ON_PROCESS_START;
     //while(tmp_inter->currentState == WAITING_ON_PROCESS_START);
     
     bci_set_state(client->barberID,WAITING_ON_PROCESS_START);
     while(bci_get_state(client->barberID) == WAITING_ON_PROCESS_START);

     if(tmp_service->request == 1)
       client->state = HAVING_A_HAIRCUT;
     else if(tmp_service->request == 2)
       client->state = HAVING_A_HAIR_WASH;
     else
       client->state = HAVING_A_SHAVE;
   
     //WARNING
     //while(tmp_inter->currentState != PROCESS_DONE);
     while(bci_get_state(client->barberID) != PROCESS_DONE);
   
     log_client(client);

     if(is_barber_chair_service(&given_service))
       client->chairPosition = -1;
     else
       client->basinPosition = -1;    
  
     log_client(client);

     //WARNING
     //if(tmp_inter->currentState == ALL_PROCESSES_DONE) {
     //  client->state = DONE;
     //} 
     
     if(bci_get_state(client->barberID) == ALL_PROCESSES_DONE) {
        client->state = DONE;
     }
     
   }

   bci_unset_barberID(client->id);
   leave_barber_shop(client->shop,client->id);

   log_client(client); // more than one in proper places!!!
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

