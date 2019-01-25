/**
 *  \brief Barber shop simulation
 *  
 * [F]SO second assignment.
 * 
 * \author Miguel Oliveira e Silva - 2018/2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include "include/thread.h"
#include "dbc.h"
#include "global.h"
#include "utils.h"
#include "box.h"
#include "timer.h"
#include "logger.h"
#include "barber.h"
#include "client.h"

static BarberShop *shop;
static Barber* allBarbers = NULL;
static Client* allClients = NULL;
static int logIdBarbersDesc;
static int logIdClientsDesc;

static pthread_t bthr[MAX_BARBERS];
static pthread_t cthr[MAX_CLIENTS];

/* internal functions */
static void help(char* prog, Parameters *params);
static void processArgs(Parameters *params, int argc, char* argv[]);
static void showParams(Parameters *params);
static void go();
static void finish();
static void initSimulation();

int main(int argc, char* argv[])
{
	// default parameter values:
	Parameters params = {
			// barber shop:
			4, 3, 2, 1, 2, 6, 1,
			// vitatily
			2, 6,
			// barbers:
			//5, 10, 100,
			1, 10, 100,
			// clients:
			//10, 1, 3, 10, 100, 60, 30, 20
			1, 1, 3, 10, 100, 60, 30, 20
	};

	set_time_unit(10); // default time unit

	global = (Parameters*)mem_alloc(sizeof(Parameters));
	*global = params;
	processArgs(global, argc, argv);
	showParams(global);
	printf("<press RETURN>");
	getchar();

	initSimulation();
	go();
	finish();

	return 0;
}

/**
 * launch threads/processes for barbers and clients
 */
static void go()
{
	require (allBarbers != NULL, "list of barbers data structures not created");
	require (allClients != NULL, "list of clients data structures not created");

	launch_logger();
	char* descText;
	descText = (char*)"Barbers:";
	send_log(logIdBarbersDesc, (char*)descText);
	descText = (char*)"Clients:";
	send_log(logIdClientsDesc, (char*)descText);
	show_barber_shop(shop);
	for(int i = 0; i < global->NUM_BARBERS; i++)
		log_barber(allBarbers+i);
	for(int i = 0; i < global->NUM_CLIENTS; i++)
		log_client(allClients+i);

	/* launching the barbers */
	for (int i = 0; i < global->NUM_BARBERS; i++)
	{
		thread_create(&bthr[i], NULL, main_barber, &allBarbers[i]);
	}

	/* launching the clients */
	for (int i = 0; i < global->NUM_CLIENTS; i++)
	{
		thread_create(&cthr[i], NULL, main_client, &allClients[i]);
	}
}

/**
 * synchronize with the termination of all active entities (barbers and clients),
 */
static void finish()
{
	/* wait for threads to conclude */
	for (int i = 0; i < global->NUM_CLIENTS; i++)
	{
		thread_join(cthr[i], NULL);
	}

	// all clients where joined, shop may be closed
	close_shop(shop);


	for (int i = 0; i < global->NUM_BARBERS; i++)
	{
		thread_join(bthr[i], NULL);
	}

	term_logger();
}

static void initSimulation()
{
	/* TODO: change this function to your needs */

	srand(time(NULL));
	init_thread_logger();
	logger_filter_out_boxes();

	shop = (BarberShop*)mem_alloc(sizeof(BarberShop));
	init_barber_shop(shop, global->NUM_BARBERS, global->NUM_BARBER_CHAIRS,
			global->NUM_SCISSORS, global->NUM_COMBS, global->NUM_RAZORS, global->NUM_WASHBASINS,
			global->NUM_CLIENT_BENCHES_SEATS, global->NUM_CLIENT_BENCHES);

	char* descText;
	descText = (char*)"Barbers:";
	char* translationsBarbers[] = {
			descText, (char*)"",
			NULL
	};
	logIdBarbersDesc = register_logger(descText, num_lines_barber_shop(shop) ,0 , 1, strlen(descText), translationsBarbers);
	allBarbers = (Barber*)mem_alloc(sizeof_barber()*global->NUM_BARBERS);
	for(int i = 0; i < global->NUM_BARBERS; i++)
		init_barber(allBarbers+i, i+1, shop, num_lines_barber_shop(shop)+1, i*num_columns_barber());

	descText = (char*)"Clients:";
	char* translationsClients[] = {
			descText, (char*)"",
			NULL
	};
	logIdClientsDesc = register_logger(descText, num_lines_barber_shop(shop)+1+num_lines_barber() ,0 , 1, strlen(descText), translationsClients);
	allClients = (Client*)mem_alloc(sizeof_client()*global->NUM_CLIENTS);
	for(int i = 0; i < global->NUM_CLIENTS; i++)
		init_client(allClients+i, i+1, shop, random_int(global->MIN_BARBER_SHOP_TRIPS, global->MAX_BARBER_SHOP_TRIPS), num_lines_barber_shop(shop)+1+num_lines_barber()+1, i*num_columns_client());

}

/*********************************************************************/
// No need to change remaining code!

static void help(char* prog, Parameters *params)
{
	require (prog != NULL, "program name argument required");
	require (params != NULL, "parameters argument required");

	printf("\n");
	printf("Usage: %s [OPTION] ...\n", prog);
	printf("\n");
	printf("Options:\n");
	printf("\n");
	printf("  -h,--help                                   show this help\n");
	printf("  -l,--line-mode\n");
	printf("  -w,--window-mode (default)\n");
	printf("  -b,--num-barbers <N>\n");
	printf("     number of barbers (default is %d)\n", params->NUM_BARBERS);
	printf("  -n,--num-clients <N>\n");
	printf("     number of clients (default is %d)\n", params->NUM_CLIENTS);
	printf("  -c,--num-chairs <N>\n");
	printf("     number of barber chairs (default is %d)\n", params->NUM_BARBER_CHAIRS);
	printf("  -t,--num-tools <SCISSORS>,<COMBS>,<RAZORS>\n");
	printf("     amount of each existing tool (default is [%d,%d,%d])\n",params->NUM_SCISSORS, params->NUM_COMBS, params->NUM_RAZORS);
	printf("  -1,--num-basins <N>\n");
	printf("     number of washbasins (default is %d)\n", params->NUM_WASHBASINS);
	printf("  -2,--num-client-benches-seats <TOTAL_SEATS>,<NUM_BENCHES>\n");
	printf("     number of client benches seats and benches (default is [%d,%d])\n", params->NUM_CLIENT_BENCHES_SEATS,params->NUM_CLIENT_BENCHES);
	printf("  -3,--work-time-units <MIN>,<MAX>\n");
	printf("     min./max. time units for each barbers's activity (default is [%d,%d])\n",params->MIN_WORK_TIME_UNITS, params->MAX_WORK_TIME_UNITS);
	printf("  -4,--barber-shop-trips <MIN>,<MAX>\n");
	printf("     min./max. number os trips to barber shop by each client (default is [%d,%d])\n",params->MIN_BARBER_SHOP_TRIPS, params->MAX_BARBER_SHOP_TRIPS);
	printf("  -5,--outside-time-units <MIN>,<MAX>\n");
	printf("     min./max. time units for each clients's activity outside the shop (default is [%d,%d])\n",params->MIN_OUTSIDE_TIME_UNITS, params->MAX_OUTSIDE_TIME_UNITS);
	printf("  -p,--prob-requests <HAIRCUT>,<WASH_HAIR>,<SHAVE>\n");
	printf("     probability for a client to select each activity (default is [%d,%d,%d])\n",params->PROB_REQUEST_HAIRCUT, params->PROB_REQUEST_WASHHAIR, params->PROB_REQUEST_SHAVE);
	printf("  -v,--vitality-time-units <MIN>,<MAX>\n");
	printf("     min./max. time units for barber/client instant speed of living (default is [%d,%d])\n",params->MIN_VITALITY_TIME_UNITS, params->MAX_VITALITY_TIME_UNITS);
	printf("  -u,--time-units <N>\n");
	printf("     simulation time unit (default is %d ms)\n", time_unit());
	printf("\n");
}

static void processArgs(Parameters *params, int argc, char* argv[])
{
	require (params != NULL, "parameters argument required");
	require (argc >= 0 && argv != NULL && argv[0] != NULL, "invalid main arguments");

	static struct option long_options[] =
	{
			{"help",                         no_argument,       NULL, 'h'},
			{"--line-mode",                  no_argument,       NULL, 'l'},
			{"--window-mode",                no_argument,       NULL, 'w'},
			{"--num-barbers",                required_argument, NULL, 'b'},
			{"--num-clients",                required_argument, NULL, 'n'},
			{"--num-chairs",                 required_argument, NULL, 'c'},
			{"--num-tools",                  required_argument, NULL, 't'},
			{"--num-basins",                 required_argument, NULL, '1'},
			{"--num-client-benches-seats",   required_argument, NULL, '2'},
			{"--work-time-units",            required_argument, NULL, '3'},
			{"--barber-shop-trips",          required_argument, NULL, '4'},
			{"--outside-time-units",         required_argument, NULL, '5'},
			{"--prob-requests",              required_argument, NULL, 'p'},
			{"--vitality-time-units",        required_argument, NULL, 'v'},
			{"--time-unit",                  required_argument, NULL, 'u'},
			{0, 0, NULL, 0}
	};
	int op=0;

	while (op != -1)
	{
		int option_index = 0;

		op = getopt_long(argc, argv, "hlwb:n:c:t:1:2:3:4:5:p:v:u:", long_options, &option_index);
		int st,n,o,p,min,max;
		switch (op)
		{
		case -1:
			break;

		case 'h':
		help(argv[0], params);
		exit(EXIT_SUCCESS);

		case 'l':
			if (!line_mode_logger())
				set_line_mode_logger();
			break;

		case 'w':
			if (line_mode_logger())
				set_window_mode_logger();
			break;

		case 'b':
			st = sscanf(optarg, "%d", &n);
			if (st != 1 || n < 1 || n > MAX_BARBERS)
			{
				fprintf(stderr, "ERROR: invalid number of barbers \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->NUM_BARBERS = n;
			break;

		case 'n':
			st = sscanf(optarg, "%d", &n);
			if (st != 1 || n < 1 || n > MAX_CLIENTS)
			{
				fprintf(stderr, "ERROR: invalid number of clients \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->NUM_CLIENTS = n;
			break;

		case 'c':
			st = sscanf(optarg, "%d", &n);
			if (st != 1 || n < 1 || n > MAX_BARBER_CHAIRS)
			{
				fprintf(stderr, "ERROR: invalid number of barber chairs \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->NUM_BARBER_CHAIRS = n;
			break;

		case 't':
			st = sscanf(optarg, "%d,%d,%d", &n, &o, &p);
			if (st != 3 || n < 1 || n > MAX_NUM_TOOLS || o < 1 || o > MAX_NUM_TOOLS || p < 1 || p > MAX_NUM_TOOLS)
			{
				fprintf(stderr, "ERROR: invalid number of tools \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->NUM_SCISSORS = n;
			params->NUM_COMBS = o;
			params->NUM_RAZORS = p;
			break;

		case '1':
			st = sscanf(optarg, "%d", &n);
			if (st != 1 || n < 1 || n > MAX_WASHBASINS)
			{
				fprintf(stderr, "ERROR: invalid number of washbasins \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->NUM_WASHBASINS = n;
			break;

		case '2':
			st = sscanf(optarg, "%d,%d", &n, &o);
			if (st != 2 || n < 1 || n > MAX_CLIENT_BENCHES_SEATS || o < 1 || o > n)
			{
				fprintf(stderr, "ERROR: invalid number of client benches seats \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->NUM_CLIENT_BENCHES_SEATS = n;
			params->NUM_CLIENT_BENCHES = o;
			break;

		case '3':
			st = sscanf(optarg, "%d,%d", &min, &max);
			if (st != 2 || min < 1 || max < min)
			{
				fprintf(stderr, "ERROR: invalid interval for barber work time units \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->MIN_WORK_TIME_UNITS = min;
			params->MAX_WORK_TIME_UNITS = max;
			break;

		case '4':
			st = sscanf(optarg, "%d,%d", &min, &max);
			if (st != 2 || min < 1 || max < min)
			{
				fprintf(stderr, "ERROR: invalid interval for client barber trips \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->MIN_BARBER_SHOP_TRIPS = min;
			params->MAX_BARBER_SHOP_TRIPS = max;
			break;

		case '5':
			st = sscanf(optarg, "%d,%d", &min, &max);
			if (st != 2 || min < 1 || max < min)
			{
				fprintf(stderr, "ERROR: invalid interval for client outside time units \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->MIN_OUTSIDE_TIME_UNITS = min;
			params->MAX_OUTSIDE_TIME_UNITS = max;
			break;

		case 'p':
			st = sscanf(optarg, "%d,%d,%d", &n, &o, &p);
			if (st != 3 || n < 0 || n > 100 || o < 0 || o > 100 || p < 0 || p > 100)
			{
				fprintf(stderr, "ERROR: invalid probabilities for barber shop service requests \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->PROB_REQUEST_HAIRCUT = n;
			params->PROB_REQUEST_WASHHAIR = o;
			params->PROB_REQUEST_SHAVE = p;
			break;

		case 'v':
			st = sscanf(optarg, "%d,%d", &min, &max);
			if (st != 2 || min < 1 || max < min)
			{
				fprintf(stderr, "ERROR: invalid interval for barber/client vitality time units \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->MIN_VITALITY_TIME_UNITS = min;
			params->MAX_VITALITY_TIME_UNITS = max;
			break;

		case 'u':
			st = sscanf(optarg, "%d", &n);
			if (st != 1 || n <= 0)
			{
				fprintf(stderr, "ERROR: invalid time unit \"%s\"\n", optarg);
				exit(EXIT_FAILURE);
			}
			set_time_unit(n);
			break;

		default:
			help(argv[0], params);
			exit(EXIT_FAILURE);
			break;
		}
	}

	if (optind < argc)
	{
		fprintf(stderr, "ERROR: invalid extra arguments\n");
		exit(EXIT_FAILURE);
	}
}

static void showParams(Parameters *params)
{
	require (params != NULL, "parameters argument required");

	printf("\n");
	printf("Simulation parameters (%s):\n", line_mode_logger() ? "line mode" : "window mode");
	printf("  --num-barbers: %d\n", params->NUM_BARBERS);
	printf("  --num-clients: %d\n", params->NUM_CLIENTS);
	printf("  --num-chairs: %d\n", params->NUM_BARBER_CHAIRS);
	printf("  --num-tools: [scissors:%d,combs:%d,razors:%d]\n", params->NUM_SCISSORS, params->NUM_COMBS, params->NUM_RAZORS);
	printf("  --num-basins: %d\n", params->NUM_WASHBASINS);
	printf("  --num-client-benches-seats: [total:%d,num-benches:%d]\n", params->NUM_CLIENT_BENCHES_SEATS, params->NUM_CLIENT_BENCHES);
	printf("  --work-time-units: [%d,%d]\n", params->MIN_WORK_TIME_UNITS, params->MAX_WORK_TIME_UNITS);
	printf("  --barber-shop-trips: [%d,%d]\n", params->MIN_BARBER_SHOP_TRIPS, params->MAX_BARBER_SHOP_TRIPS);
	printf("  --outside-time-units: [%d,%d]\n", params->MIN_OUTSIDE_TIME_UNITS, params->MAX_OUTSIDE_TIME_UNITS);
	printf("  --prob-requests: [haircut:%d,wash-hair:%d,shave:%d]\n", params->PROB_REQUEST_HAIRCUT, params->PROB_REQUEST_WASHHAIR, params->PROB_REQUEST_SHAVE);
	printf("  --vitality-time-units: [%d,%d]\n", params->MIN_VITALITY_TIME_UNITS, params->MAX_VITALITY_TIME_UNITS);
	printf("  --time-unit: %d ms\n", time_unit());
	printf("\n");
}

