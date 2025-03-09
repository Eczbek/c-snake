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

void sleep_ms(int ms) {
	nanosleep(&((struct timespec) { ms / 1000, ms % 1000 * 1'000'000 }), nullptr);
}

typedef struct {
	int x;
	int y;
} pos_t;

constexpr pos_t game_size = { 20, 20 };

pos_t rand_pos() {
	return (pos_t) { rand() % game_size.x, rand() % game_size.y };
}

void set_color_at(pos_t pos, uint_least32_t color) {
	printf("\x1B[%i;%iH\x1B[48;2;%u;%u;%um  ", pos.y + 2, pos.x * 2 + 1, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}

bool run() {
	srand((unsigned int)time(nullptr));

	pos_t apple = rand_pos();

	pos_t snake_head = rand_pos();
	pos_t snake_direction = { 0, 0 };

	static constexpr size_t snake_size = (size_t)(game_size.x * game_size.y);
	pos_t snake_body[snake_size] = { snake_head };
	size_t snake_start = 0;
	size_t snake_end = 0;
	size_t score = 0;

	while (true) {
		printf("\x1B[0m\x1B[HScore: %zu", score);
		if (score >= snake_size) {
			return true;
		}

		set_color_at(snake_head, 0x00FF00);
		snake_head = (pos_t) {
			(snake_head.x + snake_direction.x + game_size.x) % game_size.x,
			(snake_head.y + snake_direction.y + game_size.y) % game_size.y
		};
		snake_start = (snake_start + 1) % snake_size;
		snake_body[snake_start] = snake_head;
		if ((snake_head.x == apple.x) && (snake_head.y == apple.y)) {
			++score;
			apple = rand_pos();
		} else {
			set_color_at(snake_body[snake_end], 0x007FFF);
			snake_end = (snake_end + 1) % snake_size;
		}
		set_color_at(apple, 0xFF0000);
		set_color_at(snake_head, 0x00FF00);

		for (size_t i = 0; i < score; ++i) {
			const pos_t snake_part = snake_body[(snake_end + i) % snake_size];
			if ((snake_head.x == snake_part.x) && (snake_head.y == snake_part.y)) {
				return false;
			}
		}

		fflush(stdout);
		sleep_ms(100);

		const bool can_move_x = !snake_direction.x || !score;
		const bool can_move_y = !snake_direction.y || !score;
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
				case 'C':
					if (can_move_x) {
						snake_direction = (pos_t) { 1, 0 };
					}
					break;
				case 'D':
					if (can_move_x) {
						snake_direction = (pos_t) { -1, 0 };
					}
					break;
				case 'B':
					if (can_move_y) {
						snake_direction = (pos_t) { 0, 1 };
					}
					break;
				case 'A':
					if (can_move_y) {
						snake_direction = (pos_t) { 0, -1 };
					}
					break;
				}
			}
		}
	}
}

int main() {
	struct termios cooked_mode;
	tcgetattr(STDIN_FILENO, &cooked_mode);
	struct termios raw_mode = cooked_mode;
	raw_mode.c_iflag &= ~(tcflag_t)(ICRNL | IXON);
	raw_mode.c_lflag &= ~(tcflag_t)(ICANON | ECHO | IEXTEN | ISIG);
	raw_mode.c_oflag &= ~(tcflag_t)(OPOST);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw_mode);
	const int block_mode = fcntl(STDIN_FILENO, F_GETFL);
	fcntl(STDIN_FILENO, F_SETFL, block_mode | O_NONBLOCK);
	fputs("\x1B[s\x1B[?47h\x1B[?25l\x1B[2J", stdout);

	for (int x = 0; x < game_size.x; ++x) {
		for (int y = 0; y < game_size.y; ++y) {
			set_color_at((pos_t) { x, y }, 0x007FFF);
		}
	}
	printf("\x1B[0m\x1B[%iHUse arrow keys to move, press Q to quit", game_size.y + 2);

	const bool win = run();

	printf("\x1B[0m\x1B[%iH\x1B[2K", game_size.y + 2);
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
