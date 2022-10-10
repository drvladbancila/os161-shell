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
*  System File Table
*  Pointer to every opened file is added to this table
*/

#include <fs.h>
#include <kern/errno.h>
#include <lib.h>

struct fs_filetable sys_filetable;
/*
* Initialize file table head node to NULL
*/
int
filetable_init(void)
{
    int err_in, err_out, err_err;
    char con_filename[5];

    sys_filetable.lock = lock_create("sys_filetable_lock");
    if (sys_filetable.lock == NULL) {
        return ENOMEM;
    }

    sys_filetable.stdin = (struct fs_file *) kmalloc(sizeof(struct fs_file));
    sys_filetable.stdout = (struct fs_file *) kmalloc(sizeof(struct fs_file));
    sys_filetable.stderr = (struct fs_file *) kmalloc(sizeof(struct fs_file));

    if (sys_filetable.stdin == NULL || sys_filetable.stdout == NULL
     || sys_filetable.stderr == NULL) {
        return ENOMEM;
     }

    strcpy(con_filename, "con:");
    err_in = vfs_open(con_filename, O_RDONLY, 0644, &sys_filetable.stdin->f_vnode);
    strcpy(con_filename, "con:");
    err_out = vfs_open(con_filename, O_WRONLY, 0644, &sys_filetable.stdout->f_vnode);
    strcpy(con_filename, "con:");
    err_err = vfs_open(con_filename, O_WRONLY, 0644, &sys_filetable.stderr->f_vnode);

    if (err_in) {
        return err_in;
    }

    if (err_out) {
        return err_out;
    }

    if(err_err) {
        return err_err;
    }

    sys_filetable.stdin->f_lock = lock_create("stdin");
    sys_filetable.stdout->f_lock = lock_create("stdout");
    sys_filetable.stderr->f_lock = lock_create("stderr");
    if (sys_filetable.stdin->f_lock == NULL
     || sys_filetable.stdout->f_lock == NULL
     || sys_filetable.stderr->f_lock == NULL) {
        return ENOMEM;
     }

    sys_filetable.head = sys_filetable.stdin;
    sys_filetable.tail = sys_filetable.head;
    sys_filetable.head->f_next = NULL;
    sys_filetable.head->f_prev = NULL;
    filetable_addfile(sys_filetable.stdout);
    filetable_addfile(sys_filetable.stderr);

    sys_filetable.size = 3;

    return 0;
}

void
filetable_cleanup(void)
{
    lock_destroy(sys_filetable.lock);
    struct fs_file *ptr = filetable_head();

    while (ptr->f_prev != NULL) {
        ptr = ptr->f_prev;
        kfree(ptr->f_next);
    }
}

/*
* Add new file entry in the file table (which is a DLL).
* The new file gets appended and the pointer is moved so that it always points
* to the youngest/newest element.
*/
void
filetable_addfile(struct fs_file *newfile)
{
    lock_acquire(sys_filetable.lock);
    newfile->f_next = NULL;
    newfile->f_prev = sys_filetable.head;
    newfile->f_prev->f_next = newfile;
    sys_filetable.head = newfile;
    sys_filetable.size++;
    lock_release(sys_filetable.lock);
}

/*
* Removes a node from the system file table, node is usually obtained by the
* process from its local file table
*/
void
filetable_removefile(struct fs_file *rmfile)
{
    lock_acquire(sys_filetable.lock);

    /*
    *  if this is the only file in the filetable and you want to close it,
    *  then there are no stdin, stdout and stderr open so this is going to trigger
    *  a panic
    */
    KASSERT(!(rmfile->f_prev == NULL && rmfile->f_next == NULL));

    /* if prev file is null then this is the tail, so can't edit prev ptr */
    if (rmfile->f_prev != NULL) {
        rmfile->f_prev->f_next = rmfile->f_next;
    } else {
        sys_filetable.tail = rmfile->f_next;
        sys_filetable.tail->f_prev = NULL;
    }

    /* if next file is null then this is the head, so can't edit next ptr */
    if (rmfile->f_next != NULL) {
        rmfile->f_next->f_prev = rmfile->f_prev;
    } else {
        sys_filetable.head = rmfile->f_prev;
        sys_filetable.head->f_next = NULL;
    }

    sys_filetable.size--;

    /* finally, deallocate the fs_file structure from kernel space */
    lock_destroy(rmfile->f_lock);
    kfree((void *) rmfile);
    lock_release(sys_filetable.lock);
}

/*
* Returns the size of the system file pointer
*/
size_t
filetable_size(void)
{
    return sys_filetable.size;
}

/*
* Returns youngest element in the system file table
*/
struct fs_file *
filetable_head(void)
{
    return sys_filetable.head;
}

/*
* Returns oldest element in the system file table
*/
struct fs_file *
filetable_tail(void)
{
    return sys_filetable.tail;
}

/*
* Returns pointer to system filetable lock
*/
struct lock *
filetable_lock(void)
{
    return sys_filetable.lock;
}