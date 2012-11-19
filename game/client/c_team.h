//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CTeam class
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TEAM_H
#define C_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "utlvector.h"
#include "client_thinklist.h"
#include "ios_teamkit_parse.h"
#include "c_sdk_player.h"

#define MAX_MATCH_EVENTS 16
#define MAX_MATCH_EVENT_PLAYER_NAME_LENGTH 64

class C_BasePlayer;

class C_Team : public C_BaseEntity
{
	DECLARE_CLASS( C_Team, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

					C_Team();
	virtual			~C_Team();

	virtual void	PreDataUpdate( DataUpdateType_t updateType );
	virtual void	Spawn();
	// Data Handling
	virtual int		GetTeamNumber( void ) const;
	virtual int		GetOppTeamNumber( void ) const;
	C_Team			*GetOppTeam( void ) const;
	virtual bool	Get_IsClubTeam( void );
	virtual bool	Get_IsRealTeam( void );
	virtual bool	Get_HasTeamCrest( void );
	virtual char	*Get_TeamCode( void );
	virtual char	*Get_ShortTeamName( void );
	virtual char	*Get_FullTeamName( void );
	virtual char	*Get_KitName( void );
	virtual Color	&Get_HudKitColor( void );
	virtual Color	&Get_PrimaryKitColor( void );
	virtual Color	&Get_SecondaryKitColor( void );
	virtual int		Get_Goals( void );
	virtual int		Get_Ping( void );

	virtual int		Get_Possession();

	virtual C_SDKPlayer	*Get_Captain() { return m_pCaptain; }
	virtual C_SDKPlayer	*Get_FreekickTaker() { return m_pFreekickTaker; }
	virtual C_SDKPlayer	*Get_PenaltyTaker() { return m_pPenaltyTaker; }
	virtual C_SDKPlayer	*Get_CornerTaker() { return m_pCornerTaker; }
	virtual C_SDKPlayer	*Get_ThrowinTaker() { return m_pThrowinTaker; }

	virtual int		Get_TimeoutsLeft() { return m_nTimeoutsLeft; }

	// Player Handling
	virtual int		Get_Number_Players( void );
	virtual bool	ContainsPlayer( int iPlayerIndex );
	C_BasePlayer*	GetPlayer( int idx );

	// for shared code, use the same function name
	virtual int		GetNumPlayers( void ) { return Get_Number_Players(); }

	void	RemoveAllPlayers();

	void	SetKitName(const char *pKitName);
	void	DownloadTeamKit(const char *pKitName, int teamNumber);

// IClientThinkable overrides.
public:

	virtual	void				ClientThink();


public:

	bool	m_bKitDownloadFinished;
	char	m_szDownloadKitName[MAX_KITNAME_LENGTH];

	// Data received from the server
	CUtlVector< int > m_aPlayers;
	CTeamKitInfo *m_pTeamKitInfo;
	int		m_iTeamNum;
	char	m_szKitName[MAX_KITNAME_LENGTH];
	char	m_szServerKitName[MAX_KITNAME_LENGTH];
	int		m_nGoals;
	int		m_nPossession;
	int		m_nPenaltyGoals;
	int		m_nPenaltyGoalBits;
	int		m_nPenaltyRound;
	int		m_nTimeoutsLeft;

	CHandle<C_SDKPlayer> m_pCaptain;
	CHandle<C_SDKPlayer> m_pFreekickTaker;
	CHandle<C_SDKPlayer> m_pPenaltyTaker;
	CHandle<C_SDKPlayer> m_pCornerTaker;
	CHandle<C_SDKPlayer> m_pThrowinTaker;

	// Data for the scoreboard
	int		m_iPing;
	int		m_iPacketloss;

	CNetworkVector(m_vCornerLeft);
	CNetworkVector(m_vCornerRight);
	CNetworkVector(m_vGoalkickLeft);
	CNetworkVector(m_vGoalkickRight);
	CNetworkVector(m_vPenalty);
	CNetworkVector(m_vPenBoxMin);
	CNetworkVector(m_vPenBoxMax);
	int		m_nForward;
	int		m_nRight;

	char m_szMatchEventPlayers[MAX_MATCH_EVENTS][MAX_MATCH_EVENT_PLAYER_NAME_LENGTH];
	int m_eMatchEventTypes[MAX_MATCH_EVENTS];
	int m_nMatchEventSeconds[MAX_MATCH_EVENTS];
};


// Global list of client side team entities
extern CUtlVector< C_Team * > g_Teams;

// Global team handling functions
C_Team *GetLocalTeam( void );
C_Team *GetGlobalTeam( int iTeamNumber );
C_Team *GetPlayersTeam( int iPlayerIndex );
C_Team *GetPlayersTeam( C_BasePlayer *pPlayer );
bool ArePlayersOnSameTeam( int iPlayerIndex1, int iPlayerIndex2 );
extern int GetNumberOfTeams( void );

#endif // C_TEAM_H
