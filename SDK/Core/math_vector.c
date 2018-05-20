/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2009-2014 Baker and others

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
// math_vector.c -- vector3 functions

#include "core.h" // cbool may be only reason we need this
#include "math_vector.h" // Courtesy



//johnfitz -- the opposite of AngleVectors.  this takes forward and generates pitch yaw roll
//TODO: take right and up vectors to properly set yaw and roll
void VectorAngles (const vec3_t forward, vec3_t angles)
{
	vec3_t temp;

	temp[0] = forward[0];
	temp[1] = forward[1];
	temp[2] = 0;
	angles[Q_PITCH] = -atan2(forward[2], VectorLength(temp)) / M_PI_DIV_180;
	angles[Q_YAW] = atan2(forward[1], forward[0]) / M_PI_DIV_180;
	angles[Q_ROLL] = 0;
}


void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = angles[Q_YAW] * (M_PI * 2.0 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[Q_PITCH] * (M_PI * 2.0 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[Q_ROLL] * (M_PI * 2.0 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward) {
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}

	if (right) {
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up) {
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}


void VectorToAngles (const vec3_t vec, vec3_t ang)
{
	float	forward, yaw, pitch;

	if (!vec[1] && !vec[0])
	{
		yaw = 0;
		pitch = (vec[2] > 0) ? 90 : 270;
	}
	else
	{
		yaw = atan2 (vec[1], vec[0]) * 180 / M_PI;
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (vec[0]*vec[0] + vec[1]*vec[1]);
		pitch = atan2 (vec[2], forward) * 180 / M_PI;
		if (pitch < 0)
			pitch += 360;
	}

	ang[0] = pitch;
	ang[1] = yaw;
	ang[2] = 0;
}



cbool VectorCompare (const vec3_t v1, const vec3_t v2)
{
	int		i;

	for (i=0 ; i<3 ; i++)
		if (v1[i] != v2[i])
			return false;

	return true;
}

// Multiply Add
void VectorMA (const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


float _DotProduct (const vec3_t v1, const vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract (const vec3_t veca, const vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd (const vec3_t veca, const vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy (const vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

// Formerly named CrossProduct
void VectorCrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
//	return cross;
}

vec_t VectorLength(const vec3_t v)
{
	return sqrt(DotProduct(v,v));
}

float VectorNormalize (vec3_t v)
{
	float	length, ilength;

	length = sqrt(DotProduct(v,v));

	if (length)
	{
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;

}

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale (const vec3_t in, const vec_t scale, vec3_t out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

vec_t DistanceBetween2Points (const vec3_t v1, const vec3_t v2)
{
	vec3_t	v3;
	float	length;

	VectorSubtract (v1, v2, v3);

	length = v3[0] * v3[0] + v3[1] * v3[1] + v3[2] * v3[2];
	return sqrt (length);
}

void VectorAverage (const vec3_t v1, const vec3_t v2, vec3_t out)
{
	VectorAdd (v1, v2, out);
	out[0] /= 2;
	out[1] /= 2;
	out[2] /= 2;
}

void VectorExtendLimits (const vec3_t newvalue, vec3_t minlimit, vec3_t maxlimit)
{
	int i;

	for (i=0; i<3; i++)
	{
		if (newvalue[i] < minlimit[i])	minlimit[i] = newvalue[i];
		if (newvalue[i] > maxlimit[i])	maxlimit[i] = newvalue[i];
	}

}


// Baker: Very simplisitic, can't have angles.  Returns true is the point in the cube
cbool PointInCube (vec3_t point, vec3_t cube_mins, vec3_t cube_maxs)
{
	if (cube_mins[0] <= point[0] && point[0] <= cube_maxs[0])
		if (cube_mins[1] <= point[1] && point[1] <= cube_maxs[1])
			if (cube_mins[2] <= point[2] && point[2] <= cube_maxs[2])
				return true;

	return false;
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct( normal, normal );

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}


/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabs( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}


// Ascending Y can affect this.
// For a model this will be futile so don't bother even though it is tempting ...
// You would have to figure out a point inside the model to act as an inside reference point.
// Because the ones facing away should be opposite clockwiseness.  So you'd need to an inside the model perspective
// To determine verts that are intended to be front/back, otherwise it's just triangle soup.
// But if we were to try to normalize from the perspective of a viewer, say 0,0,0, what would we do?

int is_clockwise_2d (float x1, float y1, float x2, float y2, float x3, float y3, cbool is_screen_maclike_descending_y)
{
    float sum1 = (x2-x1)*(y2+y1);   //(x2-x1)(y2+y1)
    float sum2 = (x3-x2)*(y3+y2);
    float sum3 = (x1-x3)*(y1+y3);
    float sumsum = sum1 + sum2 + sum3;
    printlinef ("%g %g %g = %g", sum1, sum2, sum3, sumsum);
    
    if (is_screen_maclike_descending_y) {
        if (sumsum < 0) return -1; // Standard formula
        if (sumsum > 0) return 1;  // Standard formula
        
    else {} {}
        // We have to do the opposite of expected.
        if (sumsum < 0) return 1;
        if (sumsum > 0) return -1;
    }
    return 0;
}

void cossinf (float angle, float *mycos, float *mysin)
{
	     if (angle ==   0 )					*mycos =  1, *mysin =  0;
	else if (angle ==  90 )					*mycos =  0, *mysin =  1;
	else if (angle == 180 )					*mycos = -1, *mysin =  0;
	else if (angle == 270 || angle == -90 ) *mycos =  0, *mysin = -1;
	else { 
		float radians = DEGREES_TO_RADIANS (angle);
		*mycos = cos (radians), *mysin = sin(radians);
	}
}

void ZAngleVectorsFast (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		sp, sy, cp, cy;
	float cr =  1, sr =  0; // The calculations with roll are wrong, we will use 0.

	cossinf (angles[0], &cp, &sp); // PITCH
	cossinf (angles[2], &cy, &sy); // YAW
	
	forward[0] = cp*sy; // Passes small yaw.  Passes small pitch.  Seems to pass smal yaw + pitch.
	forward[1] = cp*cy;
	forward[2] = sp;
	if (right) {
		right[0] =  cy; // Passes small yaw.  Passes small pitch.
		right[1] = -sy; //
		right[2] = 0; // <--- this doesn't seem to be right for pitch and yaw.  Maybe it is.
	}
	if (up) {
		up[0] = sp * sy; // Passes small yaw.  Passes small pitch.  Negative SP instead?
		up[1] = sp *-cy;
		up[2] = cp;
	}

#ifdef DEBUG // I trust the other 100% ... this function, I want it to prove itself a bit.  CORRECTION: Both flawed.
//	alert ("ZAngleVectorsFast: %s", _angles4 (angles, forward, right, up));
	{	vec3_t forward2, right2, up2; float *fs1 = NULL, *fs2 = NULL; const char *badz= NULL;
		vec3_t test = {angles[0], 0, angles[2] };
		ZAngleVectors (test, forward2, right2, up2);
		
		if		(!VectorCompare (forward, forward2))	fs1= forward, fs2 = forward2, badz="forward";
		else if	(!VectorCompare (up, up2))				fs1= up, fs2 = up2, badz="up";
		else if	(!VectorCompare (right, right2))		fs1= right, fs2 = right2, badz="right";
		//if (badz && (fabs(fs1[0] - fs2[0])>0.01 || fabs(fs1[1] - fs2[1])>0.01 || fabs(fs1[2] - fs2[2])>0.01))
		//{
		//	alert ("ZAngleVectorsFast check fail for " QUOTED_S " on Angles = (pitch %g, ROLL %g, YAW %g\"
		//		"ZAngleVectorsFast answer is (%g %g %g)" NEWLINE
		//		
		//		"Checker says %g %g %g" NEWLINE
		//		"Deltas are %g %g %g", badz, test[0], test[1], test[2], fs1[0], fs1[1], fs1[2], fs2[0], fs2[1], fs2[2],
		//			fabs(fs1[0] - fs2[0]), fabs(fs1[1] - fs2[1]), fabs(fs1[2] - fs2[2]));
		//}
	}
#endif

}

double _cosd(double degrees) { return cosd(degrees); }
double _sind(double degrees) { return sind(degrees); }


void ZAngleTest (void)
{

	struct {vec3_t forward, right, up;} t1 = {0};
	//struct {vec3_t forward, right, up;} t2 = {0};
	vec3_t angles = {0}; int n =0;

	{
		void CompareFloat16 (float *a, float *b);
		glmatrix bob = {0}, bob2 = {0};
		Mat4_Identity_Set (&bob);				Mat4_Identity_Set (&bob2);
		Mat4_Rotate	(&bob,  15, 1, 0, 0);/*pitch*/		Mat4_Rotate	(&bob2,  30, 0, 1, 0); /*yaw*/
		Mat4_Rotate	(&bob,  30, 0, 1, 0);/*yaw*/		Mat4_Rotate	(&bob2,  15, 1, 0, 0); /*pitch*/
		Mat4_Translate	(&bob, 0,1,0);			Mat4_Translate	(&bob2, 0,1,0);	
		alert ("%s vs. %s", Mat4_String (&bob), Mat4_String (&bob2));
		alert ("%s vs. %s", _angles (&bob.m16[12]), _angles (&bob2.m16[12]));
		exit (0);
		Mat4_Translate	(&bob2, 1,1,1);
		Mat4_Compare (&bob, &bob2);
		Mat4_Compare (&bob, &bob2);
		// Apparently the rotate order matters a great deal.		

	}


	//while (1) {	
	VectorSet (angles, 90, 0, 90);	ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	// }

	VectorSet (angles, 0,0,0);			ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, -15,0,0);		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, 0,0,-15);		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, 0, 0,-15);		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, 90, 0, 0);		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, 180, 0, 0);		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, 270, 0, 0);		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, 0, 0, 90); 		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, 0, 0, -90); 		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, 0, 0, 135); 		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, 30, 0, 30); 		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, 45, 0, 45); 		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 
	VectorSet (angles, -75, 0, 90);		ZAngleVectorsFast (angles, t1.forward, t1.right, t1.up); 

	alert("pletye");
	exit (0);
}


void ZAngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	glmatrix _mx = {0}, *mx = &_mx, _mx2 = {0}, *d = &_mx2;
	const int z_going_up = 1;
	const int x = 12;
	const int y = z_going_up ? 14 : 13;
	const int z = z_going_up ? 13 : 14;
	Mat4_Identity_Set	(mx); // I DID NOT TEST COMBINING THEM.  BUT EACH OWN COMPONENT WORKED.  LET's TRY 2 REAL QUICK
	Mat4_Rotate			(mx, -angles[0] ,  1, 0, 0); // pitch

	switch (z_going_up) {
	default:			Mat4_Rotate			(mx,  angles[2] ,  0, 1, 0); // yaw for opengl,   but roll for z going up
						Mat4_Rotate			(mx, -angles[1] ,  0, 0, 1); // roll for opengl,  but yaw for z going up
	case_break false:	Mat4_Rotate			(mx,  angles[1] ,  0, 1, 0); // yaw for opengl,   but roll for z going up
						Mat4_Rotate			(mx, -angles[2] ,  0, 0, 1); // roll for opengl,  but yaw for z going up
	} 
	Mat4_Copy			(d, mx);    Mat4_Translate		(d,  1,  0, 0);  VectorSet (right,   d->m16[x], d->m16[y], d->m16[z]);
	Mat4_Copy			(d, mx);	Mat4_Translate		(d,  0,  1, 0);  VectorSet (up,      d->m16[x], d->m16[y], d->m16[z]);
	Mat4_Copy			(d, mx);	Mat4_Translate		(d,  0,  0, 1);  VectorSet (forward, d->m16[x], d->m16[y], d->m16[z]);

	

#if 0 //def DEBUG // I trust the other 100% ... this function, I want it to prove itself a bit.
	alert ("ZAngleVectors Mat: %s", _angles4 (angles, forward, right, up));

	{	vec3_t forward2, right2, up2; float *fs1 = NULL, *fs2 = NULL; const char *badz= NULL;
		vec3_t test = {angles[0], 0, angles[2] };
		ZAngleVectorsGL (test, forward2, right2, up2);
		
		if		(!VectorCompare (forward, forward2))	fs1= forward, fs2 = forward2, badz="forward";
		else if	(!VectorCompare (up, up2))				fs1= up, fs2 = up2, badz="up";
		else if	(!VectorCompare (right, right2))		fs1= right, fs2 = right2, badz="right";
		if (badz && (fabs(fs1[0] - fs2[0])>0.01 || fabs(fs1[1] - fs2[1])>0.01 || fabs(fs1[2] - fs2[2])>0.01))
		{
			alert ("ZAngleVectors Mat check fail for " QUOTED_S " on Angles = (pitch %g, ROLL %g, YAW %g" NEWLINE
				"ZAngleVectors Mat answer is (%g %g %g)" NEWLINE
				
				"ZAngleVectorsGL says %g %g %g" NEWLINE
				"Deltas are %g %g %g", badz, test[0], test[1], test[2], fs1[0], fs1[1], fs1[2], fs2[0], fs2[1], fs2[2],
					fabs(fs1[0] - fs2[0]), fabs(fs1[1] - fs2[1]), fabs(fs1[2] - fs2[2]));
		}
	}

#endif

}

char *_angles(const vec3_t a)
{
//	vec3_t a = {a[0], a[1], a[2]};
	return va("(%g, %g, %g) ", a[0], a[1], a[2]);
}

char *_angles4(const vec3_t angles, const vec3_t forward, const vec3_t right, const vec3_t up)
{
	return  va ("%s -> forward %s right %s up %s", _angles(angles), _angles(forward), _angles(right), _angles(up));
}


void AngleVectorsGL (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	glmatrix _mx = {0}, *mx = &_mx, _mx2 = {0}, *d = &_mx2;

	Mat4_Identity_Set	(mx); // I DID NOT TEST COMBINING THEM.  BUT EACH OWN COMPONENT WORKED.  LET's TRY 2 REAL QUICK
	Mat4_Rotate			(mx, -angles[0] ,  1, 0, 0); // pitch
	Mat4_Rotate			(mx,  angles[1] ,  0, 1, 0); // yaw for opengl,   but roll for z going up
	Mat4_Rotate			(mx, -angles[2] ,  0, 0, 1); // roll for opengl,  but yaw for z going up
	
	Mat4_Copy			(d, mx);    Mat4_Translate		(d,  1,  0, 0);  VectorSet (right,   d->m16[12], d->m16[13], d->m16[14]);
	Mat4_Copy			(d, mx);	Mat4_Translate		(d,  0,  1, 0);  VectorSet (up,      d->m16[12], d->m16[13], d->m16[14]);
	Mat4_Copy			(d, mx);	Mat4_Translate		(d,  0,  0, 1);  VectorSet (forward, d->m16[12], d->m16[13], d->m16[14]);
#ifdef _DEBUG
//	alert ("GL angles: %s", _angles4 (angles, forward, right, up));
#endif
}

void ZAngleVectorsGL (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	vec3_t fakes = { angles[0], angles[2], angles[1] }; // Supposedly our only difference is Z is up instead of Y (depth),  meaning roll and yaw flip
	AngleVectorsGL (fakes, forward, right, up);
	c_swapf (&forward[1], &forward[2]); // Then flip Y and Z
	c_swapf (&right[1], &right[2]);
	c_swapf (&up[1], &up[2]);

		
}
