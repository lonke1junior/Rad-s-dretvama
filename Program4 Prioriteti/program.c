/*
 * Jednostavni primjer za generiranje prostih brojeva korištenjem
   "The GNU Multiple Precision Arithmetic Library" (GMP)
 */

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <semaphore.h>
#include "slucajni_prosti_broj.h"
#include <sched.h>

#define N 7
#define KRAJ_RADA 1
#define NIJE_KRAJ 0
#define ZADNJA_RA 2
#define ZADNJA_NE 2
#define BROJ_I_ITERACIJA	5
#define BROJ_J_ITERACIJA	400000000

#define MASKA(bitova)			(-1 + (1<<(bitova)) )
#define UZMIBITOVE(broj,prvi,bitova) 	( ( (broj) >> (64-(prvi)) ) & MASKA(bitova) )


struct gmp_pomocno p;

uint64_t MS[10], ULAZ = 0, IZLAZ = 0, BROJAC = 0;
uint64_t kraj = NIJE_KRAJ;
uint64_t velicina_grupe;
sem_t prazni;
sem_t puni;
sem_t ko;

void stavi_u_MS(uint64_t broj)
{
	MS[ULAZ] = broj;
	ULAZ = (ULAZ + 1)%10;
	BROJAC++;
	if(BROJAC > 10){
		BROJAC--;
		IZLAZ = (IZLAZ + 1)%10;
	}
}

uint64_t uzmi_iz_MS()
{
	uint64_t broj = MS[IZLAZ];
	if(BROJAC>0){
		IZLAZ = (IZLAZ + 1)%10;
		BROJAC--;
	}
	return broj;
}

uint64_t zbrckanost (uint64_t x)
{
	uint64_t z = 0, i, b1, j, pn;
	//printf("Zbrckan sam!\n");

	for ( i = 0; i < 64 - 6; i++)
		{
			//podniz bitova od x: x[i] do x[i + 5]
			b1 = 0;
			pn = UZMIBITOVE (x, i + 4, 4);
			for(j = 0; j < 6; j++)
				if( ((1<<j) & pn)) //ako je jedan & to je logičko i, && to je binrani operator
					b1++;
			if (b1 > 4)
				z += b1 - 4;
			else if ( b1 < 2)
				z += 2 - b1;
		}
	//printf("Zbrckan sam! 2\n");
	return z;
}

uint64_t generiraj_dobar_broj (struct gmp_pomocno *p)
{
	uint64_t najbolji_broj = 0, broj;
	uint64_t najbolja_zbrckanost = 0, z;
	uint64_t i;
	//printf("Velicina_grupe = %lx\n", velicina_grupe);
	for ( i = 0;  i < velicina_grupe; i++) {
		//printf("Generiranje dobrog broja 1\n");
		broj = daj_novi_slucajan_prosti_broj (p);

		z = zbrckanost(broj);

		if( z > najbolja_zbrckanost) {

			najbolja_zbrckanost = z;
			najbolji_broj = broj;
			//printf("Generiranje najboljeg broja %lx, i = %lx\n", najbolji_broj, i);
		}
	}
	//printf("Generiranje najboljeg broja %lx\n", najbolji_broj);
	return najbolji_broj;
}

uint64_t procjeni_velicinu_grupe()
{
	uint64_t M = 20; //(ne preveliki da jedna petlja bude preduga,
	          //ni premali da dohvat vremena utječe na rezultat)
	uint64_t SEKUNDI = 5;//(može i malo manje, ali barem 5)

	time_t t = time(NULL);
	uint64_t k = 0;
	uint64_t a = t;
	uint64_t velicina_grupe = 1; //da generiraj_dobar_broj() obavi 1 iteraciju
	while( time(NULL) < t + SEKUNDI) {//time(NULL)
		k++;
		for( uint64_t i = 0; i < M-1; i++) {
			uint64_t broj;

			broj = generiraj_dobar_broj (&p);
			//printf("%lx\n", broj);
			stavi_u_MS(broj);

			//mora se osigurati da ova petlja preživi optimizaciju
			//lako se kasnije ULAZ i IZLAZ ponovno postave na nulu
		}
		a++;
	}
	uint64_t brojeva_u_sekundi = k * M / SEKUNDI;
	velicina_grupe = brojeva_u_sekundi * 2/5; // (ili bolje *2/5 ili slično)
	return velicina_grupe;
}

uint64_t br_iter = 0;

void *radna_dretva(void *id)
{
	struct gmp_pomocno p;

	uint64_t z = *((uint64_t*)id);

	if(z==ZADNJA_RA)
		{struct sched_param prio;
		prio.sched_priority = 1;
		if (pthread_setschedparam (pthread_self(), SCHED_RR, &prio)) {
			printf("Greška\n");
			exit (1);}
		}

	inicijaliziraj_generator(&p, z);


	//printf("U radnoj dretvi %ld\n", *z);

	do{

		uint64_t x = generiraj_dobar_broj(&p);

		sem_wait(&prazni);
		sem_wait(&ko);

		//udi_u_KO(*z);
		stavi_u_MS(x);
		printf("broj radne dretve   %ld, stavio %lx\n", z+1, x);
		//izadi_iz_KO(*z);

		sem_post(&ko);
		sem_post(&puni);

		for (uint64_t j = 0; j < BROJ_J_ITERACIJA; j++)
			;

		if(z==ZADNJA_RA){
			br_iter++;
			//printf("Povecao sam broj iteracija \n");
			if(br_iter%5==0){
				//printf("Spavam, pusti me radna sam!\n");
				sleep(3);}
			}

	}while (kraj!= KRAJ_RADA);

	obrisi_generator (&p);
	//printf("Završila dretva %ld\n", *z);
	return NULL;

}

void *neradna_dretva(void *id)
{
	uint64_t z = *((uint64_t*)id);

	if(z== ZADNJA_NE)
		{struct sched_param prio;
		prio.sched_priority = 1;
		if (pthread_setschedparam (pthread_self(), SCHED_RR, &prio)) {
			printf("Greška\n");
			exit (1);}
		}

	do{


		sem_wait(&puni);
		sem_wait(&ko);
		//udi_u_KO(*z);
		uint64_t y = uzmi_iz_MS();
		printf("broj neradne dretve %ld, uzeo   %lx\n", z+1, y);
		//izadi_iz_KO(*z);
		sem_post(&ko);
		sem_post(&prazni);

		for (uint64_t j = 0; j < BROJ_J_ITERACIJA; j++)
			;


		if(z==ZADNJA_NE){
			br_iter++;
			//printf("Povecao sam broj iteracija \n");
			if(br_iter%5==0){
				//printf("Spavam, pusti me neradna sam!\n");
				sleep(3);}
			}

	}while(kraj!=KRAJ_RADA);


	//printf("Gotovo neradna %ld\n", *z);
	return NULL;
}

int main(int argc, char *argv[])
{

	uint64_t broj_radnih_dretvi = ZADNJA_RA+1, broj_neradnih_dretvi = ZADNJA_NE+1;
	uint64_t BR1[broj_radnih_dretvi], BR2[broj_neradnih_dretvi];
	pthread_t t1[broj_radnih_dretvi], t2[broj_neradnih_dretvi];
	uint64_t i, j, k, l;

	//struct gmp_pomocno p;

	inicijaliziraj_generator(&p, 0);

	velicina_grupe = 1;
	velicina_grupe = procjeni_velicinu_grupe();	//izračunati veličinu grupe
	ULAZ = 0;
	IZLAZ = 0;
	BROJAC = 0;

	//time_t t = time(NULL);

	//inicijaliziraj_generator (&p, 0);

	sem_init(&puni, 0, 0);
	sem_init(&prazni, 0, 10);
	sem_init(&ko, 0, 1);


	for(i = 0; i < broj_radnih_dretvi; i++){
		BR1[i]=i;
		if(pthread_create(&t1[i], NULL, radna_dretva, &BR1[i])){
			printf("Ne mogu stvoriti novu dretvu! \n");
			exit(1);
		}
	}

	for(j = 0; j < broj_neradnih_dretvi; j++){
		BR2[j]=j;
		if(pthread_create(&t2[j], NULL, neradna_dretva, &BR2[j])){
			printf("Ne mogu stvoriti novu dretvu! \n");
			exit(1);
		}
	}

	sleep(20);
	kraj = KRAJ_RADA;
	//printf("Gotovo\n");
	for(k = 0; k < broj_radnih_dretvi; k++) {
			pthread_join( t1[k], NULL);
	}
	for(l = 0; l < broj_neradnih_dretvi; l++) {
			pthread_join( t2[l], NULL);
	}

	sem_destroy(&ko);
	sem_destroy(&puni);
	sem_destroy(&prazni);
	//printf("Gotovo\n");
	obrisi_generator(&p);

	return 0;
}

/*
  prevođenje:
  - ručno: gcc program.c slucajni_prosti_broj.c -lgmp -lm -o program
  - preko Makefile-a: make
  pokretanje:
  - ./program
  - ili: make pokreni
  nepotrebne datoteke (.o, .d, program) NE stavljati u repozitorij
  - obrisati ih ručno ili s make obrisi
*/
