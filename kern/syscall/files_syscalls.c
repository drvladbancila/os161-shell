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
 *  Implementation of the system calls related to file management.
 */

#include <types.h>
#include <copyinout.h>
#include <syscall.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <limits.h>
#include <fs.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <uio.h>
#include <vnode.h>
#include <kern/errno.h>
#include <vm.h>
#include <lib.h>

/*
* System call interface function for opening files
*/
int sys_open(userptr_t filename, int flags, int *retfd)
{
    struct fs_file *file;
    struct vnode *file_vnode;
    int fd, err;
    mode_t mode;
    char *kfilename = NULL;

    /* copy filename into kernel memory space */
    kfilename = kstrdup((char *)filename);
    /* if kstrdup fails to find enough memory return ENOMEM error */
    if (kfilename == NULL)
    {
        return ENOMEM;
    }

    /* allocate space on the heap for a file table entry */
    file = (struct fs_file *) kmalloc(sizeof(struct fs_file));
    if (file == NULL)
    {
        return ENOMEM;
    }

    mode = 0644;
    /* get vnode for the file */
    err = vfs_open(kfilename, flags, mode, &file_vnode);
    if (err == EINVAL)
    {
        return EINVAL;
    }

    /* initialize filetable entry */
    file->f_vnode = file_vnode;
    file->f_offset = 0;
    file->f_lock = false;
    file->f_refcount = 1;
    file->f_mode = mode;

    /* add file to system filetable */
    filetable_addfile(file);

    /* find first available file descriptor */
    for (fd = 0; fd < OPEN_MAX; fd++)
    {
        if (curproc->p_filetable[fd] == NULL)
        {
            break;
        }
    }
    /*
     *  save into process filetable the pointer to the entry in the system
     *  file table at the file descriptor position previously found
     */
    curproc->p_filetable[fd] = file;
    // kprintf("Opened file with file descriptor: %d\n", fd);
    /* return file descriptor in retfd parameter */
    *retfd = fd;

    return 0;
}

/*
* System call interface function for closing file
*/
int sys_close(int fd)
{
    /*
     *  check if there is an open file corresponding to the file descriptor
     *  passed as argument
     */
    if (curproc->p_filetable[fd] == NULL)
    {
        return EINVAL;
    }
    /* if the file is open, then close it with vfs_close */
    vfs_close(curproc->p_filetable[fd]->f_vnode);
    /* remove file from system filetable and process filetable */
    filetable_removefile(curproc->p_filetable[fd]);
    curproc->p_filetable[fd] = NULL;
    // kprintf("Closed file with file descriptor: %d\n", fd);
    return 0;
}

/*
* System call interface function for reading file
*/
int
sys_read(int fd, userptr_t buf, size_t buflen, int *retval)
{
    struct fs_file *openfile;
    struct uio userio;
    struct iovec iov;
    off_t offset;
    int err;
    userptr_t bufend = buf + buflen;

    /* TODO: lock file while reading */

    /*
    *  bad buffer: either you want to read from null pointer or the buffer is
    *  partially or completely in kernel address space
    */
    if (buf == NULL || (bufend >= (userptr_t) USERSPACETOP)) {
        return EFAULT;
    }

    /* retrieve pointer to fs_file struct from process filetable */
    openfile = curproc->p_filetable[fd];

    /* if pointer is NULL then the file descriptor is invalid */
    if (openfile == NULL)
    {
        return EBADF;
    }

    /* take the offset at which the file was left last time */
    offset = openfile->f_offset;

    /* initialize io vector to point to user buffer and have correct length*/
    iov.iov_ubase = buf;
    iov.iov_len = buflen;

    /* initialize user io, specifying that the buffer belongs to userspace */
    userio.uio_iov = &iov;
    userio.uio_iovcnt = 1;
    userio.uio_offset = offset;
    userio.uio_resid = buflen;
    userio.uio_segflg = UIO_USERSPACE;
    userio.uio_rw = UIO_READ;
    userio.uio_space = proc_getas();

    /* read from vnode and write to the uio (which writes to buf) */
    err = VOP_READ(openfile->f_vnode, &userio);

    /* any I/O error */
    if (err) {
        return err;
    }

    kprintf("Read: %s\n", (char *) buf);

    /* give as return value the number of bytes read */
    *retval = userio.uio_offset - openfile->f_offset;
    /* update the file offset */
    openfile->f_offset = userio.uio_offset;

    return 0;
}

/*
* System call interface function for writing file
*/
int
sys_write(int fd, userptr_t buf, size_t buflen, int *retval)
{
    struct fs_file *openfile;
    struct uio userio;
    struct iovec iov;
    unsigned int offset;
    int err;
    userptr_t bufend = buf + buflen;

    /* TODO: lock file while writing */

    /*
    *  bad buffer: either you want to write to null pointer or the buffer is
    *  partially or completely in kernel address space
    */
    if (buf == NULL || (bufend >= (userptr_t) USERSPACETOP)) {
        return EFAULT;
    }

    /* retrieve fs_file struct pointer from file descriptor */
    openfile = curproc->p_filetable[fd];
    /* invalid file descriptor */
    if (openfile == NULL) {
        return EBADF;
    }

    /* get last offset */
    offset = openfile->f_offset;

    /* initialize io vector to point to user buffer and have correct length */
    iov.iov_ubase = buf;
    iov.iov_len = buflen;

    /* initialize user io, specifying that the buffer belongs to userspace */
    userio.uio_iov = &iov;
    userio.uio_iovcnt = 1;
    userio.uio_offset = offset;
    userio.uio_resid = buflen;
    userio.uio_segflg = UIO_USERSPACE;
    userio.uio_rw = UIO_WRITE;
    userio.uio_space = proc_getas();

    /* write operation on vnode */
    err = VOP_WRITE(openfile->f_vnode, &userio);

    /*
    *  errors during the write operation
    *  err = 0 : no errors during VOP_WRITE
    *  err = EIO : I/O errors during VOP_WRITE
    *  err = ENOSPC : no space on disk
    */
    if (err) {
        return err;
    }

    /* give as return value the number of bytes read */
    *retval = userio.uio_offset - openfile->f_offset;
    /* update the file offset */
    openfile->f_offset = userio.uio_offset;

    return 0;
}

/* Stores in the address selected by buf the name of the working directory,
 * terminated with zero.
 * retval is the effective dimension of the returned buffer, -1 on fail
 * returns 0 on success and ENOENT, EIO or EFAULT on fails  
*/
int
sys___getcwd (char *buf, size_t size, int *retval) 
{
    int error;
    struct uio userio;  
    struct iovec iov;
    //char *kbuf;

    //kbuf = kstrdup(buf);

    userptr_t bufend = (userptr_t) (buf + size);
    if (buf == NULL || (bufend >= (userptr_t) USERSPACETOP)) {
        return EFAULT;
    }
    (void)retval;
    // TODO: usa checkbuffer per fare questo controllo

    /* initialize io vector to point to user buffer and have correct length */
    iov.iov_ubase = (userptr_t) buf;
    iov.iov_len = size;

    /* initialize user io, specifying that the buffer belongs to userspace */
    userio.uio_iov = &iov;
    userio.uio_iovcnt = 1;
    userio.uio_offset = 0;
    userio.uio_resid = size;
    userio.uio_segflg = UIO_USERSPACE;
    userio.uio_rw = UIO_READ;
    userio.uio_space = proc_getas();

    /*
    *  errors during the getcwd operation
    *  error = 0 : no errors during VOP_WRITE
    *  error = EIO : I/O errors during VOP_WRITE
    *  error = ENOENT	: a component of the pathname no longer exists.
    */
    error = vfs_getcwd(&userio);

    if (error) {
        *retval = -1;
        return error;
    } else {
        //null termination
        buf[size - userio.uio_resid] = 0;

        //return the length of returned data
        *retval = size - userio.uio_resid;
    }

    return 0;
}

/* Sets the current working directory to the one named in pathname 
*/
int 
sys_chdir(char *pathname, int *retval) 
{
    int error;
    char *kpath;

    //TODO: return errors as man page
    kpath = kstrdup(pathname);
    error = vfs_chdir(kpath);
    /*
    *  errors during the chdir operation
    *  err = 0 : no errors during VOP_WRITE
    *  err = EIO : I/O errors during VOP_WRITE
    *  err = ENODEV	: The device prefix of pathname did not exist 1
    *  err = ENOTDIR : pathname did not refer to a directory  1
    *  err = ENOENT : pathname did not exist   1
    *  err = EFAULT	: pathname was an invalid pointer
    */
    if (error) {
        *retval = -1;
        return error;
    } else {
        // save pathname in the variable of the current process
        set_cwd_from_path(curproc->c_cwd, pathname, strlen(pathname));
        *retval = 0;
    }

    return 0;
}