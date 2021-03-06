#ifndef _MATCH_BALL_H
#define _MATCH_BALL_H

#include "cbase.h"
#include "ball.h"

class CBall;

class CMatchBall : public CBall
{
public:

	DECLARE_CLASS( CMatchBall, CBall );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CMatchBall();
	~CMatchBall();

	void			State_Enter(ball_state_t newState, bool cancelQueuedState);
	void			State_Think();
	void			State_Leave(ball_state_t newState);
	void			State_Transition(ball_state_t nextState, float nextStateMessageDelay = 0, float nextStatePostMessageDelay = 0, bool cancelQueuedState = false);

	void State_STATIC_Enter();			void State_STATIC_Think();			void State_STATIC_Leave(ball_state_t newState);
	void State_NORMAL_Enter();			void State_NORMAL_Think();			void State_NORMAL_Leave(ball_state_t newState);
	void State_KICKOFF_Enter();			void State_KICKOFF_Think();			void State_KICKOFF_Leave(ball_state_t newState);
	void State_THROWIN_Enter();			void State_THROWIN_Think();			void State_THROWIN_Leave(ball_state_t newState);
	void State_GOALKICK_Enter();		void State_GOALKICK_Think();		void State_GOALKICK_Leave(ball_state_t newState);
	void State_CORNER_Enter();			void State_CORNER_Think();			void State_CORNER_Leave(ball_state_t newState);
	void State_GOAL_Enter();			void State_GOAL_Think();			void State_GOAL_Leave(ball_state_t newState);
	void State_FREEKICK_Enter();		void State_FREEKICK_Think();		void State_FREEKICK_Leave(ball_state_t newState);
	void State_PENALTY_Enter();			void State_PENALTY_Think();			void State_PENALTY_Leave(ball_state_t newState);
	void State_KEEPERHANDS_Enter();		void State_KEEPERHANDS_Think();		void State_KEEPERHANDS_Leave(ball_state_t newState);

	void			Spawn();
	void			Reset();
	void			GetGoalInfo(bool &isOwnGoal, int &scoringTeam, CSDKPlayer **pScorer, CSDKPlayer **pFirstAssister, CSDKPlayer **pSecondAssister);
	void			SendNotifications();

	CSDKPlayer		*GetCurrentOtherPlayer() { return m_pOtherPl; }
	CSDKPlayer		*GetLastActivePlayer() { return m_pLastActivePlayer; }
	void			SetSetpieceTaker(CSDKPlayer *pPlayer);

	void			Touched(bool isShot, body_part_t bodyPart, const Vector &oldVel);
	bool			CheckFoul(CSDKPlayer *pPl);
	void			SetFoulParams(foul_type_t type, Vector pos, CSDKPlayer *pFoulingPl, CSDKPlayer *pFouledPl = NULL);
	bool			HasProximityRestriction();
	void			SetVel(Vector vel, float spinCoeff, int spinFlags, body_part_t bodyPart, bool markOffsidePlayers, float minPostDelay, bool resetShotCharging);
	void			MarkOffsidePlayers();
	void			UnmarkOffsidePlayers();
	float			GetFieldZone();
	void			UpdatePossession(CSDKPlayer *pNewPossessor);
	void			SetPenaltyState(penalty_state_t penaltyState);
	penalty_state_t	GetPenaltyState() { return m_ePenaltyState; }
	void			SetPenaltyTaker(CSDKPlayer *pPl);
	CSDKPlayer		*GetPenaltyTaker();
	void			VPhysicsCollision(int index, gamevcollisionevent_t	*pEvent);
	bool			IsLegallyCatchableByKeeper();
	void			CheckAdvantage();
	bool			CheckOffside();
	bool			CheckSideline();
	bool			CheckGoalLine();
	bool			CheckGoal();
	void			CheckFieldZone();
	void			RemovePlayerAssignments(CSDKPlayer *pPl);

	float			m_flNextStateMessageTime;
	float			m_flSetpieceProximityStartTime;
	float			m_flSetpieceProximityEndTime;
	float			m_flStateTimelimit;

	CHandle<CSDKPlayer>	m_pOtherPl;
	CHandle<CSDKPlayer>	m_pSetpieceTaker;

	CNetworkHandle(CSDKPlayer, m_pLastActivePlayer);
	CNetworkVar(int, m_nLastActiveTeam);

	CHandle<CSDKPlayer>	m_pPossessingPl;
	int				m_nPossessingTeam;
	float			m_flPossessionStart;

	penalty_state_t m_ePenaltyState;

	CHandle<CSDKPlayer>	m_pFouledPl;
	CHandle<CSDKPlayer>	m_pFoulingPl;
	int				m_nFouledTeam;
	int				m_nFoulingTeam;
	foul_type_t		m_eFoulType;
	Vector			m_vFoulPos;
	bool			m_bIsAdvantage;
	bool			m_bBallInAirAfterThrowIn;
	float			m_flFoulTime;
	bool			m_bIsPenalty;
	int				m_nTeam;
	float			m_flFieldZone;
};

extern CMatchBall *GetMatchBall();
extern CMatchBall *CreateMatchBall(const Vector &pos);

#endif