/* autori 
	gazulli e karici 

 */


#define __FUNZIONI_H__

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <assert.h>


#define POP_SIZE 50
#define SIM_TIME 30


#define CHIAVESEMS 1
#define CHIAVESEM 4
#define CHIAVEMEMCOND 2
#define CHIAVECODAMESS 3 
#define CHIAVECODAMESSG 5
#define MAXDIMMESS 20


//Struttura del MESSAGGIO che viene inviato sulla coda
struct msgbuf {
		long mTipo; 
		char mTesto [MAXDIMMESS];
}msgBuffer;


typedef struct processo {
	int matricola;
    int nof_elems;
	int voto_AdE;
    int votoFinale;
	pid_t pid;
	int chiuso;
} student;

//Elenco definizioni delle funzioni
student *generaStudente(pid_t pid);
pid_t ricercaStudenteDaInvitare(student *s, pid_t invitati[], int abbassa);
int ricercaInMem(pid_t pid);
int controlloPari(int m);
void stampaStudente(student * s);
void aggiornaGruppo(pid_t gruppi[POP_SIZE][4], student *s, student *c);
//Funzioni per la gestione dei semafori
int attSem(int idSemaforo);
int getValSem(int idSemaforo);
int signalSem(int idSemaforo);
int setValSem(int idSemaforo, int val);
void assegnaVoto(pid_t gruppi[POP_SIZE][4]);

