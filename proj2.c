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

#define semGETLOST "/xstrna14-ios2-get_lost"
#define semEMBARKING "/xstrna14-ios2-embarking"
#define semMEM "/xstrna14-ios2-mem"
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

/*
* Loads params from program arguments into global variables.
* In case of wrong format, the function writes all the wrong numbers and their right format and then exits program.
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
void log_count_init()
{
	int shmID_log_index;
	shmID_log_index = shm_open(shmKEYlog, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmID_log_index, shmSIZE);
    shm_log_index = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_log_index, 0);
    close(shmID_log_index);
	if(shm_log_index == (void*) -1) {
		fprintf(stderr, "Chyba pri pristupu ke sdilene pameti na shmKEYlog\n");
		exit(1);
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
	//overit uspesnost!!!
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
void serf_count_init()
{
	int shmID_serf_count;
	shmID_serf_count = shm_open(shmKEYserfs, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmID_serf_count, shmSIZE);
    shm_serf_count = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_serf_count, 0);
    close(shmID_serf_count);
	if(shm_serf_count == (void*) -1) {
		fprintf(stderr, "Chyba pri pristupu ke sdilene pameti na shmKEYserfs\n");
		exit(1);
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
	//overit uspesnost!!!
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
void hack_count_init()
{
	int shmID_hack_count;
	shmID_hack_count = shm_open(shmKEYhacks, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmID_hack_count, shmSIZE);
    shm_hack_count = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_hack_count, 0);
    close(shmID_hack_count);
	if(shm_hack_count == (void*) -1) {
		fprintf(stderr, "Chyba pri pristupu ke sdilene pameti na shmKEYhacks\n");
		exit(1);
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
	//overit uspesnost!!!
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
void boat_serf_init()
{
	int shmID_boat_serf;
	shmID_boat_serf = shm_open(shmKEYboat_serf, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmID_boat_serf, shmSIZE);
    shm_boat_serf = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_boat_serf, 0);
    close(shmID_boat_serf);
	if(shm_boat_serf == (void*) -1) {
		fprintf(stderr, "Chyba pri pristupu ke sdilene pameti na shmKEYboat_serf\n");
		exit(1);
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
	//overit uspesnost!!!
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
void boat_hack_init()
{
	int shmID_boat_hack;
	shmID_boat_hack = shm_open(shmKEYboat_hack, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmID_boat_hack, shmSIZE);
    shm_boat_hack = (int*)mmap(NULL, shmSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmID_boat_hack, 0);
    close(shmID_boat_hack);
	if(shm_boat_hack == (void*) -1) {
		fprintf(stderr, "Chyba pri pristupu ke sdilene pameti na shmKEYboat_hack\n");
		exit(1);
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
	//overit uspesnost!!!
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
	if(mode) printf("%-3d     : %s %-3d     : %-20s     : %-3d     : %-3d\n", shm_log_index[0], type, id, action, shm_hack_count[0], shm_serf_count[0]);
	else printf("%-3d     : %s %-3d     : %-20s\n", shm_log_index[0], type, id, action);
	
	log_count_close();
	hack_count_close();
	serf_count_close();
}

void one_serf(int id)
{
	sem_mem = sem_open(semMEM, O_RDWR);
	
	//starting
	sem_wait(sem_mem);
	write_log("SERF", id, "starts", 0);
	sem_post(sem_mem);
	
	//going to molo
	sem_wait(sem_mem);
	serf_count_open();
	shm_serf_count[0]++;
	serf_count_close();
	write_log("SERF", id, "created", 1);
	sem_post(sem_mem);
	
	sem_close(sem_mem);
	exit(0);
}

void one_hack(int id)
{
	sem_mem = sem_open(semMEM, O_RDWR);
	sem_embarking = sem_open(semEMBARKING, O_RDWR);
	sem_get_lost = sem_open(semGETLOST, O_RDWR);
	
	sem_wait(sem_mem);
	write_log("HACK", id, "starts", 0);
	sem_post(sem_mem);
	
	int left = 0;
	int is_captain = 0;
	
	//going to molo
	while(1) { //waiting for empty space on molo
		sem_wait(sem_mem);
		if(left == 1) write_log("HACK", id, "is back", 0);
		hack_count_open();
		serf_count_open();
		if((shm_serf_count[0]+shm_hack_count[0]) < max_molo) break;
		else {
				hack_count_close();
				serf_count_close();
				write_log("HACK", id, "leaves queue", 0);
				sem_post(sem_mem);
				left = 1;
				usleep(max_wait_time);
		}		
	}
	
	//finally going onto molo, changing stats
	serf_count_close();
	shm_hack_count[0]++;
	hack_count_close();
	write_log("HACK", id, "waits", 1);
	sem_post(sem_mem);
	
	//on the molo
	while(1) {
		sem_wait(sem_embarking);
		sem_wait(sem_mem);
		
		boat_hack_open();
		boat_serf_open();
		
		if((shm_boat_hack[0] < 4 && shm_boat_serf[0] == 0) || (shm_boat_hack[0] < 2 && shm_boat_serf[0] <= 2)) break;
		
		boat_hack_close();
		boat_serf_close();
		
		sem_post(sem_mem);
		sem_post(sem_embarking);
	}
	
	//can go to the boat and goes
	shm_boat_hack[0]++;
	if(shm_boat_hack[0] == 4 || (shm_boat_hack[0] == 2 && shm_boat_serf[0] == 2)) is_captain = 1; //stává se kapitánem a nechává si semafor sem_embarking
	else sem_post(sem_embarking); //není kapitánem, posílá semafor aby další mohli nalodit
	if(is_captain)
	sem_post(sem_mem);
	
	
	usleep(2000);
	
	//leaving molo
	sem_wait(sem_mem);
	hack_count_open();
	shm_hack_count[0]--;
	hack_count_close();
	sem_post(sem_mem);
	
	sem_close(sem_mem);
	exit(0);
}

void serf_generator()
{
	pid_t pid;

	sem_mem = sem_open(semMEM, O_RDWR);
	sem_embarking = sem_open(semEMBARKING, O_RDWR);
	sem_post(sem_mem); //poslani prvniho signalu pro pristup k shm
	//jediny semafor na pristup ke sdilene pameti
	sem_close(sem_mem);
	sem_post(sem_embarking); //signál, že je možno přistupovat na loď
	sem_close(sem_embarking);
	/*
	for(int i = 0; i < generated_people; i++) {
		pid = fork();
		if(pid == 0) one_serf(i);
		usleep(gen_serf_delay);
	}
	
	for(int i = 0; i < generated_people; i++) wait(NULL);
	*/
	exit(0);
}

void hack_generator()
{
	pid_t pid;
	
	for(int i = 0; i < generated_people; i++) {
		pid = fork();
		if(pid == 0) one_hack(i);
		usleep(gen_hack_delay);
	}
	
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
	
	pid_t pid;
	
	sem_mem = sem_open(semMEM, O_CREAT, 0666, 0);
	sem_close(sem_mem);
	sem_embarking = sem_open(semEMBARKING, O_CREAT, 0666, 0);
	sem_close(sem_embarking);
	sem_get_lost = sem_open(semGETLOST, O_CREAT, 0666, 0);
	sem_close(sem_get_lost);
	
	log_count_init();
	serf_count_init();
	hack_count_init();
	boat_hack_init();
	boat_serf_init();
	//konec inicializace sdílené paměti a semaforů

	pid=fork();
	if(pid == 0) serf_generator(); //vytvoření procesu na generování serfů
	else if (pid > 0) {
		pid=fork();
		if(pid == 0) hack_generator(); //vytvoření procesu na generování hacků
		if(pid < 0) { //chyba vytvoření procesu
			perror("fork");
			exit(2);
		}
	}
	else { //chyba vytvoření procesu
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
	log_count_unlink();
	serf_count_unlink();
	hack_count_unlink();
	boat_hack_unlink();
	boat_serf_unlink();

    return 0;
}