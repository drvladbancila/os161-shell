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
 * Code to load an ELF-format executable into the current address space.
 *
 * It makes the following address space calls:
 *    - first, as_define_region once for each segment of the program;
 *    - then, as_prepare_load;
 *    - then it loads each chunk of the program;
 *    - finally, as_complete_load.
 *
 * This gives the VM code enough flexibility to deal with even grossly
 * mis-linked executables if that proves desirable. Under normal
 * circumstances, as_prepare_load and as_complete_load probably don't
 * need to do anything.
 *
 * If you wanted to support memory-mapped executables you would need
 * to rearrange this to map each segment.
 *
 * To support dynamically linked executables with shared libraries
 * you'd need to change this to load the "ELF interpreter" (dynamic
 * linker). And you'd have to write a dynamic linker...
 */

#include <types.h>
#include <copyinout.h>
#include <syscall.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <limits.h>
#include <fs.h>
#include <kern/errno.h>

/*
* System call interface function for opening file
*/
int
sys_open(userptr_t filename, int flag, int *retfd)
{
    struct fs_file *file;
    struct vnode *file_vnode;
    int fd, err;
    mode_t mode = 0;
    char *kfilename;

    file = (struct fs_file *) kmalloc(sizeof(struct fs_file));
    if (file == NULL) {
        return ENOMEM;
    }

    kfilename = (char *) filename;
    err = vfs_open(kfilename, flag, mode, &file_vnode);
    if (err == EINVAL) {
        return EINVAL;
    }

    file->f_vnode = file_vnode;
    file->f_offset = 0;
    file->f_lock = 0;
    file->f_refcount++;
    file->f_mode = flag;

    err = filetable_addfile(file);
    if (err > 0) {
        return ENOMEM;
    }

    for (fd = 0; fd < OPEN_MAX; fd++) {
        if (curproc->p_filetable[fd] == NULL) {
            break;
        }
    }
    curproc->p_filetable[fd] = file;

    (void) filename;
    (void) flag;
    (void) retfd;
    return 0;
}