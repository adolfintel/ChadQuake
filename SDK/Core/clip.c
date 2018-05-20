/*
Copyright (C) 2012-2014 Baker

*/
// clip.c -- image manipulation and loading

#define CLIP_LOCAL
#include "core.h"
#include "clip.h" // Courtesy

///////////////////////////////////////////////////////////////////////////////
//  RECT: EXPAND
///////////////////////////////////////////////////////////////////////////////

#define CLIP_LOCAL

void Rect_Test (void)
{
	crect_t zerorect = {0}, bob; cbool did_change = 8;

	bob = zerorect;
	Rect_Expand_Point (&bob, 20, 20, NULL);
	Rect_Expand_Point (&bob, 25, 25, NULL);

	bob = zerorect;
	Rect_Expand (&bob, 20, 20, 25, 25, &did_change);

	bob = zerorect;
	Rect_Expand_Triangle (&bob, 20, 20, 25, 25, 6, 55, &did_change);


	bob = zerorect;

}

void Rect_Expand_Point (crect_t *rect, int x, int y, cbool *did_change)
{
#pragma message ("This could be better optimized ...")
	int rect_x2b = rect->left + rect->width, rect_y2b = rect->top + rect->height;
	cbool expanded /*pure assigned later*/, expanded_x = false, expanded_y = false;

//#ifdef _DEBUG
	crect_t new_rect = *rect;
	int new_rect_x2b = rect_x2b, new_rect_y2b = rect_y2b;
//#endif

	// We will use rect->width of 0 as not-rect litmus test.
	if (rect->width < 0)
		log_fatal ("Negative width");

	if (!rect->width)				expanded_x = true, new_rect.left = x, new_rect_x2b = x + 1;
	else if ( x < rect->left)		expanded_x = true, new_rect.left = x;	// By definition, it can't be both unless width is 0
	else if ( rect_x2b <= x)		expanded_x = true, new_rect_x2b  = x + 1; // or negative and we handle those (and negative shouldn't happen)
	//else return false; // If we spin this out to a dimension function we could return here.

	if (expanded_x)
		new_rect.width = new_rect_x2b - new_rect.left; //, rect->width = new_rect.width;


	if (!rect->height)				expanded_y = true, new_rect.top = y, new_rect_y2b = y + 1;
	else if ( y < rect->top)		expanded_y = true, new_rect.top = y;	// By definition, it can't be both unless height is 0
	else if ( rect_y2b <= y)		expanded_y = true, new_rect_y2b  = y + 1; // or negative and we handle those (and negative shouldn't happen)
	//else return false; // If we spin this out to a dimension function we could return here.

	if (expanded_y)
		new_rect.height = new_rect_y2b - new_rect.top; //, rect->height = new_rect.height;


	expanded = expanded_x || expanded_y;
	if (expanded)
	{
		*rect = new_rect;
		if (did_change)
		{
	//#define CRECT_PRINTSTR(prect) va("%d, %d (%d x %d)", (prect)->left, (prect)->top, (prect)->width, (prect)->height )
	//Example: my_print ("%-20.20s %c", CRECT_PRINTSTR(&b->hitbox[n].rect), caption[hb->n] );
			*did_change = expanded; // Perhaps in future we increment this +1 instead of just setting 1.
		}
//		logd ("Rect expanded to: %s x2 y2", CRECT_PRINTSTR(rect));
	}
}


void Rect_Expand (crect_t *rect, int x, int y, int w, int h, cbool *did_expand)
{
#if 0 // Works fine ...
	Rect_Expand_Points_Variadic (rect, did_expand, 2, x, y, x + w - 1, y + h - 1);
#else
	Rect_Expand_Point (rect, x, y, did_expand);
	Rect_Expand_Point (rect, x + w - 1, y + h - 1, did_expand);
#endif
}

// Polygon
void Rect_Expand_Points_Array (crect_t *rect, int num_points, int *points, cbool *did_expand)
{
	// First is number.
	int n;
	for (n = 0; n < num_points; n ++)
	{
		int x = points[n*2 + 0];
		int y = points[n*2 + 1];
		Rect_Expand_Point (rect, x, y, did_expand);
	}
}

#pragma message ("Make this use a sentinel or something.  But considering it needs double NULL terminated ...")
void Rect_Expand_Points_Variadic (crect_t *rect, cbool *did_expand, int num_points, ...)
{
	int *points = calloc (num_points * 2, sizeof(int)); //{ 3, x0, y0, x1, y1, x2, y2 };

	{
		va_list ap;
		int n;

		va_start (ap, num_points); // Initialize

		for (n = 0; n < num_points; n++)
			points[n*2 + 0] = va_arg (ap, int), points[n*2 + 1] = va_arg (ap, int);

		va_end (ap); // Shutdown
	}

	Rect_Expand_Points_Array (rect, num_points, points, did_expand);

	free (points);  // Heap crash here?  Hmmm..
}


void Rect_Expand_Triangle (crect_t *rect, int x0, int y0, int x1, int y1, int x2, int y2, cbool *did_expand)
{
	//crect_t out_rect = {0};
#if 0 // Works fine ...
	int points[] = { x0, y0, x1, y1, x2, y2 };
	Rect_Expand_Points_Array (rect, 3, points, did_expand);
#else
	Rect_Expand_Point (rect, x0, y0, did_expand);
	Rect_Expand_Point (rect, x1, y1, did_expand);
	Rect_Expand_Point (rect, x2, y2, did_expand);
#endif
}




///////////////////////////////////////////////////////////////////////////////
//  RECT: CLIP
///////////////////////////////////////////////////////////////////////////////

// Contract the rect to the permitted region.  Returns true if it culled entirely.
cbool Rect_Clip_Dest_Permitted (crect_t *dst, const crect_t *permitted, cbool *did_clip)
{
	return Rect_Clip (&dst->left, &dst->top, &dst->width, &dst->height, NULL, NULL, NULL, NULL,
		&permitted->left, &permitted->top, &permitted->width, &permitted->height, did_clip);
}

// Contract the dest and source to the permitted region.  Returns true if it culled entirely.
// Intented for paste operations.  (Can any non-paste function ever use this?)
cbool Rect_Clip_Dest_Source_Permitted (crect_t *dst, crect_t *src, const crect_t *permitted, cbool *did_clip)
{
	return Rect_Clip (&dst->left, &dst->top, &dst->width, &dst->height, &src->left, &src->top, &src->width, &src->height,
		&permitted->left, &permitted->top, &permitted->width, &permitted->height, did_clip);

}



cbool Rect_Clip (int *dst_x, int *dst_y, int *dst_w, int *dst_h,
							 int *src_x, int *src_y, int *src_w, int *src_h,
							const int *permitted_x, const int *permitted_y, const int *permitted_w, const int *permitted_h,
							cbool *did_clip)
{
#pragma message ("This could be optimized more and a little clearer")

	int permitted_x2b = *permitted_x + *permitted_w;
	int permitted_y2b = *permitted_y + *permitted_h;
	int dst_x2b = *dst_x + *dst_w;
	int dst_y2b = *dst_y + *dst_h;

	if (*dst_x >= permitted_x2b) return false; // dst_x beyond width (cull)
	if (*dst_y >= permitted_y2b) return false; // dst_y beyond height (cull)
	if (dst_x2b <= *permitted_x) return false; // x2 doesn't reach 0 (cull)
	if (dst_y2b <= *permitted_y) return false; // x2 doesn't reach 0 (cull)
	{
		int original_x = *dst_x, original_y = *dst_y, original_w = *dst_w, original_h = *dst_h;
		cbool clipped = false;

		int x1 = CLAMP(*permitted_x, *dst_x, permitted_x2b - 1);
		int y1 = CLAMP(*permitted_y, *dst_y, permitted_y2b - 1);
		int x2 = CLAMP(*permitted_x, dst_x2b - 1, permitted_x2b - 1);
		int y2 = CLAMP(*permitted_y, dst_y2b - 1, permitted_y2b - 1);

		// Note: This recalculation is a waste if didn't clip.
		*dst_w = x2 - x1 + 1;
		*dst_h = y2 - y1 + 1;
		*dst_x = x1;
		*dst_y = y1;

		clipped = !(original_x == *dst_x && original_y == *dst_y && original_w == *dst_w && original_h == *dst_h);
		if (clipped)
		{
			if (src_x)	*src_x += x1 - original_x;
			if (src_y)	*src_y += y1 - original_y;
			if (src_w)	*src_w += *dst_w  - original_w;
			if (src_h)	*src_h += *dst_h  - original_h;

			if (did_clip) *did_clip = true;
//#ifdef _DEBUG
				logd ("Clipped: start xy %d, %d (%d dst_x %d)"
			                 " clipped to xy %d, %d (%d dst_x %d) canvas size (%d dst_x %d)",
				original_x, original_y, original_w, original_h, *dst_x, *dst_y, *dst_w, *dst_h, *permitted_w, *permitted_h);
//#endif
		}
	}
	return true; // Not culled.
}

//#ifndef M_PI
//#define M_PI           3.14159265358979323846
//#endif


void Circle_Get_Point_Degrees (double *out_x, double *out_y, double degrees)
{
	const double radians = DEGREES_TO_RADIANS(degrees);
	*out_x = cosd(degrees); // cos(radians);
	*out_y = sind(degrees); // (radians);
}

double *Polygon_Regular_Make_DPoints_Alloc (int num_sides, double center_x, double center_y, double radius)
{
	double *points = calloc (num_sides * 2, sizeof(double) );
	double slice = (360.0 / num_sides);
	int n;
	for (n = 0; n < num_sides; n ++)
	{
		double angle_degrees = slice * n;
		double x, y;
		Circle_Get_Point_Degrees (&x, &y, angle_degrees);
		points[n * 2 + 0] = x * radius + center_x;
		points[n * 2 + 1] = y * radius + center_y;
	}

	return points;
}

#if 0
// Point2D?  Hmmm.
int Polygon_IsClockwise (int num_sides, double *dpoints)
{
	int n;
    double sum = 0;

    for (n = 0; n < num_sides; n++) {
		int next = (n + 1) % num_sides; // Last element will wrap to 0
        int x1 = points[n*2 + 0], y1 = points[n*2 + 1];
		int x1 = points[next*2 + 0], y1 = points[next*2 + 1];
        sum += (x2 - x1) * (y2 + y1);
    }
    return sum > 0;
}
#endif

/*
int *Polygon_Regular_Make_Points_Int_Alloc (int num_sides, int center_x, int center_y, int radius)
{
	double *dpoints = Polygon_Regular_Make_Points_Alloc (num_sides, center_x, center_y, radius);
	double *points = calloc (num_sides * 2, sizeof(double) );

	return points;
}
*/


///////////////////////////////////////////////////////////////////////////////
//  RECT: BASE
///////////////////////////////////////////////////////////////////////////////


cbool Rect_Point_In_Rect_Long_Form (int x, int y, int left, int top, int width, int height)
{
	if ( x < left || left + width  <= x ||
	     y < top  || top  + height <= y) return false;
	return true;
}


cbool Rect_Point_Is_Inside (crect_t *rect, int x, int y)
{
	return Rect_Point_In_Rect_Long_Form (x, y, rect->left, rect->top, rect->width, rect->height);
}

void Rect_Set (crect_t *dst, int left, int top, int width, int height)
{
	memset (dst, 0, sizeof(*dst));
	dst->left = left, dst->top = top, dst->width = width, dst->height = height;
}


crect_t crect_t_make (int x, int y, int w, int h)
{
	crect_t rect = {x, y, w, h};
	return rect;
}

crect_t crect_t_centered (int w, int h, crect_t area)
{
	int offset_x = (area.width  - w) / 2;
	int offset_y = (area.height - h) / 2;
	crect_t rect = {area.left + offset_x, area.top + offset_y, w, h};
	return rect;
}

