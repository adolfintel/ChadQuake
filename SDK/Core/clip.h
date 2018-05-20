/*
Copyright (C) 2012-2014 Baker

*/
// clip.h -- Clipping functions

#ifndef __CLIP_H__
#define __CLIP_H__

// Name sucks and I think functions won't be all clipping.  area.h?

typedef struct
{
	int		left, top, width, height; // Why didn't we choose x, y, w, h?
} crect_t;

typedef struct
{
	int		left, top, width, height, right, bottom; // Why didn't we choose x, y, w, h?
} crectrb_t;

crect_t crect_t_make (int x, int y, int w, int h);
crect_t crect_t_centered (int w, int h, crect_t area);
#define crect_hit(rect, x, y) (rect).left <= (x) && (x) < (rect).left + (rect).width && (rect).top <= (y) && (y) < (rect).top + (rect).height

typedef struct
{
	int		left, top, right, bottom; // Why didn't we choose x, y, w, h?
} crectcoord_t;

const char *vp_str_crect (void *v, int n);
//extern vt_struct_def crect_t_def;
int crect_t_print (void *v, printline_fn_t my_printline);

#define CRECT_PRINTSTR(prect) va("%d, %d @ %d x %d (%d, %d) - (%d, %d)", (prect)->left, (prect)->top, (prect)->width, (prect)->height, (prect)->left, (prect)->top, (prect)->left + (prect)->width - 1, (prect)->top + (prect)->height - 1 )
//Example: my_print ("%-20.20s %c", CRECT_PRINTSTR(&b->hitbox[n].rect), caption[hb->n] );


///////////////////////////////////////////////////////////////////////////////
//  RECT: BASE
///////////////////////////////////////////////////////////////////////////////

cbool Rect_Point_In_Rect_Long_Form (int x, int y, int left, int top, int width, int height);
cbool Rect_Point_Is_Inside (crect_t *rect, int x, int y);
void Rect_Set (crect_t *dst, int left, int top, int width, int height);

///////////////////////////////////////////////////////////////////////////////
//  RECT: EXPAND (Or if provided zeromemory rect, effectively creates it)
///////////////////////////////////////////////////////////////////////////////

// Provide any of these function a zeromemory rect and it will build the rect from the input.
// If the rect isn't zeromemory, it will expand it.
void Rect_Expand_Point (crect_t *rect, int x, int y, cbool *did_change);
void Rect_Expand (crect_t *rect, int x, int y, int w, int h, cbool *did_expand);
void Rect_Expand_Points_Array (crect_t *rect, int num_points, int *points, cbool *did_expand);
//void Rect_Expand_Points_Variadic (crect_t *rect, int num_points, ...); // Did_expand is final param


void Rect_Expand_Points_Variadic (crect_t *rect, cbool *did_expand, int num_points, ...);

void Rect_Expand_Triangle (crect_t *rect, int x0, int y0, int x1, int y1, int x2, int y2, cbool *did_expand);

///////////////////////////////////////////////////////////////////////////////
//  RECT: CLIP
///////////////////////////////////////////////////////////////////////////////

cbool Rect_Clip_Dest_Permitted (crect_t *dst, const crect_t *permitted, cbool *did_clip);
cbool Rect_Clip_Dest_Source_Permitted (crect_t *dst, crect_t *src, const crect_t *permitted, cbool *did_clip);


cbool Rect_Clip (int *dst_x, int *dst_y, int *dst_w, int *dst_h,
							 int *src_x, int *src_y, int *src_w, int *src_h,
							const int *permitted_x, const int *permitted_y, const int *permitted_w, const int *permitted_h,
							cbool *did_clip);



double *Polygon_Regular_Make_DPoints_Alloc (int num_sides, double center_x, double center_y, double radius);


void Rect_Set (crect_t *dst, int left, int top, int width, int height);

///////////////////////////////////////////////////////////////////////////////
//  HITBOXES:  box_item_t extends crect_t.  box_item_t isn't unique to hitboxes.
//             hitboxes happen to use a box_item_t and any box_item_t array should
//             be able to use it.
///////////////////////////////////////////////////////////////////////////////


typedef struct
{
	crect_t	rect;

	int		itemdata;		// index -- don't have to use one.

	int		render_type;
	int		partnum;		// render type


} box_item_t; // Extends crect_t




#endif // __CLIP_H__


