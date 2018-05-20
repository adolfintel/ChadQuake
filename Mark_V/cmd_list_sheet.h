

CMD_DEF (Chase_Init  , DEP_CLIENT  , "chase_mode"                , Chase_Mode_f				, "Alternate chase_active 1 modes.  (0: Off, 1-8: alternate camera styles."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+attack"                   , IN_AttackDown			, "Player will start to fire the gun."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+back"                     , IN_BackDown				, "Player will start to move back."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+forward"                  , IN_ForwardDown			, "Player will start to move forward."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+jump"                     , IN_JumpDown				, "Player will start to jump."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+klook"                    , IN_KLookDown				, "When used the forward and back keys will make the player look up and down."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+left"                     , IN_LeftDown				, "Player will start to turn left."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+lookdown"                 , IN_LookdownDown			, "Player will start to look down."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+lookup"                   , IN_LookupDown			, "Player will start to look up."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+mlook"                    , IN_MLookDown				, "When used the mouse forward and back movement will make the player look up and down."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+movedown"                 , IN_DownDown				, "Player will start to move down in liquids."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+moveleft"                 , IN_MoveleftDown			, "Player will start to move left."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+moveright"                , IN_MoverightDown			, "Player will start to move right."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+moveup"                   , IN_UpDown				, "Player will start to move up in liquids."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+right"                    , IN_RightDown				, "Player will start to turn right."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+speed"                    , IN_SpeedDown				, "Player will run."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+strafe"                   , IN_StrafeDown			, "When used the turn left and turn right keys will make the player move left and move right."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "+use"                      , IN_UseDown				, "When used it will activate objects in the game.  But is unimplemented in the actual game."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-attack"                   , IN_AttackUp				, "Player will stop to fire the gun."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-back"                     , IN_BackUp				, "Player will stop to move back."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-forward"                  , IN_ForwardUp				, "Player will stop to move forward."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-jump"                     , IN_JumpUp				, "Player will stop to jump."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-klook"                    , IN_KLookUp				, "When used the forward and back keys will stop to make the player look up and down."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-left"                     , IN_LeftUp				, "Player will stop to turn left."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-lookdown"                 , IN_LookdownUp			, "Player will stop to look down."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-lookup"                   , IN_LookupUp				, "Player will stop to look up."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-mlook"                    , IN_MLookUp				, "When used the mouse forward and back movement will stop to make the player look up and down."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-movedown"                 , IN_DownUp				, "Player will stop to move down."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-moveleft"                 , IN_MoveleftUp			, "Player will stop to move left."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-moveright"                , IN_MoverightUp			, "Player will stop to move right."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-moveup"                   , IN_UpUp					, "Player will stop to move up."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-right"                    , IN_RightUp				, "Player will stop to turn right."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-speed"                    , IN_SpeedUp				, "Player will walk."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-strafe"                   , IN_StrafeUp				, "When used the turn left and turn right keys will once again perform their original functions."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "-use"                      , IN_UseUp					, "When used it will stop to activate objects in the game.  Not used in actual game."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "bestweapon"                , IN_BestWeapon			, "Set preferred weapon if available, specified by order."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "impulse"                   , IN_Impulse				, "Pass an impulse number to the server to perform a game function."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "pq_fullpitch"              , IN_PQ_Full_Pitch_f		, "External server hint limiting mouse/keyboard looking to standard Quake range for compatibility and to avoid mouse input irregularities."			)
CMD_DEF (CL_InitInput, DEP_CLIENT  , "cl_fullpitch"              , IN_PQ_Full_Pitch_f		, "External server hint limiting mouse/keyboard looking to standard Quake range for compatibility and to avoid mouse input irregularities."			)

CMD_DEF (Input_Init  , DEP_CLIENT  , "force_centerview"          , Input_Force_CenterView_f	, "Center the player's screen."			)
CMD_DEF (Input_Init  , DEP_CLIENT  , "in_info"                   , Input_Info_f				, "Internal input information."			)

#ifdef _WIN32
CMD_DEF ((voidfunc_t)Input_Local_Joystick_Startup, DEP_CLIENT  , "joyadvancedupdate"         , Input_Local_Joy_AdvancedUpdate_f, "(TODO: Unknown."		)
#endif // _WIN32

#ifdef GLQUAKE_RENDERER_SUPPORT
CMD_DEF (R_Init_Local, DEP_GL      , "envmap"				     , R_Envmap_f				, "Create an environmental map.  When this command is executed it will take environmental information in each of the 6 directions. The files will be named env0.rgb through env5.rgb and they will be stored in the id1 directory."			)
CMD_DEF (Fog_Init    , DEP_GL      , "fog"                       , Fog_FogCommand_f			, "Sets global fog, for testing purposes. All four values should be in the range 0...1. Set density to 0 to disable fog."			)
CMD_DEF (Fog_Init    , DEP_GL      , "fogex"                     , Fog_FogExCommand_f		, "Experimental alternate fog calculation."			)
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
CMD_DEF (R_Init_Local, DEP_SW      , "fog"                       , Cmd_No_Command			, "Sets global fog, for testing purposes. All four values should be in the range 0...1. Set density to 0 to disable fog."			) // Baker: CZG's honey spams this so we need it nulled out.
CMD_DEF (R_Init_Local, DEP_SW      , "fogex"                     , Cmd_No_Command			, "Experimental alternate fog calculation."			)
#endif // WINQUAKE_RENDERER_SUPPORT

CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "playmod"                   , FMOD_Play_f				, "Nehahra play music command."			)
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "stopmod"                   , FMOD_Stop_f				, "Nehahra stop playing music command."			)
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "play2"                     , S_Play2_f				, "Nehahra play a sound with no attentuation."			)
//CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "playvol"                   , S_PlayVol				, "Nehahra play a sound with a specific volume level."			)

CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "gl_fogenable"				 , Nehahra_FogEnable		, "Nehahra archaic fog enable command."			)
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "gl_fogdensity"    		 , Nehahra_FogDensity		, "Nehahra archaic fog density command."			)
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "gl_fogred"				 , Nehahra_FogRed			, "Nehahra archaic fog red command."			)
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "gl_foggreen"				 , Nehahra_FogGreen			, "Nehahra archaic fog green command."			)
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "gl_fogblue"				 , Nehahra_FogBlue			, "Nehahra archaic fog blue command."			)
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "nextchase"				 , Cmd_No_Command			, "Nehahra archaic command."			)
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "gl_notrans"				 , Cmd_No_Command			, "Nehahra archaic command."			)
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "r_oldsky"					 , Cmd_No_Command			, "Nehahra archaic command."			)
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "loadsky"					 , Cmd_No_Command			, "Nehahra archaic command."			) /* Sky_SkyCommand_f? */
CMD_DEF (Nehahra_Init, DEP_CL_NEH  , "r_nospr32"				 , Cmd_No_Command			, "Nehahra archaic command."			)

CMD_DEF (Sbar_Init   , DEP_CLIENT  , "+showscores"               , Sbar_ShowScores			, "When used the score screen will appear."			)
CMD_DEF (Sbar_Init   , DEP_CLIENT  , "-showscores"               , Sbar_DontShowScores		, "When used the score screen will disappear."			)

CMD_DEF (PR_Init     , DEP_HOST__  , "edict"                     , ED_PrintEdict_f			, "Display information about a game entity."			)
CMD_DEF (PR_Init     , DEP_HOST__  , "edictcount"                , ED_Count					, "Display the number of entities on the map."			)
CMD_DEF (PR_Init     , DEP_HOST__  , "edicts"                    , ED_PrintEdicts			, "Display information about all of the entities on the map."			)
CMD_DEF (PR_Init     , DEP_HOST__  , "profile"                   , PR_Profile_f				, "Display information about the amount of CPU cycles spend on processing game information."			)
CMD_DEF (PR_Init     , DEP_HOST__  , "qcexec"                    , PR_QC_Exec				, "Execute a QC function directly."			)
CMD_DEF (PR_Init     , DEP_HOST__  , "qcfuncs"                   , PR_Listfunctions_f		, "List QC functions."			)

CMD_DEF (Host_Init   , DEP_HOST__  , "begin"                     , Host_Begin_f				, "This command is used by the client to inform the server to begin sending game information."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "changelevel"               , Host_Changelevel_f		, "Change the map without dropping the connected clients from the server."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "color"                     , Host_Color_f				, "Set the color for the player's shirt and pants (color [shirt color] [pants color])."			)

CMD_DEF (Host_Init   , DEP_HOST__  , "connect"                   , Host_Connect_f			, "Connect to a game server (connect quake.shmack.net or connect quake.shmack.net:26001)."			)

#ifdef _DEBUG
CMD_DEF (Host_Init   , DEP_HOST__  , "crash"                      , Host_Crash_f			, "Force a crash by calling non-existent function."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "syserror"                   , Host_SysError_f			, "Force a system error."			)
#endif // _DEBUG

//CMD_DEF (Host_Init   , DEP_HOST__  , "dir"                       , Dir_f						, ""			)
//CMD_DEF (Host_Init   , DEP_HOST__  , "download"                  , Downloads_Download_f		, ""			)
//CMD_DEF (Host_Init   , DEP_HOST__  , "downloadzip"               , Download_Remote_Zip_Install_f	, ""		)
CMD_DEF (Host_Init   , DEP_HOST__  , "fly"                       , Host_Fly_f				, "Enable the fly cheat mode."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "game"                      , Host_Game_f				, "Change the gamedir (examples: game travail, game warp -quoth"			) //johnfitz

CMD_DEF (Host_Init   , DEP_HOST__  , "give"                      , Host_Give_f				, "Give the player a certain item (example: give silverkey)"			)
CMD_DEF (Host_Init   , DEP_HOST__  , "god"                       , Host_God_f				, "Enable the god mode cheat."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "kill"                      , Host_Kill_f				, "Make the player commit suicide."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "load"                      , Host_Loadgame_f			, "Load a saved game."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "map"                       , Host_Map_f				, "Change the current game map."			)
//CMD_DEF (Host_Init   , DEP_HOST__  , "mapname"                   , Host_Mapname_f			, "Display the current map."			) //johnfitz
CMD_DEF (Host_Init   , DEP_HOST__  , "name"                      , Host_Name_f				, "Set the player name."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "noclip"                    , Host_Noclip_f			, "Toggle the free flying cheat mode that allows walking through walls."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "notarget"                  , Host_Notarget_f			, "Toggle the ability of monsters to detect the player."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "pause"                     , Host_Pause_f				, "Temporarily stop the game."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "ping"                      , Host_Ping_f				, "Display the network latency times for all players on the server."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "prespawn"                  , Host_PreSpawn_f			, "Spawn the player entity on a given respawn spot."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "quit"                      , Host_Quit_f				, "Exit the game and return to the operating system."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "reconnect"                 , Host_Reconnect_f			, "Reconnect to the server."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "restart"                   , Host_Restart_f			, "Restart the current game.  It is said that this does not quite do everything in the same order as re-loading the map."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "save"                      , Host_Savegame_f			, "Save a game."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "say"                       , Host_Say_f				, "Send a message to all players on the server."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "say_team"                  , Host_Say_Team_f			, "Send a message to players only on your team."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "spawn"                     , Host_Spawn_f				, "Used to inform the client that the server is ready to send over the entity information."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "status"                    , Host_Status_f			, "Display information about current server status."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "tell"                      , Host_Tell_f				, "Send a private message to a specific player."			)
CMD_DEF (Host_Init   , DEP_HOST__  , "version"                   , Host_Version_f			, "Display the version information about the game."			)

CMD_DEF (Host_Init   , DEP_CLIENT  , "startdemos"                , Host_Startdemos_f		, "Used to list a queue of demos which should be played back by the demos command."			)
CMD_DEF (Host_Init   , DEP_CLIENT  , "stopdemo"                  , Host_Stopdemo_f			, "Stop the playback of a demo."			)
CMD_DEF (Host_Init   , DEP_NONE    , "_host_post_initialized"    , Host_Post_Initialization_f,"Internal command."            )
CMD_DEF (Host_Init   , DEP_HOST    , "mcache"                    , Mod_Print				, "Display information about all cached models."			)
CMD_DEF (Host_Init   , DEP_HOST    , "models"                    , Mod_PrintEx				, "Display a list of models currently cached."			) // Baker -- another way to get to this

CMD_DEF (Host_Init   , DEP_HOST    , "freezeall"                 , Host_Freezeall_f			, "Freeze all game entities except the player."			)
CMD_DEF (Host_Init   , DEP_HOST    , "sv_freezenonclients"       , Host_Legacy_FreezeAll_f	, "Use freezeall instead."			) // Baker -- another way to get to this

CMD_DEF (CL_Init     , DEP_CLIENT  , "cl_hints"                  , CL_Hints_List_f			, "List hints received from the server."			)
CMD_DEF (CL_Init     , DEP_CLIENT  , "disconnect"                , CL_Disconnect_f			, "Disconnect from a game server."			)
CMD_DEF (CL_Init     , DEP_CLIENT  , "entities"                  , CL_PrintEntities_f		, "Display a list of all the visible entities on the map."			)
CMD_DEF (CL_Init     , DEP_CLIENT  , "nextstartdemo"             , CL_PlayDemo_NextStartDemo_f , "Internal command."			)
CMD_DEF (CL_Init     , DEP_CLIENT  , "playdemo"                  , CL_PlayDemo_f			, "Playback a recorded game demo."			)
CMD_DEF (CL_Init     , DEP_CLIENT  , "record"                    , CL_Record_f				, "This command will start the recording of a game demo."			)
CMD_DEF (CL_Init     , DEP_CLIENT  , "stop"                      , CL_Stop_f				, "Stop the recording of a demo."			)
CMD_DEF (CL_Init     , DEP_CLIENT  , "timedemo"                  , CL_TimeDemo_f			, "Time the speed of demo playback to calculate the frames-per-second rate."			)
CMD_DEF (CL_Init     , DEP_CLIENT  , "setpos"                    , CL_Setpos_f				, "Set the player position."			)
CMD_DEF (CL_Init     , DEP_CLIENT  , "tracepos"                  , CL_Tracepos_f			, "Display impact point of trace along VPN."			) //johnfitz
CMD_DEF (CL_Init     , DEP_CLIENT  , "viewpos"                   , CL_Viewpos_f				, "Show current position and angles."			) //johnfitz
CMD_DEF (CL_Init     , DEP_CLIENT  , "r_pos"                     , CL_RPos_Legacy_f			, "Use scr_showpos instead."			) //johnfitz
CMD_DEF (CL_Init     , DEP_CLIENT  , "hdfolder"                  , HD_Folder_f				, "High definition content replacement folder.  Example: \"hdfolder hires\" would look for content replacement in c:\\quake\\hires\\ on Windows, if Quake were installed at c:\\Quake\\."			) //johnfitz

CMD_DEF (M_Init      , DEP_CLIENT  , "menu_keys"                 , M_Menu_Keys_f			, "Display the key configuration menu."			)
CMD_DEF (M_Init      , DEP_CLIENT  , "menu_load"                 , M_Menu_Load_f			, "Display the load menu for saved games."			)
CMD_DEF (M_Init      , DEP_CLIENT  , "menu_main"                 , M_Menu_Main_f			, "Display the main game menu."			)
CMD_DEF (M_Init      , DEP_CLIENT  , "menu_multiplayer"          , M_Menu_MultiPlayer_f		, "Display the multiplayer menu."			)
CMD_DEF (M_Init      , DEP_CLIENT  , "menu_options"              , M_Menu_Options_f			, "Display the options menu."			)
CMD_DEF (M_Init      , DEP_CLIENT  , "menu_quit"                 , M_Menu_Quit_f			, "Display the exit game menu."			)
CMD_DEF (M_Init      , DEP_CLIENT  , "menu_save"                 , M_Menu_Save_f			, "Display the save game menu."			)
CMD_DEF (M_Init      , DEP_CLIENT  , "menu_setup"                , M_Menu_Setup_f			, "Display the menu for player settings."			)
CMD_DEF (M_Init      , DEP_CLIENT  , "menu_singleplayer"         , M_Menu_SinglePlayer_f	, "Display the single-player menu."			)
CMD_DEF (M_Init      , DEP_CLIENT  , "menu_video"                , M_Menu_Video_f			, "Display the video options menu."			)
CMD_DEF (M_Init      , DEP_CLIENT  , "togglemenu"                , M_ToggleMenu_f			, "Display or close the main menu."			)
CMD_DEF (M_Init      , DEP_NONE    , "help"                      , _Help_f					, "Display the help screen."			)

CMD_DEF (SV_Init     , DEP_HOST__  , "sv_hints"                  , SV_Hints_List_f			, "List server hints."			) //johnfitz
CMD_DEF (SV_Init     , DEP_HOST__  , "sv_protocol"               , SV_Protocol_f			, "Sets the network protocol used by the server. Default is 666 (FitzQuake). Possible values are 15 (Quake) and 666 (FitzQuake)."			) //johnfitz

CMD_DEF (Key_Init    , DEP_CLIENT  , "bind"                      , Key_Bind_f				, "Assign a command or a set of commands to a key."			)
CMD_DEF (Key_Init    , DEP_CLIENT  , "bindlist"                  , Key_Bindlist_f			, "List current key binds."			) //johnfitz
CMD_DEF (Key_Init    , DEP_CLIENT  , "binds"                     , Key_Bindlist_f			, "List current key binds."			) // Baker --- alternate way to get here
CMD_DEF (Key_Init    , DEP_CLIENT  , "unbind"                    , Key_Unbind_f				, "Remove a binding from a key."			)
CMD_DEF (Key_Init    , DEP_CLIENT  , "unbindall"                 , Key_Unbindall_f			, "Remove bindings from all keys."			)

CMD_DEF (Cmd_Init    , DEP_HOST__  , "alias"                     , Cmd_Alias_f				, "Used to create a reference to a command or list of commands."			)
CMD_DEF (Cmd_Init    , DEP_HOST__  , "cmd"                       , Cmd_ForwardToServer		, "Send a command directly to the server."			)
CMD_DEF (Cmd_Init    , DEP_HOST__  , "cmdlist"                   , Cmd_List_f				, "List commands."			) //johnfitz
CMD_DEF (Cmd_Init    , DEP_HOST__  , "echo"                      , Cmd_Echo_f				, "Print text on the console."			)
CMD_DEF (Cmd_Init    , DEP_HOST__  , "exec"                      , Cmd_Exec_f				, "Execute a console script file."			)
CMD_DEF (Cmd_Init    , DEP_HOST__  , "files"                     , FS_List_Open_f			, "List open files."			)
CMD_DEF (Cmd_Init    , DEP_HOST__  , "stuffcmds"                 , Cmd_StuffCmds_f			, "Part of quake.rc.  Used to execute all command line commands that start with +."			)
CMD_DEF (Cmd_Init    , DEP_HOST__  , "unalias"                   , Cmd_Unalias_f			, "Remove an alias."			) //johnfitz
CMD_DEF (Cmd_Init    , DEP_HOST__  , "unaliasall"                , Cmd_Unaliasall_f			, "Remove all aliases."			) //johnfitz
CMD_DEF (Cmd_Init    , DEP_HOST__  , "wait"                      , Cmd_Wait_f				, "Stop the processing of commands for one game frame."			)

//CMD_DEF (NET_Init    , DEP_HOST__  , "ban"                       , NET_Ban_f				, ""			)
CMD_DEF (NET_Init    , DEP_HOST__  , "net_stats"                 , NET_Stats_f				, "Display network statistics."			)

// new
#ifdef INTERNAL_OR_FUTURE_COMMANDS // Don't confuse users with broken stuff.
CMD_DEF (NET_Init    , DEP_HOST__  , "ban"						 , NET_Ban_f				, "Ban by player #")
CMD_DEF (NET_Init    , DEP_HOST__  , "banip"                     , Admin_Ban_Ip_f			, "Ban by ip"   )
CMD_DEF (NET_Init    , DEP_HOST__  , "banlist"                   , Admin_Ban_List_f		    , "List adverse actions" )
CMD_DEF (NET_Init    , DEP_HOST__  , "whitelist"                 , Admin_White_List_f		, "List adverse actions" )
#endif // INTERNAL_OR_FUTURE_COMMANDS

CMD_DEF (NET_Init    , DEP_HOST__  , "kick"                      , Host_Kick_f				, "Kick a player")

#ifdef CORE_PTHREADS
CMD_DEF (NET_Init    , DEP_HOST__  , "mute"                      , Admin_Mute_f				, "Mute by player #")
CMD_DEF (NET_Init    , DEP_HOST__  , "lockserver"			     , Admin_Lock_f				, "Prevent new connections")
CMD_DEF (NET_Init    , DEP_HOST__  , "unlockserver"			     , Admin_UnLock_f				, "Re-enable new connections")
#endif // CORE_PTHREADS

CMD_DEF (NET_Init    , DEP_HOST__  , "test"                      , Test_f					, "Display information about the players connected to the server."			)
CMD_DEF (NET_Init    , DEP_HOST__  , "test2"                     , Test2_f					, "Display information about the current server settings."			)
CMD_DEF (NET_Init    , DEP_HOST__  , "slist"                     , NET_Slist_f			    , "Look for and display a list of all game servers on the local network."			)
CMD_DEF (NET_Init    , DEP_HOST__  , "listen"                    , NET_Listen_f				, "Specify the maximum number of players that can connect to the listen server."			)
CMD_DEF (NET_Init    , DEP_HOST__  , "maxplayers"                , MaxPlayers_f				, "The maximum number of players allowed on the server."			)
CMD_DEF (NET_Init    , DEP_HOST__  , "port"						 , NET_Port_f				, "The UDP port used by the game for network protocol."			)
CMD_DEF (NET_Init    , DEP_HOST__  , "rcon"						 , Rcon_f				    , "Remote console, to control a remote server as if using the console."			)

CMD_DEF (Cvar_Init   , DEP_NONE    , "cvarlist"                  , Cvar_List_f				, "List of console variables."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "find"						 , Cvar_Find_f				, "List of console variables containing text."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "apropos"					 , Host_Apropos_f			, "List of console variables containing text."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "cycle"                     , Cvar_Cycle_f				, "Cycle a console variable through a list of values."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "dec"                       , Cvar_Dec_f				, "Decrease the value of a console variable."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "resetall"                    , Cvar_ResetAll_f			, "Reset all console variables to default values."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "resetcfg"                  , Cvar_ResetCfg_f			, "Reset all console variables that write to config.cfg to default value."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "resetcvar"                 , Cvar_Reset_f				, "Reset a console variable to default value."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "toggle"                      , Cvar_Toggle_f			, "Toggle the value of a console variable between 0 and 1."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "inc"                       , Cvar_Inc_f				, "Increase the value of a console variable."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "valsave"					 , Cvar_ValSave_f			, "Save a floating point value of a console variable to a slot."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "valload"				     , Cvar_ValLoad_f			, "Set console variable to value of saved slot."			)
CMD_DEF (Cvar_Init   , DEP_NONE    , "mul"					     , Cvar_Multiply_f			, "Multiply the console variable times a certain value.." )

#ifdef SUPPORTS_AVI_CAPTURE
CMD_DEF (Movie_Init  , DEP_CLIENT  , "capturedemo"               , Movie_CaptureDemo_f		, "Video capture a demo file and turn it into an AVI file."			)
CMD_DEF (Movie_Init  , DEP_CLIENT  , "capturedemostop"           , Movie_Stop_Capturing		, "Stop video capturing a demo file."			)
CMD_DEF (Movie_Init  , DEP_CLIENT  , "capturevideo"              , Movie_Capture_Toggle_f	, "Toggle video capturing."			)
#endif // SUPPORTS_AVI_CAPTURE

CMD_DEF (S_Init      , DEP_CL_NEH  , "playvol"                   , S_PlayVol				, "Play a sound file at a given volume."			)
CMD_DEF (S_Init      , DEP_CLIENT  , "play"                      , S_Play_f					, "Play a sound file."			)
CMD_DEF (S_Init      , DEP_CLIENT  , "soundinfo"                 , S_SoundInfo_f			, "Display detailed information about the current sound settings."			)
CMD_DEF (S_Init      , DEP_CLIENT  , "soundlist"                 , S_SoundList				, "Display a list of all sound files loaded into memory."			)
CMD_DEF (S_Init      , DEP_CLIENT  , "stopsound"                 , S_StopAllSoundsC			, "Stop the playback of all sounds."			)

#ifdef GLQUAKE_RENDERER_SUPPORT
CMD_DEF (TexMgr_Init , DEP_GL      , "gl_describetexturemodes"   , TexMgr_DescribeTextureModes_f	, "Lists all texturemodes."		)
CMD_DEF (TexMgr_Init , DEP_GL      , "imagedump"                 , TexMgr_Imagedump_f		, "Dumps all loaded textures from opengl into tga files. This shows the textures as they exist in opengl texture memory. Texture names containing '*' will be renamed with '#' instead."			)
CMD_DEF (TexMgr_Init , DEP_GL      , "imagelist"                 , TexMgr_Imagelist_f		, "Displays a list of loaded textures, and their dimensions."			)
CMD_DEF (TexMgr_Init , DEP_MODEL   , "texreload"                 , TexMgr_ReloadImages_f	, "Reloads all textures."			)
CMD_DEF (TexMgr_Init , DEP_GL      , "textures"                  , TexMgr_Imagelist_f		, "Lists all textures."			) // Baker another way to get to this
#endif // GLQUAKE_RENDERER_SUPPORT

CMD_DEF (R_Init     , DEP_CLIENT  , "timerefresh"               , R_TimeRefresh_f			, "Peform a 360 degree turn and calculate the frames-per-second rate."			)
CMD_DEF (R_Init     , DEP_CLIENT  , "pointfile"                 , R_ReadPointFile_f			, "Load a point file to display leaks on the map."			)


CMD_DEF (Con_Init   , DEP_CLIENT  , "clear"                     , Con_Clear_f				, "Clears the console screen of any text."			)
CMD_DEF (Con_Init   , DEP_CLIENT  , "condump"                   , Con_Dump_f				, "Writes the console buffer to condump.txt."			) //johnfitz
CMD_DEF (Con_Init   , DEP_CLIENT  , "copy"                      , Con_Copy_f				, "Copies console to clipboard.  \"copy ents\" will copy .bsp entities to clipboard."			)
CMD_DEF (Con_Init   , DEP_CLIENT  , "messagemode"               , Con_MessageMode_f			, "Display the text line for sending messages to other players."			)
CMD_DEF (Con_Init   , DEP_CLIENT  , "messagemode2"              , Con_MessageMode2_f		, "Display the text line for sending messages to players only on your team."			)
CMD_DEF (Con_Init   , DEP_CLIENT  , "toggleconsole"             , Con_ToggleConsole_f		, "Lower or raise the console screen."			)


CMD_DEF (SCR_Init   , DEP_CLIENT  , "screenshot"                , SCR_ScreenShot_f			, "Take a picture of the current game screen."			)
CMD_DEF (SCR_Init   , DEP_CLIENT  , "sizedown"                  , SCR_SizeDown_f			, "Decrease the screen size for the game."			)
CMD_DEF (SCR_Init   , DEP_CLIENT  , "sizeup"                    , SCR_SizeUp_f				, "Increase the screen size for the game."			)


CMD_DEF (Lists_Init , DEP_CLIENT  , "configs"                   , List_Configs_f			, "List configs found in current gamedir directory."			) // Baker --- another way to get to this
CMD_DEF (Lists_Init , DEP_NONE    , "demos"                     , List_Demos_f				, "List demos found in current gamedir directory."			)
CMD_DEF (Lists_Init , DEP_NONE    , "games"                     , List_Games_f				, "List gamedirs found."			) // as an alias to "mods" -- S.A. / QuakeSpasm
CMD_DEF (Lists_Init , DEP_CLIENT  , "keys"                      , List_Keys_f				, "List key names."			)
CMD_DEF (Lists_Init , DEP_CLIENT  , "maps"                      , List_Maps_f				, "List maps found in current gamedir directory."			) //johnfitz
CMD_DEF (Lists_Init , DEP_CLIENT  , "mods"                      , List_Games_f				, "List gamedirs."			) //johnfitz
CMD_DEF (Lists_Init , DEP_CLIENT  , "mp3s"                      , List_MP3s_f				, "List maps found in current gamedir directory."			) //johnfitz
CMD_DEF (Lists_Init , DEP_NONE    , "saves"                     , List_Savegames_f			, "List saves found in current gamedir directory."			)
CMD_DEF (Lists_Init , DEP_CLIENT  , "skys"                      , List_Skys_f				, "List skyboxes found in current gamedir directory."			) //johnfitz
CMD_DEF (Lists_Init , DEP_CLIENT  , "sounds"                    , List_Sounds_f				, "List sounds found in current gamedir directory."			) // Baker --- another way to get to this

CMD_DEF (Recent_File_Init, DEP_CLIENT  , "folder"               , Recent_File_Show_f		, "Open the current gamedir folder or the folder of the most file written (demo, screenshot, ...) and highlight the file."			)
CMD_DEF (Recent_File_Init, DEP_CLIENT  , "showfile"             , Recent_File_Show_f		, "Open the current folder of most recently written file and highlight it."			)

#ifdef GLQUAKE_RENDERER_SUPPORT
CMD_DEF (VID_Init   , DEP_CLIENT  , "gl_info"                   , GL_Info_f					, "Displays opengl info which was previously displayed during initialization: vendor, renderer, version, and extensions."			)
CMD_DEF (VID_Init   , DEP_CLIENT  , "txgamma"				    , Vid_Gamma_TextureGamma_f  , "Set gamma while vid_hardwaregamma is off, applies it directly to the textures."			)
#endif // GLQUAKE_RENDERER_SUPPORT
CMD_DEF (VID_Init   , DEP_CLIENT  , "vid_restart"               , VID_Restart_f				, "Restart video mode."			)
CMD_DEF (VID_Init   , DEP_CLIENT  , "vid_test"                  , VID_Test					, "Test video mode."			)

CMD_DEF (View_Init  , DEP_CLIENT  , "bf"                        , View_BonusFlash_f			, "Perform a background screen flash."			)
CMD_DEF (View_Init  , DEP_CLIENT  , "centerview"                , View_StartPitchDrift		, "Centers the player's view."			)
CMD_DEF (View_Init  , DEP_CLIENT  , "v_cshift"                  , View_cshift_f				, "Shift the pallet for the game screen to produce color effects."			)

#ifdef GLQUAKE_RENDERER_SUPPORT
CMD_DEF (Entity_Inspector_Init, DEP_CLIENT  , "tool_inspector"   , Tool_Inspector_f			, "Toggle use of graphical entity inspector.  Switching weapon changes information display."			)
CMD_DEF (TexturePointer_Init, DEP_CLIENT  , "tool_texturepointer", Texture_Pointer_f		, "Toggle use of graphical texture pointer.  Looking at map surfaces displays texture name."			)
#endif // GLQUAKE_RENDERER_SUPPORT


CMD_DEF (COM_InitFilesystem, DEP_CLIENT  , "path"               , COM_Path_f				, "Display information about the current path locations used by the game."			)


CMD_DEF (Cache_Init , DEP_HOST__  , "flush"                     , Cache_Flush				, "Empty the game memory cache of all game information and objects."			)
CMD_DEF (Memory_Init, DEP_HOST__  , "hunk_print"                , Hunk_Print_f				, "Display information about current memory usage."			) //johnfitz


CMD_DEF (Sky_Init   , DEP_CLIENT  , "sky"                       , Sky_SkyCommand_f			, "Loads a skybox. If skyname is \"\", this will turn off skybox rendering."			)
CMD_DEF (CDAudio_Init,DEP_CLIENT  , "cd"                        , CD_f						, "Control the playback of music from the CD."			) // Needs to be available
CMD_DEF (CDAudio_Init,DEP_CL_MP3  , "setmusic"                  , Set_Music_f				, "Internal experimentation command."			) // Needs to be available

// Doesn't hurt anybody
CMD_DEF (Utilities_Init, DEP_HOST__  , "dir"                    , Dir_Command_f				, "Displays entire file tree of the current game directory."			)
CMD_DEF (Utilities_Init, DEP_HOST__  , "install"                , Install_Command_f				, "Install a single player release by gamedir name from the internet."			)
CMD_DEF (Utilities_Init, DEP_HOST__  , "uninstall"              , UnInstall_Command_f				, "Uninstall a single player release by gamedir."			)
#ifdef INTERNAL_OR_FUTURE_COMMANDS // Don't confuse users with broken stuff.
	#ifndef SERVER_ONLY
CMD_DEF (COM_InitFilesystem, DEP_CLIENT  , "exists"             , COM_Exists_f				, "Internal command."			)
CMD_DEF (Utilities_Init, DEP_HOST__  , "pak"                    , Pak_Command_f				, "Internal experimentation command."			)
CMD_DEF (Utilities_Init, DEP_HOST__  , "zip"                    , Zip_Command_f				, "Internal experimentation command."			)

CMD_DEF (Utilities_Init, DEP_HOST__  , "servefiles"             , ServeFile_Command_f				, "Internal experimentation command."			)
CMD_DEF (Utilities_Init, DEP_HOST__  , "shutdown"               , ServeFile_Shutdown_Command_f				, "Internal experimentation command."			)
CMD_DEF (Utilities_Init, DEP_HOST__  , "getfile"                , GetFile_Command_f				, "Internal experimentation command."			)

//CMD_DEF (Utilities_Init, DEP_HOST__  , "opend"                    , OpenD_Command_f				, ""			)
//CMD_DEF (Utilities_Init, DEP_HOST__  , "saved"                    , SaveD_Command_f				, ""			)
	#endif // ! SERVER_ONLY
#endif  // INTERNAL_OR_FUTURE_COMMANDS





#undef CMD_DEF

