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
 * 	Test program for getpid syscall.
 */

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

int
main()
{
	char buf[PATH_MAX+1], *p;
	int retval;
	int fd;

	// print current working directory (root)
	p = getcwd(buf, sizeof(buf));
	if (p == NULL) {
		printf("getcwd returned NULL\n");
	} else {
        printf("%s\n", p);
    }

	// change directory to prova
	retval = chdir("emu0:/prova/paolo");
	if (retval == -1) {
		printf("Chdir failed with errno: %s", strerror(errno));
	}

	// prints again the cwd 
	p = getcwd(buf, sizeof(buf));
	if (p == NULL) {
		printf("getcwd returned NULL\n");
	} else {
        printf("%s\n", p);
    }

	// change directory to prova
	retval = chdir("../..");
	if (retval == -1) {
		printf("Chdir failed with errno: %s", strerror(errno));
	}

	// prints again the cwd 
	p = getcwd(buf, sizeof(buf));
	if (p == NULL) {
		printf("getcwd returned NULL\n");
	} else {
        printf("%s\n", p);
    }

	// change directory to prova
	retval = chdir("/prova/paolo");
	if (retval == -1) {
		printf("Chdir failed with errno: %s", strerror(errno));
	}

    fd = open("hello.txt", O_CREAT);
    close(fd);

	// prints again the cwd 
	p = getcwd(buf, sizeof(buf));
	if (p == NULL) {
		printf("getcwd returned NULL\n");
	} else {
        printf("%s\n", p);
    }

	// change directory to prova
	retval = chdir("emu0:");
	if (retval == -1) {
		printf("Chdir failed with errno: %s", strerror(errno));
	}

	// prints again the cwd 
	p = getcwd(buf, sizeof(buf));
	if (p == NULL) {
		printf("getcwd returned NULL\n");
	} else {
        printf("%s\n", p);
    }

	retval = chdir(" ");
	if (retval == -1) {
		printf("Chdir failed with errno: %s\n", strerror(errno));
	}

	retval = chdir("pippo");
	if (retval == -1) {
		printf("Chdir failed with errno: %s\n", strerror(errno));
	}

	return 0;
}