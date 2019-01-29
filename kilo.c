#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>


void enableRawMode();
void disableRawMode();

struct termios originalTermios;

int main()
{
	enableRawMode();

	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
	{
		if (iscntrl(c))
		{
			printf("%d\n", c);
		}
		else
		{
			printf("%d ('%c')\n",c ,c);
		}
	}
	return 0;
}

// Makes terminal Raw mode instead of canonical
void enableRawMode()
{
	//read terminal attributes to strcut raw	
	tcgetattr(STDIN_FILENO, &originalTermios);
	atexit(disableRawMode);
	
	struct termios raw = originalTermios;
	// turns off echo and switches off canonical mode
	raw.c_lflag &= ~(ECHO | ICANON);
	
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}

//disables terminal Raw mode
void disableRawMode()
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
}