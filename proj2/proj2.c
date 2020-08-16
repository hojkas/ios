//IOS projekt 2
//River crossing problem
//Iveta Strnadová, xstrna14

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#define semGETLOST "/xstrna14-ios2-get_lost"
#define semEMBARKING "/xstrna14-ios2-embarking"
#define semMEM "/xstrna14-ios2-mem"
#define semLASTMAN "/xstrna14-ios2-last_man"
#define shmKEYlog "/xstrna14-ios2-keylog"
#define shmKEYserfs "/xstrna14-ios2-keyserfs"
#define shmKEYhacks "/xstrna14-ios2-keyhacks"
#define shmKEYboat_serf "/xstrna14-ios2-keyboat_serf"
#define shmKEYboat_hack "/xstrna14-ios2-keyboat_hack"
#define shmSIZE sizeof(int)

int generated_people;
int gen_hack_delay;
int gen_serf_delay;
int max_sail_time;
int max_wait_time;
int max_molo;

int *shm_log_index;
int *shm_serf_count;
int *shm_hack_count;
int *shm_boat_serf;
int *shm_boat_hack;

sem_t *sem_mem;
sem_t *sem_embarking;
sem_t *sem_get_lost;
sem_t *sem_last_man;

FILE *action_log;

/*
* Nahraje parametry do globalnich promennych. V pripade chyby vypise jak mel format vypadat a program se ukonci.
*/
void load_params(int argc, char* argv[])
{
	if(argc != 7) {
		fprintf(stderr, "Wrong number of parametrs.\n");
		exit(1);
	}
	
	int fatal_error = 0;
	
	//parsing P
	generated_people = strtol(argv[1], &argv[1], 10);
	if(argv[1][0] != '\0' || generated_people < 2 || generated_people%2 == 1) {
		fprintf(stderr, "Wrong format of argument 1 (P) - generated_people. Needs to be integer >= 2 and an even number.\n");
		fatal_error = 1;
	}
	
	//parsing H
	gen_hack_delay = strtol(argv[2], &argv[2], 10);
	if(argv[2][0] != '\0' || gen_hack_delay < 0 || gen_hack_delay > 2000) {
		fprintf(stderr, "Wrong format of argument 2 (H) - gen_hack_delay. Needs to be integer >= 0 && <= 2000.\n");
		fatal_error = 1;
	}
	
	//parsing S
	gen_serf_delay = strtol(argv[3], &argv[3], 10);
	if(argv[3][0] != '\0' || gen_serf_delay < 0 || gen_serf_delay > 2000) {
		fprintf(stderr, "Wrong format of argument 3 (S) - gen_serf_delay. Needs to be integer >= 0 && <= 2000.\n");
		fatal_error = 1;
	}
	
	//parsing R
	max_sail_time = strtol(argv[4], &argv[4], 10);
	if(argv[4][0] != '\0' || max_sail_time < 0 || max_sail_time > 2000) {
		fprintf(stderr, "Wrong format of argument 4 (R) - max_sail_time. Needs to be integer >= 0 && <= 2000.\n");
		fatal_error = 1;
	}
	
	//parsing W
	max_wait_time = strtol(argv[5], &argv[5], 10);
	if(argv[5][0] != '\0' || max_wait_time < 20 || max_wait_time > 2000) {
		fprintf(stderr, "Wrong format of argument 6 (W) - max_wait_time. Needs to be integer >= 20 && <= 2000.\n");
		fatal_error = 1;
	}
	
	//parsing C
	max_molo = strtol(argv[6], &argv[6], 10);
	if(argv[6][0] != '\0' || max_molo < 5) {
		fprintf(stderr, "Wrong format of argument 7 (C) - max_molo. Needs to be integer >= 5.\n");
		fatal_error = 1;
	}
	
	if(fatal_error) exit(1);
	return;
}

//FUNKCE NA VYTVOŘENÍ SHARED MEMORY PRO LOG_COUNT
void log_count_unlink();
void log_count_init()
{
	int shmID_log_index;
	shmID_log_index = shm_open(shmKEYlog, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmID_log_index, shmSIZE);
    shm_log_index = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_log_index, 0);
	close(shmID_log_index);
	if(shm_log_index == (void*) -1) {
		log_count_unlink();
		shmID_log_index = shm_open(shmKEYlog, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
		ftruncate(shmID_log_index, shmSIZE);
		shm_log_index = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_log_index, 0);
		close(shmID_log_index);
		if(shm_log_index == (void*) -1) {
			fprintf(stderr, "Chyba pri pristupu ke sdilene pameti na shmKEYlog\n");
			exit(1);
		}
	}
	shm_log_index[0] = 0;
	munmap(shm_log_index, shmSIZE);
}

void log_count_open()
{
	int shmID_log_index;
	shmID_log_index = shm_open(shmKEYlog, O_RDWR, S_IRUSR | S_IWUSR);
    shm_log_index = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shmID_log_index, 0);
    close(shmID_log_index);
}

void log_count_close()
{
	munmap(shm_log_index, shmSIZE);
}

void log_count_unlink()
{
	shm_unlink(shmKEYlog);
}
//KONEC FUNKCÍ NA VYTVOŘENÍ SHARED MEMORY PRO LOG_COUNT

//FUNKCE NA VYTVOŘENÍ SHARED MEMORY NA SERF_COUNT
void serf_count_unlink();
void serf_count_init()
{
	int shmID_serf_count;
	shmID_serf_count = shm_open(shmKEYserfs, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmID_serf_count, shmSIZE);
    shm_serf_count = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_serf_count, 0);
	close(shmID_serf_count);
	if(shm_serf_count == (void*) -1) {
		serf_count_unlink();
		shmID_serf_count = shm_open(shmKEYserfs, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
		ftruncate(shmID_serf_count, shmSIZE);
		shm_serf_count = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_serf_count, 0);
		close(shmID_serf_count);
		if(shm_serf_count == (void*) -1) {
			fprintf(stderr, "Chyba pri pristupu ke sdilene pameti na shmKEYserfs\n");
			log_count_unlink();
			exit(1);
		}
	}
	shm_serf_count[0] = 0;
	munmap(shm_serf_count, shmSIZE);
}

void serf_count_open()
{
	int shmID_serf_count;
	shmID_serf_count = shm_open(shmKEYserfs, O_RDWR, S_IRUSR | S_IWUSR);
    shm_serf_count = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shmID_serf_count, 0);
    close(shmID_serf_count);
}

void serf_count_close()
{
	munmap(shm_serf_count, shmSIZE);
}

void serf_count_unlink()
{
	shm_unlink(shmKEYserfs);
}
//KONEC FUNKCÍ NA VYTVOŘENÍ SHARED MEMORY PRO SERF_COUNT

//FUNKCE NA VYTVOŘENÍ SHARED MEMORY PRO HACK_COUNT
void hack_count_unlink();
void hack_count_init()
{
	int shmID_hack_count;
	shmID_hack_count = shm_open(shmKEYhacks, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmID_hack_count, shmSIZE);
    shm_hack_count = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_hack_count, 0);
    close(shmID_hack_count);
	if(shm_hack_count == (void*) -1) {
		hack_count_unlink();
		shmID_hack_count = shm_open(shmKEYhacks, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
		ftruncate(shmID_hack_count, shmSIZE);
		shm_hack_count = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_hack_count, 0);
		close(shmID_hack_count);
		if(shm_hack_count == (void*) -1) {
			fprintf(stderr, "Chyba pri pristupu ke sdilene pameti na shmKEYhacks\n");
			log_count_unlink();
			serf_count_unlink();
			exit(1);
		}
	}
	shm_hack_count[0] = 0;
	munmap(shm_hack_count, shmSIZE);
}

void hack_count_open()
{
	int shmID_hack_count;
	shmID_hack_count = shm_open(shmKEYhacks, O_RDWR, S_IRUSR | S_IWUSR);
    shm_hack_count = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shmID_hack_count, 0);
    close(shmID_hack_count);
}

void hack_count_close()
{
	munmap(shm_hack_count, shmSIZE);
}

void hack_count_unlink()
{
	shm_unlink(shmKEYhacks);
}
//KONEC FUNKCÍ NA VYTVOŘENÍ SHARED MEMORY PRO HACK_COUNT

//FUNKCE NA VYTVOŘENÍ SHARED MEMORY NA BOAT_SERF
void boat_serf_unlink();
void boat_serf_init()
{
	int shmID_boat_serf;
	shmID_boat_serf = shm_open(shmKEYboat_serf, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmID_boat_serf, shmSIZE);
    shm_boat_serf = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_boat_serf, 0);
    close(shmID_boat_serf);
	if(shm_boat_serf == (void*) -1) {
		boat_serf_unlink();
		shmID_boat_serf = shm_open(shmKEYboat_serf, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
		ftruncate(shmID_boat_serf, shmSIZE);
		shm_boat_serf = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_boat_serf, 0);
		close(shmID_boat_serf);
		if(shm_boat_serf == (void*) -1) {
			fprintf(stderr, "Chyba pri pristupu ke sdilene pameti na shmKEYboat_serf\n");
			log_count_unlink();
			serf_count_unlink();
			hack_count_unlink();
			exit(1);
		}
	}
	shm_boat_serf[0] = 0;
	munmap(shm_boat_serf, shmSIZE);
}

void boat_serf_open()
{
	int shmID_boat_serf;
	shmID_boat_serf = shm_open(shmKEYboat_serf, O_RDWR, S_IRUSR | S_IWUSR);
    shm_boat_serf = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shmID_boat_serf, 0);
    close(shmID_boat_serf);
}

void boat_serf_close()
{
	munmap(shm_boat_serf, shmSIZE);
}

void boat_serf_unlink()
{
	shm_unlink(shmKEYboat_serf);
}
//KONEC FUNKCÍ NA VYTVOŘENÍ SHARED MEMORY PRO BOAT_SERF

//FUNKCE NA VYTVOŘENÍ SHARED MEMORY PRO BOAT_HACK
void boat_hack_unlink();
void boat_hack_init()
{
	int shmID_boat_hack;
	shmID_boat_hack = shm_open(shmKEYboat_hack, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmID_boat_hack, shmSIZE);
    shm_boat_hack = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_boat_hack, 0);
    close(shmID_boat_hack);
	if(shm_boat_hack == (void*) -1) {
		boat_hack_unlink();
		shmID_boat_hack = shm_open(shmKEYboat_hack, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
		ftruncate(shmID_boat_hack, shmSIZE);
		shm_boat_hack = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_boat_hack, 0);
		close(shmID_boat_hack);
		if(shm_boat_hack == (void*) -1) {
			fprintf(stderr, "Chyba pri pristupu ke sdilene pameti na shmKEYboat_hack\n");
			log_count_unlink();
			serf_count_unlink();
			hack_count_unlink();
			boat_serf_unlink();
			exit(1);
		}
	}
	shm_boat_hack[0] = 0;
	munmap(shm_boat_hack, shmSIZE);
}

void boat_hack_open()
{
	int shmID_boat_hack;
	shmID_boat_hack = shm_open(shmKEYboat_hack, O_RDWR, S_IRUSR | S_IWUSR);
    shm_boat_hack = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shmID_boat_hack, 0);
    close(shmID_boat_hack);
}

void boat_hack_close()
{
	munmap(shm_boat_hack, shmSIZE);
}

void boat_hack_unlink()
{
	shm_unlink(shmKEYboat_hack);
}
//KONEC FUNKCÍ NA VYTVOŘENÍ SHARED MEMORY PRO BOAT_HACK

/*
* Vypíše log (bud i se stavem hacků a serfů na molu (mod - 1) nebo bez (mod - 0)
* POZOR! sama nehlídá právo na paměť šahat, musí celá být uvnitř kritické sekce
*/
void write_log(char* type, int id, char* action, int mode)
{
	//pristup k shared paměti
	log_count_open();
	hack_count_open();
	serf_count_open();
	
	shm_log_index[0]++;
	if(mode) fprintf(action_log, "%-3d     : %s %-3d     : %-20s     : %-3d     : %-3d\n", shm_log_index[0], type, id, action, shm_hack_count[0], shm_serf_count[0]);
	else fprintf(action_log, "%-3d     : %s %-3d     : %-20s\n", shm_log_index[0], type, id, action);
	
	log_count_close();
	hack_count_close();
	serf_count_close();
}

/*
* Životní cyklus jednoho serfu. Na konci proces končí.
*/
void one_serf(int id)
{
	//otevírání semaforů
	sem_mem = sem_open(semMEM, O_RDWR);
	sem_embarking = sem_open(semEMBARKING, O_RDWR);
	sem_get_lost = sem_open(semGETLOST, O_RDWR);
	sem_last_man = sem_open(semLASTMAN, O_RDWR);
	
	sem_wait(sem_mem);
	write_log("SERF", id, "starts", 0);
	sem_post(sem_mem);
	
	int left = 0;
	int is_captain = 0;
	int delay;
	
	//serf jde k molu
	while(1) { //serf čeká na volné místo na molu
		sem_wait(sem_mem);
		if(left == 1) write_log("SERF", id, "is back", 0);
		hack_count_open();
		serf_count_open();
		if((shm_serf_count[0]+shm_hack_count[0]) < max_molo) break; //dívá se, jestli je pro něj na molu místo (pokud ne, opustí pláž a jde se na čas projít)
		else {
				hack_count_close();
				serf_count_close();
				write_log("SERF", id, "leaves queue", 1);
				sem_post(sem_mem);
				left = 1;
				delay = (rand() % (max_wait_time - 19)) + 20;
				usleep(delay*1000);
		}		
	}
	
	//konečně vystupuje na molo, zvýší counter, vypíše log
	hack_count_close();
	shm_serf_count[0]++;
	serf_count_close();
	write_log("SERF", id, "waits", 1);
	sem_post(sem_mem);
	
	//na molu
	while(1) {
		//serf si počká na signál, že se může nastupovat (sem_embarking) a následně se podívá, jestli se podle pravidel vejde na loď
		//vejde-li se, cyklus se poruší, v opačném případě zase zavře paměti a pošle signál o možnosti nastoupit a přístupu k paměti dál
		sem_wait(sem_embarking);
		sem_wait(sem_mem);
		
		boat_hack_open();
		boat_serf_open();
		serf_count_open();
		hack_count_open();
		
		if((shm_boat_serf[0] < 4 && shm_boat_hack[0] == 0 && shm_serf_count[0] >= 4) || (shm_boat_serf[0] < 2 && shm_boat_hack[0] <= 2 && shm_serf_count[0] >=2 && shm_hack_count[0] >= 2)) break;
		
		serf_count_close();
		hack_count_close();
		boat_hack_close();
		boat_serf_close();
		
		sem_post(sem_mem);
		sem_post(sem_embarking);
	}
	
	//může jít na loď a nastupuje
	serf_count_close();
	hack_count_close();
	shm_boat_serf[0]++;
	//je-li loď plná, stává se kapitánem a nechává si semafor sem_embarking,
	//aby nikdo na molu zbytečně nekontroloval zdroje ani se nesnažil nastoupit
	if(shm_boat_serf[0] == 4 || (shm_boat_hack[0] == 2 && shm_boat_serf[0] == 2)) is_captain = 1; 
	else sem_post(sem_embarking); //není kapitánem, posílá semafor aby další mohli nalodit
	boat_hack_close();
	boat_serf_close();
	sem_post(sem_mem);
	
	if(is_captain) {
		//odepsani hacku a serfu z mola
		//vycisteni boat_serf a boat_hack
		sem_wait(sem_mem);
		serf_count_open();
		hack_count_open();
		boat_hack_open();
		boat_serf_open();
		
		if(shm_boat_serf[0] == 4) shm_serf_count[0] -= 4;
		if(shm_boat_serf[0] == 2) {
			shm_serf_count[0] -= 2;
			shm_hack_count[0] -= 2;
		}
		shm_boat_hack[0] = 0;
		shm_boat_serf[0] = 0;
		
		serf_count_close();
		hack_count_close();
		boat_hack_close();
		boat_serf_close();
		
		//oznamuje nalodeni
		write_log("SERF", id, "boards", 1);
		sem_post(sem_mem);
		
		delay = (rand() % (max_wait_time - 19)) + 20;
		usleep(delay*1000);
		
		//pošle tři signály cestujícím, aby vystoupili, sám čeká,
		//až se mu vrátí tři signály last_man značící, že už jsou všichni venku
		sem_post(sem_get_lost);
		sem_post(sem_get_lost);
		sem_post(sem_get_lost);
		sem_wait(sem_last_man);
		sem_wait(sem_last_man);
		sem_wait(sem_last_man);
		
		//pristoupi k pameti, vypise log
		sem_wait(sem_mem);
		write_log("SERF", id, "captain exits", 1);
		sem_post(sem_mem);
		//posle signal, ze uz je lod prazdna a muze se dalsi pokouset nastoupit
		sem_post(sem_embarking);
	}
	else {
		//ceka na signal, ze ma odejit
		sem_wait(sem_get_lost);
		sem_wait(sem_mem);
		write_log("SERF", id, "member exits", 1);
		sem_post(sem_mem);
		//posle signal kapitanovi, ze uz odesel
		sem_post(sem_last_man);
	}
	
	//zavreni semaforu
	sem_close(sem_last_man);
	sem_close(sem_get_lost);
	sem_close(sem_embarking);
	sem_close(sem_mem);
	exit(0);
}

/**
* stejný průběh jako u one_serf, jen kdekoliv byla proměnná s "serf" je nyní obdobná pro hack a naopak
*/
void one_hack(int id)
{
	//otevření semaforů
	sem_mem = sem_open(semMEM, O_RDWR);
	sem_embarking = sem_open(semEMBARKING, O_RDWR);
	sem_get_lost = sem_open(semGETLOST, O_RDWR);
	sem_last_man = sem_open(semLASTMAN, O_RDWR);
	
	sem_wait(sem_mem);
	write_log("HACK", id, "starts", 0);
	sem_post(sem_mem);
	
	int left = 0;
	int is_captain = 0;
	int delay;
	
	//jde k molu
	while(1) { //čekání na volné místo
		sem_wait(sem_mem);
		if(left == 1) write_log("HACK", id, "is back", 0);
		hack_count_open();
		serf_count_open();
		if((shm_serf_count[0]+shm_hack_count[0]) < max_molo) break;
		else {
				hack_count_close();
				serf_count_close();
				write_log("HACK", id, "leaves queue", 1);
				sem_post(sem_mem);
				left = 1;
				delay = (rand() % (max_wait_time - 19)) + 20;
				usleep(delay*1000);
		}		
	}
	
	//nastupuje na molo
	serf_count_close();
	shm_hack_count[0]++;
	hack_count_close();
	write_log("HACK", id, "waits", 1);
	sem_post(sem_mem);
	
	//na molu
	while(1) {
		sem_wait(sem_embarking); //čeká na signál, že se může naloďovat, pak zkontroluje, že vážně může
		sem_wait(sem_mem);
		
		boat_hack_open();
		boat_serf_open();
		hack_count_open();
		serf_count_open();
		
		if((shm_boat_hack[0] < 4 && shm_boat_serf[0] == 0 && shm_hack_count[0] >=4) || (shm_boat_hack[0] < 2 && shm_boat_serf[0] <= 2 && shm_serf_count[0] >=2 && shm_hack_count[0] >= 2)) break;
		
		hack_count_close();
		serf_count_close();
		boat_hack_close();
		boat_serf_close();
		
		sem_post(sem_mem);
		sem_post(sem_embarking);
	}
	
	//can go to the boat and goes
	hack_count_close();
	serf_count_close();
	shm_boat_hack[0]++;
	if(shm_boat_hack[0] == 4 || (shm_boat_hack[0] == 2 && shm_boat_serf[0] == 2)) is_captain = 1; //stává se kapitánem a nechává si semafor sem_embarking
	else sem_post(sem_embarking); //není kapitánem, posílá semafor aby další mohli nalodit
	boat_hack_close();
	boat_serf_close();
	sem_post(sem_mem);
	
	if(is_captain) {
		//odepsani hacku a serfu z mola
		//vycisteni boat_serf a boat_hack
		sem_wait(sem_mem);
		serf_count_open();
		hack_count_open();
		boat_hack_open();
		boat_serf_open();
		
		if(shm_boat_hack[0] == 4) shm_hack_count[0] -= 4;
		if(shm_boat_hack[0] == 2) {
			shm_serf_count[0] -= 2;
			shm_hack_count[0] -= 2;
		}
		shm_boat_hack[0] = 0;
		shm_boat_serf[0] = 0;
		
		serf_count_close();
		hack_count_close();
		boat_hack_close();
		boat_serf_close();
		
		write_log("HACK", id, "boards", 1);
		sem_post(sem_mem);
		
		delay = (rand() % (max_wait_time - 19)) + 20;
		usleep(delay*1000);
		
		//poslání 3 signálu "vystupte" a čekání na tři odezvy, než sám vystoupí
		sem_post(sem_get_lost);
		sem_post(sem_get_lost);
		sem_post(sem_get_lost);
		sem_wait(sem_last_man);
		sem_wait(sem_last_man);
		sem_wait(sem_last_man);
		
		sem_wait(sem_mem);
		write_log("HACK", id, "captain exits", 1);
		sem_post(sem_mem);
		sem_post(sem_embarking);
	}
	else {
		//čeká na signál výstupu, vypíše log, pošle signál, že je venku
		sem_wait(sem_get_lost);
		sem_wait(sem_mem);
		write_log("HACK", id, "member exits", 1);
		sem_post(sem_mem);
		sem_post(sem_last_man);
	}
	
	//zrušení semaforů
	sem_close(sem_last_man);
	sem_close(sem_get_lost);
	sem_close(sem_embarking);
	sem_close(sem_mem);
	exit(0);
}

void serf_generator()
{
	pid_t pid;
	int delay;

	sem_mem = sem_open(semMEM, O_RDWR);
	sem_embarking = sem_open(semEMBARKING, O_RDWR);
	sem_post(sem_mem); //poslani prvniho signalu pro pristup k shm
	//jediny semafor na pristup ke sdilene pameti
	sem_close(sem_mem);
	sem_post(sem_embarking); //signál, že je možno přistupovat na loď
	sem_close(sem_embarking);
	
	//cyklus generuje procesy
	for(int i = 1; i <= generated_people; i++) {
		pid = fork();
		if(pid == 0) one_serf(i);
		if(pid < 0) {
			perror("fork");
			exit(2);
		}
		delay = rand()%(gen_serf_delay+1);
		usleep(delay*1000);
	}
	
	//čeká na ukončené child procesy
	for(int i = 0; i < generated_people; i++) wait(NULL);
	
	exit(0);
}

void hack_generator()
{
	pid_t pid;
	int delay;
	
	//generování hacků
	for(int i = 1; i <= generated_people; i++) {
		pid = fork();
		if(pid == 0) one_hack(i);
		if(pid < 0) {
			perror("fork");
			exit(2);
		}
		delay = rand()%(gen_hack_delay+1);
		usleep(delay*1000);
	}
	
	//čeká na ukončení childs
	for(int i = 0; i < generated_people; i++) wait(NULL);
	
	exit(0);
}

/*
* Inicializuje všechny semafory a paměti, dvakrát se rozdvojí a vytvoří tím
* generátor serfů a hacků, počká, až generátory skončí, poté unlinkuje semafory
* a paměti a sám končí
*/
int main(int argc, char* argv[])
{
    setbuf(stdout,NULL);
    setbuf(stderr,NULL);
	
	load_params(argc, argv); //nahrání argumentů
	
	action_log = fopen("proj2.out", "w");
	if(action_log == NULL) {
		fprintf(stderr, "Error in opening file for log\n");
		exit(1);
	}
	
	setbuf(action_log, NULL);
	
	pid_t pid;
	srand(time(NULL));
	
	log_count_init();
	serf_count_init();
	hack_count_init();
	boat_serf_init();
	boat_hack_init();
	
	sem_mem = sem_open(semMEM, O_CREAT, 0666, 0);
	sem_close(sem_mem);
	sem_embarking = sem_open(semEMBARKING, O_CREAT, 0666, 0);
	sem_close(sem_embarking);
	sem_get_lost = sem_open(semGETLOST, O_CREAT, 0666, 0);
	sem_close(sem_get_lost);
	sem_last_man = sem_open(semLASTMAN, O_CREAT, 0666, 0);
	sem_close(sem_last_man);
	//konec inicializace sdílené paměti a semaforů

	pid=fork();
	if(pid == 0) serf_generator(); //vytvoření procesu na generování serfů
	else if (pid > 0) {
		pid=fork();
		if(pid == 0) hack_generator(); //vytvoření procesu na generování hacků
		if(pid < 0) { //chyba vytvoření procesu, zrušení zdrojů a ukončení se
			sem_unlink(semMEM);
			sem_unlink(semEMBARKING);
			sem_unlink(semGETLOST);
			sem_unlink(semLASTMAN);
			log_count_unlink();
			serf_count_unlink();
			hack_count_unlink();
			boat_hack_unlink();
			boat_serf_unlink();
			fclose(action_log);
			perror("fork");
			exit(2);
		}
	}
	else { //chyba vytvoření procesu, zrušení zdrojů a ukončení se
		sem_unlink(semMEM);
		sem_unlink(semEMBARKING);
		sem_unlink(semGETLOST);
		sem_unlink(semLASTMAN);
		log_count_unlink();
		serf_count_unlink();
		hack_count_unlink();
		boat_hack_unlink();
		boat_serf_unlink();
		fclose(action_log);
		perror("fork");
		exit(2);
	}
	
	//čekání na ukončení obou procesů (serf a hack generátory)
	wait(NULL);
	wait(NULL);
	
	//odstranění sdílené paměti a semaforů
	sem_unlink(semMEM);
	sem_unlink(semEMBARKING);
	sem_unlink(semGETLOST);
	sem_unlink(semLASTMAN);
	log_count_unlink();
	serf_count_unlink();
	hack_count_unlink();
	boat_hack_unlink();
	boat_serf_unlink();
	fclose(action_log);

    return 0;
}
