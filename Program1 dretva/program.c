/*
 * Jednostavni primjer za generiranje prostih brojeva korištenjem
   "The GNU Multiple Precision Arithmetic Library" (GMP)
 */

#include <stdio.h>
#include <time.h>


#include "slucajni_prosti_broj.h"

#define MASKA(bitova)			(-1 + (1<<(bitova)) )
#define UZMIBITOVE(broj,prvi,bitova) 	( ( (broj) >> (64-(prvi)) ) & MASKA(bitova) )
// ako ovdje pise  ( (broj) >> (64-(prvi)) ), dakle (prvi) dobije se drugačiji
// rezulata nego ako nema te zagrade.

/*makro - ovo kaj se definira
ukratko: MASKA - dobit će se bitovi koji su 1, broj koji ima samo jedinice prvih
bitova bitova, ako je 5 bitova onda najnižih 5 bitiova će biti 1. ako pomaknemo tu
jedinicu za 5 ona će biti na 6. mjestu i to zbrojimo na sve jedince i to će se poništiit
i ostat će samo te jedinice
UZMIBITOVE - posmaknuti broj za 63-prvi bitova i ostaviti samo prvih bitova bitova
*/
struct gmp_pomocno p;

uint64_t MS[10], ULAZ = 0, IZLAZ = 0;

void stavi_u_MS(uint64_t broj)
{
	MS[ULAZ] = broj;
	ULAZ = (ULAZ + 1)%10;
}

uint64_t uzmi_iz_MS()
{
	uint64_t broj = MS[IZLAZ];
	IZLAZ = (IZLAZ + 1)%10;
	return broj;
}

uint64_t zbrckanost (uint64_t x)
{
	uint64_t z = 0, i, b1, j, pn;

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
	return z;
}

uint64_t generiraj_dobar_broj (uint64_t velicina_grupe)
{
	uint64_t najbolji_broj = 0, broj;
	uint64_t najbolja_zbrckanost = 0, z;
	uint64_t i;
	for ( i = 0;  i < velicina_grupe-1; i++) {
		broj = daj_novi_slucajan_prosti_broj (&p);
		z = zbrckanost(broj);
		if( z > najbolja_zbrckanost) {
			najbolja_zbrckanost = z;
			najbolji_broj = broj;
			}
	}
	return najbolji_broj;
}

uint64_t procjeni_velicinu_grupe()
{
	uint64_t M = 1000; //(ne preveliki da jedna petlja bude preduga,
	          //ni premali da dohvat vremena utječe na rezultat)
	uint64_t SEKUNDI = 10;//(može i malo manje, ali barem 5)

	time_t t = time(NULL);
	uint64_t k = 0;
	uint64_t a = t;
	uint64_t velicina_grupe = 1; //da generiraj_dobar_broj() obavi 1 iteraciju
	while( a < t + SEKUNDI) {//time(NULL)
		k++;
		for( uint64_t i = 0; i < M-1; i++) {
			uint64_t broj;
			broj = generiraj_dobar_broj (velicina_grupe);
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

int main(int argc, char *argv[])
{
	//uint64_t a, b, c, d;
	uint64_t a, i = 0, b;
//	struct gmp_pomocno p;
	//izračunati veličinu grupe

	uint64_t velicina_grupe = procjeni_velicinu_grupe();

	time_t t = time(NULL);

	inicijaliziraj_generator (&p, 0);
	while (i<10) {
		a = generiraj_dobar_broj (velicina_grupe);
		stavi_u_MS(a);
		if (time(NULL) != t)
			{
				b = uzmi_iz_MS();
				printf("%lx \n", b);
				i++;
				t = time(NULL);
			}
		//a = daj_novi_slucajan_prosti_broj (&p);
		/*b = daj_novi_slucajan_prosti_broj (&p);
		c = daj_novi_slucajan_prosti_broj (&p);
		d = daj_novi_slucajan_prosti_broj (&p);
		printf ("par slucajnih 64-bitovnih brojeva u heksadekadskom obliku:\n"
			"[%" PRIx64 ", %" PRIx64 ", %" PRIx64 ", %" PRIx64 "]\n",
			a, b, c, d
		);*/
		//printf("%lx \n", a);
		//	printf("%ld\n", zbrckanost (a));
		//printf ("%ld: %" PRIx64 "\n", i, a);
	}

//	printf("%ld\n", zbrckanost (a));

	obrisi_generator (&p);

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
