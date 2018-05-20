
// included by r_part_qmb.c

float	_mathlib_temp_float1, _mathlib_temp_float2;
#define VectorSupCompare(v, w, m)								\
	(_mathlib_temp_float1 = m,								\
	(v)[0] - (w)[0] > -_mathlib_temp_float1 && (v)[0] - (w)[0] < _mathlib_temp_float1 &&	\
	(v)[1] - (w)[1] > -_mathlib_temp_float1 && (v)[1] - (w)[1] < _mathlib_temp_float1 &&	\
	(v)[2] - (w)[2] > -_mathlib_temp_float1 && (v)[2] - (w)[2] < _mathlib_temp_float1)




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

#define ColorSetRGB(v, x, y, z)			((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))
#define ColorSetRGBA(v, x, y, z, w)		((v)[0] = (x), (v)[1] = (y), (v)[2] = (z), (v)[3] = (w))
#define ColorTripleSetRGB(v, x)			((v)[0] = (x), (v)[1] = (v)[0], (v)[2] = (v)[0])


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







typedef	enum
{
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
	p_glow,
	p_missilefire,
//	p_q3blood,
//	p_q3smoke,
	PART_TYPE_LIMIT,
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
//	ptex_q3blood,
//	ptex_q3smoke,
	part_tex_max,
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

//#define	NUM_PARTICLETEXTURES	6

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

void QMB_ParticleTrail (const vec3_t start, const vec3_t end, float size, float time, color_vec4b_t color);
//cbool QMB_InitParticles (void);
//void QMB_ClearParticles (void);
//void QMB_ParseParticleEffect (void);
//void QMB_RunParticleEffect (vec3_t org, const vec3_t dir, int color, int count);
//void QMB_AnyTrail (vec3_t start, vec3_t end, vec3_t *trail_origin, trail_type_e type);
//void QMB_EntityParticles (entity_t *ent);
//void QMB_BlobExplosion (vec3_t org);
//void QMB_ParticleExplosion (vec3_t org);
//void QMB_ColorMappedExplosion (vec3_t org, int colorStart, int colorLength); // ParticleExplosion2
//void QMB_LavaSplash (vec3_t org);
//void QMB_TeleportSplash (vec3_t org);

static cbool TraceLineN (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal)
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

static byte *ColorForParticle (part_type_e type)
{
	int		lambda;
	static	color_vec4b_t	color;	// color_vec4b_t is unsigned char

	switch (type)
	{
	case p_spark:			ColorSetRGB			(color,	224 + (rand() & 31), 100 + (rand() & 31), 0);	break;
//	case p_q3smoke:			ColorTripleSetRGB	(color, 180);											break;
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
//	case p_q3blood:			ColorSetRGB			(color, 100 + (rand() & 31), 0, 0);						break;
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

#define TruePointContents(p) SV_HullPointContents(&cl.worldmodel->hulls[0], 0, p)
