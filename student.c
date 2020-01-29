/* autori 
    gazulli e karici 

 */


#include "funzioni.h"

void handle(int signal);

int main(int argc, char * argv[])
{
    
    pid_t pid = getpid();

    pid_t pidInvitare;
    int idSem, idMemCond, idMessCoda, idMessCodaG;
    int posMio=0, posInvitare=0, abbassa=0;
    int invitiAccettati = 0, accetto=0, capogruppo=0, attesa=0;
    char mess[MAXDIMMESS+sizeof(pid)];
    char pidArrivato[MAXDIMMESS];
    char *sino;
    struct msgbuf msgp;
    struct sigaction sa;
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_flg = 0;

    int NOF_INVITES = atoi(argv[1]);
    int MAX_REJECT = atoi(argv[2]);
    int contInviti = 0;
    int contRicevuti = 0;
    int contRifiuti = 0;
    int pariDispari;

    
    pid_t invitati[NOF_INVITES];
    pid_t ricevuti[POP_SIZE];
    pid_t accettati[4];

    sigset_t mask, vMask;

    for(int i = 0; i < NOF_INVITES; i++)
        invitati[i]=0;
    for(int i = 0; i < POP_SIZE; i++)
        ricevuti[i]=0;
    for(int i = 0; i < 4; i++)
        accettati[i]=0;

    //Apertura della coda dei messaggi, semaforo, memoria condivisa
    idMessCoda = msgget(CHIAVECODAMESS, 0 | IPC_CREAT);
    idMessCodaG = msgget(CHIAVECODAMESSG, 0 | IPC_CREAT);
    idSem = semget(CHIAVESEM, 1, 0);
    idMemCond = shmget(CHIAVEMEMCOND, 1, 0);

    student (*studenti)[POP_SIZE] = shmat(idMemCond, NULL, 0);

    posMio = ricercaInMem(pid);
    pariDispari = controlloPari(studenti[posMio]->matricola);
            
	//Quando finisce il tempo viene richiamata con il segnale, la funzione "handle"
	sa.sa_handler = &handle;
	sa.sa_flags = 0;
	sigaction(SIGTERM, &sa, NULL);
	//Setto la mask
	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);

    //Finchè il gruppo dello studente non è chiuso
    while(studenti[posMio]->chiuso==0)
    {
        //Blocca il semaforo
        attSem(idSem); 
        
        //Legge i messaggi di invito se ce ne sono
        while(msgrcv(idMessCoda, &msgp, MAXDIMMESS, pid, IPC_NOWAIT) != -1)
        {
            sino = strchr(msgp.mTesto, ';');
            //se il messaggio è una risposta ad un invito
            if(sino!=NULL)
            {
                //separo il pid del mittende dalla risposta
                int index = strchr(msgp.mTesto, ';') - msgp.mTesto;
                strncpy(pidArrivato, msgp.mTesto, index); 
                if(strcmp(sino,";si") == 0)
                {
                    //se risposta è positiva mando a gestore il messaggio per la creazione o l'aggiunta nel gruppo
                    sprintf(mess, "%i;%i", pid, atoi(pidArrivato));
                    strcpy(msgp.mTesto,mess);
                    msgp.mTipo=(long)getppid();
                    int ret = msgsnd(idMessCodaG,&msgp,MAXDIMMESS,0);
                    
                    capogruppo = 1;
                    accettati[invitiAccettati]=atoi(pidArrivato); 
                    invitiAccettati++;
                } 
                attesa--;  
            }else
                {      
                    //altrimenti il messaggio è un invito, quindi salvo il pid negli inviti ricevuti 
                    ricevuti[contRicevuti]=atoi(msgp.mTesto);
                    int posMittente = ricercaInMem(atoi(msgp.mTesto));
                    contRicevuti++; 
                    
                    if(contRifiuti >= MAX_REJECT){
                        //se ho già rifiutato troppe volte accetto
                        accetto=1;
                        msgp.mTipo = atol(msgp.mTesto);
                        sprintf(mess, "%i;si", pid);
                        strcpy(msgp.mTesto, mess);
                        msgsnd(idMessCoda, &msgp, MAXDIMMESS, 0);
                    }
                    if(capogruppo==0&&accetto==0&&attesa==0&&studenti[posMittente]->chiuso==0&&(studenti[posMio]->nof_elems==studenti[posMittente]->nof_elems||studenti[posMio]->voto_AdE<=studenti[posMittente]->voto_AdE+3)){
                            //se non sono capogruppo, non sono in attesa di risposte, non ho ancora accettato, il mittente non è
                            //in un gruppo chiuso e se ho una preferenza uguale al mittente o il suo voto è alto ACCETTO
                            accetto=1;
                            msgp.mTipo = atol(msgp.mTesto);
                            sprintf(mess, "%i;si", pid);
                            strcpy(msgp.mTesto, mess);
                            msgsnd(idMessCoda, &msgp, MAXDIMMESS, 0);
                    }else      
                        {
                            
                            msgp.mTipo = atol(msgp.mTesto);
                            strcpy(msgp.mTesto, ";no");
                            msgsnd(idMessCoda, &msgp, MAXDIMMESS, 0);
                        }
                }
        }
        
        //se il mio gruppo ha raggunto il numero giusto di elementi lo CHIUDO
        if(capogruppo==1 && studenti[posMio]->nof_elems-1<=invitiAccettati )
        {    
            studenti[posMio]->chiuso=1;
            while(msgrcv(idMessCoda, &msgp, MAXDIMMESS, pid, IPC_NOWAIT) != -1)
            {  
                msgp.mTipo = atol(msgp.mTesto);
                strcpy(msgp.mTesto, ";no");
                msgsnd(idMessCoda, &msgp, MAXDIMMESS, 0);
            }
            int posA;
            int ii=0;
            while(accettati[ii]!=0)
            {
                posA=ricercaInMem(accettati[ii]);
                studenti[posA]->chiuso=1;
                ii++;
            }
        }

        posInvitare=-1;
        pidInvitare = ricercaStudenteDaInvitare(studenti[posMio], invitati, NOF_INVITES);
        posInvitare = ricercaInMem(pidInvitare);

        //se non ho accettato a inviti, c'è un elemento da invitare, non ho superato gli inviti max e non ho appena chiuso
        if(accetto==0 && posInvitare!=-1 && contInviti<NOF_INVITES && studenti[posMio]->chiuso==0)
        {
            
            msgp.mTipo=(long)studenti[posInvitare]->pid;
            sprintf(mess,"%i",studenti[posMio]->pid);
            strcpy(msgp.mTesto,mess);
            //invito lo studente 
            msgsnd(idMessCoda, &msgp, MAXDIMMESS, 0);
            
            attesa++;
            invitati[contInviti]=pidInvitare;
            contInviti++;
            
        }else if (accetto==0 && posInvitare==-1 && invitiAccettati!=0 && studenti[posMio]->chiuso==0){
                //se non ce piu gente da invitare o ho finito gli inviti e sono capogruppo, chiudo il mio gruppo
                studenti[posMio]->chiuso=1;
                int posA;
                int ii=0;
                if(invitiAccettati!=0)
                {
                    while(accettati[ii]!=0)
                    {
                        posA=ricercaInMem(accettati[ii]);
                        studenti[posA]->chiuso=1;
                        ii++;
                    }
                }
            }

            signalSem(idSem);
        }

        sigprocmask(SIG_SETMASK, &vMask, NULL);
}

void handle(int signal)
{
	if (signal == SIGKILL) {
		exit(EXIT_SUCCESS);
	}
}
