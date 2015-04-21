//#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdio.h>
#include <time.h>   /*  for timer  */
#include <curses.h>
#include <signal.h>

#define TIMEOUT 100000000

typedef struct { int x, y; } point;
int dblocks[15][30];
point win_max;
point* ship_pos;
point* bomb_pos;
point* gun_pos;
bool refresh_scr = false;
bool dead = false;
timer_t gTimerid;
int key = 0;
//WINDOW *mainwin;

void gameover() {
	mvaddstr(win_max.y/2, win_max.x/2, "faggot");
}
void handleAlarm() {
	refresh_scr = true;
}
int startTimer() {
	// setup timer
	//struct itimerspec ts = { {1, TIMEOUT*1000}, {1, TIMEOUT*1000} };
	// seconds/nanos before first run, seconds/nanos for interval
	struct itimerspec ts = { {0, TIMEOUT}, {2, 0} };
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
		ship_pos->x = win_max.x-2;
		move(ship_pos->y,0);
		clrtoeol();
		ship_pos->y = rand() % (win_max.y-16)+ 1;
	} else {
		if (dead) {
			if (ship_pos->y > win_max.y) {
				gameover();
			} else {
				move(ship_pos->y-1,ship_pos->x);
				delch();
				ship_pos->x = ship_pos->x+2;
				ship_pos->y = ship_pos->y+1;
				mvaddch(ship_pos->y, ship_pos->x, (rand() % 32) + 32);
			}
		} else {
			move(ship_pos->y,ship_pos->x+1);
			delch();
			move(ship_pos->y,ship_pos->x+2);
			delch();
		}
	}
	if (!dead) {
		color_set(1, NULL);
		mvaddstr(ship_pos->y,ship_pos->x,"++");
	}
}
void updateGuns() {
	if (gun_pos == NULL) {
		// randomly attempt to fire a round
		if ((rand() % 100) > 90) {
			gun_pos = malloc(sizeof(point));
			gun_pos->x = 16;
			gun_pos->y = win_max.y-16;
		}
	} else {
		// erase previous gun pos
		move(gun_pos->y, gun_pos->x);
		delch();
		// update gun position
		gun_pos->y = gun_pos->y-1;
		gun_pos->x = gun_pos->x+1;
		if (gun_pos->y < 1) {
			free(gun_pos);
			gun_pos = NULL;
		} else {
			color_set(2, NULL);
			mvaddstr(gun_pos->y,gun_pos->x,"*");
		}

	}
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
void detectCollisions() {
	// check to see if bullet has hit the ship
	if (gun_pos != NULL) {
		if (gun_pos->y == ship_pos->y && (gun_pos->x == ship_pos->x || gun_pos->x == ship_pos->x-1)) {
			dead = true;
		}
	}
	// check to see if bomb has impacted the dam
	if (bomb_pos != NULL) {
		if (win_max.y - bomb_pos->y <= 15 && bomb_pos->x <= 30) {
			// dam hit
			if (dblocks[win_max.y-bomb_pos->y][bomb_pos->x] != 0) {
				dblocks[win_max.y-bomb_pos->y][bomb_pos->x] = 0;
				move(bomb_pos->y, bomb_pos->x);
				delch();
				free(bomb_pos);
				bomb_pos = NULL;
			}
		}
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

		init_pair(1, COLOR_BLACK,   COLOR_WHITE);
		init_pair(2, COLOR_WHITE,   COLOR_BLACK);
		init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(4, COLOR_BLUE,	COLOR_BLACK);
		init_pair(5, COLOR_BLACK,	COLOR_BLUE);

		for (y=0; y<15; y++) {
			color_set(1, NULL);
			// draw dam
			for (x=15-y; x>=0; x--) {
				dblocks[y][15+x] = " ";
				mvaddstr(win_max.y - y, 15+x, " ");
			}
			// draw water behind dam
			color_set(5, NULL);
			for (x=0; x<=15; x++) {
				dblocks[y][x] = ACS_CKBOARD;
				mvaddch(win_max.y - y, x, ACS_CKBOARD);
			}
			// draw the top of the dam and guns
			if (y==14) {
				color_set(2, NULL);
				move(win_max.y-15,0);
				hline('_',16);
				//mvaddstr(win_max.y-15,0,"_________________");
				color_set(2, NULL);
				mvaddstr(win_max.y-15,15,"//");
			}
			// draw the water in front of the dam
			else if (y==0) {
				color_set(5, NULL);
				move(win_max.y-1,30);
				hline(ACS_CKBOARD,1000);
			}
		}
		// draw the initial position of the ship
		ship_pos = malloc(sizeof(point));
		ship_pos->x = 0;
		updateShip();

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
		startTimer();
		while (key != 27) {
			key = getch();
			// drop bomb
			if (key == ' ') {
				dropBomb();
			}
			if (refresh_scr) {
				if (dead) {
					updateShip();
				}
				else {
					updateBomb();
					updateGuns();
					updateShip();
					detectCollisions();
				}
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


