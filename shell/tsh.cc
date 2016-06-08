// 
// tsh - A tiny shell program with job control
// 
// Nikhil Gaud
// login id - ngaud
//

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string>

#include "globals.h"
#include "jobs.h"
#include "helper-routines.h"

//
// Needed global variable definitions
//

static char prompt[] = "tsh> ";
int verbose = 0;

//
// You need to implement the functions eval, builtin_cmd, do_bgfg,
// waitfg, sigchld_handler, sigstp_handler, sigint_handler
//
// The code below provides the "prototypes" for those functions
// so that earlier code can refer to them. You need to fill in the
// function bodies below.
// 

void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);


sigset_t maskchld;
//
// main - The shell's main routine 
//
int main(int argc, char **argv) 
{
  int emit_prompt = 1; // emit prompt (default)

  //
  // Redirect stderr to stdout (so that driver will get all output
  // on the pipe connected to stdout)
  //
  dup2(1, 2);
  sigemptyset(&maskchld);
  sigaddset(&maskchld, SIGCHLD);
  sigaddset(&maskchld, SIGTSTP);

  /* Parse the command line */
  char c;
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
    case 'h':             // print help message
      usage();
      break;
    case 'v':             // emit additional diagnostic info
      verbose = 1;
      break;
    case 'p':             // don't print a prompt
      emit_prompt = 0;  // handy for automatic testing
      break;
    default:
      usage();
    }
  }

  //
  // Install the signal handlers
  //

  //
  // These are the ones you will need to implement
  //
  Signal(SIGINT,  sigint_handler);   // ctrl-c
  Signal(SIGTSTP, sigtstp_handler);  // ctrl-z
  Signal(SIGCHLD, sigchld_handler);  // Terminated or stopped child

  //
  // This one provides a clean way to kill the shell
  //
  Signal(SIGQUIT, sigquit_handler); 

  //
  // Initialize the job list
  //
  initjobs(jobs);

  //
  // Execute the shell's read/eval loop
  //
  for(;;) {
    //
    // Read command line
    //
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }

    char cmdline[MAXLINE];

    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) {
      app_error("fgets error");
    }
    //
    // End of file? (did user type ctrl-d?)
    //
    if (feof(stdin)) {
      fflush(stdout);
      exit(0);
    }

    //
    // Evaluate command line
    //
    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
  } 

  exit(0); //control never reaches here
}
  
/////////////////////////////////////////////////////////////////////////////
//
// eval - Evaluate the command line that the user has just typed in
// 
// If the user has requested a built-in command (quit, jobs, bg or fg)
// then execute it immediately. Otherwise, fork a child process and
// run the job in the context of the child. If the job is running in
// the foreground, wait for it to terminate and then return.  Note:
// each child process must have a unique process group ID so that our
// background children don't receive SIGINT (SIGTSTP) from the kernel
// when we type ctrl-c (ctrl-z) at the keyboard.
//
void eval(char *cmdline) 
{
  /* Parse command line */
  //
  // The 'argv' vector is filled in by the parseline
  // routine below. It provides the arguments needed
  // for the execve() routine, which you'll need to
  // use below to launch a process.
  //
  char *argv[MAXARGS];

  //
  // The 'bg' variable is TRUE if the job should run
  // in background mode or FALSE if it should run in FG
  //
  int bg = parseline(cmdline, argv); 
  struct job_t* tempJob=NULL;
  if (argv[0] == NULL)  
    return;
  int processId;
//check whether it is a built-in command else fork
  if(builtin_cmd(argv))
	{
		return;
	}
	else
	{	  //block the signal before fork
		sigprocmask(SIG_BLOCK, &maskchld, NULL);
		processId = fork();
		if(processId == 0)
			{
				//unblock the signal once child process is created
				sigprocmask(SIG_UNBLOCK, &maskchld, NULL);
				setpgid(0, 0);
				if(execve(argv[0], argv, NULL) < 0)
				{
					printf("%s: Command not found\n", argv[0]);
					exit(EXIT_SUCCESS);
				}
				return;
			}
			else
			{
				//need to add to the job list before unblocking the signal
				addjob(jobs, processId, bg?BG:FG, cmdline);
				if(bg == 1)
				{
					tempJob = getjobpid(jobs, processId);
					printf("[%d] (%d) ",tempJob->jid,tempJob->pid);
					printf("%s",tempJob->cmdline);
					//unblock the signal once job added
					sigprocmask(SIG_UNBLOCK, &maskchld, NULL);
				}
				else
				{
					//unblock the signal once job added
					sigprocmask(SIG_UNBLOCK, &maskchld, NULL);
					//keep waiting until done
					waitfg(processId);
					return;
				}
		}
	}
  return;
}


/////////////////////////////////////////////////////////////////////////////
//
// builtin_cmd - If the user has typed a built-in command then execute
// it immediately. The command name would be in argv[0] and
// is a C string. We've cast this to a C++ string type to simplify
// string comparisons; however, the do_bgfg routine will need 
// to use the argv array as well to look for a job number.
//
int builtin_cmd(char **argv) 
{
  string cmd(argv[0]);
  if(cmd == "quit")
  {
	  exit(EXIT_SUCCESS);
  }
  else if(cmd == "jobs")
  {
	listjobs(jobs);
	return 1;
  }
  else if(cmd == "bg" || cmd == "fg")
  {
	  /*if(argv[1]==NULL)
	  {
		printf("%s command requires PID or %%jobid argument",argv[0]);
		return 0;
	  }*/
	  //call do_bgfg
	  do_bgfg(argv);
	  return 1;
  }
  return 0;     /* not a builtin command */
}

/////////////////////////////////////////////////////////////////////////////
//
// do_bgfg - Execute the builtin bg and fg commands
//
void do_bgfg(char **argv) 
{
  struct job_t *jobp=NULL;
    
  /* Ignore command if no argument */
  if (argv[1] == NULL) {
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }
    
  /* Parse the required PID or %JID arg */
  if (isdigit(argv[1][0])) {
    pid_t pid = atoi(argv[1]);
    if (!(jobp = getjobpid(jobs, pid))) {
      printf("(%d): No such process\n", pid);
      return;
    }
  }
  else if (argv[1][0] == '%') {
    int jid = atoi(&argv[1][1]);
    if (!(jobp = getjobjid(jobs, jid))) {
      printf("%s: No such job\n", argv[1]);
      return;
    }
  }
  else {
    printf("%s: argument must be a PID or %%jobid\n", argv[0]);
    return;
  }

  //
  // You need to complete rest. At this point,
  // the variable 'jobp' is the job pointer
  // for the job ID specified as an argument.
  //
  // Your actions will depend on the specified command
  // so we've converted argv[0] to a string (cmd) for
  // your benefit.
  //
  string cmd(argv[0]);
  //Added for changing the state of the process to FG
  if((cmd == "fg") && ((jobp->state == BG) || (jobp->state == ST)))
  {
	//send the continue signal
	kill(-(jobp->pid), SIGCONT);
	//change the state to FG
	jobp->state = FG;
	//keep waiting until done
	waitfg(jobp->pid);
	return;
  }
  //Added for changing the state of the process to BG
  if((cmd == "bg") && (jobp->state == ST))
  {
	//send the continue signal
  	kill(-(jobp->pid), SIGCONT);
	//change the state to BG
	jobp->state = BG;
	printf("[%d] (%d) ", jobp->jid, jobp->pid);
	printf("%s", jobp->cmdline);
	///////waitfg(jobp->pid);
	return;
  }
  //////
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// waitfg - Block until process pid is no longer the foreground process
//
void waitfg(pid_t pid)
{
	int statusFlag;
	while(waitpid(pid,&statusFlag, WUNTRACED) == 0)
	{
		//do nothing, keep waiting
	}

	if(WIFEXITED(statusFlag))
	{
		//delete job if the process exited with success
		deletejob(jobs, pid);
	}
	else if(WIFSIGNALED(statusFlag))
	{
		if(WTERMSIG(statusFlag) == 2)
		{
			printf("Job [%d] (%d) terminated by signal %d\n",pid2jid(pid) ,pid ,WTERMSIG(statusFlag));
		}
		//delete job if the process was terminated by a signal
		deletejob(jobs, pid);
	}
	else if(WIFSTOPPED(statusFlag))
	{
		//update the process state if stopped
		getjobpid(jobs,pid)->state = ST;
		printf("Job [%d] (%d) stopped by signal %d\n",pid2jid(pid) ,pid ,WSTOPSIG(statusFlag));
	}
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// Signal handlers
//


/////////////////////////////////////////////////////////////////////////////
//
// sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
//     a child job terminates (becomes a zombie), or stops because it
//     received a SIGSTOP or SIGTSTP signal. The handler reaps all
//     available zombie children, but doesn't wait for any other
//     currently running children to terminate.  
//
void sigchld_handler(int sig) 
{
	pid_t processId;
	int statusFlag;
	while ((processId = waitpid(-1, &statusFlag, WUNTRACED|WNOHANG)) > 0) 
	{ 
		//do nothing, keep waiting
		//deletejob(jobs, processId);
	}
	return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigint_handler - The kernel sends a SIGINT to the shell whenver the
//    user types ctrl-c at the keyboard.  Catch it and send it along
//    to the foreground job.  
//
void sigint_handler(int sig) 
{
	/*struct job_t* procToKill=NULL;
	struct job_t* tempVar=NULL;*/
	//retrieve the process id of the foreground job and send the signal
	pid_t fg_pid;
	fg_pid = fgpid(jobs);
	if(fg_pid > 0)
	{
		kill(-fg_pid, sig);
	}
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
//     the user types ctrl-z at the keyboard. Catch it and suspend the
//     foreground job by sending it a SIGTSTP.  
//
void sigtstp_handler(int sig) 
{
	//retrieve the process id of the foreground job and send the signal
	pid_t fg_pid;
	fg_pid = fgpid(jobs);
	if(fg_pid > 0)
	{
		kill(-fg_pid, sig);
	}
  return;
}

/*********************
 * End signal handlers
 *********************/


