#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/random.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <signal.h>
#include "../../include/service-functions.h"

// MACROS
// INPUT_LEN: numero fisso di byte da leggere dall'input
#define INPUT_LEN 8

void signal_stp_handler( int );


// Funzione main che opera le operazioni per le componenti forward-facing-radar,
// surround-view-cameras e per il ciclo principale di park-assist.
// Gli argomenti richiesti sono il nome della della modalità di esecuzione
// e il nome del componente da simulare.
// Metodo di utilizzo:
// ./bytes-sensors "NORMALE"/"ARTIFICIALE" "RADAR"/"CAMERAS"
int main(int argc, char * argv[]){

	// File descriptor dei file di log e della pipe in cui scrivere il comando alla ECU
	int log_fd;
	int comm_fd;
	
	// Verifica della corretta chiamata del processo
	if(argc != 3){
		perror("bytes/sensors': syntax error");
		exit(EXIT_FAILURE);
	}

	// Inizializzazione del file descriptor tramite apertura del file
	// da cui ottenere i bytes di input.
	// Il file aperto dipende dalla modalità di esecuzione scelta e non dipende dal tipo di sensore creato.
	int input_fd;
	if(!strcmp(argv[1], "NORMALE")){
		if((input_fd = open("/dev/urandom", O_RDONLY)) < 0)
			perror("bytes-sensors: open NORMALE input");
	} else if (!strcmp(argv[1], "ARTIFICIALE")){
		if((input_fd = openat(AT_FDCWD, "urandomARTIFICIALE.binary", O_RDONLY)) < 0)
			perror("bytes-sensors: open ARTIFICIALE input");
	} else {
		perror("bytes sensors: chiamata modalita:");
		exit(EXIT_FAILURE);
	}

  // Connessione del file descriptor del log file. Apertura in sola scrittura.
  // Qualora il file non esista viene creato. Qualora il file sia presente
  // viene sovrascritto. In entrambi i casi le pipe sono non bloccanti.
	if(!strcmp(argv[2], RADAR)){
		if((log_fd = openat(AT_FDCWD, "log/radar.log", O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0){
			perror("bytes-sensors: openat log");
			exit(EXIT_FAILURE);
		}
		while((comm_fd = openat(AT_FDCWD, "tmp/radar.pipe", O_WRONLY)) < 0){
			perror("bytes-sensors: openat pipe");
			sleep(1);
		}
	} else if (!strcmp(argv[2], CAMERAS)){
		if((log_fd = openat (AT_FDCWD, "log/cameras.log", O_WRONLY | O_TRUNC | O_CREAT | O_NONBLOCK, 0644)) < 0){
			perror("bytes-sensors: openat log");
			exit(EXIT_FAILURE);
		}
		while((comm_fd = openat (AT_FDCWD, "tmp/cameras.pipe", O_WRONLY)) < 0){
			perror("bytes-sensors: openat pipe");
			sleep(1);
		}
	} else{
		perror("bytes-sensors: chiamata: tipologia funzione");
		exit(EXIT_FAILURE);
	}
	// Inizializzazione della stringa di input che rappresenta l'insieme di byte
  	// da trasmettere alla central-ECU da parte del processo.
  	// La stringa di unsigned char viene tradotta in una stringa di char lunga
	// il doppio più il carattere di fine riga.
  	// Ad ogni byte della stringa decodificata corrisponde il carattere ASCII
  	// che decodifica la meta` del corrispondente byte della stringa codificata.
  	// Quindi per ogni byte della stringa codificata occorreranno 2 byte della
  	// stringa decodificata per immagazzinare i caratteri ASCII appropriati.
  	// Il +1 nella stringa decodificata  e` necessario per inserirvi il carattere
	// di fine riga '\n'.
  	unsigned char input_str[BYTES_LEN];
	char input_hex[BYTES_CONVERTED];

	// Il ciclo successivo e` il cuore del processo.
	// Una volta al secondo esegue una read sul file descriptor del file di
	// input e, solo se il numero di unsigned  char letti e` uguale
	// al numero specificato in INPUT_LEN, decodifica la stringa letta, la invia
	// sul canale di comunicazione e la scrive nel file di log.
	// Il ciclo e` un ciclo infinito, nel caso caso si utilizzi il processo
	// per park-assist occorrerà inviare un segnale di interruzione dopo 30
	// secondi per interrompere la lettura e l'invio dei dati.
	while (true) {
		if(read_conv_broad(input_fd, input_hex, comm_fd, log_fd) <= 0){
			lseek(input_fd, 0, SEEK_SET);
			perror("bytes-sensors: input file terminated: file pointer reset");
		}
  }
}
