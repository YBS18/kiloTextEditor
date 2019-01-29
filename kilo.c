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

struct termios originalTermios;

int main()
{
	enableRawMode();

	while (1)
	{
		char c = '\0';
		//read terminal for input
		if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
		//check if control char
		if (iscntrl(c))
		{
			printf("%d\r\n", c);
		}
		else
		{
			printf("%d ('%c')\r\n", c, c);
		}
		if (c == CTRL_KEY('q')) break;
	}

	return 0;
}

// Makes terminal Raw mode instead of canonical
void enableRawMode()
{
	//read terminal attributes to strcut raw	
	if (tcgetattr(STDIN_FILENO, &originalTermios) == -1) die("tcgetattr");
	atexit(disableRawMode);
	
	struct termios raw = originalTermios;
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
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios) == -1)
		die("tcsetattr");
}

//error producing function
void die(const char *s)
{
	perror(s);
	exit(1);
}