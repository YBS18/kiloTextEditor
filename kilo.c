#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>


// defines
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}
#define KILO_VERSION "0.0.1"

struct editorConfig
{
	int cx, cy;
	struct termios originalTermios;
	int screenRows;
	int screenCols;	
};

struct editorConfig E; 

struct abuf {
	char *b;
	int len;
};

enum editorKey
{
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN
};



void enableRawMode();
void disableRawMode();
void die(const char *s);
int editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();
void editorDrawRows(struct abuf *ab);
int getWindowSize(int *rows, int *cols);
void initEditor();
int getCursorPosition(int *rows, int *cols);
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);
void editorMoveCursor(int key);






int main()
{
	enableRawMode();
	initEditor();

	while (1)
	{
		//clears screen
		editorRefreshScreen();

		editorProcessKeypress();
	}

	return 0;
}

// Makes terminal Raw mode instead of canonical
void enableRawMode()
{
	//read terminal attributes to strcut raw	
	if (tcgetattr(STDIN_FILENO, &E.originalTermios) == -1) die("tcgetattr");
	atexit(disableRawMode);
	
	struct termios raw = E.originalTermios;
	// turns off signals for raw mode
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	raw.c_cflag |= (CS8);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	//read timeout values
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr"); 
}

//disables terminal Raw mode
void disableRawMode()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.originalTermios) == -1)
		die("tcsetattr");
}

//error producing function
void die(const char *s)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
  	write(STDOUT_FILENO, "\x1b[H", 3);


	perror(s);
	exit(1);
}

//reads input from terminal
int editorReadKey()
{
	int nread = 1;
	char c;
	while((nread == read(STDIN_FILENO, &c, 1)) != 1)
	{
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	
	if (c == "\x1b")
	{
		char seq[3];

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		if (seq[0] == '[')
		{
			switch(seq[1])
			{
				case 'A' : return ARROW_UP;
				case 'B' : return ARROW_DOWN;
				case 'C' : return ARROW_RIGHT;
				case 'D' : return ARROW_LEFT;
			}
		}

		return '\x1b';
	}
	else
	{
		return c;
	}

}

//Processes each key press
void editorProcessKeypress()
{
	int c = editorReadKey();

	switch (c)
	{
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
  			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;

		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_LEFT:
		case ARROW_RIGHT:
			editorMoveCursor(c);
			break;
	}
}

//clears screen
void editorRefreshScreen()
{
	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);

	editorDrawRows(&ab);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
	abAppend(&ab, buf, strlen(buf));

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

//draws tildas to terminal based on size
void editorDrawRows(struct abuf *ab)
{
	int y;
	for (y = 0; y < E.screenRows; y++)
	{
		if (y == E.screenRows / 3)
		{
			char welcome[80];
			int welcomelen = snprintf(welcome, sizeof(welcome), 
				"Kilo editor -- version %s", KILO_VERSION);
			if (welcomelen > E.screenCols) welcomelen = E.screenCols;
			int padding = (E.screenCols - welcomelen) / 2;
			if (padding)
			{
				abAppend(ab, "~", 1);
				padding--;
			}
			while (padding--) abAppend(ab, " ", 1);
			abAppend(ab, welcome, welcomelen);
		}
		else
		{
			abAppend(ab, "~", 1);
		}

		abAppend(ab, "\x1b[K", 3);
		if (y < E.screenRows - 1)
		{
			abAppend(ab, "\r\n", 2);
		}
	}
}

//gets the terminal size
int getWindowSize(int *rows, int *cols)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
		//sends cursor right down if ioctl cant get the size of terminal
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
		{
			return -1;
		}

		return getCursorPosition(rows, cols);
	}
	else
	{
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

// initializes struct editor
void initEditor()
{
	E.cx = 0;
	E.cy = 0;

	if (getWindowSize(&E.screenRows, &E.screenCols) == -1) die("getWindowSize");

}

int getCursorPosition(int *rows, int *cols)
{
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		return -1;

	while (i < sizeof(buf) - 1)
	{
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

	return 0;
}

void abAppend(struct abuf *ab, const char *s, int len)
{
	char *new = realloc(ab->b, ab->len + len);

	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab)
{
	free(ab->b);
}

void editorMoveCursor(int key)
{
	switch (key)
	{
		case ARROW_LEFT:
			if (E.cx != 0)
			{
				E.cx--;
			}
			break;
		case ARROW_RIGHT:
			if (E.cx != E.screenCols - 1)
			{
				E.cx++;
			}
			break;
		case ARROW_UP:
			if (E.cy != 0)
			{
				E.cy--;
			}
			break;
		case ARROW_DOWN:
			if (E.cy != E.screenRows - 1)
			{
				E.cy++;
			}
			break;
	}
}