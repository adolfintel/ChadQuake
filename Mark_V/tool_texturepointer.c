#ifdef GLQUAKE // GLQUAKE specific

/*
Copyright (C) 2009-2013 Baker

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// tool_texturepointer.c -- Texture pointer



#include "quakedef.h"

cbool texturepointer_on;
// Need this to be global so other things can use it

typedef struct
{
	char			texturename[16]; // WAD sizeof name is 16, so maxlength of a texture is 15.
	gltexture_t*	glt;
	const char *	explicit_name;
	const char *	short_name;
	int				width;
	int				height;
	entity_t*		ent;
	msurface_t*		surf;
	float			distance;
} texturepointer_t;

static texturepointer_t texturepointer;

void TexturePointer_ClipboardCopy (void)
{
	if (texturepointer.glt)
	{
		TexMgr_Clipboard_Set (texturepointer.glt);
	} else Con_PrintLinef ("Could not copy %s to clipboard", texturepointer.explicit_name);

}

void TexturePointer_ClipboardPaste (void)
{
	if (texturepointer.glt)
	{
		int width, height;
		unsigned *data = Clipboard_Get_Image_Alloc (&width, &height);

		if (data)
		{
			int mark = Hunk_LowMark ();
			int endmark;
			TexMgr_ReplaceImage_RGBA (texturepointer.glt, data, width, height);

			endmark = Hunk_LowMark ();
			if (mark != endmark)
				Con_DPrintLinef ("Mark changed was %d now %d", mark, endmark);
			free (data);
		} else Con_PrintLinef ("Clipboard Paste: No 32-bit image data on clipboard");
	} else Con_PrintLinef ("No texture selected for replacement");
}

void TexturePointer_CheckChange (texturepointer_t *test)
{
	// This next IF checks if there is a surface and if the name is different than before ...
	if (test->surf && strcmp (test->surf->texinfo->texture->name, texturepointer.texturename) )
	{
		// Change of texture
//		Con_PrintLinef ("Texture changed from %s to %s", texturepointer.texturename, test->surf->texinfo->texture->name);
		strlcpy (texturepointer.texturename, test->surf->texinfo->texture->name, 16 /* WAD sizeof name */ );
		texturepointer.glt = test->surf->texinfo->texture->gltexture;

		//Con_PrintLinef ("surf dist %f %p", test->surf->plane->dist, test->surf);

		// Is water or lava, redirect to that glt
		if (!texturepointer.glt && test->surf->texinfo->texture->warpimage)
			texturepointer.glt = test->surf->texinfo->texture->warpimage;

		// Probably sky ...
		if (!texturepointer.glt)
		{
			texturepointer.explicit_name	= texturepointer.texturename; //texturepointer.surf->texinfo->texture->name;
			texturepointer.short_name		= texturepointer.texturename;
			texturepointer.width			= texturepointer.surf ? texturepointer.surf->texinfo->texture->width : 0;
			texturepointer.height			= texturepointer.surf ? texturepointer.surf->texinfo->texture->height : 0;

		}
		else
		{
			texturepointer.explicit_name	= texturepointer.glt->name;
			texturepointer.short_name		= String_Skip_Char (texturepointer.explicit_name, ':');
			texturepointer.width			= texturepointer.glt->source_width;
			texturepointer.height			= texturepointer.glt->source_height;
		}

	}
	texturepointer.surf = test->surf;
	texturepointer.ent = test->ent;
}

void Texture_Pointer_f (lparse_t *line)
{
	switch (line->count)
	{
	case 2:
		texturepointer_on = !!atoi(line->args[1]);
		break;
	case 1:
		texturepointer_on = !texturepointer_on;
		break;
	}

	TexturePointer_Reset ();
	Con_PrintLinef ("texture pointer is %s", texturepointer_on ? "ON" : "OFF");
}

static vec3_t collision_spot;

msurface_t *SurfacePoint_NodeCheck_Recursive (mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	vec3_t		mid;
	msurface_t* surf = NULL;

	// RecursiveLightPoint wouldn't exit here, btw.  We do
    // Baker: investigate in future why this can happen ...
	if (!node)
		return NULL; // I think it is because we pass brush models to it
					  // Or maybe because we pass sky and water too?

loc0:
	// didn't hit anything (CONTENTS_EMPTY or CONTENTS_WATER, etc.)
	// Baker: special contents ... I'm not sure this should be a fail here except if contents empty
	// Like do: node->contents == CONTENTS_EMPTY or  CONTENTS_SOLID return;
	// However, seems to work perfect!
	if (node->contents < 0)
		return NULL;		// didn't hit anything

// calculate mid point
	if (node->plane->type < 3)
	{
		front = start[node->plane->type] - node->plane->dist;
		back = end[node->plane->type] - node->plane->dist;
	}
	else
	{
		front = DotProduct(start, node->plane->normal) - node->plane->dist;
		back = DotProduct(end, node->plane->normal) - node->plane->dist;
	}

	// LordHavoc: optimized recursion
	if ((back < 0) == (front < 0))
	{
		node = node->children[front < 0];
		goto loc0;
	}

	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;

// go down front side
	surf = SurfacePoint_NodeCheck_Recursive (node->children[front < 0], start, mid);
	if (surf)
	{
		return surf; // hit something
	}
	else
	{
		// Didn't hit anything so ...

		unsigned int i;
		int		ds, dt;
		surf = cl.worldmodel->surfaces + node->firstsurface;

		// check for impact on this node
		// Baker: Apparently we need this if the for loop below fails
		VectorCopy (mid, collision_spot);

		for (i = 0; i < node->numsurfaces; i++, surf++)
		{
			// light would check if SURF_DRAWTILED (no lightmaps), but we want for texture pointer
			//if (surf->flags & SURF_DRAWTILED)
			//	continue; // no lightmaps

			ds = (int) ((float) DotProduct (mid, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3]);
			dt = (int) ((float) DotProduct (mid, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3]);

			if (ds < surf->texturemins[0] || dt < surf->texturemins[1])
				continue;  // out of range

			ds -= surf->texturemins[0];
			dt -= surf->texturemins[1];

			if (ds > surf->extents[0] || dt > surf->extents[1])
				continue; // out of range

			// At this point we have a collision with this surface.
			// Set return variables
			VectorCopy (mid, collision_spot);
			return surf; // success
		}

		// go down back side
		return SurfacePoint_NodeCheck_Recursive (node->children[front >= 0], mid, end);
	}
}


static texturepointer_t SurfacePoint (vec3_t startpoint, vec3_t endpoint)
{
	float collision_distance;
	texturepointer_t best = {0};
	int			i;

	msurface_t*	collision_surf = SurfacePoint_NodeCheck_Recursive (cl.worldmodel->nodes, startpoint, endpoint);

	if (collision_surf)
	{
		collision_distance	= DistanceBetween2Points (startpoint, collision_spot);

		best.ent		= NULL;
		best.surf		= collision_surf;
		best.distance	= collision_distance;
	}

	// Now check for hit with world submodels
	for (i = 0 ; i < cl.numvisedicts ; i ++)	// 0 is player.
	{
		// Note that this ONLY collides with visible entities!
		entity_t	*pe = cl.visedicts[i];
		vec3_t		adjusted_startpoint, adjusted_endpoint, adjusted_net;

		if (!pe->model)
			continue;   // no model for ent

		if (!(pe->model->surfaces == cl.worldmodel->surfaces))
			continue;	// model isnt part of world (i.e. no health boxes or what not ...)

		// Baker: We need to adjust the point locations for entity origin

		VectorSubtract (startpoint, pe->origin, adjusted_startpoint);
		VectorSubtract (endpoint,   pe->origin, adjusted_endpoint);
		VectorSubtract (startpoint, adjusted_startpoint, adjusted_net);

		// Make further adjustments if entity is rotated
		if (pe->angles[0] || pe->angles[1] || pe->angles[2])
		{
			vec3_t f, r, u, temp;
			AngleVectors(pe->angles, f, r, u);	// split entity angles to forward, right, up

			VectorCopy(adjusted_startpoint, temp);
			adjusted_startpoint[0] = DotProduct(temp, f);
			adjusted_startpoint[1] = -DotProduct(temp, r);
			adjusted_startpoint[2] = DotProduct(temp, u);

			VectorCopy(adjusted_endpoint, temp);
			adjusted_endpoint[0] = DotProduct(temp, f);
			adjusted_endpoint[1] = -DotProduct(temp, r);
			adjusted_endpoint[2] = DotProduct(temp, u);
		}

		collision_surf = SurfacePoint_NodeCheck_Recursive (pe->model->nodes+pe->model->hulls[0].firstclipnode /*pe->model->nodes*/, adjusted_startpoint, adjusted_endpoint);

		if (collision_surf)
		{
			// Baker: We have to add the origin back into the results here!
			VectorAdd (collision_spot, adjusted_net, collision_spot);

			collision_distance = DistanceBetween2Points (startpoint, collision_spot);

			if (!best.surf || collision_distance < best.distance)
			{
				// New best
				best.ent		= pe;
				best.surf		= collision_surf;
				best.distance	= collision_distance;
			}

		}
		// On to next entity ..
	}

	return best;
}


// Determine start and end test and run function to get closest collision surface.
texturepointer_t TexturePointer_SurfacePoint (void)
{
	vec3_t startingpoint, endingpoint, forward, up, right;

	// r_refdef.vieworg/viewangles is the camera position
	VectorCopy (r_refdef.vieworg, startingpoint);

	// Obtain the forward vector
	AngleVectors (r_refdef.viewangles, forward, right, up);

	// Walk it forward by 4096 units
	VectorMA (startingpoint, 4096, forward, endingpoint);

	// There is no assurance anything will be hit (i.e. noclip outside map looking at void)
	return SurfacePoint (startingpoint, endingpoint);
}

void TexturePointer_Init (void)
{
	Cmd_AddCommands (TexturePointer_Init);
}


void TexturePointer_Reset (void)
{
	memset (&texturepointer, 0, sizeof(texturepointer_t) );
}




void TexturePointer_Draw (void)
{
	if (texturepointer_on && cls.signon == SIGNONS && cl.worldmodel && texturepointer.surf)
	{
		const char *drawstring1 = va("\bTexture:\b %s", texturepointer.short_name);
		const char *drawstring2 = va("\b  %d x %d px", texturepointer.width, texturepointer.height);

		//int len = strlen(drawstring);
#pragma message ("You could canvas this, maybe it should be conscaled")
		int x = (clwidth  / 2) - 120;
		int y = (clheight / 2) - 120;

		Draw_SetCanvas (CANVAS_DEFAULT);

		Draw_StringEx (x, y + 0, drawstring1);
		Draw_StringEx (x, y + 8, drawstring2);

	}
}

void TexturePointer_Think (void)
{
	texturepointer_t test;
	if (!texturepointer_on || !cl.worldmodel || cls.signon < SIGNONS)
		return;

	test = TexturePointer_SurfacePoint ();

	if (test.surf)
	{
		//const rgba4_t linecolor = {1,1,1,1};
		rgba4_t color = {1, 0, 0, sin(realtime * 3) * 0.125f + 0.25};
		TexturePointer_CheckChange (&test);

		R_EmitSurfaceHighlight (texturepointer.ent ,texturepointer.surf, color, FILLED_POLYGON);
	}

}

#pragma message ("Baker: Get texturepointer to store caches so you can re-copy to clipboard.  I thought we had this once")
#pragma message ("Baker: Is there an issue with Mac Quake reading the config if it is, say, Quakespasm's?  Are we reading/writing it to caches ok?")

/* I think they should write a permanent texturepointer folder"

void TexturePointer_ClearCaches_NewMap (void)
{
	// Get a list of the files using dirent
//	COM_DeleteCacheFiles (com_whatever); // Deletes the caches


}

*/

#endif // GLQUAKE specific