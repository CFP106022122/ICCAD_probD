/*
 * malloc.c --
 *
 * Memory allocator.
 *   Magic no longer contains its own version of malloc(), as all modern
 *   operating systems have implementations which are as good or better.
 *
 *   The main thing this file does is to define mallocMagic and friends.
 *   freeMagic frees the previously requested memory item, not the one
 *   passed as the argument.  This allows efficient coding of loops which
 *   run through linked lists and process and free them at the same time.
 *
 *   ALWAYS use mallocMagic() with freeMagic() and NEVER mix them with
 *   malloc() and free().
 *
 *   Malloc trace routines have been removed.  There are standard methods
 *   to trace memory allocation and magic doesn't need its own built-in
 *   method.
 *
 *   The Tcl/Tk version of magic makes use of Tcl_Alloc() and Tcl_Free()
 *   which allows the Tcl/Tk version to trace memory using Tcl's methods.
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1985, 1990 Regents of the University of California. * 
 *     * Permission to use, copy, modify, and distribute this              * 
 *     * software and its documentation for any purpose and without        * 
 *     * fee is hereby granted, provided that the above copyright          * 
 *     * notice appear in all copies.  The University of California        * 
 *     * makes no representations about the suitability of this            * 
 *     * software for any purpose.  It is provided "as is" without         * 
 *     * express or implied warranty.  Export of this software outside     * 
 *     * of the United States of America may require an export license.    * 
 *     *********************************************************************
 */

#include <iostream>
#include "magic.h"
#include "malloc.h"

/* Normally we're supposed to warn against the use of standard malloc()	*/
/* and free(), but obviously that doesn't apply to this file.		*/

/*
 * Magic may reference an object after it is free'ed, but only one object.
 * This is a change from previous versions of Magic, which needed to reference
 * an arbitrary number of objects before the next call to malloc.  Only then 
 * would no further references would be made to free'ed storage.
 */

/* Delay free'ing by one call, to accomodate Magic's needs. */
static char *freeDelayedItem = NULL;

/* Local definitions */

/*
 *---------------------------------------------------------------------
 * mallocMagic() --
 *
 *	memory allocator with support for one-delayed-item free'ing
 *---------------------------------------------------------------------
 */

void* mallocMagic(unsigned int nbytes){
    void *p;
    
    if (freeDelayedItem)
    {
	/* fprintf(stderr, "freed 0x%x (delayed)\n", freeDelayedItem); fflush(stderr); */

	free(freeDelayedItem);
	freeDelayedItem=NULL;
    }

    if ((p = (void *)malloc(nbytes)) != NULL) 
    {
	/* fprintf(stderr, "alloc'd %u bytes at 0x%x\n", nbytes, p); fflush(stderr); */

	return p;
    }
    else
    {
	ASSERT(false, "Can't allocate any more memory.\n");
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------
 * freeMagic() --
 *
 *	one-delayed-item memory deallocation
 *---------------------------------------------------------------------
 */

void freeMagic(void* cp)
{
    if (cp == NULL)
        printf("freeMagic called with NULL argument.\n");
	    // TxError("freeMagic called with NULL argument.\n");
    if (freeDelayedItem)
    {
	/* fprintf(stderr, "freed 0x%x\n", freeDelayedItem); fflush(stderr); */

	free(freeDelayedItem);
    }
    freeDelayedItem = (char*) cp;
}