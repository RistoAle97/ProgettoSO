#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> /* For AFUNIX sockets */
#define DEFAULT_PROTOCOL 0
#include <sys/wait.h>
#include <unistd.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <malloc.h>
#include <limits.h>
#include <time.h>
#include "structures.h"

#define FILE_MODE ( O_CREAT | O_TRUNC | O_RDWR)


void savePlatform(int, char *, struct paths *);
int givePermission(int, char*, struct paths *, struct platform *, struct station *, int);
int createLogRBC(void);
void writeOnLog(int , char *, char *, char *, int);


//Procedura che salva il segmento cPlatform nella struttura del treno corrispondente, indicato da iTrain
void savePlatform(int iTrain, char *cPlatform, struct paths *pPaths) {
	struct element *pIndex; 
	char cFinalStation[3];
	switch(iTrain) {	//a seconda del numero del treno si salva cPlatform in un determinato percorso
		case(1):
			pIndex=pPaths->T1Path;
			strcpy(cFinalStation, "S6");
			break;
		case(2):
			pIndex=pPaths->T2Path;
			strcpy(cFinalStation, "S8");
			break;
		case(3):
			pIndex=pPaths->T3Path;
			strcpy(cFinalStation, "S8");
			break;
		case(4):
			pIndex=pPaths->T4Path;
			strcpy(cFinalStation, "S1");
			break;
		case(5):
			pIndex=pPaths->T5Path;
			strcpy(cFinalStation, "S1");
	}
	
	while(pIndex->pNext!=NULL) pIndex=pIndex->pNext; //scorro fino a quando non trovo l'ultimo binario
	strcpy(pIndex->cElement, cPlatform); 
	if (strncmp(cPlatform, cFinalStation, 2)!=0) pIndex->pNext=(struct element *)malloc(sizeof(struct element)); //alloco se non è stata trovata l'ultima stazione
	pIndex=pIndex->pNext; 
}

//procedura che crea il file di log
int createLogRBC() {
	char acBuffer[20];
	sprintf(acBuffer, "./RBC.log"); //inserisco in acBuffer il percorso del file
	int fd=open(acBuffer, FILE_MODE, 0777); //creo il file se non esiste
	return fd;
}

//Procedura incaricata di scrivere sul file di log RBC.log
void writeOnLog(int iTrain, char *cCurrPlat, char *cNextPlat, char *cPermission, int fdLog) {
	char cTextLog[155]; //buffer per la scrittura nel file di log
	time_t current_time;
   	char* c_time_string;
	current_time = time(NULL);
    c_time_string = ctime(&current_time); //in questa stringa salviamo l'ora attuale;
	char c_time[strlen(c_time_string)];
	snprintf(c_time, strlen(c_time_string), "%s", c_time_string);
	sprintf(cTextLog, "[TRENO RICHIEDENTE AUTORIZZAZIONE: T%d], [SEGMENTO ATTUALE: %s], [SEGMENTO RICHIESTO: %s], [AUTORIZZATO: %s], [DATA: %s]\n", iTrain, cCurrPlat, cNextPlat, cPermission, c_time);
	int iLogLength=strlen(cTextLog);
	write(fdLog, cTextLog, iLogLength); //scriviamo nel file di log
}

/*procedura che restituisce un intero. Questo intero rappresenta l'autorizzazione al treno a procedere (1) o ad attendere (accesso negato, 0)*/
int givePermission(int iTrain, char *cPlatform, struct paths *pPaths, struct platform *pPlatforms, struct station *pStations, int fdLog) {
	struct element *pIndex;
	int iPermission=0; //0 permesso non concesso, 1 permesso concesso
	switch(iTrain) { //a seconda del numero del treno si salva cPlatform in un determinato percorso
		case(1):
			pIndex=pPaths->T1Path;
			break;
		case(2):
			pIndex=pPaths->T2Path;
			break;
		case(3):
			pIndex=pPaths->T3Path;
			break;
		case(4):
			pIndex=pPaths->T4Path;
			break;
		case(5):
			pIndex=pPaths->T5Path;
	}
	 
	while(strcmp(cPlatform, pIndex->cElement)!=0) pIndex=pIndex->pNext; //scorriamo la lista finchè non troviamo il binario attuale
	char cNext[5];
	strcpy(cNext, pIndex->pNext->cElement); //salviamo in cNext il binario successivo
	
	if(strncmp(cNext, "S", 1)==0) { //stazione finale
		while(strcmp(cNext, pStations->cStation)!=0) pStations=pStations->pNextStation; //finchè non troviamo la stazione di arrivo in pStations scorriamo la struttura
		pStations->iState++; //incrementiamo il numero di treni presenti nella stazione
		while(strcmp(cPlatform, pPlatforms->cBinary)!=0) pPlatforms=pPlatforms->pNextPlat; //finchè non troviamo il binario dal quale arriviamo scorriamo la struttura
		pPlatforms->iState=0; //settiamo a 0 (libero) lo stato del binario
		iPermission=1; //permesso di procedere autorizzato
		writeOnLog(iTrain, cPlatform, cNext, "SI", fdLog); //scrittura sul file di log
	} else { //se il prossimo segmento non è la stazione finale
		struct platform *pFirst=pPlatforms; //puntatore alla struttura dati dei binari
		while(strcmp(cNext, pPlatforms->cBinary)!=0) pPlatforms=pPlatforms->pNextPlat;
		if(pPlatforms->iState==0) { //permesso garantito, binario libero
			pPlatforms->iState=1;
			if(strncmp(cPlatform, "S", 1)!=0) { //se la posizione attuale non è la stazione di partenza
				while(strcmp(cPlatform, pFirst->cBinary)!=0) pFirst=pFirst->pNextPlat; //scorriamo pFirst fino a quando non troveremo il binario attuale
				pFirst->iState=0; //binario libero
			} else {
				while(strcmp(cPlatform, pStations->cStation)!=0) pStations=pStations->pNextStation; //scorri finchè non trovi la stazione corrispondente
				pStations->iState--; //decrementiamo il numero di treni nella stazione
			}
			iPermission=1; 
			writeOnLog(iTrain, cPlatform, cNext, "SI", fdLog);
		} else {
			writeOnLog(iTrain, cPlatform, cNext, "NO", fdLog);	
		}
	}
	return iPermission; //restituiamo il permesso. 
}

int main(int argc, char * argv[]) {
	int serverFd, clientFd, serverLen, clientLen;
 	struct sockaddr_un serverUNIXAddress; /*Server address */
  	struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
  	struct sockaddr_un clientUNIXAddress; /*Client address */
  	struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/

  	serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
  	serverLen = sizeof (serverUNIXAddress);
  	clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
  	clientLen = sizeof (clientUNIXAddress);
  
  	serverFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
  	serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */
  	strcpy (serverUNIXAddress.sun_path, "RBC"); /* Set name */
  	unlink ("RBC"); /* Remove file if it already exists */
  	bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
  	listen (serverFd, 5); /* Maximum pending connection length */
	
	struct paths *pPaths=initializePaths();
	struct platform *pPlatforms;//=initializePlatforms();
	struct station *pStations;//=initializeStations();
	int fdLog=createLogRBC(); //crea il file log dell'RBC
	int iCounter=0; 
	/*iCounter è il flag per il numero di connessioni. Se è 0 nessun padre treno si è mai collegato, dunque inizializziamo il tutto. 
	  In questo modo non è necessario fermare l'esecuzione del server per lanciare un'altra volta ./nomeprogramma	ETCS2
	  Ogni volta le strutture di binari e stazioni verrano inizializzate non appena il padre treni passa al server il percorso di T5.
	*/
	while (1) {/* Loop forever */ 
		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen); /* Accept a client connection */
		char cTemp[150]; //buffer per la lettura da clientFd
		read(clientFd, cTemp, 1); //leggiamo un carattere per vedere il numero del treno.
		if(strncmp(cTemp, "T", 1)!=0) { //richiesta avanzata dal padre treni
			if (iCounter==0) { 
				int iTrain=(int)cTemp[0]-48; //numero del treno
				int iEnd=lseek(clientFd, 0, SEEK_END); //prendo l'offset di fine file
				int iStart=lseek(clientFd, 1, SEEK_SET); //mi posiziono all'inizio del file
				read(clientFd, cTemp, sizeof(cTemp)); // viene letto ciò che ha scritto il client
				//char cPlatform[5]; //buffer nel quale salviamo 
				char cDel[]=" "; //delimitatori per strtok, nel nostro caso solo lo spazio
				char *ptr=strtok(cTemp, cDel); 
				while (ptr!=NULL) {
				/*	passiamo alla procedura savePlatform una sottostringa delimitata da uno spazio (unico delimitatore in cDel)
				Tale sottostringa rappresenta un binario, che viene poi salvato nella struttura dati dei percorsi. */
					savePlatform(iTrain, ptr, pPaths); //chiamiamo la procedura savePlatform per memorizzare il segmento letto
					ptr=strtok(NULL, cDel);
				}
				if (iTrain==5) {
					iCounter=1;
					pPlatforms=initializePlatforms();
					pStations=initializeStations(); //inizializziamo le stazioni (in accordo con le stazioni iniziali dei vari percorsi)
				} 
			}
		} else { //richiesta avanzata da un processo treno
			char cTrain[2]; 
			read(clientFd, cTrain, 1); //leggiamo 1 carattere per vedere il numero del treno
			int iTrain=(int)cTrain[0]-48; //numero del treno
			char cPlatform[5]; //buffer nel quale salviamo il binario attuale del treno
			read(clientFd, cPlatform, 4); //leggiamo il binario attuale
			//dato il binario attuale, la procedura givePermission riconosce il binario successivo del treno
			int iMovedForward=givePermission(iTrain, cPlatform, pPaths, pPlatforms, pStations, fdLog); 
			char cPermission[2]; //buffer nel quale memorizziamo il permesso di procedere
			sprintf(cPermission, "%d", iMovedForward); //dentro cPermission scriviamo iMovedForward
			write(clientFd, cPermission, sizeof(cPermission)); //notifica del permesso da parte del server
		}
		close (clientFd); /* Close the client descriptor */
	}
	close(fdLog); //chiusura del file di log
}


