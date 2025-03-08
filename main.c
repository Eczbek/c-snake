#define _POSIX_C_SOURCE 199309

#include <ctype.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

typedef struct {
	size_t x;
	size_t y;
} pos_t;

constexpr pos_t game_size = { 20, 20 };
constexpr size_t snake_max = game_size.x * game_size.y;

pos_t rand_pos() {
	return (pos_t) { (size_t)rand() % game_size.x, (size_t)rand() % game_size.y };
}

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color_t;

constexpr color_t red = { 255, 0, 0 };
constexpr color_t green = { 0, 255, 0 };
constexpr color_t azure = { 0, 127, 255 };

void set_color_at(color_t color, pos_t pos) {
	printf("\x1B[%zu;%zuH\x1B[48;2;%u;%u;%um  ", pos.y + 2, pos.x * 2 + 1, color.r, color.g, color.b);
}

void sleep_ms(int ms) {
	if (ms > 0) {
		struct timespec time;
		time.tv_sec = ms / 1000;
		time.tv_nsec = ms % 1000 * 1000000;
		nanosleep(&time, nullptr);
	}
}

bool run() {
	srand((unsigned int)time(nullptr));

	pos_t apple = rand_pos();

	pos_t snake_body[snake_max];
	pos_t snake_head = rand_pos();
	size_t snake_start = 0;
	size_t snake_end = 0;
	size_t score = 0;
	pos_t snake_direction = { 0, 0 };

	while (true) {
		printf("\x1B[0m\x1B[HScore: %zu", score);
		if (score >= snake_max) {
			return true;
		}

		set_color_at(green, snake_head);
		snake_head = (pos_t) { (snake_head.x + snake_direction.x) % game_size.x, (snake_head.y + snake_direction.y) % game_size.y };
		snake_body[snake_start = (snake_start + 1) % snake_max] = snake_head;
		if ((snake_head.x == apple.x) && (snake_head.y == apple.y)) {
			++score;
			apple = rand_pos();
		} else {
			const pos_t snake_tail = snake_body[snake_end++];
			snake_end %= snake_max;
			set_color_at(azure, snake_tail);
		}
		set_color_at(red, apple);
		set_color_at(green, snake_head);

		for (size_t i = 0; i < score; ++i) {
			const pos_t snake_part = snake_body[(snake_end + i) % snake_max];
			if ((snake_head.x == snake_part.x) && (snake_head.y == snake_part.y)) {
				return false;
			}
		}

		fflush(stdout);
		sleep_ms(100);

		const bool can_turn_x = !snake_direction.x || (score < 1);
		const bool can_turn_y = !snake_direction.y || (score < 1);
		while (true) {
			const int input = getchar();
			if (toupper(input) == 'Q') {
				return false;
			}
			if (input < 1) {
				break;
			}
			if ((input == '\x1B') && (getchar() == '[')) {
				switch (getchar()) {
				case 'A':
					if (can_turn_y) {
						snake_direction = (pos_t) { 0, game_size.y - 1 };
					}
					break;
				case 'B':
					if (can_turn_y) {
						snake_direction = (pos_t) { 0, 1 };
					}
					break;
				case 'C':
					if (can_turn_x) {
						snake_direction = (pos_t) { 1, 0 };
					}
					break;
				case 'D':
					if (can_turn_x) {
						snake_direction = (pos_t) { game_size.x - 1, 0 };
					}
				}
			}
		}
	}
}

int main() {
	struct termios cooked_mode;
	tcgetattr(STDIN_FILENO, &cooked_mode);
	struct termios raw_mode = cooked_mode;
	raw_mode.c_iflag &= (tcflag_t)~(ICRNL | IXON);
	raw_mode.c_lflag &= (tcflag_t)~(ICANON | ECHO | IEXTEN | ISIG);
	raw_mode.c_oflag &= (tcflag_t)~(OPOST);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw_mode);
	const int block_mode = fcntl(STDIN_FILENO, F_GETFL);
	fcntl(STDIN_FILENO, F_SETFL, block_mode | O_NONBLOCK);
	fputs("\x1B[s\x1B[?47h\x1B[?25l\x1B[2J", stdout);

	for (size_t x = 0; x < game_size.x; ++x) {
		for (size_t y = 0; y < game_size.y; ++y) {
			set_color_at(azure, (pos_t) { x, y });
		}
	}
	printf("\x1B[0m\x1B[%zuHUse arrow keys to move, press Q to quit", game_size.y + 2);

	const bool win = run();

	printf("\x1B[0m\x1B[%zuH\x1B[2K", game_size.y + 2);
	if (win) {
		fputs("You win! ", stdout);
	}
	fputs("Press any key to exit", stdout);
	fflush(stdout);
	sleep_ms(500);
	while (getchar() > 0);
	fcntl(STDIN_FILENO, F_SETFL, block_mode);
	getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &cooked_mode);
	fputs("\x1B[?25h\x1B[?47l\x1B[u", stdout);
}
