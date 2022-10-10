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
#include <mips/trapframe.h>

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

/*
* System call interface function to exit from a process
* TODO: Still to understand
*/
int
sys__exit(int status)
{
    /* Record status */
    curproc->p_exit_status = status;

    /* Release address space */
    as_destroy(curproc->p_addrspace);

    /* Exit thread */
    thread_exit(); 

    return 0;
}