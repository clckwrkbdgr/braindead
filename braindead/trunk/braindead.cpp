/** braindead - simple roguelike game.
 * author antifin 2011
 * version 0.9.0
 * license WTFPLv2
 */
#include <ncurses.h>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>

const int W = 80;
const int H = 24;
const int MONSTER_COUNT_PER_LEVEL = W * H / 100;
const int ITEM_COUNT = W * H / 100;
const int FLOOR_PROB = 10;
const int MAX_DAMAGE = 10;
const int MAX_PROTECTION = 10;

const int PLAYER_STRENGTH = 3;
const int PLAYER_RESISTANCE = 1;
const int PLAYER_HP = 100;
const int PLAYER_FOV = 10;
const int MONSTER_STRENGTH = 3;
const int MONSTER_RESISTANCE = 1;
const int MONSTER_HP = 20;
const int MONSTER_FOV = 5;
const int HP_UNIT = 10;

const char FLOOR = '.';
const char WALL = '#';
const char PLAYER = '@';

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
	Item() : sprite(0), dmg(0), prt(0) {}
	Item(int d, int p) : sprite('?'), dmg(d), prt(p) {}
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
	int fov;
	Item * item;
	int(*ai)(Monster*);
	Monster() : x(0), y(0), sprite(0), hp(0), maxhp(0), hit(0), res(0), fov(0), item(NULL), ai(NULL) {}
	Monster(int px, int py, char s, int hitp, int h, int r, int vision)
		: x(px), y(py), hp(hitp), maxhp(hp), sprite(s), hit(h), res(r), fov(vision), item(NULL), ai(NULL) {}
};

int distance(Monster * a, Monster * b) {
	return std::max(abs(a->x - b->x), abs(a->y - b->y));
}

int sign(int v) {
	return v > 0 ? 1 : (v < 0 ? -1 : 0);
}

static const char move_map[10] = "ykuh.lbjn";
int level = 0;
Cell map[W*H];
int monster_count = MONSTER_COUNT_PER_LEVEL + 1; // One for player.
Monster * monsters = NULL;
#define PL (&monsters[0])

int player_controller(Monster*) {
	return getch();
}

int ai_hunter(Monster * m) {
	if(distance(PL, m) <= m->fov) {
		int sx = sign(PL->x - m->x);
		int sy = sign(PL->y - m->y);
		return move_map[(sx+1) + (sy+1)*3];
	}
	return move_map[rand() % 9];
}

int ai_watcher(Monster * m) {
	if(distance(PL, m) <= m->fov) {
		int sx = sign(PL->x - m->x);
		int sy = sign(PL->y - m->y);
		return move_map[(sx+1) + (sy+1)*3];
	}
	return '.';
}

const int AI_COUNT = 2;
int (*AI[AI_COUNT])(Monster*) = {ai_watcher, ai_hunter};

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

const int MODEL_COUNT = 10;
Monster models[MODEL_COUNT];

void make_models() {
	models[0] = Monster(0, 0, PLAYER, PLAYER_HP, PLAYER_STRENGTH, PLAYER_RESISTANCE, PLAYER_FOV);
	models[0].ai = player_controller;
	char sprite = 'A';
	for(int i = 1; i < MODEL_COUNT && sprite <= 'Z'; ++i, ++sprite) {
		models[i] = Monster(0, 0, sprite, MONSTER_HP, MONSTER_STRENGTH, MONSTER_RESISTANCE, MONSTER_FOV);
		models[i].ai = AI[rand() % AI_COUNT];
	}
}

bool spawn(Monster * m, bool p = false) {
	int pos = get_random_free_cell();
	if(!pos) return false;
	if(!alive(m)) {
		int i = p ? 0 : 1 + rand() % (MODEL_COUNT - 1);
		*m = models[i];
	}
	m->x = pos % W;
	m->y = pos / W;
	return true;
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
		map[pos].item = new Item(rand() % MAX_DAMAGE, rand() % MAX_PROTECTION);
	}

	if(!monsters) monsters = new Monster[monster_count];

	int old_monster_count = monster_count;
	int alive_monsters = 0;
	for(int i = 0; i < monster_count; ++i) if(alive(&monsters[i])) ++alive_monsters;
	monster_count = alive_monsters + MONSTER_COUNT_PER_LEVEL;
	Monster * new_monsters = new Monster[monster_count];

	// Make first monster to be PL.
	if(alive(PL)) {
		new_monsters[0] = monsters[0];
	}
	spawn(&new_monsters[0], true);

	// Spawn monsters.
	alive_monsters = 1;
	for(int i = 1; i < old_monster_count; ++i) {
		if(alive(&monsters[i])) {
			new_monsters[alive_monsters] = monsters[i];
			spawn(&new_monsters[alive_monsters]);
			++alive_monsters;
		} else {
			if(monsters[i].item)
				delete monsters[i].item;
		}
	}
	for(int i = alive_monsters; i < monster_count; ++i) {
		spawn(&new_monsters[i]);
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
	if(i && i->sprite == '?') {
		i->sprite = (i->dmg > i->prt) ? ')' : ']';
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

		case 'q': return false;
		case '>': 
			if(!generate()) {
				return false;
			}
			++level;
			break;
		default: break;
	}
	return true;
}

void run() {
	make_models();
	if(!generate()) {
		return;
	}

	// Loop.
	while(true) {
		// Status line
		Item * item = PL->item;
		mvprintw(H, 0, "lvl:%d (%d,%d) hp:%d/%d str:%d res:%d item:%c   ",
				level, PL->x, PL->y, PL->hp, PL->maxhp,
				(item && item->sprite != '?') ? damage(PL) : PL->hit,
				(item && item->sprite != '?') ? resistance(PL) : PL->res,
				item ? item->sprite : ' ');

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

		// Read keys and do actions.
		for(int i = 0; i < monster_count; ++i) {
			if(!monsters[i].ai) continue;
			if(!act(&monsters[i], monsters[i].ai(&monsters[i])))
				return;
		}
	}
}

int main() {
	init();
	run();
	free_game();
	close();
	return 0;
}
