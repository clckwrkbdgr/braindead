#include <ncurses.h>
#include <cstdlib>
#include <time.h>

const int W = 80;
const int H = 24;
const int MONSTER_COUNT = 32;
const int ITEM_COUNT = 32;
const int MAX_DAMAGE = 10;
const int MAX_PROTECTION = 10;

const int FLOOR_COUNT = 3;
char FLOOR[FLOOR_COUNT + 1] = ".,_";
const int WALL_COUNT = 3;
char WALL[WALL_COUNT + 1] = "#8&";
const int ITEM_SPRITE_COUNT = 4;
char ITEM_SPRITE[ITEM_SPRITE_COUNT + 1] = "()[]";
char PLAYER = '@';
char MONSTER = 'A';
char STAIRS = '>';

const int QUIT = 'q';
const int UP = 'k';
const int DOWN = 'j';
const int LEFT = 'h';
const int RIGHT = 'l'; // TODO yubn
const int WAIT = '.';
const int WIELD = 'w';
const int IDENTIFY = 'i';

const char * STATUS_WITH_ITEM =
	"turns: %d lvl:%d x:%d y:%d mana:%d  item:%c                             ";
const char * STATUS_WITH_ITEM_ID =
	"turns: %d lvl:%d x:%d y:%d mana:%d  item:%c dmg:%d prt:%d               ";
const char * STATUS_WITHOUT_ITEM =
	"turns: %d lvl:%d x:%d y:%d mana:%d                                      ";

void init() {
	srand(time(NULL));
	initscr();
	cbreak();
	keypad(stdscr, true);
	noecho();
}

void close() {
	echo();
	endwin();
}

struct Item {
	char sprite;
	int damage, protection;
	bool identified;
	Item() : sprite(0), damage(0), protection(0),
		identified(false) {}
	Item(char s, int d, int p) : sprite(s), damage(d), protection(p),
		identified(false) {}
};

struct Cell {
	char sprite;
	bool passable;
	Item * item;
	Cell() : sprite(0), passable(true), item(NULL) {}
};

struct Monster {
	int x, y;
	char sprite;
	Item * item;
	Monster() : x(0), y(0), sprite(0), item(NULL) {}
	Monster(int px, int py, char s) : x(px), y(py), sprite(s), item(NULL) {}
};

bool get_random_free_drop_spot(Cell * map, int * x, int * y) {
	for(int i = 0; i < 1000; ++i) {
		*x = rand() % W;
		*y = rand() % H;
		if(map[*x + *y * W].passable && map[*x + *y * W].item == NULL) {
			return true;
		}
	}
	return false;
}

bool get_random_free_cell(Cell * map, Monster * monsters, int * x, int * y) {
	for(int i = 0; i < 1000; ++i) {
		*x = rand() % W;
		*y = rand() % H;
		if(map[*x + *y * W].passable) {
			bool success = true;
			for(int j = 0; j < MONSTER_COUNT; ++j) {
				if(monsters[j].x == *x && monsters[j].y == *y) {
					success = false;
					break;
				}
			}
			if(success) return true;
		}
	}
	return false;
}

void run() {
	int level = 0;
	int turns = 0;
	int mana = 10;
	Cell map[W*H];
	Monster monsters[MONSTER_COUNT];

	// Generate and fill map.
	for(int x = 0; x < W; ++x) {
		for(int y = 0; y < H; ++y) {
			map[x + y * W].sprite = FLOOR[rand() % FLOOR_COUNT];
		}
	}
	// Drop items.
	for(int i = 0; i < ITEM_COUNT; ++i) {
		int x, y;
		if(!get_random_free_drop_spot(map, &x, &y)) {
			return;
		}
		map[x + y * W].item = new Item(ITEM_SPRITE[rand() % ITEM_SPRITE_COUNT], rand() % MAX_DAMAGE, rand() % MAX_PROTECTION);
	}
	// Spawn monsters.
	for(int i = 0; i < MONSTER_COUNT; ++i) {
		int x, y;
		if(!get_random_free_cell(map, monsters, &x, &y)) {
			return;
		}
		monsters[i] = Monster(x, y, MONSTER);
	}
	monsters[0].sprite = PLAYER; // Make first monster to be player.

	// Loop.
	while(true) {
		// Status line
		Item * item = monsters[0].item;
		if(item) {
			if(item->identified) {
				mvprintw(H, 0, STATUS_WITH_ITEM_ID, turns, level, monsters[0].x, monsters[0].y, mana, item->sprite, item->damage, item->protection);
			} else {
				mvprintw(H, 0, STATUS_WITH_ITEM, turns, level, monsters[0].x, monsters[0].y, mana, item->sprite);
			}
		} else {
			mvprintw(H, 0, STATUS_WITHOUT_ITEM, turns, level, monsters[0].x, monsters[0].y, mana);
		}

		// Draw map.
		for(int x = 0; x < W; ++x) {
			for(int y = 0; y < H; ++y) {
				int sprite = map[x + y * W].sprite;
				if(map[x + y * W].item) {
					sprite = map[x + y * W].item->sprite;
				}
				mvaddch(y, x, sprite);
			}
		}
		for(int i = 0; i < MONSTER_COUNT; ++i) {
			int sprite = monsters[i].sprite;
			mvaddch(monsters[i].y, monsters[i].x, sprite);
		}

		refresh();

		// Read keys and do actions.
		int ch = getch();
		int new_x = monsters[0].x, new_y = monsters[0].y;
		bool turn_ended = true;
		switch(ch) {
			case QUIT: return;
			case UP: --new_y; break;
			case DOWN: ++new_y; break;
			case LEFT: --new_x; break;
			case RIGHT: ++new_x; break;
			case WAIT: break;
			case IDENTIFY: {
				Item * item = monsters[0].item;
				if(mana > 0 && item && !item->identified) {
					item->identified = true;
					--mana; // TODO gather mana from dead monsters.
				}
				break;
			}
			case WIELD: {
				Item * tmp = map[monsters[0].x + monsters[0].y * W].item;
				map[monsters[0].x + monsters[0].y * W].item = monsters[0].item;
				monsters[0].item = tmp;
				break;
			}
			default: turn_ended = false;
		}
		if(turn_ended) ++turns;

		if(new_x != monsters[0].x || new_y != monsters[0].y) {
			if(new_x >= 0 && new_x < W && new_y >= 0 && new_y < H) {
				bool success = true;
				for(int j = 0; j < MONSTER_COUNT; ++j) {
					if(monsters[j].x == new_x && monsters[j].y == new_y) {
						success = false;
						break;
					}
				}
				
				if(success) {
					monsters[0].x = new_x;
					monsters[0].y = new_y;
				}
			}
		}
	}

	// TODO delete items.
}

int main() {
	init();
	run();
	close();
	return 0;
}
