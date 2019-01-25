/**
 * \brief barber bench representation
 *  
 * \author Miguel Oliveira e Silva - 2018
 */

#ifndef BARBER_BENCH_H
#define BARBER_BENCH_H

#include "global.h"

typedef struct _BarberBench_
{
	int numSeats;
	int id[MAX_BARBERS];
	int verticalOrientation;
	int logId;
	char* internal;

	pthread_mutex_t barberBenchMutex;
	pthread_cond_t barberBenchNotFull;
	pthread_cond_t barberBenchNotEmpty;
	pthread_cond_t barberBenchesAreFull;

} BarberBench;

void init_barber_bench(BarberBench* bench, int num_seats, int vertical_orientation, int line, int column);
void term_barber_bench(BarberBench* bench);
void log_barber_bench(BarberBench* bench);

int empty_barber_bench(BarberBench* bench);
int num_seats_available_barber_bench(BarberBench* bench);
int _num_seats_available_barber_bench_(BarberBench* bench);
int occupied_by_barber(BarberBench* bench, int pos, int id);

int bench_seat_occupied(BarberBench* bench, int pos);
int random_sit_in_barber_bench(BarberBench* bench, int id);
void rise_barber_bench(BarberBench* bench, int pos);
int seated_in_barber_bench(BarberBench* bench, int id);
int _seated_in_barber_bench_(BarberBench* bench, int id);

#endif
