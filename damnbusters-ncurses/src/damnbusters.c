//#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdio.h>
#include <time.h>   /*  for timer  */
#include <curses.h>
#include <signal.h>

#define TIMEOUT 150000000

typedef struct { int x, y; } point;
point win_max;
point* ship_pos;
point* bomb_pos;
point* gun_pos;
bool refresh_scr = false;
timer_t gTimerid;
int key = 0;
//WINDOW *mainwin;

void handleAlarm() {
	refresh_scr = true;
}
int startTimer() {
	// setup timer
	//struct itimerspec ts = { {1, TIMEOUT*1000}, {1, TIMEOUT*1000} };
	// seconds/nanos before first run, seconds/nanos for interval
	struct itimerspec ts = { {0, TIMEOUT}, {5, 0} };
	struct sigaction sa;
	struct sigevent se;
	se.sigev_notify = SIGEV_SIGNAL;
	se.sigev_notify_attributes = NULL;
	se.sigev_signo = SIGALRM;
	se.sigev_notify_function = handleAlarm;
    /* Setting the signal handlers before invoking timer*/
    sa.sa_handler = handleAlarm;
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

	if (timer_create(CLOCK_REALTIME, &se, &gTimerid) != 0) {
		perror("Error creating timer..");
		return -1;
	}
	if (timer_settime(gTimerid, 0, &ts, NULL) != 0) {
		perror("Error setting timeout for time...");
		return -1;
	}
	return 0;
}
void updateShip() {
	ship_pos->x = ship_pos->x-2;
	if (ship_pos->x < 0) {
		ship_pos->x = win_max.x-1;
		move(ship_pos->y,0);
		clrtoeol();
	} else {
		move(ship_pos->y,ship_pos->x+1);
		delch();
		move(ship_pos->y,ship_pos->x+2);
		delch();
	}
	color_set(1, NULL);
	mvaddstr(ship_pos->y,ship_pos->x,"++");
}
void updateBomb() {
	if (bomb_pos != NULL) {
		// erase previous bomb pos
		move(bomb_pos->y, bomb_pos->x);
		delch();
		// update bomb position
		bomb_pos->y = bomb_pos->y+1;
		bomb_pos->x = bomb_pos->x-1;
		// remove bomb if it's position is outside the terminal
		if (bomb_pos->y > win_max.y) {
			free(bomb_pos);
			bomb_pos = NULL;
		} else {
			color_set(1, NULL);
			mvaddstr(bomb_pos->y,bomb_pos->x,"-");
		}
	}
}
void dropBomb() {
	// only allow bomb if bomb pos is null
	if (bomb_pos == NULL) {
		bomb_pos = malloc(sizeof(point));
		bomb_pos->x = ship_pos->x;
		bomb_pos->y = ship_pos->y+1;
	}
}
int init(void) {
	// setup ncurses dam
	initscr();
	noecho();
	curs_set(FALSE);
	nodelay(stdscr,TRUE);
	// get our maximum window dimensions
	getmaxyx(stdscr, win_max.y, win_max.x);
	ship_pos = malloc(sizeof(point));
	ship_pos->x = win_max.x-2;
	ship_pos->y = 5;
	// set up initial windows
	//mainwin = newwin(win_max.y, win_max.x, 0, 0);
	start_color();

	/*  Make sure we are able to do what we want. If
	has_colors() returns FALSE, we cannot use colours.
	COLOR_PAIRS is the maximum number of colour pairs
	we can use. We use 13 in this program, so we check
	to make sure we have enough available.               */
	if ( has_colors() && COLOR_PAIRS >= 5 ) {
		int x,y,z=0;
		//int y=0;

		init_pair(1,  COLOR_BLACK,   COLOR_WHITE);
		init_pair(2,  COLOR_WHITE,   COLOR_BLACK);
		init_pair(3,  COLOR_MAGENTA, COLOR_BLACK);
		init_pair(4,  COLOR_BLUE, COLOR_BLACK);
		init_pair(5,  COLOR_BLACK, COLOR_BLUE);

		for (x=0; x<15; x++) {
			color_set(1, NULL);
			// draw dam
			for (y=15-x; y>=0; y--) {
				mvaddstr(win_max.y-x, 15+y, " ");
			}
			// draw water behind dam
			color_set(5, NULL);
			for (z=0; z<=15; z++) {
				mvaddch(win_max.y-x, z, ACS_CKBOARD);
			}
			if (x==14) {
				color_set(2, NULL);
				move(win_max.y-15,0);
				hline('_',16);
				//mvaddstr(win_max.y-15,0,"_________________");
				color_set(2, NULL);
				mvaddstr(win_max.y-15,15,"//");
			}
			else if (x==0) {
				color_set(5, NULL);
				move(win_max.y-1,30);
				hline(ACS_CKBOARD,1000);
			}

		}
	} else
		return 1;
	return 0;
}

void cleanup() {
	free(ship_pos);
	ship_pos = NULL;
	free(bomb_pos);
	bomb_pos = NULL;
	free(gun_pos);
	gun_pos = NULL;
}


int main(void) {
	int initval = init();
	if (initval == 0) {
		/*  Refresh the screen and sleep for a
		while to get the full screen effect  */
		//(void) signal (SIGALRM, handleAlarm);
		color_set(1, NULL);
		mvaddstr(ship_pos->y,ship_pos->x,"++");
		refresh();
		key = getch();
		startTimer();
		while (key != 27) {
			key = getch();
			// drop bomb
			if (key == ' ') {
				dropBomb();
			}
			if (refresh_scr) {
				updateShip();
				updateBomb();
				refresh();
				//startTimer();
				refresh_scr = false;
			}
		}
	}
	else
		return initval;
	/*  Clean up after ourselves  */
	//delwin(mainwin);
	endwin();
	cleanup();

	return EXIT_SUCCESS;
}


