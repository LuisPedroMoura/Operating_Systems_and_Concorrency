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
	int clientsAlive;

	CommunicationLine commLine;

	int opened;

	int logId;
	char* internal;

	// Barber Shop Mutex and cond
	pthread_mutex_t shopFloorMutex;

	// BarberChair mutex and cond
	sem_t availableBarberChair;
	sem_t accessBarberChair;

	// Washbasin mutex and cond
	sem_t availableWashbasin;
	sem_t accessWashbasin;

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
void wait_for_client_to_sit_in_washbasin(BarberShop* shop, int basinPos);

int num_available_barber_chairs(BarberShop* shop);
int reserve_empty_barber_chair(BarberShop* shop, int barberID);
int reserve_random_empty_barber_chair(BarberShop* shop, int barberID);
void release_barber_barberchair(BarberShop* shop, int barberID, int barberPos);

int num_available_washbasin(BarberShop* shop);
int reserve_empty_washbasin(BarberShop* shop, int barberID);
int reserve_random_empty_washbasin(BarberShop* shop, int barberID);
void release_barber_washbasin(BarberShop* shop, int barberID, int barberPos);

int is_client_inside(BarberShop* shop, int clientID);

Service wait_service_from_barber(BarberShop* shop, int barberID);
void inform_client_on_service(BarberShop* shop, Service service);
void inform_client_on_service_start(BarberShop* shop, Service service);
void wait_for_service_start(BarberShop* shop, int clientID);

void client_was_born(BarberShop* shop);
void client_done(BarberShop* shop, int clientID);
void client_died(BarberShop* shop);

int enter_barber_shop(BarberShop* shop, int clientID, int request);
void leave_barber_shop(BarberShop* shop, int clientID);
void receive_and_greet_client(BarberShop* shop, int barberID, int clientID);
int greet_barber(BarberShop* shop, int clientID); // returns barberID

int shop_opened(BarberShop* shop);
void sem_up(sem_t* semaphore);
void sem_down(sem_t* semaphore);
void notify_all_barbers_to_leave_shop(BarberShop* shop);
void close_shop(BarberShop* shop); // no more outside clients accepted





#endif
