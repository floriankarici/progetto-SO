#include "funzioni.h"
/* autori 
    gazulli e karici 

 */


student *generaStudente(pid_t pid)
{
    
    FILE * fp;
    fp=fopen("opt.conf", "r");
    char *res;
    char buf[50];
    int cont = 0;
    int prob2=0,prob3=0,prob4=0; 

    if( fp==NULL ) {
        perror("Errore in apertura del file");
    }else
        {
            //Lettura dei dati per la probabilità
            while(cont<3) {
                res=fgets(buf, 50, fp);
                if(cont==0)
                    prob2=atoi(buf);
                if(cont==1)
                    prob3=atoi(buf);
                if(cont==2)
                    prob4=atoi(buf);
                cont++;
            }
            fclose(fp);
            
            
            int vC = 18 + rand() % 13;
            
            
            student * s = malloc(sizeof(student));

            
            s->pid = pid;   
            s->matricola = pid;
            s->voto_AdE = vC;
            s->votoFinale = 0;
            s->chiuso=0;

            //In base alle probabilità imposta il numero di elementi che lo studente vuole nel suo gruppo
            int prob = rand() % 100;
            if(prob < prob2)
                s->nof_elems = 2;
            else if(prob < (prob2+prob3))
                s->nof_elems = 3;
            else
                s->nof_elems = 4;

             return s;
        }
       return NULL;
}

//Visualizza i dati dello studente
void stampaStudente(student * s)
{
    printf("\nMatricola: %i Voto AdE: %i Preferenza: %i Voto Finale: %i", 
            s->matricola, s->voto_AdE, s->nof_elems, s->votoFinale);
}

//Data una matricola restituisce se è pari o dispari
//Serve per verificare il turno
int controlloPari(int m)
{
    if(m%2==0)
        return 1;
    else
        return 0;
}

//Dato un pid restituisce l'indice dello studente nella lista degli studenti della memoria condivisa
int ricercaInMem(pid_t pid)
{
    int idMemCond = shmget(CHIAVEMEMCOND, 1, 0);
    student (*studenti)[POP_SIZE] = shmat(idMemCond, NULL, 0);

    for(int i=0;i<POP_SIZE;i++)
    {
        if(studenti[i]->pid==pid)
            return i;
    }
    return -1;
}

//Restituisce il PID di uno studente da invitare
pid_t ricercaStudenteDaInvitare(student *s, pid_t invitati[], int NOF_INVITES)
{
    int test = 1;

    int idMemCond = shmget(CHIAVEMEMCOND, 1, 0);
    student (*studenti)[POP_SIZE] = shmat(idMemCond, NULL, 0);

    int pM = controlloPari(s->matricola);
    for(int i = 0; i < POP_SIZE; i++)
    {
        test=1;
        
        for(int k = 0; k < NOF_INVITES; k++)
        {
            if(invitati[k]==studenti[i]->pid)
            {
                test = 0;
            }
                
        }
        //invito se non sono io, se non l'ho gia invitato, se è nel mio stesso turno, e se non è un gruppo chiuso o se ho chiuso il gruppo prima
        if((s->pid!=studenti[i]->pid)&&(test==1)&&(pM == controlloPari(studenti[i]->matricola))&& s->chiuso == 0 && studenti[i]->chiuso == 0)
        {
            return studenti[i]->pid;   
        }
    }
    return 0;
}

//Inserisce un nuovo studente all'interno di un gruppo
//C è studente capo
//S è quello da aggiungere
void aggiornaGruppo(pid_t gruppi[POP_SIZE][4], student *s, student *c)
{ 
    int test = 0;
    int ferma = 0, pos = 0;
    //Se esiste un gruppo con capogruppo c aggiungo nel primo posto libero
    if(s!=NULL)
    {
        for(int i = 0; i < POP_SIZE; i++)
        {
            if(gruppi[i][0]==c->pid)
            {
                int k=1;
                while(k<4 && gruppi[i][k]!=0)
                {
                    k++;
                }
                gruppi[i][k]=s->pid;
                test = 1; 
            }
        }
        
        //se non esiste cerco il primo gruppo vuoto e mi inserisco
        if(test == 0)
        {
            while(ferma==0 && pos<POP_SIZE)
            {   
                if(gruppi[pos][0]==0)
                {
                    gruppi[pos][0]=c->pid;
                    gruppi[pos][1]=s->pid;
                    ferma=1;
                }
                pos++;
            }
        }
    }else //Creo nuovo gruppo
        {
            while(ferma==0 && pos<POP_SIZE)
            {   
                if(gruppi[pos][0]==0)
                {
                    gruppi[pos][0]=c->pid;
                    ferma=1;
                }
                pos++;
            }
        }
    
}

//Dato l'insieme di tutti i gruppi calcola i voti finali ottenuti
void assegnaVoto(pid_t gruppi[POP_SIZE][4])
{
    int idMemCond = shmget(CHIAVEMEMCOND, 1, 0);
    student (*studenti)[POP_SIZE] = shmat(idMemCond, NULL, 0);

    for(int i = 0; i < POP_SIZE; i++)
    {
        //vettore con le posizioni nella memoria condivisa del gruppo
        int pos[4]={-1,-1,-1,-1};
        int diff=0;

        if(gruppi[i][0]!=0) //il gruppo non è vuoto
        {
            int maxV=0;
            int k=0;
            int j=0;

            //Cerco il voto max del gruppo
            while(k<4 && gruppi[i][k]!=0)
            {
                //salvo le posizioni
                pos[k]=ricercaInMem(gruppi[i][k]);
                
                //trovo il voto massimo del gruppo
                if(maxV<=studenti[pos[k]]->voto_AdE)
                    maxV=studenti[pos[k]]->voto_AdE;
                k++;
            }

            //Se il gruppo ha il numero di elementi che vuole il leader il voto ottenuto è quello max
            //Altrimenti viene diminuito di 3 voti
            while(j<4 && gruppi[i][j]!=0)
            {
                //se il gruppo ha lo stesso numero della mia preferenza assegno il voto max
                //altrimenti sottraggo tre
                if(studenti[pos[j]]->nof_elems==k)
                    studenti[pos[j]]->votoFinale=maxV;
                else
                    studenti[pos[j]]->votoFinale=maxV-3;

                j++;
            }

        }else
            break;        
    }
    

}

/*******************Funzioni per la gestione dei semafori*******************/

int attSem(int idSemaforo)
{
	struct sembuf sops;

    sops.sem_op = -1;    
    sops.sem_num = 0;    
	sops.sem_flg = 0; 

    return semop(idSemaforo, &sops, 1);
}

int signalSem(int idSemaforo)
{
	struct sembuf sops;

    sops.sem_op = +1;     
    sops.sem_num = 0;     
	sops.sem_flg = 0;

    return semop(idSemaforo, &sops, 1);
}

int getValSem(int idSemaforo)
{
    return semctl(idSemaforo, 0, GETVAL);
}

int setValSem(int idSemaforo, int val)
{
    return semctl(idSemaforo, 0, SETVAL, val);
}
