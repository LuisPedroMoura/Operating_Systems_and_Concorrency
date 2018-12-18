#include <stdlib.h>
#include "dbc.h"
#include "global.h"
#include "utils.h"
#include "box.h"
#include "timer.h"
#include "logger.h"
#include "service.h"
#include "client.h"

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
   /** TODO:
    * 1: set the client state to WANDERING_OUTSIDE
    * 2. random a time interval [global->MIN_OUTSIDE_TIME_UNITS, global->MAX_OUTSIDE_TIME_UNITS]
    **/

   require (client != NULL, "client argument required");

   log_client(client);
}

static int vacancy_in_barber_shop(Client* client)
{
   /** TODO:
    * 1: set the client state to WAITING_BARBERSHOP_VACANCY
    * 2: check if there is an empty seat in the client benches (at this instante, later on it may fail)
    **/

   require (client != NULL, "client argument required");

   int res = 0;

   log_client(client);
   return res;
}

static void select_requests(Client* client)
{
   /** TODO:
    * 1: set the client state to SELECTING_REQUESTS
    * 2: choose a random combination of requests
    **/

   require (client != NULL, "client argument required");

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

   log_client(client);
}

static void rise_from_client_benches(Client* client)
{
   /** TODO:
    * 1: (exactly what the name says)
    **/

   require (client != NULL, "client argument required");
   require (client != NULL, "client argument required");
   require (seated_in_client_benches(client_benches(client->shop), client->id), concat_3str("client ",int2str(client->id)," not seated in benches"));

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

