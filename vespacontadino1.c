/* vespacontadino1b.c - RS-2013 [impiego di fork e pipes]
   ----------------------------------------------------------------------------------
			Gioco 'Vespa e contadino', una vespa (visualizzata sullo schermo come '^')  
			si muove casualmente all'interno nella finestra di output definita, secondo 
			un percorso lineare, quindi non a passi random;	un contadino (visualizzato 
			sullo schermo come '#'), iniziamente posizionato al centro dello schermo e 
			con 3 vite a disposizione, deve muoversi tramite i tasti cursore per evitare 
			di essere punto dalla vespa e, quando ciò accade, le sue vite diminuiscono di 
			una unità; durante il gioco, a periodi prestabiliti, vengono casualmente 
			visualizzate sullo schermo 3 trappole; quando la vespa entra in contatto con 
			una di queste le vite del contadino aumentano di una unità (comunque fino ad 
			un massimo di 6); le ultime 3 trappole inserite devono eliminare le precedenti 
			e devono essere diverse nelle coordinate, sia tra loro sia dalle precedenti, 
			cioè devono sempre essere visualizzate 3 nuove trappole; la generazione e 
			visualizzazione delle trappole deve avvenire all'interno nel processo padre;
			l'informazione relativa alle vite del contadino deve sempre essere mostrata 
			nella parte in alto a sinistra dello schermo di output e il gioco deve terminare 
			con un messaggio di "GAME OVER" quando il valore delle vite  è pari a zero.
			Per la realizzazione del programma vengono impiegati 3 processi che comunicano 
			tra loro tramite 1 sola pipe; in pratica, ciascun processo figlio scrive nella 
			pipe che viene continuamente letta dal processo padre: 
			 
			(1) il processo 'padre' (Main) invoca al termine dei fork() dei processi figli 
							la funzione AreaGioco() la quale si occupa di generare e mostrare le 3 
							trappole, nonché di visualizzare le altre entità di gioco (vespa e contadino),
						 rilevando, identificando e gestendo opportunamente le eventuali collisioni; 
 
			(2) il primo processo 'figlio' (Vespa) produce un movimento casuale dell'ape sullo 
							schermo, comunicando le coordinate al processo padre per la visualizzazione e 
							il controllo di collisione; 

			(3) il secondo processo 'figlio' (Contadino) rileva la pressione dei tasti cursore, 
							calcola le nuove coordinate del contadino e le comunica al processo padre per 
							la visualizzazione. 

			Nella compilazione è necessario aggiungere le librerie 'lncurses' in
			questo modo 'gcc vespacontadino.c -o vespacontadino -lncurses
   ----------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

//aggiunta di una libreria mancante per evitare il warning relativo alla funzione kill definita implicitamente

#include <signal.h>



#define UP 65		/* Cursore sopra */
#define DW 66		/* Cursore sotto */
#define SX 68		/* Cursore sinistra */
#define DX 67		/* Cursore destra */
#define MAXX 80		/* Dimensione dello schermo di output (colonne) */
#define MAXY 20		/* Dimensione dello schermo di output (righe)   */
#define DELAY 80000 /* Ritardo nel movimento delle vespe (da adattare) */

/* Prototipi delle funzioni adoperate */
void Vespa(int pipeout);
void Contadino(int pipeout);
void Area(int pipein);

/* Struttura adoperata per veicolare le coordinate */
struct position
{
	char c; /* Identificatore dell'entità che invia i dati */
	int x;  /* Coordinata X */
	int y;  /* Coordinata Y */
};

/*
----------------------------------------------------------------------
 Funzione principale del programma 
----------------------------------------------------------------------
*/
int main()
{
	int p[2]; /* Descrittori pipe */
	int pidV; /* Pid processo figlio 'Vespa' */
	int pidC; /* Pid processo figlio 'Contadino1 */

	initscr();   /* Inizializza schermo di output */
	noecho();	/* Imposta modalità della tastiera */
	curs_set(0); /* Nasconde il cursore */
	pipe(p);	 /* Creazione pipe */

	/* Inizializza generatore di numeri casuali */
	srand(time(NULL));

	/* Creo il primo processo figlio 'Vespa' */
	pidV = fork();

	/* Se il pid == 0 -> si tratta del processo 'Vespa' */
	if (pidV == 0)
	{
		/* ed eseguo quindi la relativa funzione di gestione */
		Vespa(p[1]);
	}
	else
	{
		/* Altrimenti sono ancora nel processo padre e creo il processo 'Contadino' */
		pidC = fork();

		/* Se il pid == 0 -> si tratta del processo 'Contadino' */
		if (pidC == 0)
		{
			/* Visualizzo il contadino nella posizione iniziale */
			mvprintw(MAXY / 2, MAXX / 2, "#");

			/* ed eseguo quindi la relativa funzione di gestione */
			Contadino(p[1]);
		}
		else
		{
			/* Sono ancora nel processo padre e invoco la funzione Area() */
			Area(p[0]);
		}
	}

	/* Termino i processi Vespa e Contadino */
	kill(pidV, 1);
	kill(pidC, 1);

	/* Ripristino la modalità di funzionamento usuale */
	endwin();

	/* Termino il gioco ed esco dal programma */
	printf("\n\n\nGAME OVER\n\n\n");

	return 0;
}

/*
----------------------------------------------------------------------
 Funzione 'Vespa'
---------------------------------------------------------------------- 
*/
void Vespa(int pipeout)
{
	struct position vespa;
	int deltax; /* Spostamento orizzontale */
	int deltay; /* Spostamento orizzontale */

	vespa.x = 1;   /* Coordinata X iniziale */
	vespa.y = 1;   /* Coordinata Y iniziale */
	vespa.c = 'v'; /* Carattere identificativo */

	/* Comunico le coordinate iniziali al processo padre */
	write(pipeout, &vespa, sizeof(vespa));

	while (1)
	{
		if (random() < RAND_MAX / 2)
			deltax = 1;
		else
			deltax = -1;

		/* Se supero area X schermo inverte il movimento */
		if (vespa.x + deltax < 1 || vespa.x + deltax > MAXX)
		{
			deltax = -deltax;
		}

		/* Movimento X */
		vespa.x += deltax;

		if (random() < RAND_MAX / 2)
			deltay = 1;
		else
			deltay = -1;

		/* Se supero area Y schermo inverto il movimento */
		if (vespa.y + deltay < 1 || vespa.y + deltay > MAXY)
		{
			deltay = -deltay;
		}

		/* Movimento Y */
		vespa.y += deltay;

		/* Comunico le coordinate correnti al processo padre */
		write(pipeout, &vespa, sizeof(vespa));

		/* Inserisco una pausa per rallentare il movimento */
		usleep(DELAY);
	}
}

/*
----------------------------------------------------------------------
 Funzione 'Contadino' - Movimento tramite i tasti cursore 
----------------------------------------------------------------------
*/
void Contadino(int pipeout)
{
	struct position cpos;

	cpos.x = MAXX / 2; /* Coordinata X iniziale */
	cpos.y = MAXY / 2; /* Coordinata Y iniziale */
	cpos.c = '#';	  /* Carattere identificativo contadino*/

	/* Comunico al processo padre le coordinate iniziali del contadino */
	write(pipeout, &cpos, sizeof(cpos));

	/* Lettura dei tasti cursore */
	while (1)
	{
		char c;
		c = getch();

		if (c == UP && cpos.y > 0)
		{
			cpos.y -= 1;
		}

		if (c == DW && cpos.y < MAXY - 1)
		{
			cpos.y += 1;
		}

		if ((c == SX) && cpos.x > 0)
		{
			cpos.x -= 1;
		}

		if (c == DX && cpos.x < MAXX - 1)
		{
			cpos.x += 1;
		}

		/* Comunico al processo padre le coordinate del contadino */
		write(pipeout, &cpos, sizeof(cpos));
	}
}

/*
----------------------------------------------------------------------
 Funzione relativa al processo di visualizzazione e controllo
----------------------------------------------------------------------
*/
void Area(int pipein)
{
	struct position vespa, contadino, dato_letto;
	struct position trappola1, told1;
	struct position trappola2, told2;
	struct position trappola3, told3;
	int i = 0, vite = 3, collision = 0;

	/* Il valore -1 segnala che si tratta della prima lettura */
	vespa.x = -1;
	contadino.x = -1;

	/* Visualizzo le vite iniziali del contadino */
	mvprintw(0, 1, "%3d", vite);

	do
	{
		/* Leggo dalla pipe */
		read(pipein, &dato_letto, sizeof(dato_letto));

		/* Vespa */
		if (dato_letto.c == 'v')
		{
			/* Verifico se non si tratta della prima lettura */
			if (vespa.x >= 0)
			{
				/* Cancello il precedente carattere visualizzato */
				mvaddch(vespa.y, vespa.x, ' ');
			}

			/* Aggiorno le coordinate relative alla nuova posizione */
			vespa = dato_letto;
		}
		else
		{
			/* Contadino */
			/* Verifica se non si tratta della prima lettura */
			if (contadino.x >= 0)
			{
				/* Cancello il precedente carattere visualizzato */
				mvaddch(contadino.y, contadino.x, ' ');
			}
			/* Aggiorno le coordinate relative alla nuova posizione */
			contadino = dato_letto;
		}
		/* Visualizzo il carattere dell'entità sulle nuove coordinate */
		if (vespa.x != 1 || vespa.y != 1)
			mvaddch(dato_letto.y, dato_letto.x, dato_letto.c);

		/* Ogni 100 cicli genero 3 trappole in posizione casuale */
		if (!(i++ % 100))
		{
			/* Cancello le precedenti trappole visualizzate*/
			mvaddch(told1.y, told1.x, ' ');
			mvaddch(told2.y, told2.x, ' ');
			mvaddch(told3.y, told3.x, ' ');

			/* Genero casualmente le nuove coordinate delle 3 trappole verificando che
									le nuove coordinate non siano uguali alle precedenti o alle altre trappole */
			do
			{
				trappola1.x = rand() % MAXX;
				trappola1.y = rand() % MAXY;
			} while (trappola1.x == told1.x && trappola1.y == told1.y);

			do
			{
				trappola2.x = rand() % MAXX;
				trappola2.y = rand() % MAXY;
			} while (trappola2.x == told2.x && trappola2.y == told2.y ||
					 trappola2.x == trappola1.x && trappola2.y == trappola1.y);

			do
			{
				trappola3.x = rand() % MAXX;
				trappola3.y = rand() % MAXY;
			} while (trappola3.x == told3.x && trappola3.y == told3.y ||
					 trappola3.x == trappola1.x && trappola3.y == trappola1.y ||
					 trappola3.x == trappola2.x && trappola3.y == trappola2.y);

			/* Visualizzo le nuove trappole */
			mvaddch(trappola1.y, trappola1.x, 'X');
			mvaddch(trappola2.y, trappola2.x, 'X');
			mvaddch(trappola3.y, trappola3.x, 'X');

			/* Memorizzo le coordinate delle trappole visualizzate */
			told1.x = trappola1.x;
			told1.y = trappola1.y;
			told2.x = trappola2.x;
			told2.y = trappola2.y;
			told3.x = trappola3.x;
			told3.y = trappola3.y;
		}

		/* Visualizzo le vite rimaste al contadino */
		mvprintw(0, 1, "%3d", vite);

		/* Nascondo il cursore */
		curs_set(0);

		/* Aggiorno lo schermo di output per visualizzare le modifiche */
		refresh();

		/* Segnalo collisione e tipo (vespa/contadino oppure vespa/trappola) */
		if (contadino.x == vespa.x && contadino.y == vespa.y)
			vite--;
		if (vespa.x == trappola1.x && vespa.y == trappola1.y ||
			vespa.x == trappola2.x && vespa.y == trappola2.y ||
			vespa.x == trappola3.x && vespa.y == trappola3.y)
			if (vite < 6)
				vite++;

		/* Esce quando terminano le vite del contadino */
		if (vite < 1)
			collision = 1;

		/* Il ciclo si ripete finchè non si verifica una collisione contadino/vespa */
	} while (!collision);
}
