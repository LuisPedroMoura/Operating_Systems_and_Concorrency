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
#include "communication-line.h"

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

   CommunicationLine commLine;

   int opened;

   int logId;
   char* internal;


   // Barber Shop Mutex and cond
   pthread_mutex_t shopFloorMutex = PTHREAD_MUTEX_INITIALIZER;
   pthread_cond_t clientLeft = PTHREAD_COND_INITIALIZER;
   pthread_cond_t closeShop = PTHREAD_COND_INITIALIZER;

   // Communications Line mutex and cond
   pthread_mutex_t messagesMutex[MAX_CLIENTS];
   pthread_cond_t messageAvailable[MAX_CLIENTS];

   // barber Bench mutex and cond
   pthread_mutex_t barberBenchMutex = PTHREAD_MUTEX_INITIALIZER;

   pthread_cond_t barberBenchNotFull = PTHREAD_COND_INITIALIZER;
   pthread_cond_t barberBenchNotEmpty = PTHREAD_COND_INITIALIZER;

//   // client Bench mutex and cond
//   pthread_mutex_t clientBenchMutex = PTHREAD_MUTEX_INITIALIZER;
//
//   pthread_cond_t clientWaiting = PTHREAD_COND_INITIALIZER;
//   pthread_cond_t clientReady = PTHREAD_COND_INITIALIZER;
//   pthread_cond_t clientSeatAvailable = PTHREAD_COND_INITIALIZER;

   // BarberChair mutex and cond
   pthread_mutex_t barberChairMutex = PTHREAD_MUTEX_INITIALIZER;

   pthread_cond_t barberChairAvailable = PTHREAD_COND_INITIALIZER;
   pthread_cond_t barberChairServiceFinished = PTHREAD_COND_INITIALIZER;
   pthread_cond_t clientRoseFromBarberChair = PTHREAD_COND_INITIALIZER;
   pthread_cond_t clientSatInBarberChair = PTHREAD_COND_INITIALIZER;
   pthread_cond_t clientReadyForShave = PTHREAD_COND_INITIALIZER;

   // Washbasin mutex and cond
   pthread_mutex_t washbasinMutex = PTHREAD_MUTEX_INITIALIZER;

   pthread_cond_t washbasinAvailable = PTHREAD_COND_INITIALIZER;
   pthread_cond_t washbasinServiceFinished = PTHREAD_COND_INITIALIZER;
   pthread_cond_t clientRoseFromWashbasin = PTHREAD_COND_INITIALIZER;
   pthread_cond_t clientSatInWashbasin = PTHREAD_COND_INITIALIZER;

   // ToolsPot mutex and cond
   pthread_mutex_t toolsPotMutex = PTHREAD_MUTEX_INITIALIZER;
   pthread_cond_t availableScissor = PTHREAD_COND_INITIALIZER;
   pthread_cond_t availableComb = PTHREAD_COND_INITIALIZER;
   pthread_cond_t availableRazor = PTHREAD_COND_INITIALIZER;

   // Logger mutex and cond
   pthread_mutex_t loggerMutex = PTHREAD_MUTEX_INITIALIZER;

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

void wait_for_available_seat_in_client_bench(BarberShop* shop);

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

#endif
