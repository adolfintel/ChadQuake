#ifdef GLQUAKE

/*
Copyright (C) 2002-2003, Dr Labman, A. Nourai

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// gl_rpart.c

#include "quakedef.h"
#include <assert.h>

#include "mark_v_qmb.h"

// mi_player and friends moved to client.h
modelindex_e	cl_modelindex[modelindex_max];
char			*cl_modelnames[modelindex_max];

typedef	byte color_vec4b_t[4];

#define	DEFAULT_NUM_PARTICLES		4096
#define	ABSOLUTE_MIN_PARTICLES		256
#define	ABSOLUTE_MAX_PARTICLES		32768




// included by r_part_qmb.c

float	_mathlib_temp_float1, _mathlib_temp_float2;
#define VectorSupCompare(v, w, m)								\
	(_mathlib_temp_float1 = m,								\
	(v)[0] - (w)[0] > -_mathlib_temp_float1 && (v)[0] - (w)[0] < _mathlib_temp_float1 &&	\
	(v)[1] - (w)[1] > -_mathlib_temp_float1 && (v)[1] - (w)[1] < _mathlib_temp_float1 &&	\
	(v)[2] - (w)[2] > -_mathlib_temp_float1 && (v)[2] - (w)[2] < _mathlib_temp_float1)






typedef	enum
{
__part_type_invalid_gcc_sucks = -1, // Force int enum
	p_spark,
	p_smoke,
	p_fire,
	p_bubble,
	p_lavasplash,
	p_gunblast,
	p_chunk,
	p_shockwave,
	p_explosion,
	p_sparkray,
	p_staticbubble,
	p_trailpart,
	p_dpsmoke,
	p_dpfire,
	p_teleflare,
	p_blood1,
	p_blood2,
	p_blood3,
	p_flame,
	p_lavatrail,
	p_bubble2,
	p_streak,
	p_streaktrail,
	p_streakwave,
	p_lightningbeam,
	p_laserfire,
	p_glow,
	p_missilefire,
	p_q3blood,
	p_q3smoke,
	num_particletypes, //part_type_max, // PART_TYPE_LIMIT,
} part_type_e;

typedef	enum
{
	pm_static,
	pm_normal,
	pm_bounce,
	pm_die,
	pm_nophysics,
	pm_float,
	pm_streak,
	pm_streakwave
} part_move_e;

typedef	enum
{
	ptex_none,
	ptex_smoke,
	ptex_bubble,
	ptex_generic,
	ptex_dpsmoke,
	ptex_lava,
	ptex_blueflare,
	ptex_blood1,
	ptex_blood2,
	ptex_blood3,
	ptex_lightning,
	ptex_explosion,
	ptex_laserfire,
	ptex_q3blood,
	ptex_q3smoke,
	num_particletextures, // part_tex_max,
} part_tex_e;

typedef	enum
{
	pd_spark,
	pd_sparkray,
	pd_billboard,
	pd_billboard_vel,
	pd_hide,
	pd_beam
} part_draw_e;



typedef struct qmb_particle_s
{
	struct qmb_particle_s *next;
	vec3_t				org, endorg;
	color_vec4b_t		color;
	float				growth;
	vec3_t				vel;
	float				rotangle;
	float				rotspeed;
	float				size;
	float				start;
	float				die;
	byte				hit;
	byte				texindex;
	byte				bounces;
} qmb_particle_t;

typedef	struct particle_tree_s
{
	qmb_particle_t		*start;
	part_type_e			id;
	part_draw_e			drawtype;
	int					SrcBlend;
	int					DstBlend;
	part_tex_e			texture_numbp;
	float				startalpha;
	float				grav;
	float				accel;
	part_move_e			move;
	float				custom;
} particle_type_t;

#define ISUNDERWATER(x) ((x) == CONTENTS_WATER || (x) == CONTENTS_SLIME || (x) == CONTENTS_LAVA)

#define	MAX_PTEX_COMPONENTS_8	8
typedef struct particle_texture_s
{
	gltexture_t			*glt;
	int					components;
	float				coords[MAX_PTEX_COMPONENTS_8][4];
} particle_texture_t;


static	float	sint[7] = {0.000000, 0.781832,  0.974928,  0.433884, -0.433884, -0.974928, -0.781832};
static	float	cost[7] = {1.000000, 0.623490, -0.222521, -0.900969, -0.900969, -0.222521,  0.623490};

static	qmb_particle_t		*particles, *free_particles;
static	particle_type_t		particle_types[num_particletypes];
//static	const int			particle_types_count = ARRAY_COUNT(particle_types);

static	int					particle_type_index[num_particletypes];
static	particle_texture_t	particle_textures[num_particletextures];
//static	const int			particle_textures_count = ARRAY_COUNT (particle_textures);

static	int					r_numparticles;
static	vec3_t				zerodir = {22, 22, 22};
static	int					particle_count = 0;
static	float				particle_time;
static	vec3_t				trail_stop;






static void QMB_ParticleTrail (const vec3_t start, const vec3_t end, float size, float time, color_vec4b_t color);
//cbool QMB_InitParticles (void);
//void QMB_ClearParticles (void);
//void QMB_ParseParticleEffect (void);
//void QMB_RunParticleEffect (const vec3_t org, const vec3_t dir, int color, int count);
//void QMB_AnyTrail (const vec3_t start, const vec3_t end, vec3_t *trail_origin, trail_type_e type);
//void QMB_EntityParticles (entity_t *ent);
//void QMB_BlobExplosion (const vec3_t org);
//void QMB_ParticleExplosion (const vec3_t org);
//void QMB_ColorMappedExplosion (const vec3_t org, int colorStart, int colorLength); // ParticleExplosion2
//void QMB_LavaSplash (const vec3_t org);
//void QMB_TeleportSplash (const vec3_t org);
#define TruePointContents(p) SV_HullPointContents(&cl.worldmodel->hulls[0], 0, p)
#define ISUNDERWATER(x) ((x) == CONTENTS_WATER || (x) == CONTENTS_SLIME || (x) == CONTENTS_LAVA)

static cbool TraceLineN (const vec3_t start, const vec3_t end, vec3_t impact, vec3_t normal)
{
	trace_t	trace;

	memset (&trace, 0, sizeof(trace));
	trace.fraction = 1;
	if (SV_RecursiveHullCheck(cl.worldmodel->hulls, 0, 0, 1, start, end, &trace))
		return false;
	VectorCopy (trace.endpos, impact);
	if (normal)
		VectorCopy (trace.plane.normal, normal);

	return true;
}


#define ColorSetRGB(v, x, y, z)			((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))
#define ColorSetRGBA(v, x, y, z, w)		((v)[0] = (x), (v)[1] = (y), (v)[2] = (z), (v)[3] = (w))
#define ColorTripleSetRGB(v, x)			((v)[0] = (x), (v)[1] = (v)[0], (v)[2] = (v)[0])

static byte *ColorForParticle (part_type_e type)
{
	int		lambda;
	static	color_vec4b_t	color;	// color_vec4b_t is unsigned char

	switch (type)
	{
	case p_spark:			ColorSetRGB			(color,	224 + (rand() & 31), 100 + (rand() & 31), 0);	break;
	case p_q3smoke:			ColorTripleSetRGB	(color, 180);											break;
	case p_fire:			ColorSetRGB			(color, 255, 142, 62);									break;
	case p_gunblast:		ColorSetRGB			(color, 224 + (rand() & 31), 170 + (rand() & 31), 0);	break;
	case p_chunk:			ColorTripleSetRGB	(color, 32 + (rand() & 127));							break;
	case p_shockwave:		ColorTripleSetRGB	(color, 64 + (rand() & 31));							break;
	case p_missilefire:		ColorSetRGB			(color, 255, 56, 9);									break;
	case p_sparkray:		ColorSetRGB			(color, 255, 102, 25);									break;
	case p_dpsmoke:			ColorTripleSetRGB	(color, 48 + (((rand() & 0xFF) * 48) >> 8));			break;

	case p_teleflare:
	case p_lavasplash:		ColorTripleSetRGB	(color, 128 + (rand() & 127));							break;
	case p_blood1:
	case p_blood2:			ColorTripleSetRGB	(color, 180 + (rand() & 63));							break;
	case p_blood3:
	case p_q3blood:			ColorSetRGB			(color, 100 + (rand() & 31), 0, 0);						break;
	case p_flame:			ColorSetRGBA		(color, 255, 100, 25, 128);								break;
	case p_lavatrail:		ColorSetRGBA		(color, 255, 102, 25, 255);								break;

	case p_smoke:
	case p_glow:
	case p_explosion:		ColorTripleSetRGB	(color, 255);											break;
	case p_bubble:
	case p_bubble2:
	case p_staticbubble:	ColorTripleSetRGB	(color, 192 + (rand() & 63));							break;

	case p_dpfire:			lambda = rand() & 0xFF;
							ColorSetRGB			(color, 160 + ((lambda * 48) >> 8), 16 + ((lambda * 148) >> 8), 16 + ((lambda * 16) >> 8));		break;

	default:
		assert (!"ColorForParticle: unexpected type");
		break;
	}

	return color;
}


#define ADD_PARTICLE_TEXTURE(_ptex, _glt, _texindex, _components, _s1, _t1, _s2, _t2)\
do {																		\
	particle_textures[_ptex].glt = _glt;								\
	particle_textures[_ptex].components = _components;						\
	particle_textures[_ptex].coords[_texindex][0] = (_s1 + 1) / 256.0;		\
	particle_textures[_ptex].coords[_texindex][1] = (_t1 + 1) / 256.0;		\
	particle_textures[_ptex].coords[_texindex][2] = (_s2 - 1) / 256.0;		\
	particle_textures[_ptex].coords[_texindex][3] = (_t2 - 1) / 256.0;		\
} while(0)

#define ADD_PARTICLE_TYPE(_id, _drawtype, _SrcBlend, _DstBlend, _texture, _startalpha, _grav, _accel, _move, _custom)	\
do {																\
	particle_types[count].id = (_id);								\
	particle_types[count].drawtype = (_drawtype);					\
	particle_types[count].SrcBlend = (_SrcBlend);					\
	particle_types[count].DstBlend = (_DstBlend);					\
	particle_types[count].texture_numbp = (_texture);				\
	particle_types[count].startalpha = (_startalpha);				\
	particle_types[count].grav = 9.8 * (_grav);						\
	particle_types[count].accel = (_accel);							\
	particle_types[count].move = (_move);							\
	particle_types[count].custom = (_custom);						\
	particle_type_index[_id] = count;								\
	count++;														\
} while(0)

//const char *progs_flame0_mdl_name = "progs/flame0.mdl";

//cbool	qmb_initialized = false;

#define mglFinishedStates()		// Keep!  We should have a state manager someday.
#define mglPopStates()			// Keep!  We should have a state manager someday.
#define mglPushStates()			// Keep!  We should have a state manager someday.

gltexture_t *LoadATex (unsigned **punsigned, int ordinal, const char *qpath, const char *description, cbool require_size_256)
{
	int width = 0, height = 0;  unsigned *rgba_data = NULL; size_t rgba_data_length;
	switch (ordinal) {
	case 0:	rgba_data  = Image_Load_PNG_Memory_Alloc (qmb_particlefont_png, sizeof(qmb_particlefont_png), &width, &height, qmb_particlefont_png); break;
	case 1:	rgba_data  = Image_Load_PNG_Memory_Alloc (qmb_zing_png, sizeof(qmb_zing_png), &width, &height, qmb_zing_png); break;
	case 2:	rgba_data  = Image_Load_PNG_Memory_Alloc (qmb_laser_png, sizeof(qmb_laser_png), &width, &height, qmb_laser_png); break;
	case 3:	rgba_data  = Image_Load_PNG_Memory_Alloc (qmb_explosion_png, sizeof(qmb_explosion_png), &width, &height, qmb_explosion_png); break;
	default: break;
	}
	if (!rgba_data)
		System_Error ("QMB Couldn't load: " QUOTED_S " from memory %d", qpath, ordinal); // Failure shouldn't be possible
	rgba_data_length = width * height * RGBA_4;

	if (require_size_256 && (width !=256 || height != 256) ) {
		Con_SafePrintLinef ("QMB texture: " QUOTED_S " size invalid %d x %d (256 x 256 required)", qpath, width, height);
		//rgba_data = core_free (rgba_data); // DUH!  Can't free.  It's too late.
		return NULL;
	}

	(*punsigned) = rgba_data; //c_memdup (rgba_data, rgba_data_length); // Store!
	return TexMgr_LoadImage (NULL, -1, description, width, height, SRC_RGBA,  (*punsigned), "", (src_offset_t)(*punsigned), TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_PERSIST | TEXPREF_NOPICMIP | TEXPREF_BLENDED);
// Image_Load_PNG_Memory_Alloc
}


static unsigned *pt_persist_tex[ARRAY_COUNT(particle_textures)];

//////// SYNC
//////// SYNC
//////// SYNC
//////// SYNC
//////// SYNC
//////// SYNC
//////// SYNC
//////// SYNC
//////// SYNC
//////// SYNC
//////// SYNC
//////// SYNC
//////// SYNC

const char *QMB_InitParticles_Error (void)
{
	int	i, count = 0;
//	qmodel_t *flame0_test = NULL;
	gltexture_t *load_texture = NULL;

	if (particles) particles = core_free(particles);

	//flame0_test = Mod_ForName (progs_flame0_mdl_name, false);
	//if (!flame0_test) {
	//	return "progs/flame0.mdl not loaded";
	//}

	// Discard old memory.
	{ int n; for (n = 0; n < (int)ARRAY_COUNT (particle_textures); n ++) {
		if (pt_persist_tex[n]) pt_persist_tex[n] = core_free (pt_persist_tex[n]);
		if (particle_textures[n].glt) System_Error ("Unfreed particle texture #%d", n);	// Make sure the texture has been freed already.
	}}

	load_texture = LoadATex ( /*rgba store*/ &pt_persist_tex[0], 0, "gfx/particles/particlefont", "qmb:particlefont", true /*force 256 */);
	if (!load_texture)
		return "gfx/particles/particlefont not loaded (256 x 256 required size)";

	ADD_PARTICLE_TEXTURE(ptex_none,			0,			  0, 1,   0,   0,   0,   0);		// Chops up the particle font.
	ADD_PARTICLE_TEXTURE(ptex_blood1,		load_texture, 0, 1,   0,   0,  64,  64);
	ADD_PARTICLE_TEXTURE(ptex_blood2,		load_texture, 0, 1,  64,   0, 128,  64);
	ADD_PARTICLE_TEXTURE(ptex_lava,			load_texture, 0, 1, 128,   0, 192,  64);
	ADD_PARTICLE_TEXTURE(ptex_blueflare,	load_texture, 0, 1, 192,   0, 256,  64);
	ADD_PARTICLE_TEXTURE(ptex_generic,		load_texture, 0, 1,   0,  96,  96, 192);
	ADD_PARTICLE_TEXTURE(ptex_smoke,		load_texture, 0, 1,  96,  96, 192, 192);
	ADD_PARTICLE_TEXTURE(ptex_blood3,		load_texture, 0, 1, 192,  96, 256, 160);
	ADD_PARTICLE_TEXTURE(ptex_bubble,		load_texture, 0, 1, 192, 160, 224, 192);

	for (i = 0; i < 8; i++)
		ADD_PARTICLE_TEXTURE(ptex_dpsmoke,  load_texture, i, 8, i * 32, 64, (i + 1) * 32, 96);
	// Processing

#if 0
	if (!(particletexture = GL_LoadExternalTextureImage("gfx/particles/q3blood", "qmb:q3blood", TEX_ALPHA_TEST | TEX_COMPLAIN, NULL)))		goto qmb_bail;
	ADD_PARTICLE_TEXTURE(ptex_q3blood,		particletexture, 0, 1,   0,   0, 256, 256);

	if (!(particletexture = GL_LoadExternalTextureImage("gfx/particles/q3smoke", "qmb:q3smoke", TEX_ALPHA_TEST | TEX_COMPLAIN, NULL)))		goto qmb_bail;
	ADD_PARTICLE_TEXTURE(ptex_q3smoke,		particletexture, 0, 1,   0,   0, 256, 256);
#endif

	load_texture = LoadATex ( /*rgba store*/ &pt_persist_tex[1], 1, "gfx/particles/zing1", "qmb:lightning", false /*force 256 */);
	if (!load_texture)
		return "gfx/particles/zing1 not loaded";

	ADD_PARTICLE_TEXTURE(ptex_lightning, load_texture, 0, 1,   0,   0, 256, 256);

	load_texture = LoadATex ( /*rgba store*/ &pt_persist_tex[2], 2, "gfx/particles/laserfire", "qmb:laserfire", false /*force 256 */);
	if (!load_texture)
		return "gfx/particles/laserfire not loaded";

	ADD_PARTICLE_TEXTURE(ptex_laserfire, load_texture, 0, 1,   0,   0, 256, 256);

	load_texture = LoadATex ( /*rgba store*/ &pt_persist_tex[3], 3, "gfx/particles/explosion", "qmb:explosion", false /*force 256 */);
	if (!load_texture)
		return "gfx/particles/explosion not loaded";
	ADD_PARTICLE_TEXTURE(ptex_explosion, load_texture, 0, 1,   0,   0, 256, 256);


	if ((i = COM_CheckParm("-particles")) && i + 1 < com_argc)
	{
		r_numparticles = (int)(atoi(com_argv[i+1]));
		r_numparticles = CLAMP (ABSOLUTE_MIN_PARTICLES, r_numparticles, ABSOLUTE_MAX_PARTICLES);
	}
	else r_numparticles = DEFAULT_NUM_PARTICLES;

	// This is just texture coordinates ...

	particles = calloc (r_numparticles, sizeof(qmb_particle_t)); //, "qmb:particles");

	//				  _id,			    _drawtype,		_SrcBlend,		_DstBlend,				_texture, _startalpha, _grav, _accel, _move, _custom)
	ADD_PARTICLE_TYPE(p_spark,			pd_spark,		GL_SRC_ALPHA,	GL_ONE,					ptex_none,		255, -32,     0, pm_bounce,		 1.3	);
	ADD_PARTICLE_TYPE(p_sparkray,		pd_sparkray,	GL_SRC_ALPHA,	GL_ONE,					ptex_none,		255,  -0,     0, pm_nophysics,	 0		);
	ADD_PARTICLE_TYPE(p_gunblast,		pd_spark,		GL_SRC_ALPHA,	GL_ONE,					ptex_none,		255, -16,     0, pm_bounce,		 1.3	);

	ADD_PARTICLE_TYPE(p_fire,			pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_generic,	204,   0, -2.95, pm_die,		-1		);
	ADD_PARTICLE_TYPE(p_chunk,			pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_generic,	255, -16,     0, pm_bounce,		 1.475	);
	ADD_PARTICLE_TYPE(p_shockwave,		pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_generic,	255,   0, -4.85, pm_nophysics,	 0		);
	ADD_PARTICLE_TYPE(p_missilefire,	pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_generic,	153,   0,     0, pm_static,		 0		);
	ADD_PARTICLE_TYPE(p_trailpart,		pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_generic,	230,   0,     0, pm_static,		 0		);
	ADD_PARTICLE_TYPE(p_smoke,			pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_smoke,		140,   3,     0, pm_normal,		 0		);
	ADD_PARTICLE_TYPE(p_dpfire,			pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_dpsmoke,	144,   0,     0, pm_die,		 0		);
	ADD_PARTICLE_TYPE(p_dpsmoke,		pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_dpsmoke,	 85,   3,     0, pm_die,		 0		);

	ADD_PARTICLE_TYPE(p_teleflare,		pd_billboard,	GL_ONE,			GL_ONE,					ptex_blueflare,	255,   0,     0, pm_die,		 0		);

	ADD_PARTICLE_TYPE(p_blood1,			pd_billboard,	GL_ZERO,		GL_ONE_MINUS_SRC_COLOR,	ptex_blood1,	255, -20,     0, pm_die,		 0		);
	ADD_PARTICLE_TYPE(p_blood2,			pd_billboard_vel, GL_ZERO,		GL_ONE_MINUS_SRC_COLOR, ptex_blood2,	255, -25,     0, pm_die,		 0.018	);

	ADD_PARTICLE_TYPE(p_lavasplash,		pd_billboard,	GL_SRC_ALPHA,	GL_ONE_MINUS_SRC_ALPHA, ptex_lava,		170,   0,     0, pm_nophysics,	 0		);
	ADD_PARTICLE_TYPE(p_blood3,			pd_billboard,	GL_SRC_ALPHA,	GL_ONE_MINUS_SRC_ALPHA, ptex_blood3,	255, -20,     0, pm_normal,		 0		);
	ADD_PARTICLE_TYPE(p_bubble,			pd_billboard,	GL_SRC_ALPHA,	GL_ONE_MINUS_SRC_ALPHA, ptex_bubble,	204,   8,     0, pm_float,		 0		);
	ADD_PARTICLE_TYPE(p_staticbubble,	pd_billboard,	GL_SRC_ALPHA,	GL_ONE_MINUS_SRC_ALPHA, ptex_bubble,	204,   0,     0, pm_static,		 0		);

	ADD_PARTICLE_TYPE(p_flame,			pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_generic,	200,  10,     0, pm_die,		 0		);
	ADD_PARTICLE_TYPE(p_lavatrail,		pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_generic,	255,   3,     0, pm_normal,		 0		);
	ADD_PARTICLE_TYPE(p_bubble2,		pd_billboard,	GL_SRC_ALPHA,	GL_ONE_MINUS_SRC_ALPHA, ptex_bubble,	204,   1,     0, pm_float,		 0		);
	ADD_PARTICLE_TYPE(p_streak,			pd_hide,		GL_SRC_ALPHA,	GL_ONE,					ptex_none,		255, -64,     0, pm_streak,		 1.5	);
	ADD_PARTICLE_TYPE(p_streakwave,		pd_hide,		GL_SRC_ALPHA,	GL_ONE,					ptex_none,		255,   0,     0, pm_streakwave,  0		);
	ADD_PARTICLE_TYPE(p_streaktrail,	pd_beam,		GL_SRC_ALPHA,	GL_ONE,					ptex_none,		128,   0,     0, pm_die,		 0		);
	ADD_PARTICLE_TYPE(p_lightningbeam,	pd_beam,		GL_SRC_ALPHA,	GL_ONE,					ptex_lightning, 255,   0,     0, pm_die,		 0		);
	ADD_PARTICLE_TYPE(p_glow,			pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_generic,	204,   0,     0, pm_die,		 0		);
	ADD_PARTICLE_TYPE(p_explosion,		pd_billboard,	GL_SRC_ALPHA,	GL_ONE_MINUS_SRC_ALPHA, ptex_explosion, 255,   0,     0, pm_static,		-1		);
	ADD_PARTICLE_TYPE(p_laserfire,		pd_beam,		GL_SRC_ALPHA,	GL_ONE,					ptex_laserfire, 255,   0,     0, pm_die,		 0		);
//	ADD_PARTICLE_TYPE(p_q3blood,		pd_billboard,	GL_SRC_ALPHA,	GL_ONE_MINUS_SRC_ALPHA, ptex_q3blood,	255,-1.5,     0, pm_normal,		 0		);
//	ADD_PARTICLE_TYPE(p_q3smoke,		pd_billboard,	GL_SRC_ALPHA,	GL_ONE,					ptex_q3smoke,	140,   3,     0, pm_normal,		 0		);


	//qmb_initialized = true;
	return NULL /*success*/;

}


void QMB_ClearParticles (void)
{
	int	i;

	if (!qmb_is_available) // If QMB is possible
		return;

	memset (particles, 0, r_numparticles * sizeof(qmb_particle_t));

	free_particles						= &particles[0];
	particle_count						= 0;

	for (i = 0; i + 1 < r_numparticles; i++)
		particles[i].next				= &particles[i + 1];

	particles[r_numparticles-1].next	= NULL;

	for (i = 0; i < num_particletypes; i++)  // VERIFY ME
		particle_types[i].start			= NULL;
}


static void QMB_UpdateParticles (void)
{
	int				i;
	float			grav, bounce, frametime;
	vec3_t			oldorg, stop, normal;
	particle_type_t	*pt;
	qmb_particle_t		*p, *kill;

	if (!cl.worldmodel)		// Baker: if connected to a server with "nomap" doing a TruePointContents
		return;				//        will crash us.  Client isn't receiving ents, but is getting particles. == CRASH ... so bail if cl.worldmodel is NULL


	particle_count						= 0;
	frametime							= fabs(cl.ctime - cl.oldtime);
	grav								= sv_gravity.value / 800.0;

	for (i=0 ; i < num_particletypes; i++)
	{
		pt = &particle_types[i];

		if (pt->start)
		{
			p = pt->start;
			while (p && p->next)
			{
				kill					= p->next;
				if (kill->die <= particle_time)
				{
					p->next				= kill->next;
					kill->next			= free_particles;
					free_particles		= kill;
				}
				else
				{
					p					= p->next;
				}
			}
			if (pt->start->die <= particle_time)
			{
				kill					= pt->start;
				pt->start				= kill->next;
				kill->next				= free_particles;
				free_particles			= kill;
			}
		}

		for (p = pt->start ; p ; p = p->next)
		{
			if (particle_time < p->start)
				continue;

			particle_count++;

			p->size += p->growth * frametime;

			if (p->size <= 0)
			{
				p->die = 0;
				continue;
			}

			p->color[3] = pt->startalpha * ((p->die - particle_time) / (p->die - p->start));

			p->rotangle += p->rotspeed * frametime;

			if (p->hit)
				continue;

			p->vel[2] += pt->grav * grav * frametime;

			VectorScale (p->vel, 1 + pt->accel * frametime, p->vel);

			switch (pt->move)
			{
			case pm_static:
				break;

			case pm_normal:
				VectorCopy (p->org, oldorg);
				VectorMA (p->org, frametime, p->vel, p->org);
				if (CONTENTS_SOLID == TruePointContents(p->org))
				{
					p->hit = 1;
					VectorCopy (oldorg, p->org);
					VectorClear (p->vel);
				}
				break;

			case pm_float:
				VectorMA (p->org, frametime, p->vel, p->org);
				p->org[2] += p->size + 1;
				if (!ISUNDERWATER(TruePointContents(p->org)))
					p->die = 0;
				p->org[2] -= p->size + 1;
				break;

			case pm_nophysics:
				VectorMA (p->org, frametime, p->vel, p->org);
				break;

			case pm_die:
				VectorMA (p->org, frametime, p->vel, p->org);
				if (CONTENTS_SOLID == TruePointContents(p->org))
					p->die = 0;
				break;

			case pm_bounce:
				if (!qmb_bounceparticles.value || p->bounces)
				{
					VectorMA (p->org, frametime, p->vel, p->org);
					if (CONTENTS_SOLID == TruePointContents(p->org))
						p->die = 0;
				}
				else
				{
					VectorCopy (p->org, oldorg);
					VectorMA (p->org, frametime, p->vel, p->org);
					if (CONTENTS_SOLID == TruePointContents(p->org))
					{
						if (TraceLineN(oldorg, p->org, stop, normal))
						{
							VectorCopy (stop, p->org);
							bounce = -pt->custom * DotProduct(p->vel, normal);
							VectorMA (p->vel, bounce, normal, p->vel);
							p->bounces++;
						}
					}
				}
				break;

			case pm_streak:
				VectorCopy (p->org, oldorg);
				VectorMA (p->org, frametime, p->vel, p->org);
				if (CONTENTS_SOLID == TruePointContents(p->org))
				{
					if (TraceLineN(oldorg, p->org, stop, normal))
					{
						VectorCopy (stop, p->org);
						bounce = -pt->custom * DotProduct(p->vel, normal);
						VectorMA (p->vel, bounce, normal, p->vel);
					}
				}
				QMB_ParticleTrail (oldorg, p->org, p->size, 0.2, p->color);
				if (!VectorLength(p->vel))
					p->die = 0;
				break;

			case pm_streakwave:
				VectorCopy (p->org, oldorg);
				VectorMA (p->org, frametime, p->vel, p->org);
				QMB_ParticleTrail (oldorg, p->org, p->size, 0.5, p->color);
				p->vel[0] = 19 * p->vel[0] / 20;
				p->vel[1] = 19 * p->vel[1] / 20;
				p->vel[2] = 19 * p->vel[2] / 20;
				break;

			default:
				assert (!"QMB_UpdateParticles: unexpected pt->move");
				break;
			}
		}
	}
}

#define DRAW_PARTICLE_BILLBOARD(_ptex, _p, _coord)					\
	mglPushStates (); eglPushMatrix ();								\
	eglTranslatef (_p->org[0], _p->org[1], _p->org[2]);				\
	eglScalef (_p->size, _p->size, _p->size);						\
	if (_p->rotspeed)												\
		eglRotatef (_p->rotangle, vpn[0], vpn[1], vpn[2]);			\
																	\
	eglColor4ubv (_p->color);										\
																	\
	eglBegin (GL_QUADS);											\
	eglTexCoord2f (_ptex->coords[_p->texindex][0], ptex->coords[_p->texindex][3]); eglVertex3fv(_coord[0]);	\
	eglTexCoord2f (_ptex->coords[_p->texindex][0], ptex->coords[_p->texindex][1]); eglVertex3fv(_coord[1]);	\
	eglTexCoord2f (_ptex->coords[_p->texindex][2], ptex->coords[_p->texindex][1]); eglVertex3fv(_coord[2]);	\
	eglTexCoord2f (_ptex->coords[_p->texindex][2], ptex->coords[_p->texindex][3]); eglVertex3fv(_coord[3]);	\
	eglEnd ();														\
																	\
	eglPopMatrix (); mglPopStates ();

static void QMB_DrawParticles_Beam_CalcVerts (float *vert, const vec3_t org1, const vec3_t org2, const float width)
{
	vec3_t	right1, right2, diff, normal;

	VectorSubtract (org2, org1, normal);
	VectorNormalize (normal);

	// calculate 'right' vector for start
	VectorSubtract (r_origin, org1, diff);
	VectorNormalize (diff);
	VectorCrossProduct (normal, diff, right1);

	// calculate 'right' vector for end
	VectorSubtract (r_origin, org2, diff);
	VectorNormalize (diff);
	VectorCrossProduct (normal, diff, right2);

	vert[ 0] = org1[0] + width * right1[0];
	vert[ 1] = org1[1] + width * right1[1];
	vert[ 2] = org1[2] + width * right1[2];
	vert[ 4] = org1[0] - width * right1[0];
	vert[ 5] = org1[1] - width * right1[1];
	vert[ 6] = org1[2] - width * right1[2];
	vert[ 8] = org2[0] - width * right2[0];
	vert[ 9] = org2[1] - width * right2[1];
	vert[10] = org2[2] - width * right2[2];
	vert[12] = org2[0] + width * right2[0];
	vert[13] = org2[1] + width * right2[1];
	vert[14] = org2[2] + width * right2[2];
}

void QMB_RunParticles (void)
{

	particle_time = cl.time;

//	if (!cl.paused) // We already checked that, thanks!
		QMB_UpdateParticles ();

}

void QMB_DrawParticles (void)
{
	int					i, j, k, drawncount;
	vec3_t				v, up, right, billboard[4], velcoord[4], neworg;
	qmb_particle_t			*p;
	particle_type_t		*pt;
	particle_texture_t	*ptex;
	extern	vec3_t		player_origin[MAX_SCOREBOARD_16];
	extern	int			numplayers;


#pragma message ("Quality assurance: QMB needs moved to pre-rendering like other particle calcs")
	VectorAdd		(vup, vright, billboard[2]);
	VectorSubtract	(vright, vup, billboard[3]);
	VectorNegate	(billboard[2], billboard[0]);
	VectorNegate	(billboard[3], billboard[1]);

	mglPopStates ();

	eglDepthMask (GL_FALSE);
//eglEnable (GL_ALPHA_TEST); // Hurts badly
//eglAlphaFunc (GL_GREATER, 0.666);
//eglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
//eglColor4f(1,1,1,1);

	eglEnable (GL_BLEND);
	eglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	eglShadeModel (GL_SMOOTH);	// Baker: Really?

	for (i = 0; i < num_particletypes; i++)
	{
//		int particle_textures_count = ARRAY_COUNT(particle_textures);
//		assert (in_range (0, i, ARRAY_COUNT(particle_textures) - 1));
		pt = &particle_types[i];
		if (!pt->start || pt->drawtype == pd_hide)
			continue;

		eglBlendFunc (pt->SrcBlend, pt->DstBlend);

		switch (pt->drawtype)
		{
		case pd_beam:
//			assert (in_range (0, pt->texture_numbp, ARRAY_COUNT(particle_textures) - 1));
			ptex = &particle_textures[pt->texture_numbp];

			eglEnable (GL_TEXTURE_2D);
			GL_Bind (ptex->glt);
			mglFinishedStates ();

			for (p = pt->start ; p ; p = p->next)
			{
				if (particle_time < p->start || particle_time >= p->die)
					continue;

				eglColor4ubv (p->color);
				for (j=1 ; j>0 ; j--)
				{
					float	varray_vertex[16];

					eglBegin (GL_QUADS);
					QMB_DrawParticles_Beam_CalcVerts (varray_vertex, p->org, p->endorg, p->size / (j * 3));

					eglTexCoord2f (1, 0);
					eglVertex3f (varray_vertex[0], varray_vertex[1], varray_vertex[2]);
					eglTexCoord2f (1, 1);
					eglVertex3f (varray_vertex[4], varray_vertex[5], varray_vertex[6]);
					eglTexCoord2f (0, 1);
					eglVertex3f (varray_vertex[8], varray_vertex[9], varray_vertex[10]);
					eglTexCoord2f (0, 0);
					eglVertex3f (varray_vertex[12], varray_vertex[13], varray_vertex[14]);
					eglEnd ();
				}
			}
			break;

		case pd_spark:
			eglDisable (GL_TEXTURE_2D);

			mglFinishedStates ();

			for (p = pt->start ; p ; p = p->next)
			{
				if (particle_time < p->start || particle_time >= p->die)
					continue;

				eglBegin (GL_TRIANGLE_FAN);
				eglColor4ubv (p->color);
				eglVertex3fv (p->org);
				eglColor4ub ((GLubyte)(p->color[0] >> 1), (GLubyte)(p->color[1] >> 1), (GLubyte)(p->color[2] >> 1), 0);
				for (j=7 ; j>=0 ; j--)
				{
					for (k=0 ; k<3 ; k++)
						v[k] = p->org[k] - p->vel[k] / 8 + vright[k] * cost[j%7] * p->size + vup[k] * sint[j%7] * p->size;
					eglVertex3fv (v);
				}
				eglEnd ();
			}
			break;

		case pd_sparkray:
			eglDisable (GL_TEXTURE_2D);

			mglFinishedStates ();

			for (p = pt->start ; p ; p = p->next)
			{
				if (particle_time < p->start || particle_time >= p->die)
					continue;

				if (!TraceLineN(p->endorg, p->org, neworg, NULL))
					VectorCopy (p->org, neworg);

				eglBegin (GL_TRIANGLE_FAN);
				eglColor4ubv (p->color);
				eglVertex3fv (p->endorg);
				eglColor4ub ((GLubyte)(p->color[0] >> 1), (GLubyte)(p->color[1] >> 1), (GLubyte)(p->color[2] >> 1), 0);
				for (j=7 ; j>=0 ; j--)
				{
					for (k=0 ; k<3 ; k++)
						v[k] = neworg[k] + vright[k] * cost[j%7] * p->size + vup[k] * sint[j%7] * p->size;
					eglVertex3fv (v);
				}
				eglEnd ();
			}
			break;

		case pd_billboard:
//			assert (in_range (0, pt->texture_numbp, ARRAY_COUNT(particle_textures) - 1));
			ptex = &particle_textures[pt->texture_numbp];
			eglEnable (GL_TEXTURE_2D);
			GL_Bind (ptex->glt);

			mglFinishedStates ();

			drawncount = 0;
			for (p = pt->start ; p ; p = p->next)
			{
				if (particle_time < p->start || particle_time >= p->die)
					continue;

				for (j = 0 ; j < numplayers ; j++)
				{
					if (pt->custom != -1 &&
					    p->org[0] - player_origin[j][0] > -16 && p->org[0] - player_origin[j][0] < 16 &&
					    p->org[1] - player_origin[j][1] > -16 && p->org[1] - player_origin[j][1] < 16 &&
					    p->org[2] - player_origin[j][2] > -24 && p->org[2] - player_origin[j][2] < 32)
					{
						p->die = 0;
						continue;
					}
				}

				if (qmb_clipparticles.value)
				{
					if (drawncount >= 3 && VectorSupCompare(p->org, r_origin, 30))
						continue;
					drawncount++;
				}
				DRAW_PARTICLE_BILLBOARD(ptex, p, billboard);
			}
			break;

		case pd_billboard_vel:
			assert (in_range (0, (int)pt->texture_numbp, (int)ARRAY_COUNT(particle_textures) - 1));
			ptex = &particle_textures[pt->texture_numbp];
			eglEnable (GL_TEXTURE_2D);
			GL_Bind (ptex->glt);

			mglFinishedStates ();

			for (p = pt->start ; p ; p = p->next)
			{
				if (particle_time < p->start || particle_time >= p->die)
					continue;

				VectorCopy (p->vel, up);
				VectorCrossProduct (vpn, up, right);
				VectorNormalizeFast (right);
				VectorScale (up, pt->custom, up);

				VectorAdd (up, right, velcoord[2]);
				VectorSubtract (right, up, velcoord[3]);
				VectorNegate (velcoord[2], velcoord[0]);
				VectorNegate (velcoord[3], velcoord[1]);
				DRAW_PARTICLE_BILLBOARD(ptex, p, velcoord);
			}
			break;

		default:
			assert (!"QMB_DrawParticles: unexpected drawtype");
			break;
		}
	}

	eglEnable		(GL_TEXTURE_2D);
	eglDepthMask	(GL_TRUE);
	eglDisable		(GL_BLEND);
	eglBlendFunc	(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	eglTexEnvf		(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	mglPopStates ();
}

#define	INIT_NEW_PARTICLE(_pt, _p, _color, _size, _time)					\
	_p = free_particles;													\
	free_particles = _p->next;												\
	_p->next = _pt->start;													\
	_pt->start = _p;														\
	_p->size = _size;														\
	_p->hit = 0;															\
	_p->start = cl.time;													\
	_p->die = _p->start + _time;											\
	_p->growth = 0;															\
	_p->rotspeed = 0;														\
	_p->texindex = (rand() % particle_textures[_pt->texture_numbp].components);	\
	_p->bounces = 0;														\
	VectorCopy(_color, _p->color);

__inline static void AddParticle (part_type_e type, const vec3_t org, int count, float size, float time, color_vec4b_t col, const vec3_t dir)
{
	byte		*color;
	int			i, j;
	float		tempSize;
	qmb_particle_t	*p;
	particle_type_t	*pt;

	if (!qmb_is_available)
		System_Error ("QMB particle added without initialization");

	assert (size > 0 && time > 0);

	if (type < 0 || type >= num_particletypes)
		System_Error ("AddParticle: Invalid type (%d)", type);

	pt = &particle_types[particle_type_index[type]];

	for (i=0 ; i < count && free_particles ; i++)
	{
		color = col ? col : ColorForParticle (type);
		INIT_NEW_PARTICLE(pt, p, color, size, time);

		switch (type)
		{
		case p_spark:
			p->size = 1.175;
			VectorCopy (org, p->org);
			tempSize = size * 2;
			p->vel[0] = (rand() % (int)tempSize) - ((int)tempSize / 2);
			p->vel[1] = (rand() % (int)tempSize) - ((int)tempSize / 2);
			p->vel[2] = (rand() % (int)tempSize) - ((int)tempSize / 3);
			break;

		case p_smoke:
			for (j=0 ; j<3 ; j++)
				p->org[j] = org[j] + ((rand() & 31) - 16) / 2.0;
			for (j=0 ; j<3 ; j++)
				p->vel[j] = ((rand() % 10) - 5) / 20.0;
			break;

		case p_fire:
			VectorCopy (org, p->org);
			for (j=0 ; j<3 ; j++)
				p->vel[j] = ((rand() % 160) - 80) * (size / 25.0);
			break;

		case p_bubble:
			VectorCopy (org, p->org);
			for (j=0 ; j<2 ; j++)
				p->vel[j] = (rand() % 32) - 16;
			p->vel[2] = (rand() % 64) + 24;
			break;

		case p_lavasplash:
		case p_streak:
		case p_streakwave:
		case p_shockwave:
			VectorCopy (org, p->org);
			VectorCopy (dir, p->vel);
			break;

		case p_gunblast:
			p->size = 1;
			VectorCopy (org, p->org);
			p->vel[0] = (rand() & 127) - 64;
			p->vel[1] = (rand() & 127) - 64;
			p->vel[2] = (rand() & 127) - 10;
			break;

		case p_chunk:
			VectorCopy (org, p->org);
			p->vel[0] = (rand() % 40) - 20;
			p->vel[1] = (rand() % 40) - 20;
			p->vel[2] = (rand() % 40) - 5;
			break;

		case p_explosion:
			VectorCopy (org, p->org);
			VectorClear (p->vel);
			p->growth = 50;
			break;

		case p_sparkray:
			VectorCopy (org, p->endorg);
			VectorCopy (dir, p->org);
			for (j=0 ; j<3 ; j++)
				p->vel[j] = (rand() & 127) - 64;
			p->growth = -16;
			break;

		case p_staticbubble:
		case p_missilefire:
			VectorCopy (org, p->org);
			VectorClear (p->vel);
			break;

		case p_teleflare:
			VectorCopy (org, p->org);
			VectorCopy (dir, p->vel);
			p->growth = 1.75;
			break;

		case p_blood1:
		case p_blood2:
			for (j=0 ; j<3 ; j++)
				p->org[j] = org[j] + (rand() & 15) - 8;
			for (j=0 ; j<3 ; j++)
				p->vel[j] = (rand() & 63) - 32;
			break;

		case p_blood3:
			p->size = size * (rand() % 20) / 5.0;
			VectorCopy (org, p->org);
			for (j=0 ; j<3 ; j++)
				p->vel[j] = (rand() % 40) - 20;
			break;

		case p_flame:
			VectorCopy (org, p->org);
			p->growth = -p->size / 2;
			VectorClear (p->vel);
			for (j=0 ; j<2 ; j++)
				p->vel[j] = (rand() % 6) - 3;
			break;

		case p_glow:
			VectorCopy (org, p->org);
			VectorCopy (dir, p->vel);
#if 1
			//for (j=0 ; j<3 ; j++)
			//{
			//	p->org[j] = org[j] + ((rand()&15)-8);
			//	p->vel[j] = dir[j]*15;
			//}

#else
			p->growth = -1.5; // Here
#endif
			break;

		case p_streaktrail:
		case p_lightningbeam:
		case p_laserfire:
			VectorCopy (org, p->org);
			VectorCopy (dir, p->endorg);
			VectorClear (p->vel);
			p->growth = -p->size / time;
			break;

		default:
			assert (!"AddParticle: unexpected type");
			break;
		}
	}
}

__inline static void AddParticleTrail (part_type_e type, const vec3_t start, const vec3_t end, float size, float time, color_vec4b_t col)
{
	byte		*color;
	int		i, j, num_particles;
	float		count, length;
	vec3_t		point, delta;
	qmb_particle_t	*p;
	particle_type_t	*pt;

	if (!qmb_is_available)
		System_Error ("QMB particle added without initialization");

	assert (size > 0 && time > 0);

	if (type < 0 || type >= num_particletypes)
		System_Error ("AddParticle: Invalid type (%d)", type);

	pt = &particle_types[particle_type_index[type]];

	VectorCopy (start, point);
	VectorSubtract (end, start, delta);
	if (!(length = VectorLength(delta)))
		goto done;

	switch (type)
	{
//	case p_q3blood:
//	case p_q3smoke:
//		count = length / 25;
//		break;

	case p_trailpart:
	case p_lavatrail:
		count = length / 1.1;
		break;

	case p_blood3:
		count = length / 8;
		break;

	case p_bubble:
	case p_bubble2:
		count = length / 5;
		break;

	case p_smoke:
		count = length / 3.8;
		break;

	case p_dpsmoke:
		count = length / 2.5;
		break;

	case p_dpfire:
		count = length / 2.8;
		break;

	default:
		assert (!"AddParticleTrail: unexpected type");
		break;
	}

	if (!(num_particles = (int)count))
		goto done;

	VectorScale (delta, 1.0 / num_particles, delta);

	for (i=0 ; i < num_particles && free_particles ; i++)
	{
		color = col ? col : ColorForParticle (type);
		INIT_NEW_PARTICLE(pt, p, color, size, time);

		switch (type)
		{
		case p_trailpart:
			VectorCopy (point, p->org);
			VectorClear (p->vel);
			p->growth = -size / time;
			break;

		case p_blood3:
//		case p_q3blood:
			VectorCopy (point, p->org);
			for (j=0 ; j<3 ; j++)
				p->org[j] += ((rand() & 15) - 8) / 8.0;
			for (j=0 ; j<3 ; j++)
				p->vel[j] = ((rand() & 15) - 8) / 2.0;
			p->size = size * (rand() % 20) / 10.0;
			p->growth = 6;
			break;

		case p_bubble:
		case p_bubble2:
			VectorCopy (point, p->org);
			for (j=0 ; j<3 ; j++)
				p->vel[j] = (rand() % 10) - 5;
			break;

		case p_smoke:
			VectorCopy (point, p->org);
			for (j=0 ; j<3 ; j++)
				p->org[j] += ((rand() & 7) - 4) / 8.0;
			p->vel[0] = p->vel[1] = 0;
			p->vel[2] = rand() & 3;
			p->growth = 4.5;
			p->rotspeed = (rand() & 63) + 96;
			break;

		//case p_q3smoke:
		//	VectorCopy (point, p->org);
		//	for (j=0 ; j<3 ; j++)
		//		p->org[j] += ((rand() & 7) - 4) / 8.0;
		//	VectorClear (p->vel);
		//	p->growth = 12;
		//	break;

		case p_dpsmoke:
			VectorCopy (point, p->org);
			for (j=0 ; j<3 ; j++)
				p->vel[j] = (rand() % 10) - 5;
			p->growth = 3;
			p->rotspeed = (rand() & 63) + 96;
			break;

		case p_dpfire:
			VectorCopy (point, p->org);
			for (j=0 ; j<3 ; j++)
				p->vel[j] = (rand() % 40) - 20;
			break;

		case p_lavatrail:
			VectorCopy (point, p->org);
			for (j=0 ; j<3 ; j++)
				p->org[j] += (rand() & 7) - 4;
			p->vel[0] = p->vel[1] = 0;
			p->vel[2] = rand() & 3;
			break;

		default:
			assert (!"AddParticleTrail: unexpected type");
			break;
		}

		VectorAdd (point, delta, point);
	}

done:
	VectorCopy (point, trail_stop);
}

void QMB_ParticleExplosion (const vec3_t org)
{
	if (!cl.worldmodel)		// Baker: if connected to a server with "nomap" doing a TruePointContents
		return;				//        will crash us.  Client isn't receiving ents, but is getting particles. == CRASH ... so bail if cl.worldmodel is NULL

	if (ISUNDERWATER(TruePointContents(org)))
	{
		if (qmb_explosions.value == 2)
			AddParticle (p_explosion, org, 1, 20, 0.8, NULL, zerodir);
		else
			AddParticle (p_fire, org, 12, 14, 0.8, NULL, zerodir);

		AddParticle (p_bubble, org, 6, 3.0, 2.5, NULL, zerodir);
		AddParticle (p_bubble, org, 4, 2.35, 2.5, NULL, zerodir);

		if (qmb_explosiontype.value != 1)
		{
			AddParticle (p_spark, org, 50, 100, 0.75, NULL, zerodir);
			AddParticle (p_spark, org, 25, 60, 0.75, NULL, zerodir);
		}
	}
	else
	{
		if (qmb_explosions.value == 2)
			AddParticle (p_explosion, org, 1, 25, 0.6, NULL, zerodir);
		else
			AddParticle (p_fire, org, 16, 18, 1, NULL, zerodir);
		if (qmb_explosiontype.value != 1)
		{
			AddParticle (p_spark, org, 50, 250, 0.925f, NULL, zerodir);
			AddParticle (p_spark, org, 25, 150, 0.925f, NULL, zerodir);
		}
	}
}

void d8to24col (color_vec4b_t colourv, int colour)
{
	byte	*colourByte;

	colourByte = (byte *)&vid.d_8to24table[colour];
	colourv[0] = colourByte[0];
	colourv[1] = colourByte[1];
	colourv[2] = colourByte[2];
}

__inline static void AddColoredParticle (part_type_e type, const vec3_t org, int count, float size, float time, int colorStart, int colorLength, const vec3_t dir)
{
	color_vec4b_t		color;
	int		i, j, colorMod = 0;
	float		tempSize;
	qmb_particle_t	*p;
	particle_type_t	*pt;

	if (!qmb_is_available)
		System_Error ("QMB particle added without initialization");

	assert (size > 0 && time > 0);

	if (!in_range_beyond (0, type, num_particletypes) )
		System_Error ("AddColoredParticle: Invalid type (%d)", type);

	pt = &particle_types[particle_type_index[type]];

	for (i=0 ; i < count && free_particles ; i++)
	{
		d8to24col (color, colorStart + (colorMod % colorLength));
		colorMod++;
		INIT_NEW_PARTICLE(pt, p, color, size, time);

		switch (type)
		{
		case p_spark:
			p->size = 1.175;
			VectorCopy (org, p->org);
			tempSize = size * 2;
			p->vel[0] = (rand() % (int)tempSize) - ((int)tempSize / 4);
			p->vel[1] = (rand() % (int)tempSize) - ((int)tempSize / 4);
			p->vel[2] = (rand() % (int)tempSize) - ((int)tempSize / 6);
			break;

		case p_fire:
			VectorCopy (org, p->org);
			for (j=0 ; j<3 ; j++)
				p->vel[j] = ((rand() % 160) - 80) * (size / 25.0);
			break;

		default:
			assert (!"AddColoredParticle: unexpected type");
			break;
		}
	}
}

void QMB_ColorMappedExplosion (const vec3_t org, int colorStart, int colorLength)
{
	if (!cl.worldmodel)		// Baker: if connected to a server with "nomap" doing a TruePointContents
		return;				//        will crash us.  Client isn't receiving ents, but is getting particles. == CRASH ... so bail if cl.worldmodel is NULL

	if (ISUNDERWATER(TruePointContents(org)))
	{
		AddColoredParticle (p_fire, org, 16, 18, 1, colorStart, colorLength, zerodir);
		AddParticle (p_bubble, org, 6, 3.0, 2.5, NULL, zerodir);
		AddParticle (p_bubble, org, 4, 2.35, 2.5, NULL, zerodir);
		if (qmb_explosiontype.value != 1)
		{
			AddColoredParticle (p_spark, org, 50, 100, 0.5, colorStart, colorLength, zerodir);
			AddColoredParticle (p_spark, org, 25, 60, 0.5, colorStart, colorLength, zerodir);
		}
	}
	else
	{
		AddColoredParticle (p_fire, org, 16, 18, 1, colorStart, colorLength, zerodir);
		if (qmb_explosiontype.value != 1)
		{
			AddColoredParticle (p_spark, org, 50, 250, 0.625f, colorStart, colorLength, zerodir);
			AddColoredParticle (p_spark, org, 25, 150, 0.625f, colorStart, colorLength, zerodir);
		}
	}
}

void QMB_RunParticleEffect (const vec3_t org, const vec3_t dir, int col, int count, te_ef_effect_e te_ef_effect)
{
	color_vec4b_t	color;
	vec3_t	neworg, newdir;
	int	i, j, particlecount;

//	if (qmb_disableblood.value && (col == COLOR_UNKNOWN_BLOOD_73 || col == COLOR_EXPLOSION_225))
//		col = COLOR_SPARK_20;		// Switch to spark

	if (col == COLOR_UNKNOWN_BLOOD_73 || col == COLOR_EXPLOSION_BLOOD_225) // This really must work like this, special color of parsed effect :(
	{
		int	bloodcount;

		bloodcount = c_rint (count / 28.0);
		bloodcount = CLAMP (2, bloodcount, 8);
		for (i=0 ; i<bloodcount ; i++)
		{
			AddParticle (p_blood1, org, 1, 4.5, 2 + (rand() & 15) / 16.0, NULL, zerodir);
			AddParticle (p_blood2, org, 1, 4.5, 2 + (rand() & 15) / 16.0, NULL, zerodir);
		}
		return;
	}

	// I don't think the following ever gets used ...
	if (te_ef_effect == TE_EF_EXPLOSION_1_3) // col == COLOR_EXPLOSION_225)
	{
		int	scale;

		scale = (count > 130) ? 3 : (count > 20) ? 2 : 1;
		scale *= 0.758;
		for (i=0 ; i<(count>>1) ; i++)
		{
			for (j=0 ; j<3 ; j++)
				neworg[j] = org[j] + scale * ((rand() & 15) - 8);
			AddParticle (p_blood3, neworg, 1, 1, 2.3, NULL, zerodir);
		}
		return;
	}

	if (te_ef_effect == TE_EF_WIZSPIKE) //col == COLOR_WIZSPIKE_20 && count == WIZSPIKE_COUNT_30)
	{
		ColorSetRGB (color, 51, 255, 51);
//		color[0] = color[2] = 51;
//		color[1] = 255;
		AddParticle (p_chunk, org, 1, 1, 0.75, color, zerodir);
		AddParticle (p_spark, org, 12, 75, 0.4, color, zerodir);
		return;
	}

	if (te_ef_effect == TE_EF_KNIGHTSPIKE) // col == COLOR_KNIGHTSPIKE_226 && count == KNIGHTSPIKE_COUNT_20)
	{
		ColorSetRGB (color, 230, 204, 26);
		AddParticle (p_chunk, org, 1, 1, 0.75, color, zerodir);
		AddParticle (p_spark, org, 12, 75, 0.4, color, zerodir);
		return;
	}

	if (te_ef_effect == TE_EF_SPIKE || te_ef_effect == TE_EF_VENTILLIATION) {
		if (com_gametype == gametype_nehahra && count == VENTILLIATION_WIND_COUNT_10)	// ventillation's wind
		{
			for (i=0 ; i<count ; i++)
			{
				for (j=0 ; j<3 ; j++)
				{
					neworg[j] = org[j] + ((rand() % 24) - 12);
					newdir[j] = dir[j] * (10 + (rand() % 5));
				}
				d8to24col (color, (col & ~7) + (rand() & 7));
				AddParticle (p_glow, neworg, 1, 3, 0.3 + 0.1 * (rand() % 3), color, newdir);
			}
		}
		else
		{
			AddParticle (p_spark, org, 6, 70, 0.6, NULL, zerodir);
		}
		return;
	}

	if (te_ef_effect == TE_EF_SUPERSPIKE) { // case SUPER_SPIKE_AND_BULLETS_COUNT_20:
		AddParticle (p_spark, org, 12, 85, 0.6, NULL, zerodir);
		return;
	}

	if (te_ef_effect == TE_EF_GUNSHOT) { //case GUNSHOT_COUNT_21:	// gunshot
		particlecount = count >> 1;
		AddParticle (p_gunblast, org, 15, 5, 0.15, NULL, zerodir);
		for (i=0 ; i<particlecount ; i++)
		{
			for (j=0 ; j<3 ; j++)
				neworg[j] = org[j] + ((rand() & 15) - 8);
			AddParticle (p_smoke, neworg, 1, 4, 0.825f + ((rand() % 10) - 5) / 40.0, NULL, zerodir);
			if ((i % particlecount) == 0)
				AddParticle (p_chunk, neworg, 1, 0.75, 3.75, NULL, zerodir);
		}
		return;
	}


	if (qmb_particles_quakec_count_30.value && count == 30) {
		// Baker: Meh .. another special count.  Grrrr.
		AddParticle (p_chunk, org, 10, 1, 4, NULL, zerodir);
		AddParticle (p_spark, org, 8, 105, 0.9, NULL, zerodir);
		return;
	}


	if (com_gametype == gametype_hipnotic)
	{
		switch (count)
		{
		case 1:		// particlefields
		case 2:
		case 3:
		case 4:
			d8to24col (color, (col & ~7) + (rand() & 7));
			AddParticle (p_gunblast, org, 15, 5, 0.1 * (rand() % 5), color, zerodir);
			return;
		}
		// Apparently certain cases fall through to bottom ...
	}
	
	if (com_gametype == gametype_nehahra)
	{
		switch (count)
		{
		case 25:	// slime barrel chunks
			return;

		case 244:	// sewer
			for (i = 0 ; i < (count>>3) /*divide by 8*/ ; i++)
			{
				for (j=0 ; j<3 ; j++)
					neworg[j] = org[j] + ((rand() % 24) - 12);
				newdir[0] = dir[0] * (10 + (rand() % 5));
				newdir[1] = dir[1] * (10 + (rand() % 5));
				newdir[2] = dir[2] * 15;
				d8to24col (color, (col & ~7) + (rand() & 7));
				AddParticle (p_glow, neworg, 1, 3.5, 0.5 + 0.1 * (rand() % 3), color, newdir);
			}
			return;
		}
		// Apparently certain cases fall through to bottom ...
	}

	particlecount = c_max(1, count >> 1);  // divide by 2?
	for (i = 0 ; i < particlecount ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
#if 1
			neworg[j] = org[j] + ((rand()&15)-8);
			newdir[j] = dir[j] * 15;
#else
			neworg[j] = org[j] + ((rand() % 20) - 10);
			newdir[j] = dir[j] * (10 + (rand() % 5));
#endif
		}
		d8to24col (color, (col & ~7) + (rand() & 7));
		AddParticle (p_glow, neworg, 1, 3, 0.2 + 0.1 * (rand() % 4), color, newdir);
	}
	
	
}

void QMB_AnyTrail (const vec3_t start, const vec3_t end, vec3_t *trail_origin, trail_type_e type)
{
	color_vec4b_t		color;
	static	int	make_blood_rare = 0, make_smoke_rare = 0;

	if (!cl.worldmodel)		// Baker: if connected to a server with "nomap" doing a TruePointContents
		return;				//        will crash us.  Client isn't receiving ents, but is getting particles. == CRASH ... so bail if cl.worldmodel is NULL

	if (qmb_disableblood.value && (type == BLOOD_TRAIL_2 || type == SLIGHT_ZOM_BLOOD_TRAIL_4))
		type = GRENADE_TRAIL_1;

	switch (type)
	{
	case GRENADE_TRAIL_1:
		if (ISUNDERWATER(TruePointContents(start)))
			AddParticleTrail (p_bubble, start, end, 1.8, 1.5, NULL);
//		if (qmb_grenadetrail.value == 3)
//			AddParticleTrail (p_q3smoke, start, end, 5, 0.75, NULL);
//		else
		if (qmb_trail_grenade.value == 2) {
			AddParticleTrail (p_dpfire, start, end, 3, 0.26, NULL);
			AddParticleTrail (p_dpsmoke, start, end, 3, 0.825, NULL);
		}
		else
			AddParticleTrail (p_smoke, start, end, 1.45, 0.825, NULL);
		break;

	case BLOOD_TRAIL_2:
	case SLIGHT_ZOM_BLOOD_TRAIL_4:
//		if (cl_disable_bloodgore.value)
//		{
//			// Do nothing?
//		}
//		else
//		if (qmb_blood.value == 2)	// joe: should be replaced/fixed
//			AddParticleTrail (p_q3blood, start, end, 8, 2, NULL);
//		else
			AddParticleTrail (p_blood3, start, end, type == BLOOD_TRAIL_2 ? 1.35 : 2.4, 2, NULL);
		break;

	case TRACER1_SCRAG_TRAIL_3:
		ColorSetRGB (color, 0, 124, 0);
//		color[0] = color[2] = 0;
//		color[1] = 124;
		AddParticleTrail (p_trailpart, start, end, 3.75, 0.5, color);
		break;

	case TRACER2_HELLKNIGHT_TRAIL_5:
		ColorSetRGB (color, 255, 77, 0);
//		color[0] = 255;
//		color[1] = 77;
//		color[2] = 0;
		AddParticleTrail (p_trailpart, start, end, 3.75, 0.5, color);
		break;

	case VOOR_TRAIL_6:
		ColorSetRGB (color, 77, 0, 255);
//		color[0] = 77;
//		color[1] = 0;
//		color[2] = 255;
		AddParticleTrail (p_trailpart, start, end, 3.75, 0.5, color);
		break;

	case LAVA_TRAIL_7:
		AddParticleTrail (p_lavatrail, start, end, 5, 1, NULL);
		break;

	case BUBBLE_TRAIL_8:
		if (ISUNDERWATER(TruePointContents(start)))
			AddParticleTrail (p_bubble2, start, end, 1.5, 0.825, NULL);
		break;

	case NEHAHRA_SMOKE_9:
		AddParticleTrail (p_smoke, start, end, 0.8, 0.825, NULL);
		break;

	case ROCKET_TRAIL_0:
	default:
		if (ISUNDERWATER(TruePointContents(start)))
			AddParticleTrail (p_bubble, start, end, 1.8, 1.5, NULL);
//		if (qmb_rockettrail.value == 3)
//			AddParticleTrail (p_q3smoke, start, end, 5, 1.2, NULL);
		else if (qmb_trail_rocket.value == 2)
		{
			AddParticleTrail (p_dpfire, start, end, 3, 0.26, NULL);
			AddParticleTrail (p_dpsmoke, start, end, 3, 0.825, NULL);
		}
		else
			AddParticleTrail (p_smoke, start, end, 1.8, 0.825, NULL);
		break;
	}

	VectorCopy (trail_stop, *trail_origin);
}

void QMB_BlobExplosion (const vec3_t org)
{
	float	theta;
	color_vec4b_t	color;
	vec3_t	neworg, vel;

	ColorSetRGB (color, 60, 100, 240);
//	color[0] = 60;
//	color[1] = 100;
//	color[2] = 240;
	AddParticle (p_spark, org, 44, 250, 1.15, color, zerodir);

	ColorSetRGB (color, 90, 47, 207);
//	color[0] = 90;
//	color[1] = 47;
//	color[2] = 207;
	AddParticle (p_fire, org, 15, 30, 1.4, color, zerodir);

	vel[2] = 0;
	for (theta = 0 ; theta < 2 * M_PI ; theta += 2 * M_PI / 70)
	{
		ColorSetRGB (color, 60 + (rand() & 15), 65 + (rand() & 15), 200 + (rand() & 15));

		vel[0] = cos(theta) * 125;
		vel[1] = sin(theta) * 125;
		neworg[0] = org[0] + cos(theta) * 6;
		neworg[1] = org[1] + sin(theta) * 6;
		neworg[2] = org[2] + 0 - 10;
		AddParticle (p_shockwave, neworg, 1, 4, 0.8, color, vel);
		neworg[2] = org[2] + 0 + 10;
		AddParticle (p_shockwave, neworg, 1, 4, 0.8, color, vel);

		vel[0] *= 1.15;
		vel[1] *= 1.15;
		neworg[0] = org[0] + cos(theta) * 13;
		neworg[1] = org[1] + sin(theta) * 13;
		neworg[2] = org[2] + 0;
		AddParticle (p_shockwave, neworg, 1, 6, 1.0, color, vel);
	}
}

void QMB_LavaSplash (const vec3_t org)
{
	int	i, j;
	float	vel;
	vec3_t	dir, neworg;

	for (i = -16 ; i < 16; i++)
	{
		for (j = -16 ; j < 16 ; j++)
		{
			dir[0] = j * 8 + (rand() & 7);
			dir[1] = i * 8 + (rand() & 7);
			dir[2] = 256;

			neworg[0] = org[0] + dir[0];
			neworg[1] = org[1] + dir[1];
			neworg[2] = org[2] + (rand() & 63);

			VectorNormalizeFast (dir);
			vel = 50 + (rand() & 63);
			VectorScale (dir, vel, dir);

			AddParticle (p_lavasplash, neworg, 1, 4.5, 2.6 + (rand() & 31) * 0.02, NULL, dir);
		}
	}
}

void QMB_TeleportSplash (const vec3_t org)
{
	int	i, j, k;
	vec3_t	neworg, angle;
	color_vec4b_t	color;

	for (i = -12 ; i <= 12 ; i += 6)
	{
		for (j = -12 ; j <= 12 ; j += 6)
		{
			for (k = -24 ; k <= 32 ; k += 8)
			{
				neworg[0] = org[0] + i + (rand() & 3) - 1;
				neworg[1] = org[1] + j + (rand() & 3) - 1;
				neworg[2] = org[2] + k + (rand() & 3) - 1;
				angle[0] = (rand() & 15) - 7;
				angle[1] = (rand() & 15) - 7;
				angle[2] = (rand() % 160) - 80;
				AddParticle (p_teleflare, neworg, 1, 1.8, 0.30 + (rand() & 7) * 0.02, NULL, angle);
			}
		}
	}

	VectorSet (color, 140, 140, 255);
	VectorClear (angle);
	for (i=0 ; i<5 ; i++)
	{
		angle[2] = 0;
		for (j=0 ; j<5 ; j++)
		{
			AngleVectors (angle, NULL, NULL, neworg);
			VectorMA (org, 70, neworg, neworg);
			AddParticle (p_sparkray, org, 1, 6 + (i & 3), 5, color, neworg);
			angle[2] += 360 / 5;
		}
		angle[0] += 180 / 5;
	}
}

#define ONE_FRAME_ONLY	(1.0/1024) // (0.001)

void QMB_StaticBubble (entity_t *ent, const vec3_t origin)
{
	AddParticle (p_staticbubble, ent->origin, 1, ent->frame == 1 ? 1.85 : 2.9, ONE_FRAME_ONLY, NULL, zerodir);
}

void QMB_TorchFlame (const vec3_t org, float size, float time)
{
	if (fabs(cl.ctime - cl.oldtime))
		AddParticle (p_flame, org, 1, size, time, NULL, zerodir);
}

void QMB_MissileFire (const vec3_t org, const vec3_t start, const vec3_t end)
{
	if (fabs(cl.ctime - cl.oldtime))
		AddParticle (p_missilefire, org, 1, 20, ONE_FRAME_ONLY, NULL, zerodir);

	AddParticleTrail (p_trailpart, start, end, 2.5, 0.1, ColorForParticle(p_missilefire));
}

static void QMB_ParticleTrail (const vec3_t start, const vec3_t end, float size, float time, color_vec4b_t color)
{
	AddParticle (p_streaktrail, start, 1, size, time, color, end);
}

void QMB_ShamblerCharge (const vec3_t org)
{
	vec3_t	pos, vec, dir;
	color_vec4b_t	col = {60, 100, 240, 0};
	float	time, len;
	int	i;

	for (i = 0 ; i < 5 ; i++)
	{
		VectorClear (vec);
		VectorClear (dir);

		VectorCopy (org, pos);
		pos[0] += (rand() % 200) - 100;
		pos[1] += (rand() % 200) - 100;
		pos[2] += (rand() % 200) - 100;

		VectorSubtract (pos, org, vec);
		len = VectorLength (vec);
		VectorNormalize (vec);
		VectorMA (dir, -200, vec, dir);
		time = len / 200;

		AddParticle (p_streakwave, pos, 1, 3, time, col, dir);
	}
}


void QMB_LightningSplash (const vec3_t org)
{
	int		i, j;
	vec3_t	neworg, angle;
	color_vec4b_t	color = {200, 100, 100};

	VectorClear (angle);

	for (i = 0 ; i < 5 ; i++)
	{
		angle[2] = 0;
		for (j = 0 ; j < 5 ; j++)
		{
			AngleVectors (angle, NULL, NULL, neworg);
			VectorMA (org, 20, neworg, neworg);
			AddParticle (p_spark, org, 2, 85, 0.05, NULL, zerodir);
			AddParticle (p_spark, org, 2, 100, 0.1, color, neworg);
			angle[2] += 360 / 5;
		}
		angle[0] += 180 / 5;
	}
}


void QMB_LaserFire (const vec3_t start, const vec3_t end)
{
	float frametime = fabs(cl.ctime - cl.oldtime);
	color_vec4b_t color = {255, 77, 0, 255};
	
	if (frametime) {
		AddParticle (p_laserfire, start, 1, 80 /*100*/, frametime * 2, color, end);
	}
}


void QMB_LightningBeam (const vec3_t start, const vec3_t end)
{
	float	frametime = fabs(cl.ctime - cl.oldtime);
	color_vec4b_t	color = {120, 140, 255, 0};
	vec3_t	neworg;

	if (frametime) {
		AddParticle (p_lightningbeam, start, 1, 80 /*100*/, frametime * 2, color, end);
		if (qmb_sparks.value && TraceLineN(start, end, neworg, NULL)) {
			QMB_LightningSplash (neworg);
		}
	}

}

void QMB_GenSparks (const vec3_t org, byte col[3], float count, float size, float life)
{
	color_vec4b_t	color;
	vec3_t	dir;
	int	i, j;

	VectorCopy (col, color); // b = a
//	color[0] = col[0];
//	color[1] = col[1];
//	color[2] = col[2];

	for (i=0 ; i<count ; i++)
	{
		for (j=0 ; j<3 ; j++)
			dir[j] = (rand() % (int)size * 2) - ((int)size * 2 / 3);
		AddParticle (p_streak, org, 1, 1, life, color, dir);
	}
}

// Nothing calls this??  It's true.
//void QMB_Lightning_Splash (const vec3_t org)
//{
//	int	i, j;
//	vec3_t	neworg, angle;
//	color_vec4b_t	color;
//
//	ColorSetRGB (color, 40, 40, 128);
//	VectorClear (angle);
//
//	for (i = 0 ; i < 5 ; i++)
//	{
//		angle[2] = 0;
//		for (j = 0 ; j < 5 ; j++)
//		{
//			AngleVectors (angle, NULL, NULL, neworg);
//			VectorMA (org, 20, neworg, neworg);
//			AddParticle (p_sparkray, org, 1, 6 + (i & 3), 5, color, neworg);
//			angle[2] += 360 / 5;
//		}
//		angle[0] += 180 / 5;
//	}
//}

int	particle_mode = 0;	// 0: classic (default), 1: QMB, 2: mixed

/*void CheckParticles (void)
{
	// No on ecalls this?
	// Explain this.  Is this determining which set of values?
	if (!qmb_explosions.value && !qmb_trails.value && !qmb_spikes.value &&
	    !qmb_gunshots.value && !qmb_blood.value && !qmb_telesplash.value &&
	    !qmb_blobexplosions.value && !qmb_lavasplash.value && !qmb_inferno.value &&
	    !qmb_flames.value && !qmb_lightning.value && !qmb_spiketrails.value)
		particle_mode = 0;
	else if (qmb_explosions.value && qmb_trails.value && qmb_spikes.value &&
		 qmb_gunshots.value && qmb_blood.value && qmb_telesplash.value &&
		 qmb_blobexplosions.value && qmb_lavasplash.value && qmb_inferno.value &&
		 qmb_flames.value && qmb_lightning.value && qmb_spiketrails.value)
		particle_mode = 1;
	else
		particle_mode = 2;
} */

//void R_SetParticles (int val)
//{
//	particle_mode = val;
//
//	Cvar_SetValueQuick (&qmb_explosions, particle_mode);
//	Cvar_SetValueQuick (&qmb_trails, particle_mode);
//	Cvar_SetValueQuick (&qmb_spikes, particle_mode);
//	Cvar_SetValueQuick (&qmb_gunshots, particle_mode);
//	Cvar_SetValueQuick (&qmb_blood, particle_mode);
//	Cvar_SetValueQuick (&qmb_telesplash, particle_mode);
//	Cvar_SetValueQuick (&qmb_blobexplosions, particle_mode);
//	Cvar_SetValueQuick (&qmb_lavasplash, particle_mode);
//	Cvar_SetValueQuick (&qmb_inferno, particle_mode);
//	Cvar_SetValueQuick (&qmb_flames, particle_mode);
//	Cvar_SetValueQuick (&qmb_lightning, particle_mode);
//	Cvar_SetValueQuick (&qmb_spiketrails, particle_mode);
//}

/*
===============
R_ToggleParticles_f

function that toggles between classic and QMB particles - by joe
===============
*/
//void R_ToggleParticles_f (void)
//{
//	// toggleparticles command
//	if (cmd_source != src_command)
//		return;
//
//	R_SetParticles (!particle_mode);
//
//	Con_PrintLinef ("Using %s particles", !particle_mode ? "Classic" : "QMB");
//}


cbool QMB_FlameModelSetState (entity_t *ent)
{
	if ((!frame.qmb || !qmb_flames.value) && ent->baseline.modelindex == cl_modelindex[mi_flame1])
	{
		//ent->model = cl.model_precache[cl_modelindex[mi_flame1]];
		ent->is_fake_frame0 = false;
	}
	else if (frame.qmb && qmb_flames.value)
	{
		vec3_t	liteorg;

		VectorCopy (ent->origin, liteorg);
		//if (ent->baseline.modelindex == cl_modelindex[mi_flame0])
		//{
		//	liteorg[2] += 5.5;
		//	QMB_TorchFlame (liteorg, 7, 0.8);
		//}
		//else
		if (ent->baseline.modelindex == cl_modelindex[mi_flame1])
		{
			liteorg[2] += 5.5;
			QMB_TorchFlame (liteorg, 7, 0.8);
			//ent->model = cl.model_precache[cl_modelindex[mi_flame0]];
			ent->is_fake_frame0 = ent->model->is_original_flame_mdl ? true : false;
		}
		else if (ent->baseline.modelindex == cl_modelindex[mi_flame2])
		{
			liteorg[2] -= 1;
			QMB_TorchFlame (liteorg, 12, 1);
			return true;	// continue
		}
	}
	return false;
}


#ifdef GLQUAKE_SUPPORTS_QMB
#define F2F(_f) ((_f) / (256.0/4)) // No clean cut method but by divide by 256 works for us internally.
float dlightcolor[NUM_DLIGHTTYPES_7][3] = {
	{F2F(100), F2F(90), F2F(80)},		// dimlight or brightlight		// 0
	{F2F(100), F2F(50), F2F(10)},		// muzzleflash					// 1
	{F2F(100), F2F(50), F2F(10)},		// explosion					// 2
	{F2F(90), F2F(60), F2F(7)},		// rocket						// 3
	{F2F(128), F2F(0), F2F(0)},		// red							// 4
	{F2F(0), F2F(0), F2F(128)},		// blue							// 5
	{F2F(128), F2F(0), F2F(128)},		// red + blue					// 6
//	{F2F(0,	F2F(128, F2F(0}			// green
};
#endif // GLQUAKE_SUPPORTS_QMB

#ifdef GLQUAKE_SUPPORTS_QMB
Point3D QMB_GetDlightColor (dlighttype_e colornum, dlighttype_e def, cbool random)
{
	Point3D owt = {0};
	// 1 = red, 2 = blue, 3 = redblue, 4 = random
#define COLORS_POSSIBLE (NUM_DLIGHTTYPES_7 - 4)
	dlighttype_e colors[COLORS_POSSIBLE] = {lt_red, lt_blue, lt_redblue/*, lt_green*/};
	dlighttype_e mine = -1; //

	switch (colornum) {
	case 1:	mine = lt_red;		goto ready;
	case 2: mine = lt_blue;		goto ready;
	case 3: mine = lt_redblue;	goto ready;
	default: mine = random ? colors[ rand() % (NUM_DLIGHTTYPES_7 - 4)] : def; goto ready;  // explosions are random
	}
ready:
	owt.x = dlightcolor[mine][0]; // Byte to float
	owt.y = dlightcolor[mine][1]; // Byte to float
	owt.z = dlightcolor[mine][2]; // Byte to float
	return owt;
	//return def; // which is either lt_rocket or lt_explosion
}
#endif // GLQUAKE_SUPPORTS_QMB


#endif // GLQUAKE