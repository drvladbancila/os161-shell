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

#include <types.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <limits.h>
#include <addrspace.h>
#include <kern/errno.h>
#include <kern/wait.h>
#include <mips/trapframe.h>
#include <synch.h>
#include <copyinout.h>

static const char *arg_padding[] = {"", "\0", "\0\0", "\0\0\0"};

/*
* System call interface function to get the process ID
*/
int
sys_getpid(int *retpid)
{
    /* return process ID in retpid parameter */
    *retpid = (int) curproc->p_id;

    /* this system call is always successful */
    return 0;
}

/*
* System call interface function to get the process ID of the current process' parent
*/
int
sys_getppid(int *retpid)
{
    /* return process ID in retpid parameter */
    *retpid = (int) curproc->p_parent->p_id;

    /* this system call is always successful */
    return 0;
}

/*
* System call interface function to fork a process
*/
int
sys_fork(struct trapframe *tf, int *retval){

    struct trapframe *child_tf;
    struct proc *child;

    /* copy trap frame from parent */
    child_tf = (struct trapframe *)kmalloc(sizeof(struct trapframe));
    *child_tf = *tf;

    /* create child process */
    child = proc_create_runprogram(curproc->p_name);
	if (child == NULL) {
		return ENOMEM;
	}

    /* set address space */
    as_copy(curproc->p_addrspace, &child->p_addrspace);
	if (child->p_addrspace == NULL) {
		return ENOMEM;
	}

    /* set file descriptor table */
    for (unsigned int fd = 0; fd < OPEN_MAX; fd++) {
		child->p_filetable[fd] = curproc->p_filetable[fd];
        if(curproc->p_filetable[fd] != NULL){
            curproc->p_filetable[fd]->f_refcount++;
        }
	}

    /* set curproc as parent */
    child->p_parent = curproc;

    /* create new thread starting from parent */
    thread_fork(child->p_name, child, enter_forked_process, (void *)child_tf, 1);


////////////////////////
    // TODO : ERRORS to deal with
    // EMPROC	The current user already has too many processes.
    // ENPROC	There are already too many processes on the system.
    // ENOMEM	Sufficient virtual memory for the new process was not available.

    /* parent return child pid */
    *retval = child->p_id;

    return 0;
}

int
sys_execv(userptr_t prog, userptr_t args)
{
    char *kprogname;
    size_t prog_len;
    /*
    * this is just a cast from a userptr_t to a double pointer to char
    * so that we can use [] and treat it as if it was the usual argv
    */
    char **argv = (char **) args;
    /*
    * kbuf is the kernel buffer where we put everything
    * kbuf_ptr_begin points to the beginning of the data (arguments)
    * kbuf_ptr is moved while copying the arguments that are passed
    */
    char **kbuf, *kbuf_ptr, *kbuf_ptr_begin;
    size_t kbuf_size;
    int result;
    int i = 0;
    size_t args_size = 0, arg_size, padding_size;
    int argc = 0;
    struct vnode *v;
    struct addrspace *as;
    vaddr_t entrypoint, stackptr, stackptr_data, stackptr_argv;

    /* if program name is an invalid buffer */
    prog_len = strlen((char *) prog);
    if (check_buffer(prog, prog_len)) {
        return EFAULT;
    }
    /* copy program name into kernel space */
    kprogname = kstrdup((char *) prog);

    /* here we copy the arguments to kernel space */

    /* count number of arguments */
    while (argv[argc] != NULL && args_size <= ARG_MAX){
        arg_size = strlen(argv[argc]) + 1;
        if (check_buffer((userptr_t) argv[argc], arg_size)) {
            return EFAULT;
        }
        if (arg_size % 4 == 0) {
            /* no need for padding */
            padding_size = 0;
        } else {
            /* need for padding */
            padding_size = 4 - (arg_size % 4);
        }
        args_size += arg_size + padding_size;
        argc++;
    }

    /* size of arguments (with padding) is too large */
    if (args_size > ARG_MAX) {
        return E2BIG;
    }

    /*
    * kbuf must be big enough to accomodate the args with padding and an
    * initial pointer to the data which is made of (argc + 1) words
    */
    kbuf_size = sizeof(char) * args_size + sizeof(char *) * (argc + 1);
    /* allocate the buffer in the kernel heap */
    kbuf  = (char **) kmalloc(kbuf_size);
    /* point to the part where you will store the data (padded args) */
    kbuf_ptr_begin = (char *) (kbuf + argc + 1);
    /* copy the pointer so that you do not lose reference to the beginning */
    kbuf_ptr = kbuf_ptr_begin;

    /* for each argument ... */
    for (i = 0; i < argc; i++) {
        /* set the vector entry i to the value of the ptr where first arg begins */
        kbuf[i] = kbuf_ptr;
        /* compute arg size and check if it needs padding */
        arg_size = strlen(argv[i]) + 1;
        if (arg_size % 4 == 0) {
            padding_size = 0;
        } else {
            padding_size = 4 - (arg_size % 4);
        }
        /* copy the argument into the data part of the kernel buffer */
        copyin((userptr_t) argv[i], kbuf_ptr, arg_size);
        kbuf_ptr += arg_size;
        /* if there is padding to do, concatenate the padding string */
        if (padding_size > 0) {
            memcpy(kbuf_ptr, arg_padding[padding_size], padding_size);
        }
        kbuf_ptr += padding_size;
    }
    /* last element of the reference vector must be null also in kern buffer */
    kbuf[argc] = NULL;

    /* open ELF file */
    result = vfs_open(kprogname, O_RDONLY, 0, &v);
    if (result) {
        return result;
    }

    /* create a new address space */
    as = as_create();
	if (as == NULL) {
		vfs_close(v);
		return ENOMEM;
	}

    /* switch to new address space */
    proc_setas(as);
	as_activate();

    /* load the ELF into memory and get pointer to entry point */
    result = load_elf(v, &entrypoint);
    if (result) {
        vfs_close(v);
        return result;
    }

    /* close ELF file, we have loaded it into memory */
    vfs_close(v);

    /* define user stack in the new address space: simply assigns stackptr to 0x80000000 */
    result = as_define_stack(as, &stackptr);
    if (result) {
        return result;
    }

    /*
    * the buffer will start at top of stack - the size of the buffer that we need to
    * load into the new userspace
    */
    stackptr_argv = stackptr - kbuf_size;
    /* the data segment of the userspace buffer starts (argc + 1) words later */
    stackptr_data = stackptr_argv + sizeof(char *) * (argc + 1);
    /*
    * NOTE! Before copying everything brutally into the new userspace, we need to
    * change the references so that they point to where things will be copied
    * into the userspace
    */
    for (i = 0; i < argc; i++) {
        /*
        * This is just a mapping between kernel buffer and addrspace buffer:
        * compute the delta between where argv[i] starts and where the data segment
        * starts in the kernel buffer. Add this delta to where the data segment
        * starts in the userspace
        */
        kbuf[i] = (kbuf[i] - kbuf_ptr_begin) + (char *) stackptr_data;
    }

    /* now we can copy everything brutally as the references will be correct! */
    copyout(kbuf, (userptr_t) stackptr_argv, kbuf_size);
    /* free the memory allocated in the kernel heap */
    kfree(kbuf);
    kfree(kprogname);

    /* switch to user mode */
    enter_new_process(argc, (userptr_t) stackptr_argv, NULL, stackptr_argv, entrypoint);
    panic("enter_new_process returned in execv\n");
    return EINVAL;
}

/*
* System call interface function to exit from a process
*/
int
sys__exit(int status)
{
    struct proc *alone_proc;

    /* record status */
    curproc->p_exit_status = status;

    /* save pointer to the current process */
    alone_proc = curproc;

    if(alone_proc->p_numthreads == 1){
        /* release the lock */
        lock_release(curproc->p_lock_active);
	    /* acquire wait lock (useful in case of parent waitpid syscall) */
	    lock_acquire(curproc->p_lock_wait);
	    /* release wait lock */
	    lock_release(curproc->p_lock_wait);
        proc_remthread(curthread);
        proc_destroy(alone_proc);
    }

    /* Exit thread */
    thread_exit();

    return 0;
}

/*
* System call interface function to wait for a process to terminate given its identifier
*/
int 
sys_waitpid(__pid_t pid, int *status, int options, int *retval)
{
    struct proc *searchproc = proc_head;
    struct proc *foundproc = NULL;
    int tryval;

    /* currently no options are supported */
    if(options != 0 && options != WNOHANG){
        return EINVAL;
    }

    /* check if pid argument is the identifier of an existing process*/
    while(searchproc != NULL){
        if(searchproc->p_id == pid){
            foundproc = searchproc;
            searchproc = NULL;
        }
        else{
            searchproc = searchproc->p_prevproc;
        }
    }
    if(foundproc == NULL){
        return ESRCH;
    }

    /* check if pid argument is the identifier of a child process*/
    if(foundproc->p_parent != curproc){
        return ECHILD;
    }
    
    /* check if status argument is a valid pointer */
    if (status >= (int *) USERSPACETOP) {
		return EFAULT;
	}

    /* acquire child locks */
    lock_acquire(foundproc->p_lock_wait);
    if(options == WNOHANG){
        tryval = lock_tryacquire(foundproc->p_lock_active);
        if(tryval){
            *retval = 0;
            return 0;
        }
    }
    else{
        lock_acquire(foundproc->p_lock_active);
    }

    /* save child process exit status */
    *status = _MKWAIT_EXIT(foundproc->p_exit_status);

    /* release child locks */
    lock_release(foundproc->p_lock_wait);
    lock_release(foundproc->p_lock_active);

    /* on success, the child pid is the return value */
    *retval = (int) pid;

    return 0;
}