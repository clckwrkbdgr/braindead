/** braindead - simple roguelike game.
 * author antifin 2011
 * version 0.9.0
 * license WTFPLv2
 */
#include <ncurses.h>
#include <cstdlib>
#include <ctime>

// Interface.
void init() {srand(time(NULL));initscr();cbreak();keypad(stdscr,true);noecho();}
void close() {echo();endwin();}
int rnd(int max) {return rand() % max;}
int getkb() {return getch();}
int put(int x, int y, int sprite) {mvaddch(y, x, sprite);}
void finish_paint() {refresh();}

// Constants.
const int W = 80;
const int H = 24;
const int MONSTERS_PER_LVL = W * H / 100;
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

// Structs.
struct Item {
	char s;
	int dmg, prt;
	Item() : s(0), dmg(0), prt(0) {}
	Item(int d, int p) : s('?'), dmg(d), prt(p) {}
};
struct Cell {
	char s;
	int item;
	Cell() : s(0), item(0) {}
	Cell(char sp, bool p) : s(sp), item(0) {}
};
struct Mon {
	int x, y;
	char s;
	int hp, maxhp, hit, res, fov, item;
	int(*ai)(Mon*);
	Mon() : x(0), y(0), s(0), hp(0), maxhp(0), hit(0), res(0), fov(0), item(0), ai(NULL) {}
	Mon(int px, int py, char sp, int hitp, int h, int r, int vision)
		: x(px), y(py), hp(hitp), maxhp(hp), s(sp), hit(h), res(r), fov(vision), item(0), ai(NULL) {}
};

// Variables, internal const.
static const char move_map[10] = "ykuh.lbjn";
int level = 0;
Cell map[W*H];
int mon_count = MONSTERS_PER_LVL + 1; // One for player.
Mon * mons = NULL;
#define PL (&mons[0])
Item items[ITEM_COUNT]; //[0] is completely ignored; is used as NULL item.
const int MODEL_COUNT = 10;
Mon models[MODEL_COUNT];

// AI, aux functions.
#define SIGN(v) (v>0?1:(v<0?-1:0))
#define ABS(a) (a<0?-a:a)
#define MAX(a,b) (a>b?a:b)
int distance(Mon * a, Mon * b) {
	int x = ABS(a->x - b->x);
	int y = ABS(a->y - b->y);
	return MAX(x,y);
}
int player_controller(Mon*) {
	return getkb();
}
int ai_follow(Mon * m) {
	return move_map[(SIGN(PL->x-m->x)+1) + (SIGN(PL->y-m->y)+1)*3];
}
int ai_hunter(Mon * m) {
	return (distance(PL, m) <= m->fov) ? ai_follow(m) : move_map[rnd(9)];
}
int ai_watcher(Mon * m) {
	return (distance(PL, m) <= m->fov) ? ai_follow(m) : '.';
}
const int AI_COUNT = 2;
int (*AI[AI_COUNT])(Mon*) = {ai_watcher, ai_hunter};

// Functions
bool passable(Cell*cell) {return cell->s==FLOOR;}
bool alive(Mon * m) { return m->hp > 0; }
bool no_item_at(int i) {
	return map[i].item == 0;
}
bool no_mon_at(int i) {
	for(int j = 0; j < mon_count; ++j) {
		if(mons[j].x + mons[j].y * W == i) {
			return false;
		}
	}
	return true;
}
int get_random_free_cell_cond(bool (*cond)(int i)) {
	for(int c = 0; c < 1000; ++c) {
		int i = rnd(W * H - 1) + 1;
		if(passable(&map[i]) && cond(i)) {
			return i;
		}
	}
	return 0;
}
int get_random_free_drop_spot() {
	get_random_free_cell_cond(no_item_at);
}
int get_random_free_cell() {
	get_random_free_cell_cond(no_mon_at);
}
void make_models() {
	models[0] = Mon(0, 0, PLAYER, PLAYER_HP, PLAYER_STRENGTH, PLAYER_RESISTANCE, PLAYER_FOV);
	models[0].ai = player_controller;
	char sprite = 'A';
	for(int i = 1; i < MODEL_COUNT && sprite <= 'Z'; ++i, ++sprite) {
		models[i] = Mon(0, 0, sprite, MONSTER_HP, MONSTER_STRENGTH, MONSTER_RESISTANCE, MONSTER_FOV);
		models[i].ai = AI[rnd(AI_COUNT)];
	}
}
bool spawn(Mon * m, bool p = false) {
	int pos = get_random_free_cell();
	if(!pos) return false;
	if(!alive(m)) {
		int i = p ? 0 : 1 + rnd(MODEL_COUNT - 1);
		*m = models[i];
	}
	m->x = pos % W;
	m->y = pos / W;
	return true;
}
void generate_and_fill_map() {
	for(int i = 0; i < W * H; ++i) {
		map[i] = rnd(FLOOR_PROB) ? Cell(FLOOR, true) : Cell(WALL, false);
	}
}
bool drop_items() {
	for(int i = 0; i < ITEM_COUNT; ++i) {
		int pos = get_random_free_drop_spot();
		if(!pos) return false;
		items[i] = Item(rnd(MAX_DAMAGE), rnd(MAX_PROTECTION));
		map[pos].item = i;
	}
	return true;
}
void make_player(Mon * new_mons) {
	if(alive(PL)) {
		PL->item = 0;
		new_mons[0] = mons[0];
	}
	spawn(&new_mons[0], true);
}
void spawn_mons(Mon * new_mons, int old_mon_count) {
	int alive_mons = 1;
	for(int i = 1; i < old_mon_count; ++i) {
		if(alive(&mons[i])) {
			new_mons[alive_mons] = mons[i];
			spawn(&new_mons[alive_mons]);
			++alive_mons;
		}
	}
	for(int i = alive_mons; i < mon_count; ++i) {
		spawn(&new_mons[i]);
	}
}
void generate_mons() {
	if(!mons) mons = new Mon[mon_count];
	int old_mon_count = mon_count;
	int alive_mons = 0;
	for(int i = 0; i < mon_count; ++i) if(alive(&mons[i])) ++alive_mons;
	mon_count = alive_mons + MONSTERS_PER_LVL;
	Mon * new_mons = new Mon[mon_count];
	make_player(new_mons);
	spawn_mons(new_mons, old_mon_count);
	delete []mons;
	mons = new_mons;
}
bool generate() {
	generate_and_fill_map();
	if(!drop_items()) {
		return false;
	}
	generate_mons();
	return true;
}
void free_game() {
	delete []mons;
	mons = NULL;
}
void wield(Mon * m) {
	int t = map[m->x + m->y * W].item;
	map[m->x + m->y * W].item = m->item;
	m->item = t;
}
int damage(Mon * m) {
	return m->item ? (m->hit + items[m->item].dmg) : m->hit;
}
int resistance(Mon * m) {
	return m->item ? (m->res + items[m->item].prt) : m->res;
}
void hit(Mon * o, Mon * d) {
	int dmg = damage(o) - resistance(d);
	if(dmg > 0) {
		d->hp -= dmg;
		if(d->hp < 0) {
			d->hp = 0;
		}
	}
}
bool battle(Mon * m, int x, int y) {
	bool success = true;
	for(int i = 0; i < mon_count; ++i) {
		if(alive(&mons[i]) && mons[i].x == x && mons[i].y == y) {
			hit(m, &mons[i]);
			if(!alive(&mons[i])) {
				m->hp += HP_UNIT;
				if(m->hp > m->maxhp)
					m->hp = m->maxhp;
			}
			success = false;
			break;
		}
	}
	return success;
}
void move(Mon * m, int sx, int sy) {
	if(sx == 0 && sy == 0) return;

	int x = m->x + sx;
	int y = m->y + sy;
	if(x >= 0 && x < W && y >= 0 && y < H) {
		if(!passable(&map[x + y * W])) {
			return;
		}
		bool success = battle(m, x, y);

		if(success) {
			m->x = x;
			m->y = y;
		}
	}
}
void identify(Mon * m) {
	if(m->hp <= HP_UNIT) return;
	if(m->item) {
		Item * i = &items[m->item];
		if(i->s == '?') {
			i->s = (i->dmg > i->prt) ? ')' : ']';
			m->hp -= HP_UNIT;
		}
	}
}
bool act(Mon * m, int ch) {
	switch(ch) {
		case 'k': move(m, 0, -1); break;
		case 'j': move(m, 0, 1); break;
		case 'h': move(m, -1, 0); break;
		case 'l': move(m, 1, 0); break;
		case 'u': move(m, 1, -1); break;
		case 'n': move(m, 1, 1); break;
		case 'y': move(m, -1, -1); break;
		case 'b': move(m, -1, 1); break;
		case '.': return true;
		case 'i': identify(m); break;
		case 'w': wield(m); break;
		case 'q': return false;
		case '>': if(!generate()){return false;} ++level; break;
		default: break;
	}
	return true;
}
void draw_status_line() {
	int item = PL->item;
	mvprintw(H, 0, "lvl:%d (%d,%d) hp:%d/%d str:%d res:%d item:%c   ",
			level, PL->x, PL->y, PL->hp, PL->maxhp,
			(item && items[item].s != '?') ? damage(PL) : PL->hit,
			(item && items[item].s != '?') ? resistance(PL) : PL->res,
			item ? items[item].s : ' ');
}
void draw_map() {
	for(int x = 0; x < W; ++x) {
		for(int y = 0; y < H; ++y) {
			int s = map[x + y * W].s;
			if(map[x + y * W].item) {
				s = items[map[x + y * W].item].s;
			}
			put(x,y,s);
		}
	}
	for(int i = 0; i < mon_count; ++i) {
		if(!alive(&mons[i])) continue;
		put(mons[i].x, mons[i].y, mons[i].s);
	}
}
bool process_game() {
	for(int i = 0; i < mon_count; ++i) {
		if(!mons[i].ai) continue;
		if(!act(&mons[i], mons[i].ai(&mons[i])))
			return false;
	}
	if(!alive(PL)) return false;
	return true;
}
void run() {
	make_models();
	if(!generate()) {
		return;
	}

	while(true) {
		draw_status_line();
		draw_map();
		finish_paint();
		if(!process_game()) {
			return;
		}
	}
}

// Main.
int main() {
	init();
	run();
	free_game();
	close();
	return 0;
}
