/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * 	Test program for all the implemented system calls
 */

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>

int
main()
{
    char *argv[3];
    int err, fd, status, ret;
    pid_t pid;
    size_t bytes = 0;
    char buf[PATH_MAX+1], *p;
    char buffer[] = "it works!";

	argv[0] = (char *) "testdemochild";
    argv[1] = (char *) "file.txt";
	argv[2] = NULL;

    pid = getpid();
    printf("P: I'm the parent process, my PID is: %d\n", pid);
    printf("P: Now I will move to directory /prova and create a file\n"); 

    // change directory to prova with absolute path
	err = chdir("emu0:/prova");
	if (err == -1) {
		printf("Chdir failed with errno: %s", strerror(errno));
	}

    // prints the cwd 
	p = getcwd(buf, sizeof(buf));
	if (p == NULL) {
		printf("getcwd returned NULL\n");
	} else {
        printf("P: directory: %s\n", p);
    }

    // create or open a file
    fd = open("file.txt", O_CREAT|O_WRONLY);

    printf("P: I'm writing on the file...\n"); 
    // write on the file
    while (bytes < strlen(buffer)) {
        bytes += write(fd, &buffer[bytes], 1);
    }

    printf("P: Now I will fork and wait the end of my child\n"); 
    pid = fork();

    if (pid < 0) {
        printf("P: Could not create child process\n");
        return -1;
    } else if (pid == 0) {
        printf("C: I am child process with pid %d, my parent's pid is %d, and I'm calling testdemochild...\n", getpid(), getppid());
        err = execv("/testbin/testdemochild", argv);
        if (err < 0) {
            printf("C: Error during execv\n");
        }
    } else { // parent process
        ret = waitpid(pid, &status, 0);
        printf("P: I'm the parent, my child has returned: %d\n", WEXITSTATUS(status));
        printf("P: I'm the parent, waitpid has returned: %d\n", ret);
    }

    close(fd);
    return 0;
}