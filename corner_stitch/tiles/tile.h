/*
 * tile.h --
 *
 * Definitions of the basic tile structures
 * The definitions in this file are all that is visible to
 * the Ti (tile) modules.
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
 * rcsid "$Header: /usr/cvsroot/magic-7.5/tiles/tile.h,v 1.7 2010/05/03 15:16:57 tim Exp $"
 */

#ifndef _TILES_H
#define	_TILES_H

#include "../utils/magic.h"

/*
 * A tile is the basic unit used for representing both space and
 * solid area in a plane.  It has the following structure:
 *
 *				       RT
 *					^
 *					|
 *		-------------------------
 *		|			| ---> TR
 *		|			|
 *		|			|
 *		| (lower left)		|
 *	BL <--- -------------------------
 *		|
 *		v
 *	        LB
 *
 * The (x, y) coordinates of the lower left corner of the tile are stored,
 * along with four "corner stitches": RT, TR, BL, LB.
 *
 * Space tiles are distinguished at a higher level by having a distinguished
 * tile body.
 */

typedef struct
{
    double p_x;
    double p_y;
} Point;

typedef struct
{
    Point r_ll;			/* Lower-left corner of rectangle. */
    Point r_ur;			/* Upper-right corner of rectangle. */
} Rect;

typedef struct tile
{
    ClientData	 ti_body;	/* Body of tile */
    struct tile	*ti_lb;		/* Left bottom corner stitch */
    struct tile	*ti_bl;		/* Bottom left corner stitch */
    struct tile	*ti_tr;		/* Top right corner stitch */
    struct tile	*ti_rt;		/* Right top corner stitch */
    Point	 ti_ll;		/* Lower left coordinate */
    ClientData	 ti_client;	/* This space for hire.  Warning: the default
				 * value for this field, to which all users
				 * should return it when done, is CLNTDEFAULT
				 * instead of NULL.
				 */
} Tile;

    /*
     * The following macros make it appear as though both
     * the lower left and upper right coordinates of a tile
     * are stored inside it.
     */

#define	BOTTOM(tp)		((tp)->ti_ll.p_y)
#define	LEFT(tp)		((tp)->ti_ll.p_x)
#define	TOP(tp)			(BOTTOM(RT(tp)))
#define	RIGHT(tp)		(LEFT(TR(tp)))

#define	LB(tp)		((tp)->ti_lb)
#define	BL(tp)		((tp)->ti_bl)
#define	TR(tp)		((tp)->ti_tr)
#define	RT(tp)		((tp)->ti_rt)


/* ----------------------- Tile planes -------------------------------- */

/*
 * A plane of tiles consists of the four special tiles needed to
 * surround all internal tiles on all sides.  Logically, these
 * tiles appear as below, except for the fact that all are located
 * off at infinity.
 *
 *	 --------------------------------------
 *	 |\				         /|
 *	 | \				    / |
 *	 |  \		   TOP     /  |
 *	 |   \				  /   |
 *	 |    \				 /    |
 *	 |     --------------     |
 *	 |     |			|     |
 *	 |LEFT |			|RIGHT|
 *	 |     |			|     |
 *	 |     --------------     |
 *	 |    /				 \    |
 *	 |   /				  \   |
 *	 |  /		 BOTTOM    \  |
 *	 | /				    \ |
 *	 |/				         \|
 *	 --------------------------------------
 */

typedef struct
{
    Tile	*pl_left;	/* Left pseudo-tile */
    Tile	*pl_top;	/* Top pseudo-tile */
    Tile	*pl_right;	/* Right pseudo-tile */
    Tile	*pl_bottom;	/* Bottom pseudo-tile */
    Tile	*pl_hint;	/* Pointer to a "hint" at which to
				 * begin searching.
				 */
} Plane;

/*
 * The following coordinate, INFINITY, is used to represent a
 * tile location outside of the tile plane.
 *
 * It must be possible to represent INFINITY+1 as well as
 * INFINITY.
 *
 * Also, because locations involving INFINITY may be transformed,
 * it is desirable that additions and subtractions of small integers
 * from either INFINITY or MINFINITY not cause overflow.
 *
 * Consequently, we define INFINITY to be the largest integer
 * representable in wordsize - 2 bits.
 */

#undef INFINITY
#define	INFINITY	((1 << (8*sizeof (int) - 2)) - 4)
#define	MINFINITY	(-INFINITY)

/* CLIENTDEFAULT differs from MINFINITY on 64-bit systems, where it	*/
/* prevents problems arising from MINFINITY being two different values	*/
/* depending on whether it is cast into a 32 or a 64 bit word.		*/

#define	CLIENTMAX	(((pointertype)1 << (8 * sizeof(pointertype) - 2)) - 4)
#define	CLIENTDEFAULT	(-CLIENTMAX)

/* ------------------------ Flags, etc -------------------------------- */

#define	BADTILE		((Tile *) -1)	/* Invalid tile pointer */

/* ============== Function headers and external interface ============= */

/*
 * The following macros and procedures should be all that are
 * ever needed by modules other than the tile module.
 */

extern Plane* TiNewPlane(Tile* tile);
extern void TiFreePlane(Plane* plane);
extern Tile *TiSplitX(Tile* tile, double x);
extern Tile *TiSplitY(Tile* tile, double y);
extern void  TiJoinX(Tile* tile1, Tile* tile2, Plane* plane);
extern void  TiJoinY(Tile* tile1, Tile* tile2, Plane* plane);
extern int   TiSrArea(Tile* hintTile, Plane* plane, Rect* rect, int (*func)(Tile *tile, ClientData cdata), ClientData arg);
extern Tile *TiSrPoint(Tile* hintTile, Plane* plane, Point* point);

#define	TiBottom(tp)		(BOTTOM(tp))
#define	TiLeft(tp)		(LEFT(tp))
#define	TiTop(tp)		(TOP(tp))
#define	TiRight(tp)		(RIGHT(tp))

/*
 * For the following to work, the caller must include database.h
 * (to get the definition of TileType).
 */

#define	TiGetType(tp)		((TileType)(spointertype) (tp)->ti_body)
#define TiGetTypeExact(tp)	TiGetType(tp)
#define TiGetLeftType(tp)	TiGetType(tp)
#define TiGetRightType(tp)	TiGetType(tp)
#define TiGetTopType(tp)	TiGetType(tp)
#define TiGetBottomType(tp)	TiGetType(tp)
#define IsSplit(tp)		0

#define	TiGetBody(tp)		((tp)->ti_body)
/* See diagnostic subroutine version in tile.c */
#define	TiSetBody(tp, b)	((tp)->ti_body = (ClientData)(pointertype) (b))
#define	TiGetClient(tp)		((tp)->ti_client)
#define	TiSetClient(tp,b)	((tp)->ti_client = (ClientData)(pointertype) (b))

Tile *TiAlloc();
void TiFree(Tile* tp);

#define EnclosePoint(tile,point)	((LEFT(tile)   <= (point)->p_x ) && \
					 ((point)->p_x   <  RIGHT(tile)) && \
					 (BOTTOM(tile) <= (point)->p_y ) && \
					 ((point)->p_y   <  TOP(tile)  ))

#define EnclosePoint4Sides(tile,point)	((LEFT(tile)   <= (point)->p_x ) && \
					 ((point)->p_x  <=  RIGHT(tile)) && \
					 (BOTTOM(tile) <= (point)->p_y ) && \
					 ((point)->p_y  <=  TOP(tile)  ))

/* The four macros below are for finding next tile RIGHT, UP, LEFT or DOWN 
 * from current tile at a given coordinate value.
 *
 * For example, NEXT_TILE_RIGHT points tResult to tile to right of t 
 * at y-coordinate y.
 */

#define NEXT_TILE_RIGHT(tResult, t, y) \
    for ((tResult) = TR(t); BOTTOM(tResult) > (y); (tResult) = LB(tResult)) \
        /* Nothing */;

#define NEXT_TILE_UP(tResult, t, x) \
    for ((tResult) = RT(t); LEFT(tResult) > (x); (tResult) = BL(tResult)) \
        /* Nothing */;

#define NEXT_TILE_LEFT(tResult, t, y) \
    for ((tResult) = BL(t); TOP(tResult) <= (y); (tResult) = RT(tResult)) \
        /* Nothing */;
 
#define NEXT_TILE_DOWN(tResult, t, x) \
    for ((tResult) = LB(t); RIGHT(tResult) <= (x); (tResult) = TR(tResult)) \
        /* Nothing */;

#define	TiSrPointNoHint(plane, point)	(TiSrPoint((Tile *) NULL, plane, point))

/*
 * GOTOPOINT is used whenever a macroized version of TiSrPoint is
 * needed.
 */

#define	GOTOPOINT(tp, p) \
    { \
	if ((p)->p_y < BOTTOM(tp)) \
	    do tp = LB(tp); while ((p)->p_y < BOTTOM(tp)); \
	else \
	    while ((p)->p_y >= TOP(tp)) tp = RT(tp); \
	if ((p)->p_x < LEFT(tp)) \
	    do  \
	    { \
		do tp = BL(tp); while ((p)->p_x < LEFT(tp)); \
		if ((p)->p_y < TOP(tp)) break; \
		do tp = RT(tp); while ((p)->p_y >= TOP(tp)); \
	    } \
	    while ((p)->p_x < LEFT(tp)); \
	else \
	    while ((p)->p_x >= RIGHT(tp)) \
	    { \
		do tp = TR(tp); while ((p)->p_x >= RIGHT(tp)); \
		if ((p)->p_y >= BOTTOM(tp)) break; \
		do tp = LB(tp); while ((p)->p_y < BOTTOM(tp)); \
	    } \
    }


#endif /* _TILES_H */
