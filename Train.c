#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <malloc.h>
#include <limits.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h> /* For AF_UNIX sockets */
#include "structures.h"

#define FILE_MODE ( O_CREAT | O_TRUNC | O_RDWR)
#define MAX 5
#define DEFAULT_PROTOCOL 0

struct stat st={0};

int createLogFile(char *);
void mission(struct element *, char *, int, char*);
int missionETCS1(struct element *);
int missionETCS2(struct element *, char*);

//procedura che crea il file di log del treno e ne restituisce il descrittore
int createLogFile(char *cTrain) {
	char acBuffer[20];
	sprintf(acBuffer, "./TrainLogs/T%s.log", cTrain); //inserisco in acBuffer il percorso del file
	int fd=open(acBuffer, FILE_MODE, 0777); //creo il file se non esiste
	return fd;
}

//modalità ETCS1: effettua il controllo sulla stazione successiva per vedere se è libera. In caso affermativo libera la stazione corrente e occupa la successiva.
int missionETCS1(struct element * pPath) {
	int iMovedForward=0;
	char cNext[MAX];
	strcpy(cNext, pPath->pNext->cElement);
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));//otteniamo la working directory corrente
	sprintf(cwd, "%s/Binari", cwd); //salviamo in cwd il percorso della cartella nella quale ci vogliamo spostare
	chdir(cwd);
	if (strncmp(cNext, "S", 1)==0) { //la prossima destinazione è la stazione di arrivo
		int iCurrentPlat=open(pPath->cElement, O_RDWR);
		ftruncate(iCurrentPlat, 0); //tronco il file a 0 byte
		lseek(iCurrentPlat, 0, SEEK_SET); //mi posiziono all'inizio del file
		write(iCurrentPlat, "0", 1);//liberiamo il binario attuale
		close(iCurrentPlat);
		iMovedForward=1;
	} else { //se la prossima destinazione è un binario 
		int iNextPlat=open(cNext, O_RDWR);
		char cPermission[2]; //buffer nel quale salviamo il numero letto dal binario
		int iRead=read(iNextPlat, cPermission, 1);
		if(strncmp(cPermission, "1", 1)!=0) { //permesso garantito, binario libero 
			ftruncate(iNextPlat, 0); //tronco il file a 0 byte
			lseek(iNextPlat, 0, SEEK_SET); //mi posiziono all'inizio del file
			//liberiamo il binario attuale
			if(strncmp(pPath->cElement, "S", 1)!=0) { //se la posizione attuale non è la stazione di partenza
				int iCurrentPlat=open(pPath->cElement, O_RDWR);
				write(iCurrentPlat, "0", 1);//liberiamo il binario attuale
				close(iCurrentPlat); //chiusura del file descriptor del segmento successivo
			}
			write(iNextPlat, "1", 1); //occupiamo il binario successivo
			iMovedForward=1;
		} else { //permesso negato
			iMovedForward=0;
		}
		close(iNextPlat);
	}
	chdir("../"); //torniamo nella cartella a livello superiore
	return iMovedForward;
}

//modalità ETCS2: crea la connessione col socket per accedere al binario successivo
int missionETCS2(struct element *pPath, char *cTrain) {
	int iMovedForward=0;
	int clientFd, serverLen, result;
	struct sockaddr_un serverUNIXAddress;
	struct sockaddr* serverSockAddrPtr;
	serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
	serverLen = sizeof (serverUNIXAddress);
 	clientFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
	serverUNIXAddress.sun_family = AF_UNIX; // Dominio del server
	strcpy(serverUNIXAddress.sun_path, "RBC"); // nome server
	
	do { /* Loop until a connection is made with the server */
		result = connect (clientFd, serverSockAddrPtr, serverLen); //result è 0 se la connessione ha successo, -1 altrimenti
		if (result == -1){
			printf("connection problem;re-try in 2 sec\n");
			sleep (2); /* Wait and then try again */
		 }
	} while (result == -1);
	char cPlatform[5]; //binario attuale
	strcpy(cPlatform, pPath->cElement);
	char cMessage[6];
	sprintf(cMessage, "%s%s", cTrain, cPlatform);
	write(clientFd, cMessage, sizeof(cMessage));
	char cPermission[2];
	while(read(clientFd, cPermission, sizeof(cPermission))<0); //il processo treno attende la risposta del server, una volta ottenuta la memorizza in cPermission
	close(clientFd); //chiusura del file del socket.
	iMovedForward=(int)cPermission[0]-48; //conversione da carattere a intero
	return iMovedForward;
}
		  
/*Procedura che fa eseguire ai treni la loro missione. Se cMode è "ETCS1" verrà 
 richiamata la proceduta missionETCS1, se invece è "ETCS2" si richiama missionETCS2 */ 
void mission(struct element *pPath, char *cMode, int fdLog, char *cTrain) {
	char cTextLog[50];
	int iMovedForward=1; //0 treno rimasto fermo, 1 il treno ha proseguito
	time_t current_time;
   	char* c_time_string;
	
	while (1) {
		//scrittura dell'attuale stato sul corrispettivo log
		if (iMovedForward==1) {
			current_time = time(NULL);
    		c_time_string = ctime(&current_time);
			//scriviamo in cTextLog ciò che  deve essere scritto sul file di log
			sprintf(cTextLog, "[Attuale: %s], [Next: %s] %s", pPath->cElement, pPath->pNext->cElement, c_time_string); 
			int iLogLength=strlen(cTextLog); //lunghezza di cTextLg
			write(fdLog, cTextLog, iLogLength); //scrittura di cTextLog sul file di log
			iMovedForward=0;
		}
		if (strncmp(pPath->cElement, "S", 1)==0 && pPath->pNext->cElement==NULL) { //controllo se il treno è arrivato a destinazione
			 break;
		}
		if (strncmp(cMode, "ETCS1", 5)==0) { //modalità ETCS1
			iMovedForward=missionETCS1(pPath);
		} else { // modalità ETCS2
			iMovedForward=missionETCS2(pPath, cTrain);
			if (iMovedForward==1) { //qui il processo treno cambia il contenuto dei file coinvolti nello spostamento
				char cwd[PATH_MAX];
				getcwd(cwd, sizeof(cwd));//otteniamo la working directory corrente
				sprintf(cwd, "%s/Binari", cwd); //salviamo in cwd il percorso della cartella nella quale ci vogliamo spostare
				chdir(cwd);
				if (strncmp(pPath->cElement,"S", 1)!=0) {
					int iCurrentPlat=open(pPath->cElement, O_RDWR);
					ftruncate(iCurrentPlat, 0); //tronco il file a 0 byte
					lseek(iCurrentPlat, 0, SEEK_SET); //mi posiziono all'inizio del file
					write(iCurrentPlat, "0", 1);//liberiamo il binario attuale
					close(iCurrentPlat);
				}
				if (strncmp(pPath->pNext->cElement, "S", 1)!=0) {
					int iNextPlat=open(pPath->pNext->cElement, O_RDWR);
					ftruncate(iNextPlat, 0); //tronco il file a 0 byte
					lseek(iNextPlat, 0, SEEK_SET); //mi posiziono all'inizio del file
					write(iNextPlat, "1", 1);//liberiamo il binario attuale
					close(iNextPlat);
				}
				chdir("../");
			}
		}
		if (iMovedForward==1) pPath=pPath->pNext; //proseguo nella lista dei binari
		sleep(3);	
	}
}

int main(int argc, char *argv[]) {
	if (stat("./TrainLogs", &st) == -1) { //controllo se la directory TrainLogs esiste già
		mkdir("./TrainLogs", 0777); //crea tale directory nel caso non esistesse
	}
	int fdLog=createLogFile(argv[1]);
	struct element *pPath;	//puntatore della lista
	pPath=fillPath(argv[1]); //creaiamo il percorso
	char cTrain[3];
	sprintf(cTrain, "T%s", argv[1]); //in cTrain memorizziamo argv[1], che contiene il numero del treno
	mission(pPath, argv[2], fdLog, cTrain); //il treno avvia la sua missione
	printf("Termine treno %s\n", argv[1]);
	close(fdLog); //chiusura del file di log
	exit(0);
	return(0);
}
