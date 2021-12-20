#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <errno.h>
#include <elf.h>

#define NUM_CHLD_SYSCALLS 10

#define PARENT_TRACE 0
#define CHILD_TRACE 1
#define HELPER_TRACE 2

extern char **environ;

int interp_status(int status)
{
	int rc;

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 0) {
			rc = 0;
		} else {
			rc = -WEXITSTATUS(status);
		}
	} else {
		rc = -ECONNABORTED;	/* overload to mean child signal */
	}

	return rc;
}

#ifdef PTRACE_GETREGSET
#  if defined(__x86_64__) || defined(__i386__)
#    define ARCH_REGS_STRUCT struct user_regs_struct
#  elif defined(__aarch64__)
#    if (__GLIBC__ > 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 20))
#      define ARCH_REGS_STRUCT struct user_regs_struct
#    else
#      define ARCH_REGS_STRUCT struct user_pt_regs
#    endif
#  elif defined(__arm__) || defined(__powerpc__) || defined(__powerpc64__)
#    define ARCH_REGS_STRUCT struct pt_regs
#  elif defined(__s390__) || defined(__s390x__)
#    define ARCH_REGS_STRUCT struct _user_regs_struct
#  else
#    error "Need to define ARCH_REGS_STRUCT for this architecture"
#  endif

int read_ptrace_registers(pid_t pid)
{
	ARCH_REGS_STRUCT regs;
	struct iovec iov;

	iov.iov_base = &regs;
	iov.iov_len = sizeof(regs);

	memset(&regs, 0, sizeof(regs));
	if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1) {
		perror("FAIL:  parent ptrace(PTRACE_GETREGS) failed - ");
		return errno;
	}

	return 0;
}
#else /* ! PTRACE_GETREGSET so use PTRACE_GETREGS instead */
int read_ptrace_registers(pid_t pid)
{
	struct user regs;

	memset(&regs, 0, sizeof(regs));
	if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
		perror("FAIL:  parent ptrace(PTRACE_GETREGS) failed - ");
		return errno;
	}

	return 0;
}
#endif


/* return 0 on success.  Child failure -errorno, parent failure errno */
int do_parent(pid_t pid, int trace, int num_syscall)
{
	int status, i;
	unsigned int rc;

	/* child is paused */
	rc = alarm(5);
	if (rc != 0) {
		fprintf(stderr, "FAIL: unexpected alarm already set\n");
		return errno;
	}

//fprintf(stderr, "waiting ... ");
	if (waitpid(pid, &status, WUNTRACED) == -1)
		return errno;
	if (!WIFSTOPPED(status))
		return interp_status(status);
//fprintf(stderr, " done initial wait\n");
	if (trace) {
		/* this sends a child SIGSTOP */
		if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
			perror("FAIL: parent ptrace(PTRACE_ATTACH) failed - ");
			return errno;
		}
	} else {
		/* continue child so it can attach to parent */
		kill(pid, SIGCONT);
//fprintf(stderr, "waiting2 ... ");
	if (waitpid(pid, &status, WUNTRACED) == -1)
		return errno;
//fprintf(stderr, " done\n");

	if (!WIFSTOPPED(status))
		return interp_status(status);

	}

	for (i = 0; i < num_syscall * 2; i++){
		/* this will restart stopped child */
		if (ptrace(PTRACE_SYSCALL, pid, NULL, 0) == -1) {
			perror("FAIL: parent ptrace(PTRACE_SINGLESTEP) failed - ");
			return errno;
		}

//fprintf(stderr, "waiting3 ... ");
		if (waitpid(pid, &status, WUNTRACED) == -1)
			return errno;
//fprintf(stderr, " done\n");

		if (!WIFSTOPPED(status))
			return interp_status(status);
	
		rc = read_ptrace_registers(pid);
		if (rc != 0)
			return rc;
	}

	if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1) {
		perror("FAIL:  parent ptrace(PTRACE_DETACH) failed - ");
		return errno;
	}
	return 0;
}

/* returns 0 on success or error code of failure */
int do_child(char *argv[], int child_trace, int helper)
{
	if (helper) {
		/* for helper we want to transition before ptrace occurs
		 * so don't stop here, let the helper do that
		 */
		if (child_trace) {
			 putenv("_tracer=child");
		} else {
			 putenv("_tracer=parent");
		}
//fprintf(stderr, "child trace %d\n", child_trace);
	} else {
		/* stop child to ensure it doesn't finish before it is traced */
		if (raise(SIGSTOP) != 0){
			perror("FAIL: child/helper SIGSTOP itself failed -");
			return errno;
		}

		if (child_trace) {
			if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1){
				perror("FAIL: child ptrace(PTRACE_TRACEME) failed - ");
				return errno;
			}
			if (raise(SIGSTOP) != 0){
				perror("FAIL: child SIGSTOP itself failed -");
				return errno;
			}
			/* ok we're stopped, wait for parent to trace (continue) us */
		}

	}

	execve(argv[0], argv, environ);

	perror("FAIL: child exec failed - ");

	return errno;
}

/* make pid_t global so the alarm handler can kill off children */
pid_t pid;

void sigalrm_handler(int sig) {
	fprintf(stderr, "FAIL: parent timed out waiting for child\n");
	kill(pid, SIGKILL);
	exit(1);
}

int main(int argc, char *argv[])
{
	int parent_trace = 1,
	    use_helper = 0,
	    num_syscall = NUM_CHLD_SYSCALLS, 
	    opt,
	    ret = 0;
	const char *usage = "usage: %s [-c] [-n #syscall] program [args ...]\n";
	char **args;

	if (signal(SIGALRM, sigalrm_handler) == SIG_ERR) {
		perror ("FAIL - signal failed: ");
		return(1);
        }

	opterr = 0;
	while (1) {
		opt = getopt(argc, argv, "chn:");

		if (opt == -1)
			break;
		switch (opt) {
			case 'c': parent_trace = 0;
				  break;
			case 'h': use_helper = 1;
				  break;
			case 'n': num_syscall = atoi(optarg); 
				  break;
			default:
				  fprintf(stderr, usage, argv[0]);
				  break;
		}
	}

	if (argc < 2) {
		fprintf(stderr, usage, argv[0]);
		return 1;
	}

	args = &argv[optind];

	pid = fork();
	if (pid > 0){	/*parent */
		int stat;

		ret = do_parent(pid, parent_trace, num_syscall);

		kill(pid, SIGKILL);

		if (ret >= 0) {
			/* wait for child */
			waitpid(pid, &stat, 0);
		}

		if (ret > 0) {
			perror("FAIL: parent failed: ");
		} else if (ret == 0) {
			printf("PASS\n");
			return 0;
		} else if (ret == -ECONNABORTED) {
			errno = -ret;
			perror("FAIL: child killed: ");
		} else {
			errno = -ret;
			perror("FAIL: child failed: ");
		}
	} else if (pid == 0) {	/* child */
		if (do_child(args, !parent_trace, use_helper))
			return 0;
			
	} else {
		perror("FAIL: fork failed - ");
	}

	return ret;
}
