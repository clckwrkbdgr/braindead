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
	int dmg, prt;
	bool id;
	Item() : sprite(0), dmg(0), prt(0), id(false) {}
	Item(char s, int d, int p) : sprite(s), dmg(d), prt(p),
		id(false) {}
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
	int hp, maxhp;
	int hit;
	int res;
	Item * item;
	Monster() : x(0), y(0), sprite(0), hp(0), hit(0), res(0), item(NULL) {}
	Monster(int px, int py, char s, int hitp, int h, int r) : x(px), y(py), hp(hitp), sprite(s), hit(h), res(r), item(NULL) {}
};

int level = 0;
Cell map[W*H];
Monster monsters[MONSTER_COUNT];

bool alive(Monster * m) { return m->hp > 0; }

int get_random_free_drop_spot() {
	for(int c = 0; c < 1000; ++c) {
		int i = rand() % (W * H - 1) + 1;
		if(map[i].passable && map[i].item == NULL) {
			return i;
		}
	}
	return 0;
}

int get_random_free_cell() {
	for(int c = 0; c < 1000; ++c) {
		int i = rand() % (W * H - 1) + 1;
		if(map[i].passable) {
			bool success = true;
			for(int j = 0; j < MONSTER_COUNT; ++j) {
				if(monsters[j].x + monsters[j].y * W == i) {
					success = false;
					break;
				}
			}
			if(success) return i;
		}
	}
	return 0;
}

bool generate() {
	// Generate and fill map.
	for(int i = 0; i < W * H; ++i) {
		if(map[i].item) delete map[i].item;
		map[i] = (rand() % FLOOR_PROB) ? Cell(FLOOR, true) : Cell(WALL, false);
	}
	// Drop items.
	for(int i = 0; i < ITEM_COUNT; ++i) {
		int pos = get_random_free_drop_spot();
		if(!pos) return false;
		map[pos].item = new Item(ITEM_SPRITE, rand() % MAX_DAMAGE, rand() % MAX_PROTECTION);
	}
	// Make first monster to be player.
	if(!alive(&monsters[0])) {
		int pos = get_random_free_cell();
		if(!pos) return false;
		monsters[0] = Monster(pos % W, pos / W, PLAYER, PLAYER_HP, PLAYER_STRENGTH, PLAYER_RESISTANCE);
	}
	// Spawn monsters.
	for(int i = 1; i < MONSTER_COUNT; ++i) {
		if(monsters[i].item)
			delete monsters[i].item;

		int pos = get_random_free_cell();
		if(!pos) return false;
		monsters[i] = Monster(pos % W, pos / W, MONSTER, MONSTER_HP, MONSTER_STRENGTH, MONSTER_RESISTANCE);
	}
	return true;
}

void free_game() {
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

void wield(Monster * m) {
	Item * t = map[m->x + m->y * W].item;
	map[m->x + m->y * W].item = m->item;
	m->item = t;
}

int damage(Monster * m) {
	return m->item ? (m->hit + m->item->dmg) : m->hit;
}

int resistance(Monster * m) {
	return m->item ? (m->res + m->item->prt) : m->res;
}

void hit(Monster * o, Monster * d) {
	int dmg = damage(o) - resistance(d);
	if(dmg > 0) {
		d->hp -= dmg;
		if(d->hp < 0) {
			d->hp = 0;
		}
	}
}

void move(Monster * m, int sx, int sy, int MAX_HP) {
	if(sx == 0 && sy == 0) return;

	int x = m->x + sx;
	int y = m->y + sy;
	if(x >= 0 && x < W && y >= 0 && y < H) {
		if(!map[x + y * W].passable) {
			return;
		}
		bool success = true;
		for(int i = 0; i < MONSTER_COUNT; ++i) {
			if(alive(&monsters[i]) && monsters[i].x == x && monsters[i].y == y) {
				hit(m, &monsters[i]);
				if(!alive(&monsters[i])) {
					m->hp += HP_UNIT;
					if(m->hp > MAX_HP)
						m->hp = MAX_HP;
				}
				success = false;
				break;
			}
		}

		if(success) {
			m->x = x;
			m->y = y;
		}
	}
}

void identify(Monster * m) {
	if(m->hp <= HP_UNIT) return;
	Item * i = m->item;
	if(i && !i->id) {
		i->id = true;
		m->hp -= HP_UNIT;
	}
}

void run() {
	if(!generate()) {
		free_game();
		return;
	}

	// Loop.
	while(true) {
		// Status line
		Item * item = monsters[0].item;
		if(item) {
			if(item->id) {
				mvprintw(H, 0, STATUS_WITH_ITEM_ID, level, monsters[0].x, monsters[0].y, monsters[0].hp, PLAYER_HP, damage(&monsters[0]), resistance(&monsters[0]), item->sprite);
			} else {
				mvprintw(H, 0, STATUS_WITH_ITEM, level, monsters[0].x, monsters[0].y, monsters[0].hp, PLAYER_HP, monsters[0].hit, monsters[0].res, item->sprite);
			}
		} else {
			mvprintw(H, 0, STATUS_WITHOUT_ITEM, level, monsters[0].x, monsters[0].y, monsters[0].hp, PLAYER_HP, monsters[0].hit, monsters[0].res);
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
			if(!alive(&monsters[i])) continue;
			mvaddch(monsters[i].y, monsters[i].x, monsters[i].sprite);
		}

		refresh();

		// TODO AI for monsters.
		// TODO saving monsters.

		// Read keys and do actions.
		int ch = getch();
		switch(ch) {
			case QUIT: free_game(); return;
			case UP: move(&monsters[0], 0, -1, PLAYER_HP); break;
			case DOWN: move(&monsters[0], 0, 1, PLAYER_HP); break;
			case LEFT: move(&monsters[0], -1, 0, PLAYER_HP); break;
			case RIGHT: move(&monsters[0], 1, 0, PLAYER_HP); break;
			case UP_RIGHT: move(&monsters[0], 1, -1, PLAYER_HP); break;
			case DOWN_RIGHT: move(&monsters[0], 1, 1, PLAYER_HP); break;
			case UP_LEFT: move(&monsters[0], -1, -1, PLAYER_HP); break;
			case DOWN_LEFT: move(&monsters[0], -1, 1, PLAYER_HP); break;
			case WAIT: break;
			case IDENTIFY: identify(&monsters[0]); break;
			case WIELD: wield(&monsters[0]); break;
			case NEXT: 
				if(!generate()) {
					free_game();
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
