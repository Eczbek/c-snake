#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

// Declare `nanosleep` to avoid implicit declaration warning
int nanosleep(const struct timespec*, struct timespec*);

struct Position {
	int x;
	int y;
};

struct Color {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

int getRandom(const int min, const int max) {
	return rand() % (max - min) - min;
}

void sleepMilliseconds(const int milliseconds) {
	if (milliseconds > 0) {
		struct timespec time;
		time.tv_sec = milliseconds / 1000;
		time.tv_nsec = milliseconds % 1000 * 1000000;
		nanosleep(&time, NULL);
	}
}

char readCharacter() {
	char input;
	return (read(STDIN_FILENO, &input, 1) > 0) ? input : 0;
}

int main() {
	srand(time(NULL));

	const struct Position gameSize = {
		20,
		20
	};

	struct Position apple = {
		getRandom(0, gameSize.x),
		getRandom(0, gameSize.y)
	};

	int bodySize = 1;
	struct Position body[gameSize.x * gameSize.y];
	body[0].x = getRandom(0, gameSize.x);
	body[0].y = getRandom(0, gameSize.y);

	struct Position currentDirection = {
		0,
		0
	};

	const struct Color red = {
		255,
		0,
		0
	};
	const struct Color lime = {
		127,
		255,
		0
	};
	const struct Color green = {
		0,
		255,
		0
	};
	const struct Color azure = {
		0,
		127,
		255
	};
	struct Color canvas[gameSize.x][gameSize.y];
	for (int x = 0; x < gameSize.x; ++x) {
		for (int y = 0; y < gameSize.y; ++y) {
			canvas[x][y] = azure;
		}
	}

	struct termios cooked;
	tcgetattr(STDIN_FILENO, &cooked);
	struct termios raw = cooked;
	raw.c_iflag &= ~(ICRNL | IXON);
	raw.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG);
	raw.c_oflag &= ~(OPOST);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	const int blocking = fcntl(STDIN_FILENO, F_GETFL);
	fcntl(STDIN_FILENO, F_SETFL, blocking | O_NONBLOCK);
	printf("\x1b[?47h\x1b[?25l");

	bool gameOver = false;
	while (!gameOver) {
		const struct Position head = {
			(body[0].x + currentDirection.x + gameSize.x) % gameSize.x,
			(body[0].y + currentDirection.y + gameSize.y) % gameSize.y
		};

		if ((head.x == apple.x) && (head.y == apple.y)) {
			canvas[apple.x][apple.y] = azure;
			apple.x = getRandom(0, gameSize.x);
			apple.y = getRandom(0, gameSize.y);
			++bodySize;
		} else {
			canvas[body[bodySize - 1].x][body[bodySize - 1].y] = azure;
		}

		for (int i = bodySize - 1; i > 0; --i) {
			const struct Position part = body[i] = body[i - 1];
			if ((part.x == head.x) && (part.y == head.y)) {
				gameOver = true;
				break;
			}
			canvas[part.x][part.y] = lime;
		}
		if (gameOver) {
			break;
		}
		body[0] = head;
		canvas[head.x][head.y] = green;
		canvas[apple.x][apple.y] = red;

		printf("\x1b[HScore: %i\n\r", bodySize - 1);
		for (int y = gameSize.y; y--;) {
			for (int x = 0; x < gameSize.x; ++x) {
				printf("\x1b[48;2;%u;%u;%um  ", canvas[x][y].red, canvas[x][y].green, canvas[x][y].blue);
			}
			printf("\x1b[0m\n\r");
		}
		printf("Use arrow keys to move, press q to quit");
		fflush(stdout);
		if (bodySize == (gameSize.x * gameSize.y)) {
			break;
		}

		sleepMilliseconds(100);
		struct Position newDirection = currentDirection;
		while (true) {
			const char input = readCharacter();
			if ((char)tolower((unsigned char)input) == 'q') {
				gameOver = true;
			}
			if (!input || gameOver) {
				break;
			}
			if ((input == '\x1b') && (readCharacter() == '[')) {
				switch (readCharacter()) {
					case 'A':
						if (!currentDirection.y || (bodySize < 2)) {
							newDirection.x = 0;
							newDirection.y = 1;
						}
						break;
					case 'B':
						if (!currentDirection.y || (bodySize < 2)) {
							newDirection.x = 0;
							newDirection.y = -1;
						}
						break;
					case 'C':
						if (!currentDirection.x || (bodySize < 2)) {
							newDirection.x = 1;
							newDirection.y = 0;
						}
						break;
					case 'D':
						if (!currentDirection.x || (bodySize < 2)) {
							newDirection.x = -1;
							newDirection.y = 0;
						}
				}
			}
		}
		currentDirection = newDirection;
	}

	printf("\x1b[2K\x1b[0G");
	if (!gameOver) {
		printf("You win! ");
	}
	printf("Press any key to exit");
	fflush(stdout);

	sleepMilliseconds(1000);
	while (readCharacter());
	fcntl(STDIN_FILENO, F_SETFL, blocking);
	fgetc(stdin);

	tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
	printf("\x1b[?25h\x1b[?47l");
}
