/* autori 
    gazulli e karici 

 */


#include "funzioni.h"

int idSem, idSemS, idMemCond,idMessCoda, idMessCodaG,fine=1;

//Matrice dei gruppi
//pid_t è un tipo di variabile speciale che indica il PID di un processo
//in realtà è solo un intero
pid_t gruppi[POP_SIZE][4];

void cancIpc();
void handleSignal(int signal);

int main(int argc, char * argv[])
{
    //Cancella oggetti IPC che sono : semafori, memorie condivise, code messaggi se sono rimaste aperte
    cancIpc();

    int cont2 = 0, cont3 = 0, cont4 = 0, fine = 1;
    FILE * fp;
    fp=fopen("opt.conf", "r");
    char *res;
    char buf[50];
    int cont = 0;
    char MAX_REJECT[50], NOF_INVITES[50]; 

    //Lettura del file opt.conf per leggere NOF_INVITES E MAX_REJECTS
    //fp è il puntatore al file
    if( fp==NULL ) {
        perror("Errore in apertura del file");
    }else
        {
            do{
                res=fgets(buf, 50, fp);
                if(cont==3)
                    strncpy(NOF_INVITES, buf, 50);
                if(cont==4)
                    strncpy(MAX_REJECT, buf, 18);
                cont++;    
            }while(res!=NULL); 
            fclose (fp);
        }    

    char *args[] = { "", NOF_INVITES, MAX_REJECT, NULL };

    time_t inizio, contr;
    sigset_t  mask, vMask;
    struct msgbuf msgp;
    struct sembuf sops_start;
	struct sigaction sigact;
    struct sembuf sops;

    //Creazione della coda dei messaggi
    idMessCoda = msgget(CHIAVECODAMESS, 0600 | IPC_CREAT | IPC_EXCL);
    idMessCodaG = msgget(CHIAVECODAMESSG, 0600 | IPC_CREAT | IPC_EXCL);
    
    //Creazione della memoria condivisa
    idMemCond = shmget(CHIAVEMEMCOND, POP_SIZE * sizeof(student) * POP_SIZE * sizeof(student*), 0666 | IPC_CREAT);

    //Creazione del semaforo
	idSemS = semget(CHIAVESEMS, 1, 0666 | IPC_CREAT | IPC_EXCL);
	//Impostazione del valore del semaforo a 0
    setValSem(idSemS, 0);
    
    //Creazione del semaforo
    idSem = semget(CHIAVESEM, 1, 0666 | IPC_CREAT | IPC_EXCL);
    //Impostazione valore semaforo a 1
    setValSem(idSem,1);

    student ** s;
    s = (student**)malloc(POP_SIZE * sizeof(student*));

    student (*studenti)[POP_SIZE] = shmat(idMemCond, NULL, 0);
    inizio = time(NULL);

    for(int i = 0; i <POP_SIZE; i++)
    {
        for(int k = 0; k < 4; k++)
        {
            gruppi[i][k] = 0;
        }
    }

    srand(time(NULL));

    //Creazione degli studenti
    for(int i = 0;i < POP_SIZE;i++)
    {
        //Creazione del processo figlio
        int pidS = fork(); 

        //Impostazione dei dati dello studente
        s[i] = generaStudente(pidS); 

        //Conta il numero di studenti con 2, 3 o 4 elementi
        if (s[i]->nof_elems == 2)
            cont2++;
        else if (s[i]->nof_elems == 3)
            cont3++;
        else
            cont4++;

        //pidS
        //Al padre pidS = pid del figlio creato
        //Nel figlio pidS = 0
        //Se invece pidS = -1 c'è stato un errore
        switch (pidS) {
			case -1:
				fprintf(stderr, "Errore durante la creazione dei processi figli");
				break;

			case 0: //Figlio

                //Si blocca sul semaforo
                attSem(idSemS);

                //Questa viene eseguita quando viene sbloccato alla fine della creazione dei processi
                //Vanno a eseguire il file student.c
                execv("./student", args);
                break;                

            //Altrimenti è il padre
			default: 
                //Copia i dati del figlio creato nella memoria condivisa
                memcpy(studenti[i], s[i], sizeof(student));
				break;
		}

    }

    //Visualizza quanti gruppi da 2, 3 o 4 elementi
    printf("\nCont2 %i, Cont3: %i, Cont4: %i\n\n", cont2, cont3, cont4);     
    
    //Stampa tutti gli studenti
    for(int i = 0;i < POP_SIZE;i++)
    { 
        stampaStudente(studenti[i]);
    }    

    //Viene impostato il segnale di terminazione

    //Alla fine del SIM_TIME viene richiamata la funzione "handleSignal"
	sigact.sa_handler = &handleSignal;
	sigact.sa_flags = 0;
	sigaction(SIGALRM, &sigact, NULL);
    sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
    
    //Dopo SIM_TIME secondi, viene generato il segnale SIGALRM
    alarm(SIM_TIME);    

    //Viene sbloccato il semaforo su cui sono bloccati i figli
    setValSem(idSemS, POP_SIZE);

    
    int index;
    char pidCapo[MAXDIMMESS];
    int posCapo, posAggiungere;
    char pidAggiungere[MAXDIMMESS];
    long pid=getpid();

    //Finchè non è scaduto il tempo il padre si mette in attesa di ricevere messaggi dagli studenti
    while(fine)
    {
        //quando ricevo un messaggio blocco gli altri
        msgrcv(idMessCodaG, &msgp, MAXDIMMESS, pid,0);
        attSem(idSem);
    
        //Imposto maschera
		sigprocmask(SIG_BLOCK, &mask, &vMask);

        // Ricavo i due pid(nel messaggio si passa "capo;aggiungere")
		index = strchr(msgp.mTesto, ';') - msgp.mTesto;
		strncpy(pidCapo, msgp.mTesto, index);
		for (int i = 0; i < strlen(msgp.mTesto); i++)
			pidAggiungere[i] = msgp.mTesto[index + i + 1];

        if(atoi(pidAggiungere)!=0)
        {
            //creo un gruppo o lo aggiorno
            posCapo = ricercaInMem(atoi(pidCapo));
            posAggiungere = ricercaInMem(atoi(pidAggiungere));
            
            //SE LO STUDENTE CHE VIENE AGGIUNTO HA DELLE RICHIESTE, VENGONO RISPOSTI CON UN RIFIUTO
            while (msgrcv(idMessCoda, &msgp, MAXDIMMESS, atoi(pidAggiungere), IPC_NOWAIT) != -1) {
                msgp.mTipo = atol(msgp.mTesto);
                strcpy(msgp.mTesto, "no");
                msgsnd(idMessCoda, &msgp, MAXDIMMESS, 0);
            }

            aggiornaGruppo(gruppi, studenti[posAggiungere], studenti[posCapo]);
        }else
            { 
                //se è zero creo un gruppo con solo il capo
                posCapo = ricercaInMem(atoi(pidCapo));  
                aggiornaGruppo(gruppi, NULL, studenti[posCapo]);
            }
        signalSem(idSem);
    }
}

//Funzione finale che viene richiamata quando scade il tempo
//E vengono assegnati i voti ai gruppi
void handleSignal(int signal)
{
    student (*studenti)[POP_SIZE] = shmat(idMemCond, NULL, 0);
    switch (signal) {
        case SIGALRM: //fine simulazione
            fine = 0;
            int cont=0;
            int somma=0;
            float media=0;
            
            //Calcola il voto di ogni gruppo
            assegnaVoto(gruppi);
            
            //Per ogni voto dal 18 al 30
            for(int i = 18; i <= 30; i++)
            {
                //Per ogni studente
                for(int k = 0; k < POP_SIZE; k++)
                {
                    //Conta quanti hanno preso il voto "i"
                    if(studenti[k]->voto_AdE==i)
                    {
                        cont++;
                        //Somma tutti i voti
                        somma=somma+studenti[k]->voto_AdE;
                    }
                }
                printf("\nVoto AdE: %i Numero di studenti: %i",i,cont);
                cont=0;
            }

            //Calcola la media
            media=somma/POP_SIZE;
            printf("\nMedia voto Ade: %f",media);
            cont=0;
            somma=0;

            //Stessa cosa per il voto finale
            for(int i = 0; i <= 30; i++)
            {
                for(int k = 0; k < POP_SIZE; k++)
                {
                    if(studenti[k]->votoFinale==i)
                    {
                        cont++;
                        somma=somma+studenti[k]->votoFinale;
                    }
                }
                if(i==0||i>=15)
                    printf("\nVoto Finale: %i Numero di studenti: %i",i,cont);
                
                cont=0;
            }
            //Media
            media=somma/POP_SIZE;
            printf("\nMedia voto Finale: %f",media);

            //Stampa tutti gli studenti e li termina
            for (int i = 0; i < POP_SIZE; i++) {
                stampaStudente(studenti[i]);
			    kill(studenti[i]->pid, SIGKILL);
		    }
            exit(0);
            break;
	    default: 
		    break;
	    }
}

void cancIpc()
{
	system("ipcrm -S 1");
    
    system("ipcrm -M 2");  
	system("ipcrm -Q 3");
    system("ipcrm -S 4");
    system("ipcrm -Q 5");
    system("ipcrm -Q 6");
}


