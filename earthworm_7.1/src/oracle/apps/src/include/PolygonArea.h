/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: PolygonArea.h,v 1.1 2001/07/01 21:55:17 davidk Exp $
 *    Revision history:
 *
 *    $Log: PolygonArea.h,v $
 *    Revision 1.1  2001/07/01 21:55:17  davidk
 *    Initial revision
 *
 *
 *
 *
 *
 *****************************************************************/


int PolygonArea(int n, float *x, float *y, float *u, float *v);
/************************************************************************
 * PolygonArea:
 *    Function calculates whether a point(u,v) lies inside the 
 *    polygon defined by arrays x and y.  If the point lays inside, then
 *    the function returns TRUE, otherwise the function returns FALSE.
 *  
 *
 * input params:
 * 	n - number of sides to the polygon 
 * 	   ( must be less or equal to 20) 
 * 	x - array of dimension 21, contains n+1 x coordinates of polygon 
 * 	y - array of dimension 21, contains n+1 y coordinates of polygon 
 * 	  *** note *** the first point of the polygon is duplicated 
 *         in location n+1 of the x and y arrays. 
 * 	u - x coordinate of point to check 
 * 	v - y coordinate of point to check 
 *
 ************************************************************************/
