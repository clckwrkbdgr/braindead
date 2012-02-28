#include <ncurses.h>
#include <cstdlib>
#include <time.h>

const int W = 80;
const int H = 24;
const int MONSTER_COUNT = 3;
const int ITEM_COUNT = 3;
const int FLOOR_PROB = 100;
const int MAX_DAMAGE = 10;
const int MAX_PROTECTION = 10;
const int PLAYER_STRENGTH = 3;
const int PLAYER_RESISTANCE = 1;
const int PLAYER_HP = 100;
const int MONSTER_STRENGTH = 3;
const int MONSTER_RESISTANCE = 1;
const int MONSTER_HP = 20;
const int HP_UNIT = 10;

// TODO different sprites for monsters. different pre-settings for them
const char FLOOR = '.';
const char WALL = '#';
const char ITEM_SPRITE = '(';
const char PLAYER = '@';
const char MONSTER = 'M';

const int QUIT = 'q';
const int NEXT = '>';
const int UP = 'k';
const int DOWN = 'j';
const int LEFT = 'h';
const int RIGHT = 'l';
const int UP_RIGHT = 'u';
const int DOWN_RIGHT = 'n';
const int UP_LEFT = 'y';
const int DOWN_LEFT = 'b';
const int WAIT = '.';
const int WIELD = 'w';
const int IDENTIFY = 'i';

const char * STATUS_WITH_ITEM =
	"lvl:%d (%d,%d) hp:%d/%d str:%d res:%d item:%c            ";
const char * STATUS_WITH_ITEM_ID =
	"lvl:%d (%d,%d) hp:%d/%d str:%d res:%d item:%c id'd       ";
const char * STATUS_WITHOUT_ITEM =
	"lvl:%d (%d,%d) hp:%d/%d str:%d res:%d                    ";

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
	Cell(char s, bool p) : sprite(s), passable(p), item(NULL) {}
};

struct Monster {
	int x, y;
	char sprite;
	int hp;
	int hit_strength;
	int resistance;
	Item * item;
	Monster() : x(0), y(0), sprite(0), hp(0), hit_strength(0), resistance(0), item(NULL) {}
	Monster(int px, int py, char s, int hitp, int hit, int res) : x(px), y(py), hp(hitp), sprite(s), hit_strength(hit), resistance(res), item(NULL) {}
};

bool is_alive(Monster * monster) {
	return monster->hp > 0;
}

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

bool generate(Cell * map, Monster * monsters) {
	// Generate and fill map.
	for(int x = 0; x < W; ++x) {
		for(int y = 0; y < H; ++y) {
			if(map[x + y * W].item)
				delete map[x + y * W].item;
			map[x + y * W] = (rand() % FLOOR_PROB) ? Cell(FLOOR, true) : Cell(WALL, false);
		}
	}
	// Drop items.
	for(int i = 0; i < ITEM_COUNT; ++i) {
		int x, y;
		if(!get_random_free_drop_spot(map, &x, &y)) {
			return false;
		}
		map[x + y * W].item = new Item(ITEM_SPRITE, rand() % MAX_DAMAGE, rand() % MAX_PROTECTION);
	}
	// Make first monster to be player.
	if(!is_alive(&monsters[0])) {
		int x, y;
		if(!get_random_free_cell(map, monsters, &x, &y)) {
			return false;
		}
		monsters[0] = Monster(x, y, PLAYER, PLAYER_HP, PLAYER_STRENGTH, PLAYER_RESISTANCE);
	}
	// Spawn monsters.
	for(int i = 1; i < MONSTER_COUNT; ++i) {
		if(monsters[i].item)
			delete monsters[i].item;

		int x, y;
		if(!get_random_free_cell(map, monsters, &x, &y)) {
			return false;
		}
		monsters[i] = Monster(x, y, MONSTER, MONSTER_HP, MONSTER_STRENGTH, MONSTER_RESISTANCE);
	}
	return true;
}

void free_game(Cell * map, Monster * monsters) {
	for(int i = 0; i < W*H; ++i) {
		if(map[i].item) {
			delete map[i].item;
		}
	}
	for(int i = 0; i < MONSTER_COUNT; ++i) {
		if(monsters[i].item) {
			delete monsters[i].item;
		}
	}
}

void wield(Cell * map, Monster * monster) {
	Item * tmp = map[monster->x + monster->y * W].item;
	map[monster->x + monster->y * W].item = monster->item;
	monster->item = tmp;
}

void hit(Monster * off, Monster * def) {
	int damage = off->item ? (off->hit_strength + off->item->damage) : off->hit_strength;
	int resistance = def->item ? (def->resistance + def->item->protection) : def->resistance;
	if(damage > resistance) {
		def->hp -= damage - resistance;
		if(def->hp < 0) {
			def->hp = 0;
		}
	}
}

void move_monster(Cell * map, Monster * monsters, Monster * walker, int sx, int sy, int MAX_HP) {
	if(sx == 0 && sy == 0) return;

	int new_x = walker->x + sx;
	int new_y = walker->y + sy;
	if(new_x >= 0 && new_x < W && new_y >= 0 && new_y < H) {
		if(!map[new_x + new_y * W].passable) {
			return;
		}
		bool success = true;
		for(int i = 0; i < MONSTER_COUNT; ++i) {
			if(is_alive(&monsters[i]) && monsters[i].x == new_x && monsters[i].y == new_y) {
				hit(walker, &monsters[i]);
				if(!is_alive(&monsters[i])) {
					walker->hp += HP_UNIT;
					if(walker->hp > MAX_HP)
						walker->hp = MAX_HP;
				}
				success = false;
				break;
			}
		}

		if(success) {
			walker->x = new_x;
			walker->y = new_y;
		}
	}
}

void identify(Monster * monster) {
	if(monster->hp <= HP_UNIT) return;
	Item * item = monster->item;
	if(item && !item->identified) {
		item->identified = true;
		monster->hp -= HP_UNIT;
	}
}

void run() {
	int level = 0;
	Cell map[W*H];
	Monster monsters[MONSTER_COUNT];

	if(!generate(map, monsters)) {
		free_game(map, monsters);
		return;
	}

	// Loop.
	while(true) {
		// Status line
		Item * item = monsters[0].item;
		if(item) {
			if(item->identified) {
				mvprintw(H, 0, STATUS_WITH_ITEM_ID, level, monsters[0].x, monsters[0].y, monsters[0].hp, PLAYER_HP, monsters[0].hit_strength + item->damage, monsters[0].resistance + item->protection, item->sprite);
			} else {
				mvprintw(H, 0, STATUS_WITH_ITEM, level, monsters[0].x, monsters[0].y, monsters[0].hp, PLAYER_HP, monsters[0].hit_strength, monsters[0].resistance, item->sprite);
			}
		} else {
			mvprintw(H, 0, STATUS_WITHOUT_ITEM, level, monsters[0].x, monsters[0].y, monsters[0].hp, PLAYER_HP, monsters[0].hit_strength, monsters[0].resistance);
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
			if(!is_alive(&monsters[i])) continue;
			int sprite = monsters[i].sprite;
			mvaddch(monsters[i].y, monsters[i].x, sprite);
		}

		refresh();

		// TODO AI for monsters.
		// TODO saving monsters.

		// Read keys and do actions.
		int ch = getch();
		switch(ch) {
			case QUIT: free_game(map, monsters); return;
			case UP: move_monster(map, monsters, &monsters[0], 0, -1, PLAYER_HP); break;
			case DOWN: move_monster(map, monsters, &monsters[0], 0, 1, PLAYER_HP); break;
			case LEFT: move_monster(map, monsters, &monsters[0], -1, 0, PLAYER_HP); break;
			case RIGHT: move_monster(map, monsters, &monsters[0], 1, 0, PLAYER_HP); break;
			case UP_RIGHT: move_monster(map, monsters, &monsters[0], 1, -1, PLAYER_HP); break;
			case DOWN_RIGHT: move_monster(map, monsters, &monsters[0], 1, 1, PLAYER_HP); break;
			case UP_LEFT: move_monster(map, monsters, &monsters[0], -1, -1, PLAYER_HP); break;
			case DOWN_LEFT: move_monster(map, monsters, &monsters[0], -1, 1, PLAYER_HP); break;
			case WAIT: break;
			case IDENTIFY: identify(&monsters[0]); break;
			case WIELD: wield(map, &monsters[0]); break;
			case NEXT: 
				if(!generate(map, monsters)) {
					free_game(map, monsters);
					return;
				}
				++level;
				break;
			default: break;
		}
	}
}

int main() {
	init();
	run();
	close();
	return 0;
}
