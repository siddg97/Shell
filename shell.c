// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10
char history[HISTORY_DEPTH][COMMAND_LENGTH];
int commands = 0;
int ctrlPressed = 0;

/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */

int tokenize_command(char *buff, char *tokens[])
{
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}

	tokens[token_count] = NULL;
	return token_count;
}
/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */
void read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;
	strcpy(buff, "");
	// Read input
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

	if ( (length < 0) && (errno !=EINTR) ) {
		perror("Unable to read command from keyboard. Terminating.\n");
		exit(-1);
	}
	// Null terminate and strip \n.
	buff[length] = '\0';

	if (buff[strlen(buff) - 1] == '\n')
	{
		buff[strlen(buff) - 1] = '\0';
	}

	if ( strcmp(buff, "!!") == 0 )
	{
		if(commands > 0 && commands <= 10)
		{
			strcpy(buff, history[commands - 1]);
			write(STDOUT_FILENO, history[commands - 1], strlen(history[commands - 1]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		else if ( commands == 0 )
		{
			write(STDOUT_FILENO, "SHELL: Unknown history command.", strlen("SHELL: Unknown history command."));
			write(STDOUT_FILENO, "\n", strlen("\n"));
			strcpy(buff, "");
		}
		else
		{
			strcpy(buff, history[HISTORY_DEPTH - 1]);
			write(STDOUT_FILENO, history[HISTORY_DEPTH - 1], strlen(history[HISTORY_DEPTH - 1]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
	}
	else if ( buff[0] == '!' )
	{

			int hist = atoi(&buff[1]);

			if ( hist > commands || hist == 0)
			{
				write(STDOUT_FILENO, "SHELL: Unknown history command.", strlen("SHELL: Unknown history command."));
				write(STDOUT_FILENO, "\n", strlen("\n"));
				return;
			}
			else if ( hist > 10 && hist <= commands && hist > (commands - 10))
			{
					strcpy(buff, history[hist - commands + HISTORY_DEPTH - 1]);
					write(STDOUT_FILENO, history[hist - commands + HISTORY_DEPTH - 1], strlen(history[hist - commands + HISTORY_DEPTH - 1]) );
					write(STDOUT_FILENO, "\n", strlen("\n"));
			}
			else if ( hist <= 10 )
			{
					strcpy(buff, history[hist - 1]);
					write(STDOUT_FILENO, history[hist - 1], strlen(history[hist - 1]) );
					write(STDOUT_FILENO, "\n", strlen("\n"));
			}

	}

	if (commands < 10)
		strcpy(history[commands % 10], buff); // HISTORY
	else if(ctrlPressed != 1)								//
	{
		for(int i = 0; i < (HISTORY_DEPTH - 1) ; i++)
		{
			strcpy(history[i], history[i + 1]);
		}
		strcpy(history[HISTORY_DEPTH - 1], buff);
	}

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0)
	{
		return;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
		tokens[token_count - 1] = 0;
	}

}

void printHistory()
{
	int i = 1, j = 0;
	if ( commands > 10)
	 i = commands - (HISTORY_DEPTH - 1) ;

	for( ; i <= commands; i++ )
	{
			char jinstr[10];
			sprintf(jinstr, "%d", i);
			write(STDOUT_FILENO, jinstr, strlen(jinstr));
			write(STDOUT_FILENO, "\t", strlen("\t"));
			write(STDOUT_FILENO, history[j], strlen(history[j]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
			j++;
	}
}

void handle_SIGINT()
{
		write(STDOUT_FILENO, "\n", strlen("\n"));
		ctrlPressed = 1;
    printHistory();
}

/**
 * Main and Execute Commands
 */
int main(int argc, char* argv[])
{
	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS];
	char pwd[256];
	pid_t pid = 0;
	getcwd(pwd, sizeof(pwd));

	struct sigaction handler;
	handler.sa_handler = handle_SIGINT;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	sigaction(SIGINT, &handler, NULL);

	while (true) {

		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
		 write(STDOUT_FILENO, pwd, strlen(pwd));
		 write(STDOUT_FILENO, "> ", strlen("> "));
		_Bool in_background = false;
		read_command(input_buffer, tokens, &in_background);

		if (tokens[0] != NULL)
		{
			commands++;
		}
		// Cleanup any previously exited background child processes
		// (The zombies)
		while (waitpid(-1, NULL, WNOHANG) > 0)
			; // do nothing.

		if(tokens[0] == NULL) continue; // If no command is entered, ask for another command
		else if ( strcmp(tokens[0], "exit") == 0 )	exit(0); // Exit the shell
		else if ( strcmp(tokens[0], "pwd") == 0 ) // Prints the working directory.
		{
			write(STDOUT_FILENO, pwd, strlen(pwd));
    	write(STDOUT_FILENO, "\n", strlen("\n"));
			continue;
		}
		else if(strcmp(tokens[0], "cd") == 0)
		{
			int changeSuccess = chdir(tokens[1]);
			if( changeSuccess != 0 )
			{
				write(STDOUT_FILENO, "Invalid directory", sizeof("Invalid directory"));
				write(STDOUT_FILENO, "\n", strlen("\n"));
			}
		else{
			getcwd(pwd, sizeof(pwd));
		}

					continue;
			}
			else if( strcmp(tokens[0], "history") == 0 )
			{
				printHistory();
				continue;
			}
/*
			if (in_background)
			{
				write(STDOUT_FILENO, "Run in background.", strlen("Run in background."));
			}
*/
		/**
		 * Steps For Basic Shell:
		 * 1. Fork a child process
		 * 2. Child process invokes execvp() using results in token array.
		 * 3. If in_background is false, parent waits for
		 *    child to finish. Otherwise, parent loops back to
		 *    read_command() again immediately.
		 */

		 pid = fork();
		 if ( pid < 0 )
		 {
			 write(STDOUT_FILENO, "FORK failed to create child process.\n", strlen("FORK failed to create child process.\n"));
			 exit(1);
		 }
		 else	if ( pid == 0 )
		 {
			 // Child Process
			 int isCommand = execvp(tokens[0], tokens);
			 if ( isCommand == -1 )
			 {
				 write(STDOUT_FILENO, tokens[0], strlen(tokens[0]));
				 write(STDOUT_FILENO, ": Unknown command.", strlen(": Unknown command."));
				 write(STDOUT_FILENO, "\n", strlen("\n"));
			 }
			 exit(1);
		 }
		 else
		 {
			 // Parent Process
			 if ( in_background == false )
				 while(waitpid(pid, NULL, 0) > 0)
					 ;
		 }

	}
	return 0;
}
