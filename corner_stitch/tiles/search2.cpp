/*
 * search2.c --
 *
 * Area searching.
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
#include "../utils/magic.h"
#include "tile.h"

/*
 * --------------------------------------------------------------------
 *
 * tiSrAreaEnum --
 *
 * Perform the recursive edge search of the tile which has just been
 * enumerated in an area search.  The arguments passed are the RT
 * corner stitch and bottom coordinate of the tile just enumerated.
 *
 * Results:
 *	0 is returned if the search completed normally, 1 if
 *	it was aborted.
 *
 * Side effects:
 *	Attempts to enumerate recursively each tile found in walking
 *	along the right edge of the tile just enumerated.  Whatever
 *	side effects occur result from the application of the client's
 *	filter function.
 *
 * --------------------------------------------------------------------
 */

int tiSrAreaEnum(Tile* enumRT, double enumBottom, Rect* rect, int (*func)(Tile *tile, ClientData cdata), ClientData arg){
    Tile *tp, *tpLB, *tpTR;
    double tpRight, tpNextTop, tpBottom, srchBottom;
    int atBottom = (enumBottom <= rect->r_ll.p_y);


    /*
     * Begin examination of tiles along right edge.
     * A tile to the right of the one being enumerated is enumerable if:
     *	- its bottom lies at or above that of the tile being enumerated, or,
     *	- the bottom of the tile being enumerated lies at or below the
     *	  bottom of the search rectangle.
     */

    if ((srchBottom = enumBottom) < rect->r_ll.p_y)
	srchBottom = rect->r_ll.p_y;

    for (tp = enumRT, tpNextTop = TOP(tp); tpNextTop > srchBottom; tp = tpLB)
    {
	/*
	 * Since the client's filter function may result in this tile
	 * being deallocated or otherwise modified, we must extract
	 * all the information we will need from the tile before we
	 * apply the filter function.
	 */

	tpLB = LB(tp);
	tpNextTop = TOP(tpLB);	/* Since TOP(tpLB) comes from tp */

	if (BOTTOM(tp) < rect->r_ur.p_y && (atBottom || BOTTOM(tp) >= enumBottom))
	{
	    /*
	     * We extract more information from the tile, which we will use
	     * after applying the filter function.
	     */

	    tpRight = RIGHT(tp);
	    tpBottom = BOTTOM(tp);
	    tpTR = TR(tp);
	    if ((*func)(tp, arg)) return 1;

	    /*
	     * If the right boundary of the tile being enumerated is
	     * inside of the search area, recursively enumerate
	     * tiles to its right.
	     */

	    if (tpRight < rect->r_ur.p_x)
		if (tiSrAreaEnum(tpTR, tpBottom, rect, func, arg))
		    return 1;
	}
    }
    return 0;
}

/*
 * --------------------------------------------------------------------
 *
 * TiSrArea --
 *
 * Find all tiles contained in or incident upon a given area.
 * Applies the given procedure to all tiles found.  The procedure
 * should be of the following form:
 *
 *	int
 *	func(tile, cdata)
 *	    Tile *tile;
 *	    ClientData cdata;
 *	{
 *	}
 *
 * Func normally should return 0.  If it returns 1 then the search
 * will be aborted.
 *
 * THIS PROCEDURE IS OBSOLETE EXCEPT FOR THE SUBCELL PLANE.  USE
 * DBSrPaintArea() IF YOU WANT TO SEARCH FOR PAINT TILES.
 *
 * Results:
 *	0 is returned if the search completed normally.  1 is returned
 *	if it aborted.
 *
 * Side effects:
 *	Whatever side effects result from application of the
 *	supplied procedure.
 *
 * NOTE:
 *	The procedure called is free to do anything it wishes to tiles
 *	which have already been visited in the area search, but it must
 *	not affect anything about tiles not visited other than possibly
 *	corner stitches to tiles already visited.
 *
 * *************************************************************************
 * *************************************************************************
 * ****									****
 * ****				  WARNING				****
 * ****									****
 * ****		This code is INCREDIBLY sensitive to modification!	****
 * ****		Change it only with the utmost caution, or you'll	****
 * ****		be verrry sorry!					****
 * ****									****
 * *************************************************************************
 * *************************************************************************
 *
 * --------------------------------------------------------------------
 */

int TiSrArea(Tile* hintTile, Plane* plane, Rect* rect, int (*func)(Tile *tile, ClientData cdata), ClientData arg){
    Point here;
    Tile *tp, *enumTR, *enumTile;
    double enumRight, enumBottom;

    /*
     * We will scan from top to bottom along the left hand edge
     * of the search area, searching for tiles.  Each tile we
     * find in this search will be enumerated.
     */

    here.p_x = rect->r_ll.p_x;
    here.p_y = rect->r_ur.p_y - 1;
    enumTile = hintTile ? hintTile : plane->pl_hint;
    GOTOPOINT(enumTile, &here);
    plane->pl_hint = enumTile;

    while (here.p_y >= rect->r_ll.p_y)
    {
	/*
	 * Find the tile (tp) immediately below the one to be
	 * enumerated (enumTile).  This must be done before we enumerate
	 * the tile, as the filter function applied to enumerate
	 * it can result in its deallocation or modification in
	 * some other way.
	 */
	here.p_y = BOTTOM(enumTile) - 1;
	tp = enumTile;
	GOTOPOINT(tp, &here);
	plane->pl_hint = tp;

	enumRight = RIGHT(enumTile);
	enumBottom = BOTTOM(enumTile);
	enumTR = TR(enumTile);
	if ((*func)(enumTile, arg)) return 1;

	/*
	 * If the right boundary of the tile being enumerated is
	 * inside of the search area, recursively enumerate
	 * tiles to its right.
	 */

	if (enumRight < rect->r_ur.p_x)
	    if (tiSrAreaEnum(enumTR, enumBottom, rect, func, arg))
		return 1;
	enumTile = tp;
    }
    return 0;
}
