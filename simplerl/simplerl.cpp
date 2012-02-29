#include <ncurses.h>
#include <cstdlib>
#include <time.h>

const int W = 80;
const int H = 24;
const int MONSTER_COUNT_PER_LEVEL = 3;
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
	Monster() : x(0), y(0), sprite(0), hp(0), maxhp(0), hit(0), res(0), item(NULL) {}
	Monster(int px, int py, char s, int hitp, int h, int r) : x(px), y(py), hp(hitp), maxhp(hp), sprite(s), hit(h), res(r), item(NULL) {}
};

int level = 0;
Cell map[W*H];
int monster_count = MONSTER_COUNT_PER_LEVEL + 1; // One for player.
Monster * monsters = NULL; //[MONSTER_COUNT];

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
			for(int j = 0; j < monster_count; ++j) {
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

	if(!monsters) monsters = new Monster[monster_count];

	int old_monster_count = monster_count;
	int alive_monsters = 0;
	for(int i = 0; i < monster_count; ++i) if(alive(&monsters[i])) ++alive_monsters;
	monster_count = alive_monsters + MONSTER_COUNT_PER_LEVEL;
	Monster * new_monsters = new Monster[monster_count];

	// Make first monster to be player.
	if(alive(&monsters[0])) {
		new_monsters[0] = monsters[0];
	} else {
		int pos = get_random_free_cell();
		if(!pos) return false;
		new_monsters[0] = Monster(pos % W, pos / W, PLAYER, PLAYER_HP, PLAYER_STRENGTH, PLAYER_RESISTANCE);
	}
	// Spawn monsters.
	alive_monsters = 1;
	for(int i = 1; i < old_monster_count; ++i) {
		if(alive(&monsters[i])) {
			new_monsters[alive_monsters++] = monsters[i];
		} else {
			if(monsters[i].item)
				delete monsters[i].item;
		}
	}
	for(int i = alive_monsters; i < monster_count; ++i) {
		int pos = get_random_free_cell();
		if(!pos) return false;
		new_monsters[i] = Monster(pos % W, pos / W, MONSTER, MONSTER_HP, MONSTER_STRENGTH, MONSTER_RESISTANCE);
	}
	delete []monsters;
	monsters = new_monsters;
	return true;
}

void free_game() {
	for(int i = 0; i < W*H; ++i) {
		if(map[i].item) {
			delete map[i].item;
		}
	}
	for(int i = 0; i < monster_count; ++i) {
		if(monsters[i].item) {
			delete monsters[i].item;
		}
	}
	delete []monsters;
	monsters = NULL;
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

void move(Monster * m, int sx, int sy) {
	if(sx == 0 && sy == 0) return;

	int x = m->x + sx;
	int y = m->y + sy;
	if(x >= 0 && x < W && y >= 0 && y < H) {
		if(!map[x + y * W].passable) {
			return;
		}
		bool success = true;
		for(int i = 0; i < monster_count; ++i) {
			if(alive(&monsters[i]) && monsters[i].x == x && monsters[i].y == y) {
				hit(m, &monsters[i]);
				if(!alive(&monsters[i])) {
					m->hp += HP_UNIT;
					if(m->hp > m->maxhp)
						m->hp = m->maxhp;
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

bool act(Monster * monster, int ch) {
	switch(ch) {
		case 'k': move(monster, 0, -1); break;
		case 'j': move(monster, 0, 1); break;
		case 'h': move(monster, -1, 0); break;
		case 'l': move(monster, 1, 0); break;
		case 'u': move(monster, 1, -1); break;
		case 'n': move(monster, 1, 1); break;
		case 'y': move(monster, -1, -1); break;
		case 'b': move(monster, -1, 1); break;
		case '.': return true;
		case 'i': identify(monster); break;
		case 'w': wield(monster); break;

		case 'q': free_game(); return false;
		case '>': 
			if(!generate()) {
				free_game();
				return false;
			}
			++level;
			break;
		default: break;
	}
	return true;
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
		mvprintw(H, 0, "lvl:%d (%d,%d) hp:%d/%d str:%d res:%d item:%c%c   ",
				level, monsters[0].x, monsters[0].y, monsters[0].hp, monsters[0].maxhp,
				(item && item->id) ? damage(&monsters[0]) : monsters[0].hit,
				(item && item->id) ? resistance(&monsters[0]) : monsters[0].res,
				item ? item->sprite : ' ', (item && item->id) ? ' ' : '?');

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
		for(int i = 0; i < monster_count; ++i) {
			if(!alive(&monsters[i])) continue;
			mvaddch(monsters[i].y, monsters[i].x, monsters[i].sprite);
		}

		refresh();

		// TODO AI for monsters.

		// Read keys and do actions.
		int ch = getch();
		if(!act(&monsters[0], ch))
			return;
	}
}

int main() {
	init();
	run();
	close();
	return 0;
}
