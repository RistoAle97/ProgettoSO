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
#include <sys/socket.h>
#include <sys/un.h> /* For AF_UNIX sockets */

#define FILE_MODE ( O_CREAT | O_TRUNC | O_RDWR)
#define DEFAULT_PROTOCOL 0

void createTrains(int*, char*);
void createPlatforms(void);
void tellPathsToServer(int);
struct stat st={0};

/*
Procedura che crea i 5 processi treni
*/
void createTrains(int *aiTreni, char *cMode) {
	int iFatherPID=getpid();
	system("cc -o ./Train ./Train.c"); //compiliamo il file Train.c
	for(int i=0;i<5;i++) {
		aiTreni[i]=fork();	//creaiamo i processi figli
		if (getpid()!=iFatherPID) {
			char cIndex[5];
			snprintf(cIndex, sizeof(cIndex), "%d", i+1); //copiamo i+1 in cIndex, che diventa una stringa 
			char *args[]={"./Train", cIndex, cMode, NULL}; //argomenti da passare al treno
			execvp(args[0], args); //eseguiamo il processo treno
		}
	}
}

/*
Procedura incaricata di creare i 16 binari e di scriverci 0 (disponibile) dentro
*/
void createPlatforms() { 
	int aiPlatforms[16]; //array dei 16 binary
	for(int i=0;i<16;i++) {
		char acBuffer[14];
		int iN=sprintf(acBuffer, "./Binari/MA%d",i+1); //inserisco in acBuffer il percorso del file
		aiPlatforms[i]=open(acBuffer, FILE_MODE, 0777); //creo il file se non esiste
		write(aiPlatforms[i], "0", 1); //inserisco il valore 0 dentro il file
		int iClose=close(aiPlatforms[i]);
	}
}

/*
Procedura che che, dopo aver stabilito una connessione col server socket, comunica i percorsi dei 5 treni
*/
void tellPathsToServer(int i) {
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
	
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));//otteniamo la working directory corrente
	sprintf(cwd, "%s/T1-5", cwd); //salviamo in cwd il percorso della cartella nella quale ci vogliamo spostare
	chdir(cwd); //ci spostiamo nella directory contenente i percorsi
	printf("-----Treno %d-----\n", i+1);
	char acBuffer[2]; //buffer nel quale verrà memorizzato il nome del file dei percorsi da aprire
	char cIndex[2]; 
	sprintf(cIndex, "%d", i+1); //copiamo il numero del treno in cIndex, che quindi diventa una stringa
	sprintf(acBuffer, "T%s", cIndex);
	int fd=open(acBuffer,  O_RDONLY); //apriamo il file corrispondente
	int iEnd=lseek(fd, 0, SEEK_END); //prendo l'offset di fine file
	int iStart=lseek(fd, 0, SEEK_SET); //mi posiziono all'inizio del file
	char acRead[2]; //qui memorizziamo l'ultimo carattere letto
	char acTemp[150];//qui salvo via via le stazioni/binari
	sprintf(acTemp, "%d", i+1); //si salva in acTemp il numero del treno
	while (1) {
		int iRead=read(fd, acRead, 1); //lettura di un carattere alla volta
		if (lseek(fd, 0, SEEK_CUR)==iEnd) { //carattere letto carattere di fine stringa
			write(clientFd, acTemp, sizeof(acTemp));
			printf("Tragitto: %s\n\n", acTemp);
			strcpy(acTemp, "");
			break;
		}else if(strncmp(acRead," ", 1)==0 || strncmp(acRead, "\n", 1)==0) { //letto uno spazio
			//non faccio nulla
		}else if(strncmp(acRead,",", 1)==0) {//letta una virgola
			sprintf(acTemp, "%s ", acTemp);
		}else{ //letto un qualunque altro carattere
			sprintf(acTemp, "%s%s", acTemp, acRead);
		}
		lseek(fd, 0, SEEK_CUR);	
	}
	close(fd);
	chdir("../");
	close(clientFd); //chiude il socket
}

int main(int argc, char *argv[]) {
	int aiTreni[5]; //si crea un array con gli id dei processi treni
	if (stat("./Binari", &st) == -1) { //controllo se la directory Binari esiste già
		mkdir("./Binari", 0777); //crea tale directory nel caso non esistesse
	}
	createPlatforms(); //vengono creati i file dei binari
		
	if (strncmp(argv[1], "ETCS2", 5)==0) { //programma avviato in modalità ETCS2
		for (int i=0;i<5;i++) {
			tellPathsToServer(i); // per 5 volte effettuiamo una connessione col server. Ogni volta passiamo il percorso di un treno
		}
	} 
	createTrains(aiTreni, argv[1]); //creiamo i 5 processi treni
	int iFatherPID=getpid(); //otteniamo il pid del processo padre
	int status=0;
	while ((iFatherPID=wait(&status))>0); //il processo padre aspetta che i figli termino
	printf("Processo padre terminato\n"); 
	exit(0);
	return 0;
}
