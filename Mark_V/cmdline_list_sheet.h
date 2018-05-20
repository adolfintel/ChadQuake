// Command line parameters.  Except for -dedicated there is little to no need to ever use any of them.
// Gamedir changing can be done from the console and the video mode can be set in the menu on-the-fly.
// Sound speed change be changed using sndspeed in the console.
//
// -dedicated 12 -game ctf -condebug -ip 66.55.44.33 -port 26001 +hostname "My game server"

#if 0
enum plat_e
{
	platform_windows = 0,
	platform_osx = 1,
};

enum rend_e
{
	renderer_software = 0,	// Like WinQuake
	renderer_hardware = 1,	// OpenGL or Direct3D
};

enum host_e
{
	host_both = 0,			// Both
	host_server_only = 1,	// Can't client
	host_client_only = 2,	// Can't serve
};


// How much could this cut down on ifdefs?

void f (void)
{
#if id386
	build.assembly_langauage = true;
#endif // id386 --- Otherwise it is false

#ifdef SERVER_ONLY
	// Baker:  This is not the same as -dedicated.
	// A server only .exe cannot be a client and may have differences
	// in expectations.
	build.host_type = host_server_only;
#else
	build.host_type = host_both;
#endif // SERVERONLY

#ifdef GLQUAKE
	build.renderer = renderer_hardware;
#else
	build.renderer = renderer_software;
#endif // GLQUAKE

//#ifdef DIRECT3DX_WRAPPER // This code is commented out.  Possibly endangered.
//	build.direct3d = true;
//#endif // DIRECT3DX_WRAPPER --- Otherwise it is false

#ifdef SUPPORTS_MP3
	build.music_mp3 = true;
#endif // SUPPORTS_MP3 -- Otherwise it is false

#ifdef SUPPORTS_CD
	build.music_cd = true;
#endif // SUPPORTS_CD -- Otherwise it is false

#ifdef SUPPORTS_AVI_CAPTURE
	build.video_avi_capture = true;
#endif // SUPPORTS_AVI_CAPTURE -- Otherwise it is false

#ifdef CORE_GL // GLQUAKE_SUPPORTS_VSYNC
	build.video_vsync = true;
#endif // GLQUAKE_SUPPORTS_VSYNC -- Otherwise it is false

#ifdef WINQUAKE_SUPPORTS_VSYNC
	build.video_vsync = true;
#endif // WINQUAKE_SUPPORTS_VSYNC -- Otherwise it is false

}

typedef struct

what are these going to be used for?
to eliminate ifdefs

{
	cbool		assembly_langauage;
	enum host_e	host_type;
	enum rend_e	renderer;
	cbool		direct3d;
//	cbool		input joystick, touchpad, mouse, whatever ...
	cbool		music_mp3;
	cbool		music_cd;
	cbool		video_avi_capture;
	cbool		video_vsync;

	enum plat_e	platform;
} build_t;
build_t build;
#endif

// These aren't used at this time.

CMDLNE_PARAM (STARTUP    , NONE       , DEV  , basedir          ,  1,  "Set working directory (i.e. -basedir 'c:\quake')"           )
CMDLNE_PARAM (INPUT      , NONE       , DEV  , developer        ,  0,  "Set developer 1 on startup"                                 )
CMDLNE_PARAM (GLQUAKE    , CLIENT     , DEV  , noadd            ,  0,  "Disable OpenGL texture_env_add"                             )
CMDLNE_PARAM (GLQUAKE    , CLIENT     , DEV  , nomultisample    ,  0,  "Disable OpenGL multisample"                             	)
CMDLNE_PARAM (GLQUAKE    , CLIENT     , DEV  , nostencil    	,  0,  "Disable OpenGL stencil buffer"                             	)
CMDLNE_PARAM (GLQUAKE    , CLIENT     , DEV  , nocombine        ,  0,  "Disable OpenGL texture_env_combine"                         )
CMDLNE_PARAM (NETWORKING , NONE       , DEV  , nolan            ,  0,  "Disable all networking"                                     )
CMDLNE_PARAM (GLQUAKE    , CLIENT     , DEV  , nomtex           ,  0,  "Disable OpenGL multitexture"                                )
CMDLNE_PARAM (GLQUAKE    , CLIENT     , DEV  , nonpot           ,  0,  "Disable OpenGL texture_non_power_of_two"                    )
CMDLNE_PARAM (NETWORKING , NONE       , DEV  , noudp            ,  0,  "Disable UDP"                                                )
CMDLNE_PARAM (AVI        , CLIENT     , NONE , capturedemo      ,  1,  "Capture demo to AVI and exit (i.e. '-capturedemo demo1')"   )
CMDLNE_PARAM (MENU       , CLIENT     , NONE , classic          ,  0,  "Disables extra menus"                                       )
CMDLNE_PARAM (STARTUP    , NONE       , NONE , condebug         , -1,  "Write output to qconsole.log"                               )
CMDLNE_PARAM (STARTUP    , CLIENT     , NONE , consize          ,  1,  "Set console buffer allocation, default is 65536"            )
CMDLNE_PARAM (VIDEO      , CLIENT     , NONE , current          ,  0,  "Use current desktop resolution as game resolution"          )
CMDLNE_PARAM (DEDTERM    , DEDICATED  , NONE , dedicated        , -1,  "Dedicated server and # max players (-dedicated 8)"          )
CMDLNE_PARAM (DEDTERM    , DEDICATED  , NONE , developer        ,  0,  "Start with developer 1 on"          						)
CMDLNE_PARAM (GAME       , NONE       , NONE , game             ,  1,  "Specifies game (i.e. '-game travail' ) "                    )
CMDLNE_PARAM (VIDEO      , CLIENT     , NONE , height           ,  0,  "Specify height of window"                                   )
CMDLNE_PARAM (GAME       , NONE       , NONE , hipnotic         ,  0,  "Specifies game Hipnotic (Mission Pack 2)"                   )
CMDLNE_PARAM (NETWORKING , NONE       , NONE , ip               ,  0,  "Specify the IP address to use (i.e. -ip 66.55.44.33)"       )
CMDLNE_PARAM (NETWORKING , NONE       , NONE , ip6              ,  0,  "Specify the IPv6 address to use"       						)
CMDLNE_PARAM (NETWORKING , NONE       , NONE , noudp            ,  0,  "Disable UDP networking"       								)
CMDLNE_PARAM (NETWORKING , NONE       , NONE , noudp4           ,  0,  "Disable IPv4 networking, which is to force IPv6 only."		)
CMDLNE_PARAM (NETWORKING , NONE       , NONE , noudp6           ,  0,  "Disable IPv6 networking, which is to force IPv4 only."		)
CMDLNE_PARAM (GAME       , NONE       , NONE , nehahra          ,  0,  "Specifies game Nehahra"                                     )
CMDLNE_PARAM (AUDIO      , CLIENT     , NONE , nocdaudio        ,  0,  "Disables cd audio"                                          )
CMDLNE_PARAM (INPUT      , NONE       , NONE , nomouse          ,  0,  "Disable mouse"                                              )
CMDLNE_PARAM (AUDIO      , CLIENT     , NONE , nosound          ,  0,  "Disables hardware sound"                                    )
CMDLNE_PARAM (NETWORKING , NONE       , NONE , port             ,  0,  "Specify the port, default is 26000 (i.e. -port 26001)"      )
CMDLNE_PARAM (GAME       , NONE       , NONE , quoth            ,  0,  "Specifies game Quoth"                                       )
CMDLNE_PARAM (GLQUAKE    , CLIENT     , NONE , resizable        ,  0,  "Windows + OpenGL only, allow resizable on-the-fly window"   )
CMDLNE_PARAM (GAME       , NONE       , NONE , rogue            ,  0,  "Rogue Mission Pack 2"                                       )
CMDLNE_PARAM (VIDEO      , CLIENT     , NONE , width            ,  0,  "Specify width of window"                                    )
CMDLNE_PARAM (VIDEO      , CLIENT     , NONE , window           ,  0,  "Set windowed mode"                                          )
CMDLNE_PARAM (DEDTERM    , DEDICATED  , OBS  , HCHILD           ,  1,  "Set handle child for Qhost in dedicated server mode"        )
CMDLNE_PARAM (DEDTERM    , DEDICATED  , OBS  , HFILE            ,  1,  "Set handle file for Qhost in dedicated server mode"         )
CMDLNE_PARAM (DEDTERM    , DEDICATED  , OBS  , HPARENT          ,  1,  "Set handle parent for Qhost in dedicated server mode"       )
CMDLNE_PARAM (DEDTERM    , DEDICATED  , OBS  , listen           , -1,  "Listen server and # max players (-listen 4)"                )
CMDLNE_PARAM (INPUT      , NONE       , OBS  , nojoy            ,  0,  "Disable joystick"                                           )
CMDLNE_PARAM (GLQUAKE    , CLIENT     , OBS  , particles        ,  0,  "Specify max number on-screen particles, 2048 default"       )
CMDLNE_PARAM (AUDIO      , CLIENT     , OBS  , primarysound     ,  0,  "Windows, use primary sound buffer for DirectSound"          )
CMDLNE_PARAM (AUDIO      , CLIENT     , OBS  , simsound         ,  0,  "Windows, disable sound playback but enable sound functions ")
CMDLNE_PARAM (AUDIO      , CLIENT     , OBS  , sndspeed         ,  1,  "Sound speed in Hz, but there is cvar to do this in-game"    )
CMDLNE_PARAM (AUDIO      , CLIENT     , OBS  , snoforceformat   ,  0,  "Windows, disable forcing of sound format)"                  )
CMDLNE_PARAM (WINQUAKE   , CLIENT     , OBS  , surfacecachesize ,  0,  "Set surface cache size in kb"                               )
CMDLNE_PARAM (AUDIO      , CLIENT     , OBS  , wavonly          ,  0,  "Windows, disable DirectSound"                               )
CMDLNE_PARAM (VIDEO      , NONE       , OBS  , window           ,  0,  "Dynamic memory for aliases in kb (i.e. -zone 8192)"         )
CMDLNE_PARAM (MEMORY     , NONE       , OBS  , heapsize         ,  0,  "Memory in kb (i.e. -heapsize), default is plenty."    		)                                          )
CMDLNE_PARAM (MEMORY     , NONE       , OBS  , zone             ,  0,  "Memory in kb (i.e. -zone 8192), default is plenty"    		)                                          )


