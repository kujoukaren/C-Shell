//Li, Mengqi: 92059150
//Kim, IL: 36806039

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define	MAXLINE	 8192
#define MAXARGS 128

typedef void handler_t(int);

char **environ;

void Pause();
char *Fgets(char *ptr, int n, FILE *stream);
void eval(char *cmdline);
int builtin_command(char **argv);
int parseline(char *buf, char **argv);
pid_t Fork(void);
void unix_error(char *msg);
void error_msg(char *msg);
void sigchldHandler(int sig);
handler_t *Signal(int signum, handler_t *handler);
pid_t Waitpid(pid_t pid, int *iptr, int options);

int main() {
    char cmdline[MAXLINE]; 
	
	/* Register the Signal handler to reap child processes in the foreground and the background */
	Signal(SIGCHLD, sigchldHandler);
	
    while (1) {
		/* read */
		printf("prompt> ");                   
		Fgets(cmdline, MAXLINE, stdin); 
		if (feof(stdin)) 
		{
			exit(0);
		}
		
		/* evaluate */
		eval(cmdline);
			
	}
     
}

void eval(char *cmdline) {
    char *argv[MAXARGS]; /* argv for execve() */
    int bg;              /* should the job run in bg or fg? */
    pid_t pid;           /* process id */
	
    bg = parseline(cmdline, argv); 
	
	if (argv[0] == NULL) {
        return;   /* Ignore empty lines */
    }
	
    if (!builtin_command(argv)) 
	{ 
		if ((pid = Fork()) == 0) 
		{   /* child runs user job */
			if (execve(argv[0], argv, environ) < 0) 
			{
				printf("%s: Command not found.\n", argv[0]);
				exit(0);
			}
		}

		/* parent waits for fg job to terminate */
		if (!bg) 
		{   
			/* Block this process until sigchldHandler returns */
			Pause();
		}
		/* otherwise, don’t wait for bg job */
		else
		{
			printf("%d %s\n", pid, cmdline);
			/* Reap the child processes in the background */
		}
	}
}

int builtin_command(char **argv) 
{
    /* command for quit shell */
	if (!strcmp(argv[0], "quit")) 
	{
		exit(0); 
	}
	else if (!strcmp(argv[0], "exit")) 
	{
		exit(0); 
	}
    
	/* Ignore singleton & */
	if (!strcmp(argv[0], "&")) 
	{
		return 1;
	}
	/* Not a builtin command */
    return 0;
}

int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    
	while (*buf && (*buf == ' ')) 
	{ 
		buf++;                 /* Ignore leading spaces */
	}
	
    /* Build the argv list */
    argc = 0;
	
    while ((delim = strchr(buf, ' '))) 
	{
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) 
		{
            buf++;            /* Ignore spaces */
		}
    }
	
    argv[argc] = NULL;
    
    if (argc == 0) 
	{
		return 1;      /* Ignore blank line */
	}
	
    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) 
	{
		argv[--argc] = NULL;
	}
    return bg;
}

char *Fgets(char *ptr, int n, FILE *stream) 
{
    char *rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
	{
		error_msg("Fgets error");
	}
    return rptr;
}

void error_msg(char *msg) /* return error message */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}

void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void sigchldHandler(int sig) 
{
    pid_t pid;

    while((pid = Waitpid(-1, NULL, 0)) > 0) 
	{
        switch (errno) 
		{
            case EINTR:
                continue;

            case ECHILD:
                continue;
        }
        return;
    }
}

void Pause() 
{
    (void)pause();
    return;
}

pid_t Waitpid(pid_t pid, int *iptr, int options)
{
    pid_t retpid;

    retpid  = waitpid(pid, iptr, options);
    if (retpid < 0 && errno != ECHILD)
	{
        unix_error("Waitpid error");
    }
	return(retpid);
}

handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0) 
	{
        unix_error("Signal error");
    }
    /* Returns the un-overidden signal action */
    return (old_action.sa_handler);
}