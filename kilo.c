#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>


// defines
#define CTRL_KEY(k) ((k) & 0x1f)

void enableRawMode();
void disableRawMode();
void die(const char *s);
char editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();
void editorDrawRows();


struct editorConfig
{
	struct termios originalTermios;	
};

struct editorConfig E; 


int main()
{
	enableRawMode();

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
char editorReadKey()
{
	int nread = 1;
	char c;
	while((nread == read(STDIN_FILENO, &c, 1)) != 1)
	{
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
}

//Processes each key press
void editorProcessKeypress()
{
	char c = editorReadKey();

	switch (c)
	{
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
  			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
	}
}

void editorRefreshScreen()
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	editorDrawRows();

	write(STDOUT_FILENO, "\x1b[H", 3);
}

void editorDrawRows()
{
	int y;
	for (y = 0; y < 24; y++)
	{
		write(STDOUT_FILENO, "~\r\n", 3);
	}
}