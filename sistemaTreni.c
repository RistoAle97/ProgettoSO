#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
	if(argc==1) { 
		printf("Nessun parametro in ingresso inserito\nSi prega di inserire dei parametri\n");
	}else if(strncmp(argv[1], "ETCS1", 5)==0) { 
		printf("Avvio del programma in modalità ETCS1 \n");
		system("cc -o ./padreTreni ./padreTreni.c"); //compiliamo il file padreTreni.c creando l'eseguibile padreTreni
		execl("./padreTreni", "padreTreni", "ETCS1", NULL); //eseguiamo l'eseguibile padreTreni in modalità ETCS1
	}else if(strncmp(argv[1], "ETCS2", 5)==0) {
		printf("Avvio del programma in modalità ETCS2, avviato: ");
		if (argc==3 && strncmp(argv[2], "RBC", 3)==0) { //se sono stati inseriti due parametri controlla che il secondo sia RBC. In tal caso RBC viene eseguito
			printf("RBC\n");
			system("cc -o ./RBCServer ./RBC.c"); //compiliamo il file RBC, che servirà come server socket
			execl("./RBCServer", "RBC", NULL); //eseguimamo il file RBC
		} else { //altrimenti viene eseguito padreTreni in modalità ETCS2
			printf("padreTreni\n");
			system("cc -o ./padreTreni ./padreTreni.c");
			execl("./padreTreni", "padreTreni", "ETCS2", NULL); //eseguiamo l'eseguibile padreTreni in modalità ETCS2
		}
	}
	exit(0);
	return 0;
}
