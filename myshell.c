#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

/*
define all macros
*/
#define WAIT_PID_VALID_ERROR(error_num) (errno == ECHILD || errno == EINTR)
#define AMP_SYMBL "&" // End-of-line command, case: 1
#define PIPE "|" // Pipe symbol, case: 2
#define STIN_SYMBL "<" // Start-of-line command, case: 3
#define STOUT_SYMBL ">" // Start-of-line command, case: 4

/* 
find the index of the pipe symbol ('|') in the arglist
*/
int get_pipe_index(int count, char** arglist) {
	
	int idx = 1;
	while (idx < count-1) {
		if (strcmp(arglist[idx],PIPE) == 0) {
			return idx;
		}
		idx++;
	}
	return -1; // not possible to reach here by problem definition
}

/*
signal handler for SIGCHLD signal
we use this signal to handle child processes that have terminated
*/
void sigchld_handler(int signal_num) {
	/* While you have one or mor children processes alive that are finished and ready to be killed,
	kill them on the spot without waiting for other children that aren't done yet by using WNOHANG flag
	WNOHANG makes waitpid non-blocking, meaning it won't wait for a child to terminate if none has yet */
	pid_t pid;
	while ((pid = waitpid(-1,NULL, WNOHANG)) > 0); // waitpid kills child process
	if ((pid == -1) && !WAIT_PID_VALID_ERROR(errno)) {
			perror("waitpid failed in sigchld handler");
			exit(1);
		}
}

/*
helper function to determine which one of the 5 cases are we:
0: regular execute command 
1: run command in background
2: pipe the stdout of 1st cmd to the stdin of the 2nd cmd 
3: stdin of cmd is redirected from 2nd file
4: stdout of cmd is redirected to 2nd file
*/
int return_case(int count, char** arglist) {

	int idx = 1;
	int cmdcase = 0; // 0: execute 1: &, 2: |, 3: <, 4: >

    if (count == 1) { // takes care of 1 worded commands such as 'ls' to prevent index errors
        return cmdcase;
    }

	else if (strcmp(arglist[count-1],AMP_SYMBL) == 0) {
		cmdcase = 1;
		return cmdcase;
	}
	else if (strcmp(arglist[count-2],STIN_SYMBL) == 0) {
		cmdcase = 3;
		return cmdcase;
	}
	else if (strcmp(arglist[count-2],STOUT_SYMBL) == 0) {
		cmdcase = 4;
		return cmdcase;
	}

	while (idx < count-1) {
		if (strcmp(arglist[idx],PIPE) == 0) {
			cmdcase = 2;
			break;
		}
		idx++;
	}

	return cmdcase;
}

/*
function to execute a command:
	1. fork a child process
	2. update signal handler for child process
	3. execute the command using execvp
	4. wait for child process to finish
*/
int execute(int count, char** arglist) {
	
	pid_t pid;
	
	// fork a child proccess
	pid = fork();
	if (pid < 0) {
		perror("fork failed in execute function");
		return 0;
	}

	else if (pid == 0) { // child process
		// update signal handler for child process
		if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
			perror("signal failed in execute function");
			exit(1);
		}
		// execute the command
		if (execvp(arglist[0], arglist) == -1) {
			perror("execvp failed in execute function");
			exit(1);
		}
	}
	
	else { // parent process
		// wait for child process to finish
		if ((waitpid(pid,NULL,0) == -1) && !WAIT_PID_VALID_ERROR(errno)) {
			perror("waitpid failed in execute function");
			return 0;
		}
	}
	
	return 1;
}

/*
function to execute a command in the background:
	1. fork a child process
	2. update signal handler for child process
	3. execute the command using execvp
	4. parent process doesn't wait for child process to finish
	5. the child process will be reaped by the signal handler - sigchld_handler
*/
int execute_amp(int count, char** arglist) {
	
	pid_t pid;
	arglist[count-1] = NULL; // update the last argument to NULL
	
	// fork a child process
	pid = fork();
	if (pid < 0) {
		perror("fork failed in execute_amp function");
		return 0;
	}

	else if (pid == 0) { // child process
		// update signal handler for child process
		if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
			perror("signal failed in execute_amp function");
			exit(1);
		}
		// execute the command
		if (execvp(arglist[0], arglist) == -1) {
			perror("execvp failed in execute_amp function");
			exit(1);
		}
	}
	
	// parent process doesn't wait for child process to finish
	
	return 1;
}

/*
function to execute a command with stdin redirection:
	1. fork a child process
	2. update signal handler for child process
	3. open file for reading only
	4. redirect our file to stdin (stdin fd is 0)
	5. execute the command using execvp
	6. wait for child process to finish
*/
int execute_stin(int count, char** arglist) {
	
	pid_t pid;
	int fd;

	// fork a child process
    pid = fork();
    if (pid < 0) {
        perror("fork failed in execute_stin function");
        return 0;
    }

    else if (pid == 0) { // child process
		// update signal handler for child process
		if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
			perror("signal failed in execute_stin function");
			exit(1);
		}
		// open file for reading only
		fd = open(arglist[count-1], O_RDONLY);
		if (fd == -1) {
			perror("openfile failed in execute_stin function");
			exit(1);
		}
        // redirect our file to stdin (stdin fd is 0)
        if (dup2(fd, 0) == -1) {
            perror("dup2 failed in execute_stin function");
            close(fd);
            exit(1);
        }
        close(fd); // done with our file in the child process

		arglist[count-2] = NULL; // update the last argument to NULL

		// execute the command
        if (execvp(arglist[0], arglist) == -1) {
            perror("execvp failed in execute_stin function");
            exit(1);
        }
    }

	else { // parent process
		// wait for child process to finish
		if ((waitpid(pid,NULL,0) == -1) && !WAIT_PID_VALID_ERROR(errno)) {
			perror("waitpid failed in execute_stin function");
			return 0;
		}
	}

	return 1;
}

/*
function to execute a command with stdout redirection:
	1. fork a child process
	2. update signal handler for child process
	3. open file for writing only
	4. redirect our file to stdout (stdout fd is 1)
	5. execute the command using execvp
	6. wait for child process to finish
*/
int execute_stout(int count, char** arglist) {
	
	pid_t pid;
	int fd;

	// fork a child process
	pid = fork();
	if (pid < 0) {
		perror("fork failed in execute_stout function");
		return 0;
	}

	else if (pid == 0) { // child process
		// update signal handler for child process
		if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
			perror("signal failed in execute_stout function");
			exit(1);
		}
		/* open file for writing only
		O_WRONLY: write only, O_CREAT: create file if it doesn't exist, O_TRUNC: clear file if it exists */
		fd = open(arglist[count-1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
		if (fd == -1) {
			perror("openfile failed in execute_stout function");
			exit(1);
		}
		// redirect our file to stdout (stdout fd is 1)
		if (dup2(fd, 1) == -1) {
			perror("dup2 failed in execute_stout function");
			close(fd);
			exit(1);
		}
		close(fd); // done with our file in the child process

		arglist[count-2] = NULL; // update the last argument to NULL

		// execute the command
		if (execvp(arglist[0], arglist) == -1) {
			perror("execvp failed in execute_stout function");
			exit(1);
		}
	}

	else { // parent process
		// wait for child process to finish
		if ((waitpid(pid,NULL,0) == -1) && !WAIT_PID_VALID_ERROR(errno)) {
			perror("waitpid failed in execute_stout function");
			return 0;
		}
	}

	return 1;
}

/*
function to execute a command with pipe:
	1. find the index of the pipe symbol ('|') in the arglist
	2. NULLify the pipe symbol for abstraction of 2 arglists without allocating new memory
	3. set up pipe
	4. fork first child process
	5. update signal handler for child process
	6. redirect stdout to the write end of the pipe
	7. execute the first command using execvp
	8. fork second child process
	9. update signal handler for child process
	10. redirect stdin to the read end of the pipe
	11. execute the second command using execvp
	12. close both ends of the pipe
	13. wait for both child processes to finish
*/
int execute_pipe(int count, char** arglist) {
	
	int i = get_pipe_index(count, arglist);
	pid_t pid1, pid2;
	int flag = 1;

	// will 'NULLify' the pipe symbol for abstraction of 2 agrlists without allocating new memory
	arglist[i] = NULL;

	// set up pipe
	int pipefd[2]; // pipefd[0] is the read end, pipefd[1] is the write end
	if (pipe(pipefd) == -1) {
		perror("pipe failed in execute_pipe function");
		return 0;
	}

	// fork first child process
	pid1 = fork();
	if (pid1 < 0) {
		perror("fork failed in execute_pipe function");
		return 0;
	}

	else if (pid1 == 0) { // child process
		// update signal handler for child process
		if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
			perror("signal failed in execute_pipe function");
			exit(1);
		}
		// close the read end of the pipe
		close(pipefd[0]);

		// redirect stdout to the write end of the pipe
		if (dup2(pipefd[1], 1) == -1) {
			perror("dup2 failed in execute_pipe function");
			close(pipefd[1]);
			exit(1);
		}
		close(pipefd[1]); // done with the write end of the pipe

		// execute the first command
		if (execvp(arglist[0], arglist) == -1) {
			perror("execvp failed in execute_pipe function");
			exit(1);
		}
	}

	// else parent process continues to fork the second child process

	// fork second child process
	pid2 = fork();
	if (pid2 < 0) {
		perror("fork failed in execute_pipe function");
		return 0;
	}

	else if (pid2 == 0) { // child process
		// update signal handler for child process
		if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
			perror("signal failed in execute_pipe function");
			exit(1);
		}
		// close the write end of the pipe
		close(pipefd[1]);

		// redirect stdin to the read end of the pipe
		if (dup2(pipefd[0], 0) == -1) {
			perror("dup2 failed in execute_pipe function");
			close(pipefd[0]);
			exit(1);
		}
		close(pipefd[0]); // done with the read end of the pipe

		// execute the second command
		if (execvp(arglist[i+1], &arglist[i+1]) == -1) {
			perror("execvp failed in execute_pipe function");
			exit(1);
		}
	}

	else { // parent process
		// close both ends of the pipe
		close(pipefd[0]);
		close(pipefd[1]);

		// wait for both child processes to finish
		if ((waitpid(pid1,NULL,0) == -1) && !WAIT_PID_VALID_ERROR(errno)) {
			perror("waitpid failed");
			flag = 0;
		}
		if ((waitpid(pid2,NULL,0) == -1) && !WAIT_PID_VALID_ERROR(errno)) {
			perror("waitpid failed");
			return 0;
		}
		// making sure both children had a chance to terminate
		if (flag == 0) {
			return 0;
		}
	}

	return 1;
}

/*
arglist - a list of char* arguments (words) provided by the user
it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
RETURNS - 1 if should continue, 0 otherwise
*/
int process_arglist(int count, char** arglist){

	int ret_val;
	int cmdcase = return_case(count, arglist);

	switch (cmdcase)
	{
	case 0:
		ret_val = execute(count, arglist);
		break;

	case 1:
		ret_val = execute_amp(count, arglist);
		break;

	case 2:
		ret_val = execute_pipe(count, arglist);
		break;

	case 3:
		ret_val = execute_stin(count, arglist);
		break;

	case 4:
		ret_val = execute_stout(count, arglist);
		break;
	
	default:
		break;
	}
	
	return ret_val;
}

/*
prep the shell for execution by configuring the signal handler for SIGCHLD signal to handle child processes that have terminated
in addition, ignore the SIGINT signal (Ctrl+C) as default for the parent process
*/
int prepare(void){

	// Struct for handling SIGCHLD signal
	struct sigaction sa_child;
	sa_child.sa_handler = sigchld_handler;
	sa_child.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	int valid = sigaction(SIGCHLD, &sa_child, NULL); // configure signal handler to the SIGCHLD signal (child process terminated)
	if (valid == -1) {
		perror("sigaction failed in prepare");
		return -1;
	}

	if (signal(SIGINT, SIG_IGN) == SIG_ERR) { // ignore SIGINT signal (Ctrl+C) as default for parent process
		perror("signal failed in prepare");
		return -1;
	}

    return 0;
}

/*
returns 0 if the shell is successfully finalized
*/
int finalize(void){
	return 0;
}

/*
Help online:
	1. GPT conversations:
		1.1. finding the correct includes for signals and files: https://chatgpt.com/c/67613d8a-bc8c-8000-bd2e-844a66ed9692
	2. learning about file open flags and modes: https://stackoverflow.com/questions/53807679/whats-the-connection-between-flags-and-mode-in-open-file-function-in-c
	3. learning about difference between signal and sigaction: https://stackoverflow.com/questions/231912/what-is-the-difference-between-sigaction-and-signal
	4. learning about exit() function: https://www.scaler.com/topics/exit-function-in-c/
*/
