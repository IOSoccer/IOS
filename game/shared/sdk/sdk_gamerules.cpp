//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "multiplay_gamerules.h"
#include "sdk_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "weapon_sdkbase.h"

extern void Bot_RunAll( void );

#ifdef CLIENT_DLL

	#include "precache_register.h"
	#include "c_sdk_player.h"
	#include "c_sdk_team.h"

#else
	
	#include "voice_gamemgr.h"
	#include "sdk_player.h"
	#include "sdk_team.h"
	#include "sdk_playerclass_info_parse.h"
	#include "player_resource.h"
	#include "mapentities.h"
	#include "sdk_basegrenade_projectile.h"
	#include "sdk_player.h"		//ios
	#include "game.h"			//ios
	#include "ios_mapentities.h"

	#include "movehelper_server.h"
#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CUniformRandomStream g_IOSRand;

const s_KitData gKitDesc[] =
{
	{	"Brazil", "Brazil",		YELLOW },
	{	"England", "England",		WHITE },
	{	"Germany", "Germany",		WHITE },
	{	"Italy",	"Italy",	BLUE },
	{	"Scotland", "Scotland",		BLUE },
	{	"Barcelona", "FC Barcelona",	BLUE },
	{	"Bayern",	"FC Bayern Munich",	RED },
	{	"Liverpool", "Liverpool FC",	RED },
	{	"Milan",	"AC Milan", 	RED },
	{	"Palmeiras", "Sociedade Esportiva Palmeiras",	GREEN },
	{	"END", "END", END },
};

const char g_szPosNames[21][5] =
{
	"GK", "SWP", "LB", "RB", "CB", "LCB", "RCB", "LWB", "RWB", "LM", "RM", "DM", "CM", "AM", "LF", "RF", "CF", "ST", "SS", "LW", "RW"
};

enum g_PosNames_t
{
	GK = 0, SWP, LB, RB, CB, LCB, RCB, LWB, RWB, LM, RM, DM, CM, AM, LF, RF, CF, ST, SS, LW, RW
};

#define HIDDEN { -1, -1, -1, -1 }

const float g_Positions[11][11][4] =
{
	{//1
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//2
								{ 1.5f, 1, CM, 10 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//3
					{ 0.5f, 1, LM, 11 }, { 2.5f, 1, RM, 9 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//4
					{ 0.5f, 1, LF, 11 }, { 2.5f, 1, RF, 9 },
								{ 1, 2, CM, 10 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//5
					{ 0.5f, 1, LM, 11 }, { 2.5f, 1, RM, 9 },
						{ 0, 2, LB, 2 }, { 3, 2, RB, 3 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//6
					{ 0.5f, 0.5f, LF, 11 }, { 2.5f, 0.5f, RF, 9 },
								{ 1.5f, 1, CM, 10 },
					{ 0.75f, 2, LB, 2 }, { 2.25f, 2, RB, 3 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//7
								{ 1.5f, 0, CF, 10 },
			{ 0.5f, 1, LM, 8 }, { 1.5f, 1, CM, 6 }, { 2.5f, 1, RM, 7 },
						{ 0, 2, LB, 2 }, { 3, 2, RB, 3 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//8
								{ 1.5f, 0, CF, 10 },
			{ 0.5f, 1, LM, 11 }, { 1.5f, 1, CM, 6 }, { 2.5f, 1, RM, 7 },
				{ 0, 2, LB, 3 }, { 1, 2, CB, 4 }, { 3, 2, RB, 5 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN
	},
	{//9
					{ 0.5f, 0, LF, 11 }, { 2.5f, 0, RF, 9 },
			{ 0.5f, 1, LM, 11 }, { 1.5f, 1, CM, 10 }, { 2.5f, 1, RM, 7 },
							{ 0, 2, LB, 2 }, { 1, 2, CB, 3 }, { 3, 2, RB, 4 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN
	},
	{//10
			{ 0.5f, 0, LW, 11 }, { 1.5f, 0, CF, 10 }, { 2.5f, 0, RW, 9 },
			{ 0.5f, 1, LM, 8 }, { 1.5f, 1, CM, 6 }, { 2.5f, 1, RM, 7 },
				{ 0, 2, LB, 2 }, { 1, 2, CB, 3 }, { 3, 2, RB, 4 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN
	},
	{//11
			{ 0.5f, 0, LW, 11 }, { 1.5f, 0, CF, 10 }, { 2.5f, 0, RW, 9 },
			{ 0.5f, 1, LM, 8 }, { 1.5f, 1, CM, 6 }, { 2.5f, 1, RM, 7 },
		{ 0, 2, RB, 2 }, { 1, 2, LCB, 3 }, { 2, 2, RCB, 4 }, { 3, 2, RB, 5 },
								{ 1.5f, 3, GK, 1 }
	}
};

bool IsValidPosition(int posIndex)
{
	return g_Positions[mp_maxplayers.GetInt() - 1][posIndex][POS_NUMBER] != -1;
}

int GetKeeperPosIndex()
{
	for (int posIndex = 0; posIndex < 11; posIndex++)
	{
		if (g_Positions[mp_maxplayers.GetInt() - 1][posIndex][POS_NAME] == GK)
			return posIndex;
	}
	return 0;
}

#ifndef CLIENT_DLL

class CSpawnPoint : public CPointEntity
{
public:
	bool IsDisabled() { return m_bDisabled; }
	void InputEnable( inputdata_t &inputdata ) { m_bDisabled = false; }
	void InputDisable( inputdata_t &inputdata ) { m_bDisabled = true; }

private:
	bool m_bDisabled;
	DECLARE_DATADESC();
};

BEGIN_DATADESC(CSpawnPoint)

	// Keyfields
	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),

END_DATADESC();

	LINK_ENTITY_TO_CLASS( info_player_deathmatch, CSpawnPoint );
#if defined( SDK_USE_TEAMS )
	LINK_ENTITY_TO_CLASS( info_player_blue, CSpawnPoint );
	LINK_ENTITY_TO_CLASS( info_player_red, CSpawnPoint );
#endif

#endif


REGISTER_GAMERULES_CLASS( CSDKGameRules );

//#ifdef CLIENT_DLL
//void RecvProxy_MatchState( const CRecvProxyData *pData, void *pStruct, void *pOut )
//{
//	CSDKGameRules *pGamerules = ( CSDKGameRules *)pStruct;
//	match_state_t eMatchState = (match_state_t)pData->m_Value.m_Int;
//	pGamerules->SetMatchState( eMatchState );
//}
//#endif 

BEGIN_NETWORK_TABLE_NOBASE( CSDKGameRules, DT_SDKGameRules )
#if defined ( CLIENT_DLL )
	RecvPropFloat( RECVINFO( m_flStateEnterTime ) ),
	//RecvPropFloat( RECVINFO( m_fStart) ),
	//RecvPropInt( RECVINFO( m_iDuration) ),
	RecvPropInt( RECVINFO( m_eMatchState) ),// 0, RecvProxy_MatchState ),
	RecvPropInt( RECVINFO( m_nAnnouncedInjuryTime) ),// 0, RecvProxy_MatchState ),
	RecvPropFloat( RECVINFO( m_flInjuryTimeStart) ),// 0, RecvProxy_MatchState ),
	RecvPropFloat( RECVINFO( m_flInjuryTime) ),// 0, RecvProxy_MatchState ),

	RecvPropInt(RECVINFO(m_nShieldType)),
	RecvPropInt(RECVINFO(m_nShieldSide)),
	RecvPropInt(RECVINFO(m_nShieldRadius)),
	RecvPropVector(RECVINFO(m_vShieldPos)),

	RecvPropVector(RECVINFO(m_vFieldMin)),
	RecvPropVector(RECVINFO(m_vFieldMax)),
	RecvPropVector(RECVINFO(m_vKickOff)),
#else
	SendPropFloat( SENDINFO( m_flStateEnterTime ), 32, SPROP_NOSCALE ),
	//SendPropFloat( SENDINFO( m_fStart) ),
	//SendPropInt( SENDINFO( m_iDuration) ),
	SendPropInt( SENDINFO( m_eMatchState )),
	SendPropInt( SENDINFO( m_nAnnouncedInjuryTime )),
	SendPropFloat( SENDINFO( m_flInjuryTimeStart ), 32, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flInjuryTime ), 32, SPROP_NOSCALE ),

	SendPropInt(SENDINFO(m_nShieldType)),
	SendPropInt(SENDINFO(m_nShieldSide)),
	SendPropInt(SENDINFO(m_nShieldRadius)),
	SendPropVector(SENDINFO(m_vShieldPos), -1, SPROP_COORD),

	SendPropVector(SENDINFO(m_vFieldMin), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vFieldMax), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vKickOff), -1, SPROP_COORD),
#endif
END_NETWORK_TABLE()

#if defined ( SDK_USE_PLAYERCLASSES )
	ConVar mp_allowrandomclass( "mp_allowrandomclass", "1", FCVAR_REPLICATED, "Allow players to select random class" );
#endif


LINK_ENTITY_TO_CLASS( sdk_gamerules, CSDKGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( SDKGameRulesProxy, DT_SDKGameRulesProxy )


#ifdef CLIENT_DLL
	void RecvProxy_SDKGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CSDKGameRules *pRules = SDKGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CSDKGameRulesProxy, DT_SDKGameRulesProxy )
		RecvPropDataTable( "sdk_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_SDKGameRules ), RecvProxy_SDKGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_SDKGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CSDKGameRules *pRules = SDKGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CSDKGameRulesProxy, DT_SDKGameRulesProxy )
		SendPropDataTable( "sdk_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_SDKGameRules ), SendProxy_SDKGameRules )
	END_SEND_TABLE()
#endif

#ifndef CLIENT_DLL
	ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade","0");		
#endif

ConVar mp_limitteams( 
	"mp_limitteams", 
	"2", 
	FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Max # of players 1 team can have over another (0 disables check)",
	true, 0,	// min value
	true, 30	// max value
);

static CSDKViewVectors g_SDKViewVectors(

	Vector( 0, 0, 58 ),			//VEC_VIEW
	//Vector( 0, 0, 100 ),			//VEC_VIEW
								
	//Vector(-16, -16, 0 ),		//VEC_HULL_MIN
	//Vector( 16,  16,  72 ),		//VEC_HULL_MAX
	
	Vector(-10, -10, 0 ),		//VEC_HULL_MIN
	Vector( 10,  10,  72 ),		//VEC_HULL_MAX
													
	Vector(-16, -16, 0 ),		//VEC_DUCK_HULL_MIN
	Vector( 16,  16, 45 ),		//VEC_DUCK_HULL_MAX
	Vector( 0, 0, 34 ),			//VEC_DUCK_VIEW
													
	Vector(-10, -10, -10 ),		//VEC_OBS_HULL_MIN
	Vector( 10,  10,  10 ),		//VEC_OBS_HULL_MAX
													
	Vector( 0, 0, 14 )			//VEC_DEAD_VIEWHEIGHT
#if defined ( SDK_USE_PRONE )			
	,Vector(-16, -16, 0 ),		//VEC_PRONE_HULL_MIN
	Vector( 16,  16, 24 ),		//VEC_PRONE_HULL_MAX
	Vector( 0,	0, 10 )			//VEC_PRONE_VIEW
#endif
);

const CViewVectors* CSDKGameRules::GetViewVectors() const
{
	return (CViewVectors*)GetSDKViewVectors();
}

const CSDKViewVectors *CSDKGameRules::GetSDKViewVectors() const
{
	return &g_SDKViewVectors;
}

#ifdef CLIENT_DLL


#else

// --------------------------------------------------------------------------------------------------- //
// Voice helper
// --------------------------------------------------------------------------------------------------- //

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		// Dead players can only be heard by other dead team mates
		if ( pTalker->IsAlive() == false )
		{
			if ( pListener->IsAlive() == false )
				return ( pListener->InSameTeam( pTalker ) );

			return false;
		}

		return ( pListener->InSameTeam( pTalker ) );
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;



// --------------------------------------------------------------------------------------------------- //
// Globals.
// --------------------------------------------------------------------------------------------------- //
static const char *s_PreserveEnts[] =
{
	"player",
	"viewmodel",
	"worldspawn",
	"soundent",
	"ai_network",
	"ai_hint",
	"sdk_gamerules",
	"sdk_team_manager",
	"sdk_team_unassigned",
	"sdk_team_blue",
	"sdk_team_red",
	"sdk_player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sprite",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_brush",
	"func_wall",
	"func_illusionary",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_red",
	"info_player_blue",
	"point_viewcontrol",
	"shadow_control",
	"sky_camera",
	"scene_manager",
	"trigger_soundscape",
	"point_commentary_node",
	"func_precipitation",
	"func_team_wall",
	"", // END Marker
};

// --------------------------------------------------------------------------------------------------- //
// Global helper functions.
// --------------------------------------------------------------------------------------------------- //

// World.cpp calls this but we don't use it in SDK.
void InitBodyQue()
{
}


// --------------------------------------------------------------------------------------------------- //
// CSDKGameRules implementation.
// --------------------------------------------------------------------------------------------------- //

CSDKGameRules::CSDKGameRules()
{
	g_IOSRand.SetSeed(Plat_FloatTime() * 1000);

	m_pCurStateInfo = NULL;

	m_nShieldType = SHIELD_NONE;
	m_vShieldPos = vec3_invalid;
	m_bAreTeamsSwapped = false;
	m_flStateTimeLeft = 0;
	m_flNextPenalty = gpGlobals->curtime;
	m_nPenaltyTakingTeam = TEAM_A;
	m_flInjuryTime = 0;
	m_flInjuryTimeStart = -1;

	//ios m_bLevelInitialized = false;
	//m_flMatchStartTime = 0;
}
void CSDKGameRules::ServerActivate()
{
	//Tony; initialize the level
	//ios CheckLevelInitialized();

	//Tony; do any post stuff
	//m_flMatchStartTime = gpGlobals->curtime;
	/*if ( !IsFinite( m_flMatchStartTime.Get() ) )
	{
		Warning( "Trying to set a NaN game start time\n" );
		m_flMatchStartTime.GetForModify() = 0.0f;
	}*/

	InitTeams();

	CBaseEntity *pEnt = gEntList.FindEntityByClassname(NULL, "info_ball_start");
	trace_t tr;
	UTIL_TraceLine(pEnt->GetLocalOrigin(), Vector(0, 0, -500), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr);

	m_vKickOff = Vector(pEnt->GetLocalOrigin().x, pEnt->GetLocalOrigin().y, tr.endpos.z);
	
	float minX = -FLT_MAX;
	float maxX = FLT_MAX;

	CBaseEntity *pSidelineTrigger = NULL;
	while (pSidelineTrigger = gEntList.FindEntityByClassname(pSidelineTrigger, "trigger_SideLine"))
	{
		Vector min, max;
		pSidelineTrigger->CollisionProp()->WorldSpaceTriggerBounds(&min, &max);
		if (max.x > m_vKickOff.GetX())
		{
			if (max.x < maxX)
				maxX = max.x;
		}
		else
		{
			if (min.x > minX)
				minX = min.x;
		}
	}

	float minY = -FLT_MAX;
	float maxY = FLT_MAX;

	CBaseEntity *pGoalTrigger = NULL;
	while (pGoalTrigger = gEntList.FindEntityByClassname(pGoalTrigger, "trigger_goal"))
	{
		Vector min, max;
		pGoalTrigger->CollisionProp()->WorldSpaceTriggerBounds(&min, &max);
		if (max.y > m_vKickOff.GetY())
		{
			if (max.y < maxY)
				maxY = max.y;
		}
		else
		{
			if (min.y > minY)
				minY = min.y;
		}
	}

	m_vFieldMin = Vector(minX, minY, m_vKickOff.GetZ());
	m_vFieldMax = Vector(maxX, maxY, m_vKickOff.GetZ());

	GetGlobalTeam(TEAM_A)->InitFieldSpots(TEAM_A);
	GetGlobalTeam(TEAM_B)->InitFieldSpots(TEAM_B);

	m_pPrecip = (CPrecipitation *)CreateEntityByName("func_precipitation");
	m_pPrecip->SetType(PRECIPITATION_TYPE_NONE);
	m_pPrecip->Spawn();


	//TODO: remove this
	CBaseEntity *pCrossbar = NULL;
	while (pCrossbar = gEntList.FindEntityByModel(pCrossbar, "goalposts.mdl"))
	{
		pCrossbar->SetRenderMode(kRenderTransColor);
		pCrossbar->SetRenderColorA(75);
	}

	State_Transition(MATCH_INIT);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSDKGameRules::~CSDKGameRules()
{
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: TF2 Specific Client Commands
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CSDKGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CSDKPlayer *pPlayer = ToSDKPlayer( pEdict );
#if 0
	const char *pcmd = args[0];
	if ( FStrEq( pcmd, "somecommand" ) )
	{
		if ( args.ArgC() < 2 )
			return true;

		// Do something here!

		return true;
	}
	else 
#endif
	// Handle some player commands here as they relate more directly to gamerules state
	if ( pPlayer->ClientCommand( args ) )
	{
		return true;
	}
	else if ( BaseClass::ClientCommand( pEdict, args ) )
	{
		return true;
	}
	return false;
}

void CSDKGameRules::Think()
{
	State_Think();
	//BaseClass::Think();		//this breaks stuff, like voice comms!

	GetVoiceGameMgr()->Update( gpGlobals->frametime );

	//Bot_RunAll();	//ios

	//IOS hold on intermission
	if (m_flIntermissionEndTime > gpGlobals->curtime)
		return;

	if ( g_fGameOver )   // someone else quit the game already
	{
		ChangeLevel(); // intermission is over
		return;
	}

	//if (GetMapRemainingTime() < 0)
	//	GoToIntermission();
}

Vector DropToGround( 
					CBaseEntity *pMainEnt, 
					const Vector &vPos, 
					const Vector &vMins, 
					const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}

//the 'recountteams' in teamplay_gamerules looks like the hl1 version so usin CTeam stuff here
void CSDKGameRules::CountTeams(void)
{
	//clr current
	for (int j = 0; j < TEAMS_COUNT; j++)
		m_PlayersOnTeam[j] = 0;

	//count players on team
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )	
	{
		CSDKPlayer *plr = (CSDKPlayer*)UTIL_PlayerByIndex( i );

		if (!plr)
			continue;

		if (!plr->IsPlayer())
			continue;

		if ( !plr->IsConnected() )
			continue;

		//check for null,disconnected player
		if (strlen(plr->GetPlayerName()) == 0)
			continue;

		m_PlayersOnTeam[plr->GetTeamNumber()]++;
	}
}

extern ConVar tv_delaymapchange;
#include "hltvdirector.h"
#include "viewport_panel_names.h"

void CSDKGameRules::GoToIntermission( void )
{
	if ( g_fGameOver )
		return;

	g_fGameOver = true;

	float flWaitTime = 0;

	if ( tv_delaymapchange.GetBool() && HLTVDirector()->IsActive() )	
	{
		flWaitTime = HLTVDirector()->GetDelay();
	}
			
	m_flIntermissionEndTime = gpGlobals->curtime + flWaitTime;
}


///////////////////////////////////////////////////
// auto balance teams if mismatched
//
void CSDKGameRules::AutobalanceTeams(void)
{
	CSDKPlayer *pSwapPlayer=NULL;
	int teamNow = 0, teamToBe;
	float mostRecent = 0.0f;

	bool bDoAutobalance = autobalance.GetBool();

	if (!bDoAutobalance)
		return;

	//check if teams need autobalancing
	CountTeams();
	if (m_PlayersOnTeam[TEAM_A] > m_PlayersOnTeam[TEAM_B]+1) 
	{
		teamNow  = TEAM_A;
		teamToBe = TEAM_B;
	} 
	else if (m_PlayersOnTeam[TEAM_B] > m_PlayersOnTeam[TEAM_A]+1) 
	{
		teamNow  = TEAM_B;
		teamToBe = TEAM_A;
	}
	else 
	{
		//no balancing required
		return;
	}

   //find 1 player to change per round?
   bool bFoundValidPlayer = false;
	//clear all the players data
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )	
	{
		CSDKPlayer *plr = (CSDKPlayer*)UTIL_PlayerByIndex( i );

		if (!plr)
			continue;

		if (!plr->IsPlayer())
			continue;

		if ( !plr->IsConnected() )
			continue;

		//check for null,disconnected player
		if (strlen(plr->GetPlayerName()) == 0)
			continue;

		//ignore bots
		if (plr->GetFlags() & FL_FAKECLIENT)
			continue;

		if (plr->GetTeamNumber() < TEAM_A)
			continue;

		//dont switch keepers
		//if (plr->m_TeamPos == 1)
		//	continue;

		//found one
		if (plr->GetTeamNumber() == teamNow) 
		{
			//see if player is new to the game
			if (plr->m_JoinTime > mostRecent) 
			{
				pSwapPlayer = plr;
				mostRecent = plr->m_JoinTime;
				bFoundValidPlayer = true;
			}
		}
	}  

	if (!bFoundValidPlayer)
		return;

	//mimic vgui team selection
	//SetPlayerTeam(swapPlayer, teamToBe+1);
	pSwapPlayer->ChangeTeam(teamToBe);

	//int trypos = 11;
	//while (!pSwapPlayer->TeamPosFree(teamToBe, trypos) && trypos > 1) 
	//{
	//	trypos--;
	//}

	//pSwapPlayer->m_TeamPos = trypos;
	//pSwapPlayer->ChooseModel();

	pSwapPlayer->Spawn();
	g_pGameRules->GetPlayerSpawnSpot( pSwapPlayer );
	pSwapPlayer->RemoveEffects( EF_NODRAW );
	pSwapPlayer->SetSolid( SOLID_BBOX );
	pSwapPlayer->RemoveFlag(FL_FROZEN);


	//print autobalance msg - markg
	//char text[256];
	//sprintf( text, "%s was team autobalanced\n",  STRING( swapPlayer->pev->netname ));
	//MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
	//	WRITE_BYTE( ENTINDEX(swapPlayer->edict()) );
	//	WRITE_STRING( text );
	//MESSAGE_END();
}

void CSDKGameRules::PlayerSpawn( CBasePlayer *p )
{	
	CSDKPlayer *pPlayer = ToSDKPlayer( p );

	int team = pPlayer->GetTeamNumber();

	if( team != TEAM_SPECTATOR )
	{
		//pPlayer->PrecacheModel( "models/player/barcelona/barcelona.mdl" );
		//pPlayer->SetModel( "models/player/barcelona/barcelona.mdl" );
		//pPlayer->SetHitboxSet( 0 );
	}
}

void CSDKGameRules::InitTeams( void )
{
	Assert( g_Teams.Count() == 0 );

	g_Teams.Purge();	// just in case

	ChooseTeamNames();

	// Create the team managers
	for ( int i = 0; i < ARRAYSIZE( pszTeamNames ); i++ )
	{
		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "sdk_team_manager" ));

		pTeam->Init( pszTeamNames[i], i );

		g_Teams.AddToTail( pTeam );
	}

	CreateEntityByName( "sdk_gamerules" );
}

//ios
void CSDKGameRules::ChooseTeamNames()
{
	int numKits = 0;

	//count the available kits
	while (strcmp(gKitDesc[numKits].m_KitName, "END"))
		numKits++;

	numKits--;		//adjust the final values

	//now look for two random that dont have the same colour
	int teamtype1 = 0;
	int teamtype2 = 0;

	while (gKitDesc[teamtype1].m_KitColour == gKitDesc[teamtype2].m_KitColour)
	{
		teamtype1 = g_IOSRand.RandomInt(0,numKits);
		teamtype2 = g_IOSRand.RandomInt(0,numKits);
	}

	SetTeams(gKitDesc[teamtype1].m_KitName, gKitDesc[teamtype2].m_KitName, false);
}

/* create some proxy entities that we use for transmitting data */
void CSDKGameRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "sdk_player_manager", vec3_origin, vec3_angle );

	// Create the entity that will send our data to the client.
#ifdef _DEBUG
	CBaseEntity *pEnt = 
#endif
		CBaseEntity::Create( "sdk_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
}

#endif

ConVar mp_ball_player_collision("mp_ball_player_collision", "0", FCVAR_ARCHIVE | FCVAR_NOTIFY | FCVAR_REPLICATED);

bool CSDKGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_PUSHAWAY )
	{
		if (mp_ball_player_collision.GetBool())
			return true;
		else
			return false;
	}

	//Don't stand on COLLISION_GROUP_WEAPON
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

//Tony; keep this in shared space.
#if defined ( SDK_USE_PLAYERCLASSES )
const char *CSDKGameRules::GetPlayerClassName( int cls, int team )
{
	CSDKTeam *pTeam = GetGlobalSDKTeam( team );

	if( cls == PLAYERCLASS_RANDOM )
	{
		return "#class_random";
	}

	if( cls < 0 || cls >= pTeam->GetNumPlayerClasses() )
	{
		Assert( false );
		return NULL;
	}

	const CSDKPlayerClassInfo &pClassInfo = pTeam->GetPlayerClassInfo( cls );

	return pClassInfo.m_szPrintName;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Init CS ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;

		for (int i=WEAPON_NONE+1;i<WEAPON_MAX;i++)
		{
			//Tony; ignore grenades, shotgun and the crowbar, grenades and shotgun are handled seperately because of their damage type not being DMG_BULLET.
			if (i == SDK_WEAPON_GRENADE || i == SDK_WEAPON_CROWBAR || i == SDK_WEAPON_SHOTGUN)
				continue;

			def.AddAmmoType( WeaponIDToAlias(i), DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 200/*max carry*/, 1, 0 );
		}

		// def.AddAmmoType( BULLET_PLAYER_50AE,		DMG_BULLET, TRACER_LINE, 0, 0, "ammo_50AE_max",		2400, 0, 10, 14 );
		def.AddAmmoType( "shotgun", DMG_BUCKSHOT, TRACER_NONE, 0, 0,	200/*max carry*/, 1, 0 );
		def.AddAmmoType( "grenades", DMG_BLAST, TRACER_NONE, 0, 0,	4/*max carry*/, 1, 0 );

		//Tony; added for the sdk_jeep
		def.AddAmmoType( "JeepAmmo",	DMG_SHOCK,					TRACER_NONE,			"sdk_jeep_weapon_damage",		"sdk_jeep_weapon_damage", "sdk_jeep_max_rounds", BULLET_IMPULSE(650, 8000), 0 );
	}

	return &def;
}


#ifndef CLIENT_DLL

const char *CSDKGameRules::GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer )
{
	//Tony; no prefix for now, it isn't needed.
	//ios return "";

	if (bTeamOnly)
		return "(TEAM)";
	else
		return "";
}

const char *CSDKGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
		return NULL;

	const char *pszFormat = NULL;

	if ( bTeamOnly )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			pszFormat = "SDK_Chat_Spec";
		else
		{
			if (pPlayer->m_lifeState != LIFE_ALIVE )
				pszFormat = "SDK_Chat_Team_Dead";
			else
				pszFormat = "SDK_Chat_Team_Loc";
		}
	}
	else
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR)
			pszFormat = "SDK_Chat_AllSpec";
		else
		{
			if (pPlayer->m_lifeState != LIFE_ALIVE )
				pszFormat = "SDK_Chat_All_Dead";
			else
				pszFormat = "SDK_Chat_All_Loc";
		}
	}

	return pszFormat;
}

const char *CSDKGameRules::GetChatLocation( bool bTeamOnly, CBasePlayer *pPlayer )
{
	return g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][ToSDKPlayer(pPlayer)->GetTeamPosIndex()][POS_NAME]];
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Find the relationship between players (teamplay vs. deathmatch)
//-----------------------------------------------------------------------------
int CSDKGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
#ifndef CLIENT_DLL
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() || IsTeamplay() == false )
		return GR_NOTTEAMMATE;

	if ( (*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp( GetTeamID(pPlayer), GetTeamID(pTarget) ) )
		return GR_TEAMMATE;

#endif

	return GR_NOTTEAMMATE;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Player has just left the game
//-----------------------------------------------------------------------------
void CSDKGameRules::ClientDisconnected( edict_t *pClient )
{
	CSDKPlayer *pPl = (CSDKPlayer *)CBaseEntity::Instance(pClient);
	CPlayerPersistentData::SavePlayerData(pPl);

	BaseClass::ClientDisconnected( pClient );
}
#endif



#ifndef CLIENT_DLL

void CSDKGameRules::RestartMatch()
{
	GetBall()->CreateVPhysics();
	State_Transition(MATCH_WARMUP);
}

void CC_SV_Restart(const CCommand &args)
{
	//ffs!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

	if (args.ArgC() > 1)
		mp_timelimit_warmup.SetValue((float)atof(args[1]));

	SDKGameRules()->RestartMatch();
}


ConCommand sv_restart( "sv_restart", CC_SV_Restart, "Restart game", 0 );

void CSDKGameRules::SetWeather(PrecipitationType_t type)
{
	m_pPrecip->SetType(type);
}

static void OnWeatherTypeChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	SDKGameRules()->SetWeather((PrecipitationType_t)((ConVar*)var)->GetInt());
}
 
ConVar mp_weather("mp_weather", "0", 0, "Weather (0 = sunny, 1 = rainy, 2 = snowy)", true, 0, true, 2, OnWeatherTypeChange );

#endif

ConVar mp_showstatetransitions( "mp_showstatetransitions", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show game state transitions." );

ConVar mp_timelimit_match( "mp_timelimit_match", "10", FCVAR_NOTIFY|FCVAR_REPLICATED, "match duration in minutes without breaks (90 is real time)" );
ConVar mp_timelimit_warmup( "mp_timelimit_warmup", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time before match start" );
ConVar mp_timelimit_cooldown( "mp_timelimit_cooldown", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time after match end" );
ConVar mp_timelimit_halftime( "mp_timelimit_halftime", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "half time duration" );
ConVar mp_timelimit_extratime_halftime( "mp_timelimit_extratime_halftime", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "extra time halftime duration" );
ConVar mp_timelimit_extratime_intermission( "mp_timelimit_extratime_intermission", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time before extra time start" );
ConVar mp_timelimit_penalties( "mp_timelimit_penalties", "1", FCVAR_NOTIFY|FCVAR_REPLICATED, "limit for penalties duration" );
ConVar mp_timelimit_penalties_intermission( "mp_timelimit_penalties_intermission", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time before penalties start" );
ConVar mp_extratime( "mp_extratime", "1", FCVAR_NOTIFY|FCVAR_REPLICATED );
ConVar mp_penalties( "mp_penalties", "1", FCVAR_NOTIFY|FCVAR_REPLICATED );

ConVar mp_shield_throwin_radius("mp_shield_throwin_radius", "100", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_freekick_radius("mp_shield_freekick_radius", "360", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_corner_radius("mp_shield_corner_radius", "360", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_kickoff_radius("mp_shield_kickoff_radius", "360", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_offside("mp_offside", "1", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_joindelay("mp_joindelay", "3", FCVAR_NOTIFY|FCVAR_REPLICATED);

static void OnMaxPlayersChange(IConVar *var, const char *pOldValue, float flOldValue)
{
#ifdef GAME_DLL
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		pPl->ChangeTeamPos(TEAM_SPECTATOR, 0, true);
	}
#endif
}

ConVar mp_maxplayers("mp_maxplayers", "6", FCVAR_NOTIFY|FCVAR_REPLICATED, "Maximum number of players per team <1-11>", true, 1, true, 11, OnMaxPlayersChange);

#ifdef GAME_DLL

void CSDKGameRules::State_Transition( match_state_t newState )
{
	State_Leave();
	State_Enter( newState );
}

void CSDKGameRules::State_Enter( match_state_t newState )
{
	m_eMatchState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );
	m_flStateEnterTime = gpGlobals->curtime;
	m_flInjuryTime = 0.0f;
	m_flInjuryTimeStart = -1;
	m_nAnnouncedInjuryTime = 0;

	if ( mp_showstatetransitions.GetInt() > 0 )
	{
		if ( m_pCurStateInfo )
			Msg( "Gamerules: entering state '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			Msg( "Gamerules: entering state #%d\n", newState );
	}

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		(this->*m_pCurStateInfo->pfnEnterState)();
	}

	if (IsIntermissionState())
		GetBall()->State_Transition(BALL_NORMAL);
	else
		GetBall()->RemovePlayerBalls();
}

void CSDKGameRules::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}

void CSDKGameRules::State_Think()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnThink )
	{
		if (m_pCurStateInfo->m_MinDurationConVar == NULL)
			m_flStateTimeLeft = 0.0f;
		else
			m_flStateTimeLeft = (m_flStateEnterTime + m_pCurStateInfo->m_MinDurationConVar->GetFloat() * 60 / m_pCurStateInfo->m_flMinDurationDivisor) - gpGlobals->curtime;
		
		(this->*m_pCurStateInfo->pfnThink)();
	}
}

CSDKGameRulesStateInfo* CSDKGameRules::State_LookupInfo( match_state_t state )
{
	static CSDKGameRulesStateInfo gameRulesStateInfos[] =
	{
		{ MATCH_INIT,						"MATCH_INIT",						&CSDKGameRules::State_INIT_Enter,					NULL, &CSDKGameRules::State_INIT_Think,						NULL, 1	},
		{ MATCH_WARMUP,						"MATCH_WARMUP",						&CSDKGameRules::State_WARMUP_Enter,					NULL, &CSDKGameRules::State_WARMUP_Think,					&mp_timelimit_warmup, 1	},
		{ MATCH_FIRST_HALF,					"MATCH_FIRST_HALF",					&CSDKGameRules::State_FIRST_HALF_Enter,				NULL, &CSDKGameRules::State_FIRST_HALF_Think,				&mp_timelimit_match, 2 },
		{ MATCH_HALFTIME,					"MATCH_HALFTIME",					&CSDKGameRules::State_HALFTIME_Enter,				NULL, &CSDKGameRules::State_HALFTIME_Think,					&mp_timelimit_halftime, 1 },
		{ MATCH_SECOND_HALF,				"MATCH_SECOND_HALF",				&CSDKGameRules::State_SECOND_HALF_Enter,			NULL, &CSDKGameRules::State_SECOND_HALF_Think,				&mp_timelimit_match, 2 },
		{ MATCH_EXTRATIME_INTERMISSION,		"MATCH_EXTRATIME_INTERMISSION",		&CSDKGameRules::State_EXTRATIME_INTERMISSION_Enter, NULL, &CSDKGameRules::State_EXTRATIME_INTERMISSION_Think,	&mp_timelimit_extratime_intermission, 1	},
		{ MATCH_EXTRATIME_FIRST_HALF,		"MATCH_EXTRATIME_FIRST_HALF",		&CSDKGameRules::State_EXTRATIME_FIRST_HALF_Enter,	NULL, &CSDKGameRules::State_EXTRATIME_FIRST_HALF_Think,		&mp_timelimit_match, 6 },
		{ MATCH_EXTRATIME_HALFTIME,			"MATCH_EXTRATIME_HALFTIME",			&CSDKGameRules::State_EXTRATIME_HALFTIME_Enter,		NULL, &CSDKGameRules::State_EXTRATIME_HALFTIME_Think,		&mp_timelimit_extratime_halftime, 1 },
		{ MATCH_EXTRATIME_SECOND_HALF,		"MATCH_EXTRATIME_SECOND_HALF",		&CSDKGameRules::State_EXTRATIME_SECOND_HALF_Enter,	NULL, &CSDKGameRules::State_EXTRATIME_SECOND_HALF_Think,	&mp_timelimit_match, 6 },
		{ MATCH_PENALTIES_INTERMISSION,		"MATCH_PENALTIES_INTERMISSION",		&CSDKGameRules::State_PENALTIES_INTERMISSION_Enter, NULL, &CSDKGameRules::State_PENALTIES_INTERMISSION_Think,	&mp_timelimit_penalties_intermission, 1 },
		{ MATCH_PENALTIES,					"MATCH_PENALTIES",					&CSDKGameRules::State_PENALTIES_Enter,				NULL, &CSDKGameRules::State_PENALTIES_Think,				&mp_timelimit_penalties, 1 },
		{ MATCH_COOLDOWN,					"MATCH_COOLDOWN",					&CSDKGameRules::State_COOLDOWN_Enter,				NULL, &CSDKGameRules::State_COOLDOWN_Think,					&mp_timelimit_cooldown, 1 },
		{ MATCH_END,						"MATCH_END",						&CSDKGameRules::State_END_Enter,					NULL, &CSDKGameRules::State_END_Think,						NULL, 1},
	};

	for ( int i=0; i < ARRAYSIZE( gameRulesStateInfos ); i++ )
	{
		if ( gameRulesStateInfos[i].m_eMatchState == state )
			return &gameRulesStateInfos[i];
	}

	return NULL;
}

void CSDKGameRules::State_INIT_Enter()
{
}

void CSDKGameRules::State_INIT_Think()
{
	State_Transition(MATCH_WARMUP);
}

void CSDKGameRules::State_WARMUP_Enter()
{
	SetAreTeamsSwapped(false);
}

void CSDKGameRules::State_WARMUP_Think()
{
	if (m_flStateTimeLeft <= 0.0f)
		State_Transition(MATCH_FIRST_HALF);
}

void CSDKGameRules::State_FIRST_HALF_Enter()
{
	//GetBall()->CreateVPhysics();
	GetBall()->ResetStats();
	GetBall()->SetIsKickOffAfterGoal(false);
	m_nKickOffTeam = g_IOSRand.RandomInt(TEAM_A, TEAM_B);
	GetBall()->State_Transition(BALL_KICKOFF);
}

void CSDKGameRules::State_FIRST_HALF_Think()
{
	if (m_flStateTimeLeft <= 10 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = max(1, (int)m_flInjuryTime);
	}
	else if (m_flStateTimeLeft + m_flInjuryTime <= 0)
	{	
		//if (m_pBall->IsNearGoal())
		//	m_flInjuryTime += 5; // let players finish their attack
		//else
		State_Transition(MATCH_HALFTIME);
	}
}

void CSDKGameRules::State_HALFTIME_Enter()
{
	GetBall()->EmitSound("Ball.whistle");
	GetBall()->EmitSound("Ball.cheer");
}

void CSDKGameRules::State_HALFTIME_Think()
{
	if (m_flStateTimeLeft <= 0)
		State_Transition(MATCH_SECOND_HALF);
}

void CSDKGameRules::State_SECOND_HALF_Enter()
{
	SetAreTeamsSwapped(true);
	GetBall()->SetIsKickOffAfterGoal(false);
	GetBall()->State_Transition(BALL_KICKOFF);
}

void CSDKGameRules::State_SECOND_HALF_Think()
{
	if (m_flStateTimeLeft <= 10 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = max(1, (int)m_flInjuryTime);
	}
	else if (m_flStateTimeLeft + m_flInjuryTime <= 0)
	{
		if (mp_extratime.GetBool() && GetGlobalTeam(TEAM_A)->GetGoals() == GetGlobalTeam(TEAM_B)->GetGoals())
			State_Transition(MATCH_EXTRATIME_INTERMISSION);
		else if (mp_penalties.GetBool() && GetGlobalTeam(TEAM_A)->GetGoals() == GetGlobalTeam(TEAM_B)->GetGoals())
			State_Transition(MATCH_PENALTIES_INTERMISSION);
		else
			State_Transition(MATCH_COOLDOWN);
	}
}

void CSDKGameRules::State_EXTRATIME_INTERMISSION_Enter()
{
	GetBall()->EmitSound("Ball.whistle");
	GetBall()->EmitSound("Ball.cheer");
}

void CSDKGameRules::State_EXTRATIME_INTERMISSION_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		State_Transition(MATCH_EXTRATIME_FIRST_HALF);
	}
}

void CSDKGameRules::State_EXTRATIME_FIRST_HALF_Enter()
{
	SetAreTeamsSwapped(false);
	GetBall()->SetIsKickOffAfterGoal(false);
	GetBall()->State_Transition(BALL_KICKOFF);
}

void CSDKGameRules::State_EXTRATIME_FIRST_HALF_Think()
{
	if (m_flStateTimeLeft <= 10 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = max(1, (int)m_flInjuryTime);
	}
	else if (m_flStateTimeLeft + m_flInjuryTime <= 0)
	{
		State_Transition(MATCH_EXTRATIME_HALFTIME);
	}
}

void CSDKGameRules::State_EXTRATIME_HALFTIME_Enter()
{
	GetBall()->EmitSound("Ball.whistle");
	GetBall()->EmitSound("Ball.cheer");
}

void CSDKGameRules::State_EXTRATIME_HALFTIME_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		State_Transition(MATCH_EXTRATIME_SECOND_HALF);
	}
}

void CSDKGameRules::State_EXTRATIME_SECOND_HALF_Enter()
{
	SetAreTeamsSwapped(true);
	GetBall()->State_Transition(BALL_KICKOFF);
}

void CSDKGameRules::State_EXTRATIME_SECOND_HALF_Think()
{
	if (m_flStateTimeLeft <= 10 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = max(1, (int)m_flInjuryTime);
	}
	else if (m_flStateTimeLeft + m_flInjuryTime <= 0)
	{
		if (mp_penalties.GetBool() && GetGlobalTeam(TEAM_A)->GetGoals() == GetGlobalTeam(TEAM_B)->GetGoals())
			State_Transition(MATCH_PENALTIES_INTERMISSION);
		else
			State_Transition(MATCH_COOLDOWN);
	}
}

void CSDKGameRules::State_PENALTIES_INTERMISSION_Enter()
{
	GetBall()->EmitSound("Ball.whistle");
	GetBall()->EmitSound("Ball.cheer");
}

void CSDKGameRules::State_PENALTIES_INTERMISSION_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		State_Transition(MATCH_PENALTIES);
	}
}

void CSDKGameRules::State_PENALTIES_Enter()
{
	m_nPenaltyTakingTeam = g_IOSRand.RandomInt(TEAM_A, TEAM_B);
	GetBall()->SetPenaltyState(PENALTY_NONE);
}

void CSDKGameRules::State_PENALTIES_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		State_Transition(MATCH_COOLDOWN);
		return;
	}

	if (GetBall()->GetPenaltyState() == PENALTY_KICKED)
	{
		//GetBall()->SetIgnoreTriggers(true);
		GetBall()->SetPenaltyState(PENALTY_NONE);
		m_flNextPenalty = gpGlobals->curtime + 5;
		m_nPenaltyTakingTeam = m_nPenaltyTakingTeam == TEAM_A ? TEAM_B : TEAM_A;
	}

	if ((GetBall()->GetPenaltyState() == PENALTY_NONE || GetBall()->GetPenaltyState() == PENALTY_ABORTED) && m_flNextPenalty <= gpGlobals->curtime)
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
			if (!CSDKPlayer::IsOnField(pPl))
				continue;

			if (pPl->GetTeamNumber() != m_nPenaltyTakingTeam || pPl->m_ePenaltyState == PENALTY_KICKED)
				continue;

			GetBall()->SetPenaltyTaker(pPl);
			GetBall()->SetPenaltyState(PENALTY_ASSIGNED);
			GetBall()->State_Transition(BALL_PENALTY);
			//m_flNextPenalty = gpGlobals->curtime + 5;
			break;
		}
	}
}

void CSDKGameRules::State_COOLDOWN_Enter()
{
	//who won?
	int winners = 0;
	int scoreA = GetGlobalTeam( TEAM_A )->GetGoals();
	int scoreB = GetGlobalTeam( TEAM_B )->GetGoals();
	if (scoreA > scoreB)
		winners = TEAM_A;
	if (scoreB > scoreA)
		winners = TEAM_B;

	//for ( int i = 0; i < MAX_PLAYERS; i++ )		//maxclients?
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSDKPlayer *pPlayer = (CSDKPlayer*)UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		//pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );

		//is this player on the winning team
		if (pPlayer->GetTeamNumber() == winners)
		{
			pPlayer->AddFlag (FL_CELEB);
		}
		//else
		//	pPlayer->AddFlag (FL_ATCONTROLS);

		//freezes the players
		//pPlayer->AddFlag (FL_ATCONTROLS);
	}
}

void CSDKGameRules::State_COOLDOWN_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		State_Transition(MATCH_END);
	}
}

void CSDKGameRules::State_END_Enter()
{
	GoToIntermission();
}

void CSDKGameRules::State_END_Think()
{
}

#endif

//#ifdef CLIENT_DLL
//
//void CSDKGameRules::SetMatchState(match_state_t eMatchState)
//{
//	m_eMatchState = eMatchState;
//}
//
//#endif

#ifdef GAME_DLL

void SetTeams(const char *teamHome, const char *teamAway, bool bInitialize)
{
	Q_strncpy(pszTeamNames[TEAM_A], teamHome, sizeof(pszTeamNames[TEAM_A]));
	Q_strncpy(pszTeamNames[TEAM_B], teamAway, sizeof(pszTeamNames[TEAM_B]));

	if (bInitialize)
	{
		//update the team names
		for ( int i = 0; i < ARRAYSIZE( pszTeamNames ); i++ )
		{
			CTeam *pTeam = g_Teams[i];
			pTeam->Init( pszTeamNames[i], i );
		}
	}
}

//void cc_Teams( const CCommand& args )
//{
//	if ( !UTIL_IsCommandIssuedByServerAdmin() )
//		return;
//
//	if ( args.ArgC() < 3 )
//	{
//		Msg( "Format: mp_teams <home team> <away team>\n" );
//		return;
//	}
//
//	SetTeams(args[1], args[2]);
//}
//
//static ConCommand mp_teams( "mp_teams", cc_Teams, "Set teams" );

void OnTeamlistChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	//if (gpGlobals->curtime > 10)
	if (SDKGameRules() != NULL)
	{
		char teamlist[256];
		Q_strncpy(teamlist, ((ConVar*)var)->GetString(), sizeof(teamlist));
		//CUtlVector<char*, CUtlMemory<char*> > teams;
		//Q_SplitString(teamlist, ";", teams);
		//teams
		char *home = strtok(teamlist, ";");
		char *away = strtok(NULL, ";");
		if (home == NULL || away == NULL)
			Msg( "Format: mp_teamlist \"<home team>;<away team>\"\n" );
		else
		{
			//ReadTeamInfo(home);
			//ReadTeamInfo(away);
			SetTeams(home, away);
		}
	}
}
static ConVar mp_teamlist("mp_teamlist", "ENGLAND;BRAZIL", FCVAR_REPLICATED|FCVAR_NOTIFY, "Set team names", &OnTeamlistChange);
static ConVar sv_teamrotation("mp_teamrotation", "brazil;germany;italy;scotland;barcelona;bayern;liverpool;milan;palmeiras", 0, "Set available teams");


void CSDKGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	/* TODO: handle skin, model & team changes 

  	char text[1024];

	// skin/color/model changes
	int iTeam = Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_team" ) );
	int iClass = Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_class" ) );

	if ( defaultteam.GetBool() )
	{
		// int clientIndex = pPlayer->entindex();

		// engine->SetClientKeyValue( clientIndex, "model", pPlayer->TeamName() );
		// engine->SetClientKeyValue( clientIndex, "team", pPlayer->TeamName() );
		UTIL_SayText( "Not allowed to change teams in this game!\n", pPlayer );
		return;
	}

	if ( defaultteam.GetFloat() || !IsValidTeam( mdls ) )
	{
		// int clientIndex = pPlayer->entindex();

		// engine->SetClientKeyValue( clientIndex, "model", pPlayer->TeamName() );
		Q_snprintf( text,sizeof(text), "Can't change team to \'%s\'\n", mdls );
		UTIL_SayText( text, pPlayer );
		Q_snprintf( text,sizeof(text), "Server limits teams to \'%s\'\n", m_szTeamList );
		UTIL_SayText( text, pPlayer );
		return;
	}

	ChangePlayerTeam( pPlayer, mdls, true, true );
	// recound stuff
	RecountTeams(); */

	const char *pszClubName = engine->GetClientConVarValue( pPlayer->entindex(), "clubname" );
	((CSDKPlayer *)pPlayer)->SetClubName(pszClubName);

	const char *pszCountryName = engine->GetClientConVarValue( pPlayer->entindex(), "countryname" );
	((CSDKPlayer *)pPlayer)->SetCountryName(pszCountryName);

	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	const char *pszOldName = pPlayer->GetPlayerName();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strcmp( pszOldName, pszName ) )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "oldname", pszOldName );
			event->SetString( "newname", pszName );
			gameeventmanager->FireEvent( event );
		}
		
		pPlayer->SetPlayerName( pszName );
	}
}

void CSDKGameRules::EnableShield(int type, int sideOrRadius, Vector pos /*= vec3_invalid*/)
{
	m_nShieldType = type;
	m_vShieldPos = Vector(pos.x, pos.y, SDKGameRules()->m_vKickOff.GetZ());
	m_nShieldSide = sideOrRadius;
	m_nShieldRadius = sideOrRadius;
}

void CSDKGameRules::DisableShield()
{
	m_nShieldType = SHIELD_NONE;

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->RemoveFlag(FL_REMOTECONTROLLED | FL_SHIELD_KEEP_IN | FL_SHIELD_KEEP_OUT);
		pPl->SetMoveType(MOVETYPE_WALK);
		pPl->m_bIsAtTargetPos = true;
	}
}

void CSDKGameRules::SetAreTeamsSwapped(bool swapped)
{
	if (swapped != m_bAreTeamsSwapped)
	{
		GetGlobalTeam(TEAM_A)->InitFieldSpots(swapped ? TEAM_B : TEAM_A);
		GetGlobalTeam(TEAM_B)->InitFieldSpots(swapped ? TEAM_A : TEAM_B);
		m_bAreTeamsSwapped = swapped;
	}
}

CBaseEntity *CSDKGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	Vector spawnPos = pPlayer->GetTeam()->m_vPlayerSpawns[ToSDKPlayer(pPlayer)->GetTeamPosition() - 1];
	Vector dir = Vector(0, pPlayer->GetTeam()->m_nForward, 0);
	QAngle ang;
	VectorAngles(dir, ang);
	pPlayer->SetLocalOrigin(spawnPos);
	pPlayer->SetLocalVelocity(vec3_origin);
	pPlayer->SetLocalAngles(ang);
	pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
	pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
	pPlayer->SnapEyeAngles(ang);

	return NULL;
}

void CSDKGameRules::StartInjuryTime()
{
	EndInjuryTime();
	m_flInjuryTimeStart = gpGlobals->curtime;
}

void CSDKGameRules::EndInjuryTime()
{
	if (m_flInjuryTimeStart != -1)
	{
		m_flInjuryTime += gpGlobals->curtime - m_flInjuryTimeStart;
		m_flInjuryTimeStart = -1;
	}
}

#endif

bool CSDKGameRules::IsIntermissionState()
{
	switch (State_Get())
	{
	case MATCH_WARMUP:
	case MATCH_HALFTIME:
	case MATCH_EXTRATIME_INTERMISSION:
	case MATCH_EXTRATIME_HALFTIME:
	case MATCH_PENALTIES_INTERMISSION:
	case MATCH_COOLDOWN:
		return true;
	default:
		return false;
	}
}













#ifdef CLIENT_DLL

#include "Filesystem.h"
#include "utlbuffer.h"

struct kit
{
	char type[16];
	char firstColor[16];
	char secondColor[16];
};

struct teamInfo
{
	char teamCode[8];
	char shortName[16];
	char fullName[32];
	kit kits[8];
};

void ReadTeamInfo(const char *teamname)
{
	//char filename[64];
	//Q_snprintf(filename, sizeof(filename), "materials/models/player_new/%s/teaminfo", teamname);
	//V_SetExtension(filename, ".txt", sizeof(filename));
	//V_FixSlashes(filename);

	//CUtlBuffer buf;
	//if (filesystem->ReadFile(filename, "GAME", buf))
	//{
	//	char* gameInfo = new char[buf.Size() + 1];
	//	buf.GetString(gameInfo);
	//	gameInfo[buf.Size()] = 0; // null terminator

	//	DevMsg("Team info: %s\n", gameInfo);

	//	delete[] gameInfo;
	//}

	char path[64], filename[64];
	Q_snprintf(path, sizeof(path), "materials/models/player_new/%s", teamname);

	int length;
	CUtlVector<char*, CUtlMemory<char*> > lines, values;
	Q_snprintf(filename, sizeof(filename), "%s/teaminfo.txt", path);
	char *teaminfostr = (char *)UTIL_LoadFileForMe(filename, &length);
	if (teaminfostr && length > 0)
	{
		const char *separators[2] = { "\n", ";" };
		Q_SplitString2(teaminfostr, separators, 2, lines);
		//teamInfo t = { teaminfo[0], teaminfo[1], teaminfo[2], teaminfo[3], teaminfo[4], teaminfo[5] };
		teamInfo ti;
		Q_strncpy(ti.teamCode, lines[0], sizeof(ti.teamCode));
		Q_strncpy(ti.shortName, lines[1], sizeof(ti.shortName));
		Q_strncpy(ti.fullName, lines[2], sizeof(ti.fullName));

		for (int i = 3; i < lines.Count(); i += 3)
		{
			kit k;
			Q_strncpy(k.type, lines[i], sizeof(k.type));
			Q_strncpy(k.firstColor, lines[i + 1], sizeof(k.firstColor));
			Q_strncpy(k.secondColor, lines[i + 2], sizeof(k.secondColor));
			ti.kits[i/3-1] = k; //todo: neue variable vom stack?
		}
	}
}

#include "curl/curl.h"
#include "Filesystem.h"
#include "utlbuffer.h"
  
struct curl_t
{
	char filename[32];
	CUtlBuffer buf;
	FileHandle_t fh;
};

// Called when curl receives data from the server
static size_t rcvData(void *ptr, size_t size, size_t nmemb, curl_t* vars)
{
	//Msg((char*)ptr); // up to 989 characters each time
	//CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
	//vars->buf.Put(ptr, nmemb);
	filesystem->Write(ptr, nmemb, vars->fh);
	//filesystem->WriteFile(VarArgs("materials/models/player_new/foobar/%s", vars->filename), "MOD", buf);
	return size * nmemb;
}

 
unsigned DoCurl( void *params )
{
	curl_t* vars = (curl_t*) params; // always use a struct!
 
	// do some stuff
 
	/*vars->buf = CUtlBuffer(0, 0, CUtlBuffer::TEXT_BUFFER);
	vars->buf.SetBufferType(false, false);*/
	//vars->buf = CUtlBuffer();

	//filesystem->UnzipFile("test.zip", "MOD", "unziptest");

	vars->fh = filesystem->Open(VarArgs("materials/models/player_new/foobar/%s", vars->filename), "a+b", "MOD");

	if (vars->fh)
	{
		CURL *curl;
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, VarArgs("http://127.0.0.1:8000/%s", vars->filename));
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rcvData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, vars);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		filesystem->Close(vars->fh);

		//filesystem->WriteFile(VarArgs("materials/models/player_new/foobar/%s", vars->filename), "MOD", vars->buf);

		//Msg("Cannot print to console from this threaded function\n");
	}
	// clean up the memory
	delete vars;

	return 0;
}

void Curl(const CCommand &args)
{
	if (args.ArgC() < 2)
	{
		Msg("Which file?\n");
		return;
	}

	curl_t* vars = new curl_t;
	Q_strncpy(vars->filename, args[1], sizeof(vars->filename));
	CreateSimpleThread( DoCurl, vars );
}

static ConCommand cl_curl("cl_curl", Curl);

#endif
