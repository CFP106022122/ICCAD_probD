/*
 * magic.h --
 *
 * Global definitions for all MAGIC modules
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
 *
 * rcsid="$Header"
 */

#ifndef _MAGIC_H
#define	_MAGIC_H

/* ------------------- Universal pointer typecast --------------------- */

typedef unsigned long pointertype;
typedef signed long spointertype;


/* --------------------- Universal pointer type ----------------------- */

#ifndef _CLIENTDATA
typedef pointertype ClientData;
#endif

/* --------------------- Debugging and assertions --------------------- */

/* To enable assertions, undefine NDEBUG in file defs.mak */

#include <assert.h>
#define	ASSERT(p, where) assert(p)	/* "where" is ignored */


#endif /* _MAGIC_H */
