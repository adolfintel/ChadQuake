/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

// mathlib.c -- math primitives

#include "cmdlib.h"
#include "txmathlib.h"

vec3_t tx_vec3_origin = {0,0,0};

#if 1 // Rotating brush support
/*
=================
tx_RadiusFromBounds(NULL);
=================
*/
vec_t tx_RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	vec_t mind, maxd;

	if (mins[0] < mins[1])
		mind = mins[0];
	else
		mind = mins[1];

	if (mins[2] < mind)
		mind = mins[2];

	mind = fabs (mind);

	if (maxs[0] > maxs[1])
		maxd = maxs[0];
	else
		maxd = maxs[1];

	if (maxs[2] > maxd)
		maxd = maxs[2];

	maxd = fabs (maxd);

	if (mind > maxd)
		return mind;

	return maxd;
}
#endif

double tx_VectorLength(vec3_t v)
{
	int		i;
	double	length;

	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}

qboolean tx_VectorCompare (vec3_t v1, vec3_t v2)
{
	int		i;

	for (i=0 ; i<3 ; i++)
		if (fabs(v1[i]-v2[i]) > EQUAL_EPSILON)
			return false;

	return true;
}

vec_t tx_Q_rint (vec_t in)
{
	return floor (in + 0.5);
}

void tx_VectorMA (vec3_t va, double scale, vec3_t vb, vec3_t vc)
{
	vc[0] = va[0] + scale*vb[0];
	vc[1] = va[1] + scale*vb[1];
	vc[2] = va[2] + scale*vb[2];
}

void tx_CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t tx_DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void tx_VectorSubtract (vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0]-vb[0];
	out[1] = va[1]-vb[1];
	out[2] = va[2]-vb[2];
}

void tx_VectorAdd (vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0]+vb[0];
	out[1] = va[1]+vb[1];
	out[2] = va[2]+vb[2];
}

void tx_VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

vec_t tx_VectorNormalize (vec3_t v)
{
	int		i;
	double	length;

	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);
	if (length == 0)
		return 0;

	for (i=0 ; i< 3 ; i++)
		v[i] /= length;

	return length;
}

void tx_VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void tx_VectorScale (vec3_t v, vec_t scale, vec3_t out)
{
	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
}

