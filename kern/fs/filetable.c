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

#include <fs.h>
#include <kern/errno.h>
#include <lib.h>

struct fs_file *sys_filetable;
unsigned int sys_filetable_size;

/*
* Initialize file table head node to NULL
*/
void
filetable_init(void)
{
    sys_filetable = NULL;
    sys_filetable_size = 0;
}

/*
* Add new file entry in the file table (which is a DLL).
* The new file gets appended and the pointer is moved so that it always points
* to the youngest/newest element.
*/
void
filetable_addfile(struct fs_file *newfile)
{
    if (sys_filetable == NULL) {
        sys_filetable->f_next = NULL;
        sys_filetable->f_prev = NULL;
    } else {
        newfile->f_next = NULL;
        newfile->f_prev = sys_filetable;
        newfile->f_prev->f_next = newfile;
    }
    sys_filetable = newfile;
    sys_filetable_size++;
}

/*
* Removes a node from the system file table, node is usually obtained by the
* process from its local file table
*/
void
filetable_removefile(struct fs_file *rmfile)
{
    rmfile->f_prev->f_next = rmfile->f_next;
    rmfile->f_next->f_prev = rmfile->f_prev;
    sys_filetable_size--;
    kfree((void *) rmfile);
}

/*
* Returns the size of the system file pointer
*/
unsigned int
filetable_size(void)
{
    return sys_filetable_size;
}