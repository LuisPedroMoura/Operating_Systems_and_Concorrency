#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "dbc.h"
#include "utils.h"
#include "box.h"
#include "logger.h"
#include "global.h"
#include "barber-shop.h"

/* TODO: take a careful look to all the non static (public) functions, to check
 * if a proper synchronization is needed.
 */


/* TODO: change here this file to your needs */

//shared memory structure
static int shmid = 74;
static BCInterface * bcinterfaces;

const long key = 0x213AL;

static const int skel_length = 10000;
static char skel[skel_length];

static char* to_string_barber_shop(BarberShop* shop);
static void lock();
static void unlock();

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

   /* create the shared memory */
   shmid = shmget(key, sizeof(BCInterface), 0600 | IPC_CREAT | IPC_EXCL);
   if (shmid == -1)
   {
       perror("Fail creating shared data");
       exit(EXIT_FAILURE);
   }

   /* attach shared memory to process addressing space */ 
   bcinterfaces = (BCInterface*)shmat(shmid, NULL, 0);
   if (bcinterfaces == (void*)-1)
   {
       perror("Fail connecting to shared data");
       exit(EXIT_FAILURE);
   }

   /* create access locker */
    bcinterfaces->semid = semget(key, 1, 0600 | IPC_CREAT | IPC_EXCL);
    if (bcinterfaces->semid == -1)
    {
        perror("Fail creating locker semaphore");
        exit(EXIT_FAILURE);
    }

   /* unlock shared data structure */
   unlock();

   /* detach shared memory from process addressing space */
   shmdt(bcinterfaces);
   bcinterfaces = NULL;

   bci_connect();
   bci_open_shop();

   for(int initsemc = 0; initsemc < MAX_CLIENTS; initsemc++){
     psem_init(&bcinterfaces->semClientAccess[initsemc++],1,0);
   }

   for(int initsem = 0; initsem < MAX_BARBERS; initsem++){
     psem_init(&bcinterfaces->semReserved[initsem],1,0);
     psem_init(&bcinterfaces->semServiceInfoAvailable[initsem],1,0);
     psem_init(&bcinterfaces->semWaitingOnClientSit[initsem],1,0);
     psem_init(&bcinterfaces->semClientSeated[initsem],1,0);
     psem_init(&bcinterfaces->semProcessing[initsem],1,0);
     psem_init(&bcinterfaces->semWaitingOnRise[initsem],1,0);
     psem_init(&bcinterfaces->semClientRisen[initsem],1,0);
     psem_init(&bcinterfaces->semProcessDone[initsem],1,0);
     psem_init(&bcinterfaces->semAllProcessesDone[initsem],1,0);
   }

   psem_init(&bcinterfaces->semBarberBench,1,1);  
   psem_init(&bcinterfaces->semClientBench,1,0);
   psem_init(&bcinterfaces->semEnterClientBench,1,1);
   psem_init(&bcinterfaces->semChairBasin,1,1);
   psem_init(&bcinterfaces->semToolPot,1,1);

   psem_init(&bcinterfaces->semScissors,1,num_scissors);
   psem_init(&bcinterfaces->semCombs,1,num_combs);
   psem_init(&bcinterfaces->semRazors,1,num_razors);

   bci_set_syncBenches(*client_benches(shop));
   bci_set_syncBBBench(*barber_bench(shop));
   bci_set_syncToolsPot(*tools_pot(shop));

   bci_set_syncBBChairAll(shop->barberChair);
   bci_set_syncWashbasinAll(shop->washbasin);

   for(int bb = 0; bb < MAX_BARBERS; bb++) {
     bci_set_barberFirstTrip(bb+1);
   }
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

int num_available_barber_chairs(BarberShop* shop)
{
   require (shop != NULL, "shop argument required");

   int res = 0;
   for(int pos = 0; pos < shop->numChairs ; pos++)
      if (empty_barber_chair(shop->barberChair+pos))
         res++;

   return res;
}

int reserve_random_empty_barber_chair(BarberShop* shop, int barberID)
{
   /** TODO:
    * function called from a barber, when reserving a empty barber chair
    **/

   require (shop != NULL, "shop argument required");
   require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));
   require (num_available_barber_chairs(shop) > 0, "barber chair not available");

   int r = random_int(1, num_available_barber_chairs(shop));
   int res;
   for(res = 0; r > 0 && res < shop->numChairs ; res++)
      if (empty_barber_chair(shop->barberChair+res))
         r--;
   res--;
   reserve_barber_chair(shop->barberChair+res, barberID);

   ensure (res >= 0 && res < shop->numChairs, "");

   return res;
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
   /** TODO:
    * function called from a barber, when reserving a empty washbasin
    **/

   require (shop != NULL, "shop argument required");
   require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));
   require (num_available_washbasin(shop) > 0, "washbasin not available");

   int r = random_int(1, num_available_washbasin(shop));
   int res;
   for(res = 0; r > 0 && res < shop->numWashbasins ; res++)
      if (empty_washbasin(shop->washbasin+res))
         r--;
   res--;
   reserve_washbasin(shop->washbasin+res, barberID);

   ensure (res >= 0 && res < shop->numWashbasins, "");

   return res;
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

Service wait_service_from_barber(BarberShop* shop, int barberID)
{
   /** TODO:
    * function called from a client, expecting to be informed of the next Service to be provided by a barber
    **/

   require (shop != NULL, "shop argument required");
   require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));

   Service tmp_servicerin;
   bci_get_service_by_barberID(barberID,&tmp_servicerin);

   return tmp_servicerin;
}

void inform_client_on_service(BarberShop* shop, Service service)
{
   /** TODO:
    * function called from a barber, expecting to inform a client of its next service
    **/

   require (shop != NULL, "shop argument required");
   Service* tmp_serv = &service;

   bci_set_service(tmp_serv->barberID,service);
}

void client_done(BarberShop* shop, int clientID)
{
   /** TODO:
    * function called from a barber, notifying a client that all its services are done
    **/

   require (shop != NULL, "shop argument required");
   require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

   Service tmp_service;
   bci_get_service_by_clientID(clientID,&tmp_service);
}

int enter_barber_shop(BarberShop* shop, int clientID, int request)
{
   /** TODO:
    * Function called from a client when entering the barbershop
    **/

   require (shop != NULL, "shop argument required");
   require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));
   require (request > 0 && request < 8, concat_3str("invalid request (", int2str(request), ")"));
   require (num_available_benches_seats(client_benches(shop)) > 0, "empty seat not available in client benches");
   require (!is_client_inside(shop, clientID), concat_3str("client ", int2str(clientID), " already inside barber shop"));

   int res = random_sit_in_client_benches(&shop->clientBenches, clientID, request);
	
   bci_set_syncBenches(*client_benches(shop));

   shop->clientsInside[shop->numClientsInside++] = clientID;
   return res;
}

void leave_barber_shop(BarberShop* shop, int clientID)
{
   /** TODO:
    * Function called from a client when leaving the barbershop
    **/

   require (shop != NULL, "shop argument required");
   require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));
   require (is_client_inside(shop, clientID), concat_3str("client ", int2str(clientID), " already inside barber shop"));

   int i;
   for(i = 0; shop->clientsInside[i] != clientID; i++)
      ;
   shop->numClientsInside--;
   check (shop->clientsInside[i] == clientID, "");
   for(; i < shop->numClientsInside; i++)
      shop->clientsInside[i] = shop->clientsInside[i+1];
}

void receive_and_greet_client(BarberShop* shop, int barberID, int clientID)
{
   /** TODO:
    * function called from a barber, when receiving a new client
    * it must send the barber ID to the client
    **/

   require (shop != NULL, "shop argument required");
   require (barberID > 0, concat_3str("invalid barber id (", int2str(barberID), ")"));
   require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

   bci_get_syncBenches(&(shop->clientBenches));

   bci_set_clientID(barberID,clientID);
   bci_set_barberID(barberID,clientID);
}

int greet_barber(BarberShop* shop, int clientID)
{
   /** TODO:
    * function called from a client, expecting to receive its barber's ID
    **/

   require (shop != NULL, "shop argument required");
   require (clientID > 0, concat_3str("invalid client id (", int2str(clientID), ")"));

   return bci_get_barberID(clientID);
}

int shop_opened(BarberShop* shop)
{
   require (shop != NULL, "shop argument required");

   return bci_get_shop_status();;
}

void close_shop(BarberShop* shop)
{
   require (shop != NULL, "shop argument required");
   require (bci_get_shop_status(), "barber shop already closed");

   bci_close_shop();

   bci_destroy();
   shop->opened = 0;
}

static char* to_string_barber_shop(BarberShop* shop)
{
   return gen_boxes(shop->internal, skel_length, skel);
}

static void lock()
{
    struct sembuf down = {0, -1, 0};
    if (semop(bcinterfaces->semid, &down, 1) == -1)
    {
        perror("lock");
        exit(EXIT_FAILURE);
    }
}

static void unlock()
{
    struct sembuf up = {0, 1, 0};
    if (semop(bcinterfaces->semid, &up, 1) == -1)
    {
        perror("unlock");
        exit(EXIT_FAILURE);
    }
}

void bci_connect()
{
    /* get access to the shared memory */
    shmid = shmget(key, 0, 0);
    if (shmid == -1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }

    /* attach shared memory to process addressing space */ 
    bcinterfaces = (BCInterface*)shmat(shmid, NULL, 0);
    if (bcinterfaces == (void*)-1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }
}

void bci_destroy()
{
    /* destroy the locker semaphore */
    semctl(bcinterfaces->semid, 0, IPC_RMID, NULL);

    /* detach shared memory from process addressing space */
    shmdt(bcinterfaces);
    bcinterfaces = NULL;

    /* ask OS to destroy the shared memory */
    shmctl(shmid, IPC_RMID, NULL);
    shmid = -1;
}

void bci_set_service(int barberID, Service service)
{
    lock();

	bcinterfaces->service[barberID-1] = service;

    unlock();
}

void bci_set_clientID(int barberID, int clientID)
{
    lock();

	bcinterfaces->clientIDs[clientID-1] = barberID;
    
    unlock();
}

void bci_set_barberID(int barberID, int clientID)
{
    lock();

	bcinterfaces->barberIDs[barberID-1] = clientID;
    
    unlock();
}

void bci_set_syncBenches(ClientBenches clientBenches)
{
    lock();

        bcinterfaces->syncBenches = clientBenches;

    unlock();
}

void bci_set_syncBBBench(BarberBench bbbench)
{
    lock();

        bcinterfaces->syncBBBench = bbbench;

    unlock();
}

void bci_set_syncToolsPot(ToolsPot toolsp)
{
    lock();

        bcinterfaces->syncToolPot = toolsp;

    unlock();
}

void bci_set_syncBBChairAll(BarberChair* bbchairs)
{
    lock();

        bcinterfaces->syncBBChairAll = bbchairs;

    unlock();
}

void bci_set_syncWashbasinAll(Washbasin* washbasins)
{
    lock();
  
        bcinterfaces->syncWashbasinAll = washbasins;

    unlock();
}

void bci_set_syncBBChair(BarberChair bbChair, int barberID)
{
    lock();

        bcinterfaces->syncBBChairs[barberID-1] = bbChair;

    unlock();
}

void bci_set_syncWashbasin(Washbasin basin, int barberID)
{
    lock();

        bcinterfaces->syncWashbasins[barberID-1] = basin;

    unlock();
}

void bci_set_request(int clientID,int request)
{
    lock();

	bcinterfaces->clientRequests[clientID-1] = request;

    unlock();
}

void bci_client_left()
{
    lock();

        bcinterfaces->numClientsThatLeft += 1;

    unlock();
}

void bci_open_shop()
{
    lock();

	bcinterfaces->shopOpen = 1;

    unlock();
}

void bci_close_shop()
{
    lock();

	bcinterfaces->shopOpen = 0;
}

void bci_did_request(int clientID)
{
    lock();

	if(bcinterfaces->clientRequests[clientID-1] >= 4)
	   bcinterfaces->clientRequests[clientID-1] -= 4;
	else if(bcinterfaces->clientRequests[clientID-1] >= 2)
	   bcinterfaces->clientRequests[clientID-1] -= 2;
	else
	   bcinterfaces->clientRequests[clientID-1] -= 1;

    unlock();
}

void bci_unset_clientID(int barberID)
{
    lock();

	for(int o=0; o<MAX_CLIENTS; o++) {
	  if(bcinterfaces->clientIDs[o] == barberID)
	    bcinterfaces->clientIDs[o] = 0;
	}
    
    unlock();
}

void bci_unset_barberID(int clientID)
{
    lock();

	for(int b=0; b<MAX_BARBERS; b++) {
	  if(bcinterfaces->barberIDs[b] == clientID)
	    bcinterfaces->barberIDs[b] = 0;
	}
    
    unlock();
}

void bci_client_in()
{
    lock();

	bcinterfaces->numClientsInBench += 1;
    
    unlock();
}

void bci_client_out()
{
    lock();

	bcinterfaces->numClientsInBench -= 1;
    
    unlock();
}

void bci_get_service_by_barberID(int barberID,Service* service)
{
    lock();

	*service = bcinterfaces->service[barberID-1];

    unlock();
}

void bci_get_service_by_clientID(int clientID,Service* service)
{
    lock();
     
        for(int l=0; l < MAX_BARBERS; l++) { 
	  Service* tmp_ls = &(bcinterfaces->service[l]);	
	  if(tmp_ls->clientID == clientID) 
 	    *service = bcinterfaces->service[l];
	}

    unlock();
}

int bci_get_client_access(int clientID)
{
    lock();

	int tmp_access = bcinterfaces->clientAccess[clientID-1];

    unlock();
    return tmp_access;
}

int bci_get_barberID(int clientID)
{
    lock();

 	int tmp_bid;
	for(int y=0; y<MAX_BARBERS; y++) {
	  if(bcinterfaces->barberIDs[y] == clientID) 
	    tmp_bid = y+1;
	}

    unlock();
    return tmp_bid;
}

int bci_get_clientID(int barberID)
{
    lock();

	int tmp_cid;
	for(int z=0; z<MAX_CLIENTS; z++) {
	  if(bcinterfaces->clientIDs[z] == barberID) 
	    tmp_cid = z+1;
	}

    unlock();
    return tmp_cid;
}

int bci_get_num_clients_in_bench()
{
    lock();

	int tmp_num = bcinterfaces->numClientsInBench;

    unlock();
    return tmp_num;
}

void bci_get_syncBenches(ClientBenches* clientBenches) 
{
    lock();

        *clientBenches = bcinterfaces->syncBenches;

    unlock();
}

void bci_get_syncBBBench(BarberBench* bbbench)
{
    lock();

        *bbbench = bcinterfaces->syncBBBench;

    unlock();
}

void bci_get_syncToolsPot(ToolsPot* toolsp)
{
    lock();

        *toolsp = bcinterfaces->syncToolPot;

    unlock();
}

void bci_get_syncBBChairAll(BarberChair* bbchairs)
{
    lock();

        bbchairs = bcinterfaces->syncBBChairAll;

    unlock(); 
}

void bci_get_syncWashbasinAll(Washbasin* washbasins)
{
    lock();

        washbasins = bcinterfaces->syncWashbasinAll;

    unlock(); 
}

void bci_get_syncBBChair(BarberChair* bbChair, int barberID)
{
    lock();

        *bbChair = bcinterfaces->syncBBChairs[barberID-1];

    unlock(); 
}

void bci_get_syncWashbasin(Washbasin* basin, int barberID)
{
    lock();

        *basin = bcinterfaces->syncWashbasins[barberID-1];

    unlock(); 
}

int bci_get_request(int clientID)
{
    lock();

	int tmp_req = bcinterfaces->clientRequests[clientID-1]; 

    unlock();
    return tmp_req;
}

int bci_get_next_request(int clientID)
{
    lock();

	int tmp_nreq = 0;
	if(bcinterfaces->clientRequests[clientID-1] >= 4)
	   tmp_nreq = 4;
	else if(bcinterfaces->clientRequests[clientID-1] >= 2)
	   tmp_nreq = 2;
	else if(bcinterfaces->clientRequests[clientID-1] >= 1)
	   tmp_nreq = 1;
        else
           tmp_nreq = 0;

    unlock();
    return tmp_nreq;
}

int bci_get_shop_status()
{
    lock();

        int state = bcinterfaces->shopOpen;

    unlock();
    return state;
}

int bci_get_numClientsThatLeft()
{
    lock();

        int numC = bcinterfaces->numClientsThatLeft;

    unlock();
    return numC;
}

void bci_grant_client_access(int clientID) 
{
    lock();

	bcinterfaces->clientAccess[clientID-1] = 1;

    unlock();
}

void bci_revoke_client_access(int clientID) 
{
    lock();

	bcinterfaces->clientAccess[clientID-1] = 0;

    unlock();
}

void bci_wait_semClientAccess(int clientID)
{
        sem_t* tmp_r = &(bcinterfaces->semClientAccess[clientID-1]);
        psem_wait(tmp_r);
}

void bci_wait_semReserved(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semReserved[barberID-1]);
        psem_wait(tmp_r);
}

void bci_wait_semServiceInfoAvailable(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semServiceInfoAvailable[barberID-1]);
        psem_wait(tmp_r);
}

void bci_wait_semWaitingOnClientSit(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semWaitingOnClientSit[barberID-1]);
        psem_wait(tmp_r);
}

void bci_wait_semClientSeated(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semClientSeated[barberID-1]);
        psem_wait(tmp_r);
}

void bci_wait_semProcessing(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semProcessing[barberID-1]);
        psem_wait(tmp_r);
}

void bci_wait_semWaitingOnRise(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semWaitingOnRise[barberID-1]);
        psem_wait(tmp_r);
}

void bci_wait_semClientRisen(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semClientRisen[barberID-1]);
        psem_wait(tmp_r);
}

void bci_wait_semProcessDone(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semProcessDone[barberID-1]);
        psem_wait(tmp_r);
}

void bci_wait_semAllProcessesDone(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semAllProcessesDone[barberID-1]);
        psem_wait(tmp_r);
}

void bci_post_semClientAccess(int clientID)
{
        sem_t* tmp_r = &(bcinterfaces->semClientAccess[clientID-1]);
        psem_post(tmp_r);
}

void bci_post_semReserved(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semReserved[barberID-1]);
        psem_post(tmp_r);
}

void bci_post_semServiceInfoAvailable(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semServiceInfoAvailable[barberID-1]);
        psem_post(tmp_r);
}

void bci_post_semWaitingOnClientSit(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semWaitingOnClientSit[barberID-1]);
        psem_post(tmp_r);
}

void bci_post_semClientSeated(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semClientSeated[barberID-1]);
        psem_post(tmp_r);
}

void bci_post_semProcessing(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semProcessing[barberID-1]);
        psem_post(tmp_r);
}

void bci_post_semWaitingOnRise(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semWaitingOnRise[barberID-1]);
        psem_post(tmp_r);
}

void bci_post_semClientRisen(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semClientRisen[barberID-1]);
        psem_post(tmp_r);
}

void bci_post_semProcessDone(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semProcessDone[barberID-1]);
        psem_post(tmp_r);
}

void bci_post_semAllProcessesDone(int barberID)
{
        sem_t* tmp_r = &(bcinterfaces->semAllProcessesDone[barberID-1]);
        psem_post(tmp_r);
}

int bci_get_semClientAccessValue(int clientID)
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semClientAccess[clientID-1]);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;
}

int bci_get_semReservedValue(int barberID)
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semReserved[barberID-1]);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;
}

int bci_get_semServiceInfoAvailableValue(int barberID)
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semServiceInfoAvailable[barberID-1]);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;
}

int bci_get_semWaitingOnClientSitValue(int barberID)
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semWaitingOnClientSit[barberID-1]);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;
}

int bci_get_semClientSeatedValue(int barberID)
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semClientSeated[barberID-1]);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;
}

int bci_get_semProcessingValue(int barberID)
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semProcessing[barberID-1]);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;
}

int bci_get_semWaitingOnRiseValue(int barberID)
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semWaitingOnRise[barberID-1]);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;
}

int bci_get_semClientRisenValue(int barberID)
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semClientRisen[barberID-1]);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;
}

int bci_get_semProcessDoneValue(int barberID)
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semProcessDone[barberID-1]);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;
}

int bci_get_semAllProcessesDoneValue(int barberID)
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semAllProcessesDone[barberID-1]);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;
}

void bci_wait_semBarberBench()
{
        sem_t* tmp_r = &(bcinterfaces->semBarberBench);
        psem_wait(tmp_r);
}

void bci_wait_semClientBench()
{
        sem_t* tmp_r = &(bcinterfaces->semClientBench);
        psem_wait(tmp_r);	
}

void bci_wait_semEnterClientBench()
{
        sem_t* tmp_r = &(bcinterfaces->semEnterClientBench);
        psem_wait(tmp_r);	
}

void bci_wait_semChairBasin()
{
        sem_t* tmp_r = &(bcinterfaces->semChairBasin);
        psem_wait(tmp_r);	
}

void bci_wait_semToolPot()
{
        sem_t* tmp_r = &(bcinterfaces->semToolPot);
        psem_wait(tmp_r);	
}

void bci_wait_semScissor()
{
        sem_t* tmp_r = &(bcinterfaces->semScissors);
        psem_wait(tmp_r);	
}

void bci_wait_semComb()
{
        sem_t* tmp_r = &(bcinterfaces->semCombs);
        psem_wait(tmp_r);	
}

void bci_wait_semRazor()
{
        sem_t* tmp_r = &(bcinterfaces->semRazors);
        psem_wait(tmp_r);	
}

void bci_post_semBarberBench()
{        
    lock();

        sem_t* tmp_r = &(bcinterfaces->semBarberBench);
        psem_post(tmp_r);
    
    unlock();	
}

void bci_post_semClientBench()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semClientBench);
        psem_post(tmp_r);

    unlock();	
}

void bci_post_semEnterClientBench()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semEnterClientBench);
        psem_post(tmp_r);

    unlock();	
}

void bci_post_semChairBasin()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semChairBasin);
        psem_post(tmp_r);

    unlock();	
}

void bci_post_semToolPot()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semToolPot);
        psem_post(tmp_r);	

    unlock();
}

void bci_post_semScissor()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semScissors);
        psem_post(tmp_r);	

    unlock();
}

void bci_post_semComb()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semCombs);
        psem_post(tmp_r);	

    unlock();
}

void bci_post_semRazor()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semRazors);
        psem_post(tmp_r);	

    unlock();
}

int bci_get_semBarberBenchValue()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semBarberBench);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;	
}

int bci_get_semClientBenchValue()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semClientBench);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;	
}

int bci_get_semEnterClientBenchValue()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semEnterClientBench);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;	
}

int bci_get_semChairBasinValue()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semChairBasin);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;	
}

int bci_get_semToolPotValue()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semToolPot);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;	
}

int bci_get_semScissorValue()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semScissors);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;	
}

int bci_get_semCombValue()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semCombs);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;	
}

int bci_get_semRazorValue()
{
    lock();

        sem_t* tmp_r = &(bcinterfaces->semRazors);
        int val; 
        int* pval = &val;
        sem_getvalue(tmp_r,pval);
        
    unlock();
    return *pval;	
}

void bci_set_barberFirstTrip(int barberID)
{
    lock();

        bcinterfaces->barberFirstTrip[barberID-1] = 1;

    unlock();
}

void bci_unset_barberFirstTrip(int barberID)
{
    lock();

        bcinterfaces->barberFirstTrip[barberID-1] = 0;

    unlock();
}

int bci_get_barberFirstTrip(int barberID)
{
    lock();

        int tmp_resp = bcinterfaces->barberFirstTrip[barberID-1];

    unlock();
    return tmp_resp;
}

