#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX 5

/* Lista di stringhe. Questa struttura memorizza il percorso del treno */
struct element {
	char cElement[MAX];
	struct element *pNext; //puntatore all'elemento successivo
};

//struttura dati per tutti i percorsi. T1Path è il percorso del treno 1, T2Path del treno 2...
struct paths {
	struct element *T1Path;
	struct element *T2Path;
	struct element *T3Path;
	struct element *T4Path;
	struct element *T5Path;
};

//Struttura dati per i binari
struct platform {
	char cBinary[5]; //nome del binario
	int iState; //stato, 0 libero 1 occupato
	struct platform *pNextPlat; //puntatore alla struttra del binario successivo
};

//Struttura dati per le stazioni
struct station {
	char cStation[3]; //nome della stazione
	int iState; //numero di treno dentro la stazione
	struct station *pNextStation;
};

struct element *fillPath(char*);
void printPath(struct element*);

struct paths *initializePaths(void);
void printPaths(struct paths *);

struct platform *initializePlatforms(void);
void printPlatforms(struct platform *);

struct station *initializeStations(void);
void printStations(struct station *);

//legge il percorso del treno dal file apposito e lo memorizza in una struct element
struct element *fillPath(char * cTrain) {
	struct element *pFirst, *pIndex; //pFirst punta al primo elemento della lista, pIndex serve a scorrerla.
	pFirst=(struct element *)malloc(sizeof(struct element)); //allochiamo lo spazio necessario
	pIndex=pFirst; 
	char acBuffer[2];
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));//otteniamo la working directory corrente
	sprintf(cwd, "%s/T1-5", cwd); //salviamo in cwd il percorso della cartella nella quale ci vogliamo spostare
	chdir(cwd); //cambiamo directory, ci spostiamo nel percorso indicato da cwd.
	sprintf(acBuffer, "T%s", cTrain);
	int fd=open(acBuffer,  O_RDONLY);
	int iEnd=lseek(fd, 0, SEEK_END); //prendo l'offset di fine file
	int iStart=lseek(fd, 0, SEEK_SET); //mi posiziono all'inizio del file
	char acRead[2];
	char acTemp[5];//qui salvo via via le stazioni/binari
	while (1) {
		int iRead=read(fd, acRead, 1);
		if (lseek(fd, 0, SEEK_CUR)==iEnd){ //carattere letto carattere di fine stringa
			strcpy(pIndex->cElement, acTemp); //copio acTemp nel cElement corrente
			break;
		}else if(strncmp(acRead," ", 1)==0 || strncmp(acRead, "\n", 1)==0){ //letto uno spazio
			//non faccio nulla
		}else if(strncmp(acRead,",", 1)==0){//letta una virgola
			strcpy(pIndex->cElement, acTemp);
			pIndex ->pNext=(struct element *)malloc(sizeof(struct element)); //allochiamo la memoria necessaria alla struttura element
			pIndex=pIndex->pNext; //pIndex punta all'elemento successivo
			strcpy(acTemp, "");
		}else{ //letto un qualunque altro carattere
			sprintf(acTemp, "%s%s", acTemp, acRead);
		}
		lseek(fd, 0, SEEK_CUR);
	}
	pIndex->pNext=NULL; //marcatore fine lista
	close(fd);
	chdir("../");
	return (pFirst);
}

//per debugging, stampa una struct element
void printPath(struct element *p) {
	printf("\nLista-> ");
	while(p!=NULL){
		printf("%s", p->cElement);
		printf(" - ");
		p=p->pNext; //si scorre di un elemento
	}
	printf("NULL\n");
}

//inizializza una struttura paths allocando la memoria per sè stessa e per i path al suo interno.
struct paths *initializePaths() {
	struct paths *pPath;
	pPath=(struct paths *)malloc(sizeof(struct paths));
	pPath->T1Path=(struct element *)malloc(sizeof(struct element));
	pPath->T2Path=(struct element *)malloc(sizeof(struct element));
	pPath->T3Path=(struct element *)malloc(sizeof(struct element));
	pPath->T4Path=(struct element *)malloc(sizeof(struct element));
	pPath->T5Path=(struct element *)malloc(sizeof(struct element));
	return pPath;
}

//inizializzazione dei binari
struct platform *initializePlatforms() {
	struct platform *pPlatform, *pFirst;  //pFirst è il puntatore al primo elemento, pPlatform scorrerà la lista
	pFirst=(struct platform *)malloc(sizeof(struct platform)); //allochiamo lo spazio necessario
	pPlatform=pFirst;
	for (int i=0;i<16;i++) { //ciclo for con 16 iterazioni, tante quanti sono i binari
		char acBuffer[5];
		sprintf(acBuffer, "MA%d", i+1);
		strcpy(pPlatform->cBinary, acBuffer); //assegnazione del nome del binario
		pPlatform->iState=0; //stato settato a libero (all'inizio nessun treno è in un binario)
		if (i!=15) {
			pPlatform->pNextPlat=(struct platform *)malloc(sizeof(struct platform)); 
		}
		pPlatform=pPlatform->pNextPlat; //scorriamo la lista
	}
	return pFirst;
}

//inizializzazione delle stazioni
struct station *initializeStations() {
	struct station *pStation, *pFirst;
	pFirst=(struct station *)malloc(sizeof(struct station)); //allochiamo lo spazio necessario
	pStation=pFirst;
	for (int i=0;i<8;i++) {
		char acBuffer[3];
		sprintf(acBuffer, "S%d", i+1);
		strcpy(pStation->cStation, acBuffer);
		if (i>=1 && i<=5) { //all'inizio ci sono treno in tutte le stazioni, tranne che in S1, S7 e S8
			pStation->iState=1;
		} else { 
			pStation->iState=0;
		}
		if (i!=7) { //se non è l'ultima stazione alloca lo spazio per la prossima
			pStation->pNextStation=(struct station *)malloc(sizeof(struct station));
		}
		pStation=pStation->pNextStation; //scorriamo la lista
	}
	return pFirst;
}

//per debugging, stampa la lista dei binari
void printPlatforms(struct platform *pIndex) {
	printf("\nLista-> ");
	while(pIndex!=NULL) {
		printf("%s occupato: %d", pIndex->cBinary, pIndex->iState);
		printf(" - ");
		pIndex=pIndex->pNextPlat; //si scorre di un elemento
	}
	printf("NULL\n");
}

//per debugging, stampa la lista delle stazioni
void printStations(struct station *pIndex) {
	printf("\nLista-> ");
	while(pIndex!=NULL) {
		printf("%s numero treni: %d", pIndex->cStation, pIndex->iState);
		printf(" - ");
		pIndex=pIndex->pNextStation; //si scorre di un elemento
	}
	printf("NULL\n");
}

//per debugging, stampa tutti i percorsi dei treni
void printPaths(struct paths* pPaths) {
	printPath(pPaths->T1Path);
	printPath(pPaths->T2Path);
	printPath(pPaths->T3Path);
	printPath(pPaths->T4Path);
	printPath(pPaths->T5Path);	
}


