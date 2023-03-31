#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

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

void sleepMilliseconds(int milliseconds) {
	struct timespec time = {
		milliseconds / 1000,
		milliseconds % 1000 * 1000000
	};
	nanosleep(&time, NULL);
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

	struct Position head;

	struct Position currentDirection = {
		0,
		0
	};
	struct Position newDirection;

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

	const int inputSize = 1024;
	char input[inputSize];

	bool running = true;

	struct termios cooked;
	tcgetattr(STDIN_FILENO, &cooked);
	struct termios raw = cooked;
	cfmakeraw(&raw);
	raw.c_lflag &= ~(ICANON);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	int blocking = fcntl(STDIN_FILENO, F_GETFL);
	fcntl(STDIN_FILENO, F_SETFL, blocking | O_NONBLOCK);
	printf("\x1b[?47h\x1b[?25l");

	while (running) {
		head.x = (body[0].x + currentDirection.x + gameSize.x) % gameSize.x;
		head.y = (body[0].y + currentDirection.y + gameSize.y) % gameSize.y;

		if ((head.x == apple.x) && (head.y == apple.y)) {
			apple.x = getRandom(0, gameSize.x);
			apple.y = getRandom(0, gameSize.y);
		} else
			--bodySize;

		for (int i = 3; i < bodySize; ++i)
			if ((head.x == body[i].x) && (head.y == body[i].y)) {
				running = false;
				break;
			}

		if (!running)
			break;

		for (int i = bodySize; i > 0; --i)
			body[i] = body[i - 1];
		body[0] = head;
		++bodySize;

		for (int x = 0; x < gameSize.x; ++x)
			for (int y = 0; y < gameSize.y; ++y)
				canvas[x][y] = azure;
		for (int i = 1; i < bodySize; ++i)
			canvas[body[i].x][body[i].y] = lime;
		canvas[body[0].x][body[0].y] = green;
		canvas[apple.x][apple.y] = red;

		printf("\x1b[2J\x1b[HScore: %i\n\r", bodySize - 1);
		for (int y = gameSize.y; y--;) {
			for (int x = 0; x < gameSize.x; ++x)
				printf("\x1b[48;2;%u;%u;%um  ", canvas[x][y].red, canvas[x][y].green, canvas[x][y].blue);
			printf("\x1b[0m\n\r");
		}
		printf("Use arrow keys to move, press q to quit");
		fflush(stdout);

		sleepMilliseconds(100);

		newDirection = currentDirection;
		int readCount = 0;
		do
			if (read(STDIN_FILENO, &input[readCount], 1) == -1)
				break;
		while ((readCount < inputSize) && input[readCount++]);
		for (int i = 0; i < readCount; ++i) {
			switch (input[i]) {
				case 'q':
					running = false;
					break;
				case '\x1b':
					if ((i < readCount - 2) && (input[++i] == '['))
						switch (input[++i]) {
							case 'A':
								if (!currentDirection.y || (bodySize == 1)) {
									newDirection.x = 0;
									newDirection.y = 1;
								}
								break;
							case 'B':
								if (!currentDirection.y || (bodySize == 1)) {
									newDirection.x = 0;
									newDirection.y = -1;
								}
								break;
							case 'C':
								if (!currentDirection.x || (bodySize == 1)) {
									newDirection.x = 1;
									newDirection.y = 0;
								}
								break;
							case 'D':
								if (!currentDirection.x || (bodySize == 1)) {
									newDirection.x = -1;
									newDirection.y = 0;
								}
								break;
						}
			}
		}
		currentDirection = newDirection;
	}

	printf("\x1b[2K\x1b[0GPress any key to exit");
	fflush(stdout);
	sleepMilliseconds(1000);
	char character = 0;
	do
		if (read(STDIN_FILENO, &character, 1) == -1)
			break;
	while (character);
	fcntl(STDIN_FILENO, F_SETFL, blocking);
	fgetc(stdin);

	tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
	printf("\x1b[?25h\x1b[?47l");

	return 0;
}
