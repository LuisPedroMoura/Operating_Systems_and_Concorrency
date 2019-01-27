/**
 * \brief barber shop representation
 *  
 * \author Miguel Oliveira e Silva - 2018
 */

#ifndef BARBER_SHOP_H
#define BARBER_SHOP_H

#include "global.h"
#include "barber-chair.h"
#include "tools-pot.h"
#include "washbasin.h"
#include "barber-bench.h"
#include "service.h"
#include "client-benches.h"

typedef struct _BCInterface_
{	
   int semid;
   int shopOpen;
   
   int clientAccess[MAX_CLIENTS];

   ClientBenches syncBenches;
   BarberBench syncBBBench;
   ToolsPot syncToolPot;
   BarberChair* syncBBChairAll;
   Washbasin* syncWashbasinAll;  

   BarberChair syncBBChairs[MAX_BARBERS];
   Washbasin syncWashbasins[MAX_BARBERS];

   int barberIDs[MAX_BARBERS];
   int clientIDs[MAX_CLIENTS];

   int numClientsInBench;

   Service service[MAX_BARBERS];
   int clientRequests[MAX_CLIENTS]; 

   int barberFirstTrip[MAX_BARBERS];

   int numClientsThatLeft;  

   sem_t semClientAccess[MAX_CLIENTS];

   sem_t semReserved[MAX_BARBERS];
   sem_t semServiceInfoAvailable[MAX_BARBERS];
   sem_t semWaitingOnClientSit[MAX_BARBERS];
   sem_t semClientSeated[MAX_BARBERS];
   sem_t semProcessing[MAX_BARBERS];
   sem_t semWaitingOnRise[MAX_BARBERS];
   sem_t semClientRisen[MAX_BARBERS];
   sem_t semProcessDone[MAX_BARBERS];
   sem_t semAllProcessesDone[MAX_BARBERS];

   sem_t semBarberBench;
   sem_t semClientBench;
   sem_t semEnterClientBench;
   sem_t semChairBasin;
   sem_t semToolPot;

   sem_t semScissors;
   sem_t semCombs;
   sem_t semRazors;

} BCInterface;

typedef struct _BarberShop_
{
   int numBarbers;

   int numChairs;                         // num barber chairs
   BarberChair barberChair[MAX_BARBERS]; // index related with position

   int numScissors;
   int numCombs;
   int numRazors;
   ToolsPot toolsPot;

   int numWashbasins;
   Washbasin washbasin[MAX_WASHBASINS];  // index related with position

   BarberBench barberBench;

   int numClientBenchesSeats;
   int numClientBenches;
   ClientBenches clientBenches;

   int numClientsInside;
   int clientsInside[MAX_CLIENTS];

   int opened;

   int logId;
   char* internal;
} BarberShop;

int num_lines_barber_shop(BarberShop* shop);
int num_columns_barber_shop(BarberShop* shop);
void init_barber_shop(BarberShop* shop, int num_barbers, int num_chairs,
                      int num_scissors, int num_combs, int num_razors, int num_basins, 
                      int num_client_benches_seats, int num_client_benches);
void term_barber_shop(BarberShop* shop);
void show_barber_shop(BarberShop* shop);
void log_barber_shop(BarberShop* shop);

int valid_barber_chair_pos(BarberShop* shop, int pos);
BarberChair* barber_chair(BarberShop* shop, int pos);
ToolsPot* tools_pot(BarberShop* shop);
Washbasin* washbasin(BarberShop* shop, int pos);
BarberBench* barber_bench(BarberShop* shop);
ClientBenches* client_benches(BarberShop* shop);

int num_available_barber_chairs(BarberShop* shop);
int reserve_random_empty_barber_chair(BarberShop* shop, int barberID);
int num_available_washbasin(BarberShop* shop);
int reserve_random_empty_washbasin(BarberShop* shop, int barberID);

int is_client_inside(BarberShop* shop, int clientID);

Service wait_service_from_barber(BarberShop* shop, int barberID);
void inform_client_on_service(BarberShop* shop, Service service);

void client_done(BarberShop* shop, int clientID);

int enter_barber_shop(BarberShop* shop, int clientID, int request);
void leave_barber_shop(BarberShop* shop, int clientID);
void receive_and_greet_client(BarberShop* shop, int barberID, int clientID);
int greet_barber(BarberShop* shop, int clientID); // returns barberID

int shop_opened(BarberShop* shop);
void close_shop(BarberShop* shop); // no more outside clients accepted

void bci_connect();
void bci_destroy();

void bci_set_service(int barberID, Service service);
void bci_set_clientID(int barberID, int clientID);
void bci_set_barberID(int barberID, int clientID);
void bci_set_syncBenches(ClientBenches clientBenches);
void bci_set_syncBBBench(BarberBench bbbench);
void bci_set_syncToolsPot(ToolsPot toolsp);
void bci_set_syncBBChairAll(BarberChair* bbchairs);
void bci_set_syncWashbasinAll(Washbasin* washbasins);
void bci_set_syncBBChair(BarberChair bbChair,int barberID);
void bci_set_syncWashbasin(Washbasin basin, int barberID);
void bci_set_request(int clientID,int request);

void bci_client_left();
void bci_open_shop();
void bci_close_shop();
void bci_did_request(int clientID);
void bci_unset_clientID(int barberID);
void bci_unset_barberID(int clientID);
void bci_client_in();
void bci_client_out();

void bci_get_service_by_barberID(int barberID,Service* service);
void bci_get_service_by_clientID(int clientID,Service* service);
int bci_get_client_access(int clientID);
int bci_get_num_clients_in_bench();
int bci_get_barberID(int clientID);
int bci_get_clientID(int barberID);
void bci_get_syncBenches(ClientBenches* clientBenches);
void bci_get_syncBBBench(BarberBench* bbbench);
void bci_get_syncToolsPot(ToolsPot* toolsp);
void bci_get_syncBBChairAll(BarberChair* bbchairs);
void bci_get_syncWashbasinAll(Washbasin* washbasins);
void bci_get_syncBBChair(BarberChair* bbChair, int barberID);
void bci_get_syncWashbasin(Washbasin* basin, int barberID);
int bci_get_request(int clientID);
int bci_get_next_request(int clientID);
int bci_get_shop_status();
int bci_get_numClientsThatLeft();

void bci_grant_client_access(int clientID);
void bci_revoke_client_access(int clientID);

void bci_wait_semClientAccess(int clientID);
void bci_wait_semReserved(int barberID);
void bci_wait_semServiceInfoAvailable(int barberID);
void bci_wait_semWaitingOnClientSit(int barberID);
void bci_wait_semClientSeated(int barberID);
void bci_wait_semProcessing(int barberID);
void bci_wait_semWaitingOnRise(int barberID);
void bci_wait_semClientRisen(int barberID);
void bci_wait_semProcessDone(int barberID);
void bci_wait_semAllProcessesDone(int barberID);

void bci_post_semClientAccess(int clientID);
void bci_post_semReserved(int barberID);
void bci_post_semServiceInfoAvailable(int barberID);
void bci_post_semWaitingOnClientSit(int barberID);
void bci_post_semClientSeated(int barberID);
void bci_post_semProcessing(int barberID);
void bci_post_semWaitingOnRise(int barberID);
void bci_post_semClientRisen(int barberID);
void bci_post_semProcessDone(int barberID);
void bci_post_semAllProcessesDone(int barberID);

int bci_get_semClientAccessValue(int clientID);
int bci_get_semReservedValue(int barberID);
int bci_get_semServiceInfoAvailableValue(int barberID);
int bci_get_semWaitingOnClientSitValue(int barberID);
int bci_get_semClientSeatedValue(int barberID);
int bci_get_semProcessingValue(int barberID);
int bci_get_semWaitingOnRiseValue(int barberID);
int bci_get_semClientRisenValue(int barberID);
int bci_get_semProcessDoneValue(int barberID);
int bci_get_semAllProcessesDoneValue(int barberID);

void bci_wait_semBarberBench();
void bci_wait_semClientBench();
void bci_wait_semEnterClientBench();
void bci_wait_semChairBasin();
void bci_wait_semToolPot();
void bci_wait_semScissor();
void bci_wait_semComb();
void bci_wait_semRazor();

void bci_post_semBarberBench();
void bci_post_semClientBench();
void bci_post_semEnterClientBench();
void bci_post_semChairBasin();
void bci_post_semToolPot();
void bci_post_semScissor();
void bci_post_semComb();
void bci_post_semRazor();

int bci_get_semBarberBenchValue();
int bci_get_semClientBenchValue();
int bci_get_semEnterClientBenchValue();
int bci_get_semChairBasinValue();
int bci_get_semToolPotValue();
int bci_get_semScissorValue();
int bci_get_semCombValue();
int bci_get_semRazorValue();

void bci_set_barberFirstTrip(int barberID);
void bci_unset_barberFirstTrip(int barberID);
int bci_get_barberFirstTrip(int barberID);

#endif
