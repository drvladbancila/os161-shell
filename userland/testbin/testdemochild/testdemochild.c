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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

int
main(int argc, char **argv)
{
    int fd, bytes, i = 0;
    char buffer[50];

    if (argc < 2) {
        printf("C: You did not supply a filename!\n");
        exit(EXIT_FAILURE);
    }

    fd = open(argv[1], O_RDONLY);
    printf("C: I'm demochild, reading what my parent wrote in the file:\n");
    bytes = 1;
    while (bytes != 0) {
        bytes = read(fd, &buffer[i], 1);
        i++;
    }
    printf("C: I've read: %s\n", buffer);

    for (int j = 0; j < i + 1; j++) {
        buffer[j] = 0;
    }

    printf("C: I try to read again the last 4 char with lseek... Just to be sure\n");
    lseek(fd, -3, SEEK_END);
    bytes = 1;
    i = 0;
    while (bytes != 0) {
        bytes = read(fd, &buffer[i], 1);
        i++;
    }
    printf("C: I've read: %s\nC: Now I can go!\n", buffer);


    close(fd);
    exit(EXIT_SUCCESS);
}