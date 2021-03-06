//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"

#include "sdk_playeranimstate.h"
#include "datacache/imdlcache.h"

#ifdef CLIENT_DLL
#include "c_sdk_player.h"
#else
#include "sdk_player.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : CMultiPlayerAnimState*
//-----------------------------------------------------------------------------
CSDKPlayerAnimState* CreateSDKPlayerAnimState( CSDKPlayer *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	// Create animation state for this player.
	CSDKPlayerAnimState *pRet = new CSDKPlayerAnimState( pPlayer );

	// Specific SDK player initialization.
	//pRet->InitSDKAnimState( pPlayer );

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CSDKPlayerAnimState::CSDKPlayerAnimState()
#ifdef CLIENT_DLL
	: m_iv_flMaxGroundSpeed( "CMultiPlayerAnimState::m_iv_flMaxGroundSpeed" )
#endif
{
	m_pSDKPlayer = NULL;

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
CSDKPlayerAnimState::CSDKPlayerAnimState( CBasePlayer *pPlayer )
{
	m_pSDKPlayer = ToSDKPlayer(pPlayer);

	// Pose parameters.
	m_bPoseParameterInit = false;
	m_PoseParameterData.Init();
	m_DebugAnimData.Init();

	m_angRender.Init();

	m_flEyeYaw = 0.0f;
	m_flEyePitch = 0.0f;
	m_flGoalFeetYaw = 0.0f;
	m_flCurrentFeetYaw = 0.0f;
	m_flLastAimTurnTime = 0.0f;

	// Jumping.
	m_bJumping = false;
	m_flJumpStartTime = 0.0f;	
	m_bFirstJumpFrame = false;

	// Swimming
	m_bInSwim = false;
	m_bFirstSwimFrame = true;

	// Dying
	m_bDying = false;
	m_bFirstDyingFrame = true;

	m_eCurrentMainSequenceActivity = ACT_INVALID;	

	// Ground speed interpolators.
#ifdef CLIENT_DLL
	m_iv_flMaxGroundSpeed.Setup( &m_flMaxGroundSpeed, LATCH_ANIMATION_VAR | INTERPOLATE_LINEAR_ONLY );
	m_flLastGroundSpeedUpdateTime = 0.0f;
#endif

	m_flMaxGroundSpeed = 0.0f;

	m_bForceAimYaw = true; //ios false;

	//InitGestureSlots();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CSDKPlayerAnimState::~CSDKPlayerAnimState()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::ClearAnimationState( void )
{
	m_bIsActionSequenceActive = false;
	m_bIsCarrySequenceActive = false;
	m_bIsGestureSequenceActive = false;
	m_bJumping = false;
	m_bDying = false;
	ClearAnimationLayers();
}

void CSDKPlayerAnimState::ClearAnimationLayers()
{
	if ( !GetBasePlayer() )
		return;

	GetBasePlayer()->SetNumAnimOverlays( NUM_LAYERS_WANTED );
	for ( int i=0; i < GetBasePlayer()->GetNumAnimOverlays(); i++ )
	{
		GetBasePlayer()->GetAnimOverlay( i )->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );
#ifndef CLIENT_DLL
		GetBasePlayer()->GetAnimOverlay( i )->m_fFlags = 0;
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CSDKPlayerAnimState::TranslateActivity( Activity actDesired )
{
	return actDesired;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	// Profile the animation update.
	VPROF( "CMultiPlayerAnimState::Update" );

	// Clear animation overlays because we're about to completely reconstruct them.
	ClearAnimationLayers();

	// Some mods don't want to update the player's animation state if they're dead and ragdolled.
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Get the SDK player.
	CSDKPlayer *pSDKPlayer = GetSDKPlayer();
	if ( !pSDKPlayer )
		return;

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pSDKPlayer->GetModelPtr();
	if ( !pStudioHdr )
		return;

	// Check to see if we should be updating the animation state - dead, ragdolled?
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Store the eye angles.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );

	// Compute the player sequences.
	ComputeSequences( pStudioHdr );

	if ( SetupPoseParameters( pStudioHdr ) )
	{
		// Pose parameter - what direction are the player's legs running in.
		ComputePoseParam_MoveYaw( pStudioHdr );

		// Pose parameter - Torso aiming (up/down).
		ComputePoseParam_AimPitch( pStudioHdr );

		// Pose parameter - Torso aiming (rotation).
		ComputePoseParam_AimYaw( pStudioHdr );
	}

#ifdef CLIENT_DLL 
	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
	{
		m_pSDKPlayer->SetPlaybackRate( 1.0f );
	}
#endif
}

void CSDKPlayerAnimState::ComputeActionSequence(CStudioHdr *pStudioHdr)
{
	UpdateLayerSequenceGeneric(pStudioHdr, ACTIONSEQUENCE_LAYER, m_bIsActionSequenceActive, m_flActionSequenceCycle, m_nActionSequence, m_bActionSequenceWaitAtEnd);
}

void CSDKPlayerAnimState::ComputeCarrySequence(CStudioHdr *pStudioHdr)
{
	UpdateLayerSequenceGeneric(pStudioHdr, CARRYSEQUENCE_LAYER, m_bIsCarrySequenceActive, m_flCarrySequenceCycle, m_nCarrySequence, true);
}

void CSDKPlayerAnimState::ComputeGestureSequence(CStudioHdr *pStudioHdr)
{
	UpdateLayerSequenceGeneric(pStudioHdr, GESTURESEQUENCE_LAYER, m_bIsGestureSequenceActive, m_flGestureSequenceCycle, m_nGestureSequence, false);

	if (!m_bIsGestureSequenceActive && m_pSDKPlayer->m_Shared.GetGesture() != PLAYERANIMEVENT_NONE)
		m_pSDKPlayer->m_Shared.SetGesture(PLAYERANIMEVENT_NONE);
}

int CSDKPlayerAnimState::CalcActionSequence(PlayerAnimEvent_t event)
{
	switch (event)
	{
	case PLAYERANIMEVENT_KICK_DRIBBLE: return CalcSequenceIndex("kick_dribble");
	case PLAYERANIMEVENT_KICK_WEAK: return CalcSequenceIndex("kick_weak");
	case PLAYERANIMEVENT_KICK_STRONG: return CalcSequenceIndex("kick_strong");
	case PLAYERANIMEVENT_VOLLEY: return CalcSequenceIndex("volley");
	case PLAYERANIMEVENT_HEEL_KICK: return CalcSequenceIndex("heel_kick");
	case PLAYERANIMEVENT_HEADER_WEAK: return CalcSequenceIndex("header_weak");
	case PLAYERANIMEVENT_HEADER_STRONG: return CalcSequenceIndex("header_strong");
	case PLAYERANIMEVENT_THROW_IN_THROW: return CalcSequenceIndex("throw_in_throw");
	case PLAYERANIMEVENT_SLIDE_TACKLE: return CalcSequenceIndex("slide_tackle");
	case PLAYERANIMEVENT_STANDING_TACKLE: return CalcSequenceIndex("kick_weak");
	case PLAYERANIMEVENT_TACKLED_FORWARD: return CalcSequenceIndex("tackled_forward");
	case PLAYERANIMEVENT_TACKLED_BACKWARD: return CalcSequenceIndex("tackled_backward");
	case PLAYERANIMEVENT_DIVING_HEADER: return CalcSequenceIndex("diving_header");
	case PLAYERANIMEVENT_KEEPER_DIVE_FORWARD: return CalcSequenceIndex("keeper_dive_forward");
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT_FORWARD: return CalcSequenceIndex("keeper_dive_right");
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT: return CalcSequenceIndex("keeper_dive_right");
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT_BACKWARD: return CalcSequenceIndex("keeper_dive_right");
	case PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD: return CalcSequenceIndex("keeper_dive_backward");
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT_BACKWARD: return CalcSequenceIndex("keeper_dive_left");
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT: return CalcSequenceIndex("keeper_dive_left");
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT_FORWARD: return CalcSequenceIndex("keeper_dive_left");
	case PLAYERANIMEVENT_KEEPER_HANDS_THROW: return CalcSequenceIndex("keeper_hands_throw");
	case PLAYERANIMEVENT_KEEPER_HANDS_VOLLEY: return CalcSequenceIndex("keeper_hands_volley");
	case PLAYERANIMEVENT_KEEPER_HANDS_PUNCH: return CalcSequenceIndex("keeper_hands_punch");
	case PLAYERANIMEVENT_FAKE_SHOT: return CalcSequenceIndex("fake_shot");
	case PLAYERANIMEVENT_RAINBOW_FLICK: return CalcSequenceIndex("rainbow_flick");
	case PLAYERANIMEVENT_BICYCLE_KICK: return CalcSequenceIndex("bicycle_kick");
	case PLAYERANIMEVENT_CELEB_SLIDE: return CalcSequenceIndex("celeb_slide");
	default: return 0;
	}
}

int CSDKPlayerAnimState::CalcCarrySequence(PlayerAnimEvent_t event)
{
	switch (event)
	{
	case PLAYERANIMEVENT_KEEPER_HANDS_CARRY: return CalcSequenceIndex("keeper_hands_hold");
	case PLAYERANIMEVENT_THROW_IN_CARRY: return CalcSequenceIndex("throw_in_hold");
	default: return 0;
	}
}

int CSDKPlayerAnimState::CalcGestureSequence(PlayerAnimEvent_t event)
{
	switch (event)
	{
	case PLAYERANIMEVENT_GESTURE_POINT: return CalcSequenceIndex("gesture_point");
	case PLAYERANIMEVENT_GESTURE_WAVE: return CalcSequenceIndex("gesture_wave");
	default: return 0;
	}
}

int CSDKPlayerAnimState::CalcSequenceIndex( const char *pBaseName, ... )
{
	char szFullName[512];
	va_list marker;
	va_start( marker, pBaseName );
	Q_vsnprintf( szFullName, sizeof( szFullName ), pBaseName, marker );
	va_end( marker );
	int iSequence = GetBasePlayer()->LookupSequence( szFullName );
	
	// Show warnings if we can't find anything here.
	if ( iSequence == -1 )
	{
		static CUtlDict<int,int> dict;
		if ( dict.Find( szFullName ) == -1 )
		{
			dict.Insert( szFullName, 0 );
			Warning( "CalcSequenceIndex: can't find '%s'.\n", szFullName );
		}

		iSequence = 0;
	}

	return iSequence;
}

void CSDKPlayerAnimState::UpdateLayerSequenceGeneric( CStudioHdr *pStudioHdr, int iLayer, bool &bEnabled, float &flCurCycle, int &iSequence, bool bWaitAtEnd )
{
	if ( !bEnabled )
		return;

	// Increment the fire sequence's cycle.
	flCurCycle += GetBasePlayer()->GetSequenceCycleRate( pStudioHdr, iSequence ) * gpGlobals->frametime;
	if ( flCurCycle > 1 )
	{
		if ( bWaitAtEnd )
		{
			flCurCycle = 1;
		}
		else
		{
			// Not firing anymore.
			bEnabled = false;
			iSequence = 0;
			return;
		}
	}

	// Now dump the state into its animation layer.
	CAnimationLayer *pLayer = GetBasePlayer()->GetAnimOverlay( iLayer );

	pLayer->m_flCycle = flCurCycle;
	pLayer->m_nSequence = iSequence;

	pLayer->m_flPlaybackRate = 1.0;
	pLayer->m_flWeight = 1.0f;
	pLayer->m_nOrder = iLayer;
#ifndef CLIENT_DLL
	pLayer->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
}

extern ConVar mp_reset_spin_toggles_on_shot;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::DoAnimationEvent(PlayerAnimEvent_t event)
{
	bool isAction = false;

	switch (event)
	{
	// Set current animation to none (e.g. after playing another animation)
	case PLAYERANIMEVENT_NONE:
	{
		GetSDKPlayer()->m_Shared.SetAction(event);
		GetSDKPlayer()->RemoveFlag(FL_FREECAM);
		m_bIsActionSequenceActive = false;
		return;
	}
	case PLAYERANIMEVENT_GESTURE_POINT:
	case PLAYERANIMEVENT_GESTURE_WAVE:
	{
		m_nGestureSequence = CalcGestureSequence(event);
		m_flGestureSequenceCycle = 0;
		m_bIsGestureSequenceActive = m_nGestureSequence != 0;
		GetSDKPlayer()->m_Shared.SetGesture(event);
		return;
	}
	case PLAYERANIMEVENT_KEEPER_HANDS_CARRY:
	case PLAYERANIMEVENT_THROW_IN_CARRY:
	{
		m_nCarrySequence = CalcCarrySequence(event);
		m_flCarrySequenceCycle = 0;
		m_bIsCarrySequenceActive = true;
		GetSDKPlayer()->m_Shared.SetCarryAnimation(event);
		return;
	}
	case PLAYERANIMEVENT_CARRY_END:
	{
		m_bIsCarrySequenceActive = false;
		GetSDKPlayer()->m_Shared.SetCarryAnimation(PLAYERANIMEVENT_NONE);
		return;
	}
	case PLAYERANIMEVENT_SLIDE_TACKLE:
	case PLAYERANIMEVENT_STANDING_TACKLE:
	case PLAYERANIMEVENT_CELEB_SLIDE:
	case PLAYERANIMEVENT_DIVING_HEADER:
	case PLAYERANIMEVENT_BICYCLE_KICK:
	case PLAYERANIMEVENT_TACKLED_FORWARD:
	case PLAYERANIMEVENT_TACKLED_BACKWARD:
	case PLAYERANIMEVENT_KEEPER_DIVE_FORWARD:
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT_FORWARD:
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT:
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT_BACKWARD:
	case PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD:
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT_BACKWARD:
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT:
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT_FORWARD:
	{
		isAction = true;
	}
	case PLAYERANIMEVENT_KICK_DRIBBLE:
	case PLAYERANIMEVENT_KICK_WEAK:
	case PLAYERANIMEVENT_KICK_STRONG:
	case PLAYERANIMEVENT_VOLLEY:
	case PLAYERANIMEVENT_HEEL_KICK:
	case PLAYERANIMEVENT_HEADER_WEAK:
	case PLAYERANIMEVENT_HEADER_STRONG:
	case PLAYERANIMEVENT_THROW_IN_THROW:
	case PLAYERANIMEVENT_KEEPER_HANDS_THROW:
	case PLAYERANIMEVENT_KEEPER_HANDS_VOLLEY:
	case PLAYERANIMEVENT_KEEPER_HANDS_PUNCH:
	case PLAYERANIMEVENT_LIFT_UP:
	case PLAYERANIMEVENT_BALL_ROLL_LEFT:
	case PLAYERANIMEVENT_BALL_ROLL_RIGHT:
	case PLAYERANIMEVENT_FAKE_SHOT:
	case PLAYERANIMEVENT_RAINBOW_FLICK:
	{
		// Reset event if no action, so the event after throw-in or keeper holding is handled properly
		GetSDKPlayer()->m_Shared.SetAction(isAction ? event : PLAYERANIMEVENT_NONE);

		m_nActionSequence = CalcActionSequence(event);
		m_flActionSequenceCycle = 0;
		m_bIsActionSequenceActive = m_nActionSequence != 0;
		m_bActionSequenceWaitAtEnd = event == PLAYERANIMEVENT_KEEPER_HANDS_CARRY || event == PLAYERANIMEVENT_THROW_IN_CARRY;

		if (event == PLAYERANIMEVENT_TACKLED_FORWARD || event == PLAYERANIMEVENT_TACKLED_BACKWARD)
			GetSDKPlayer()->AddFlag(FL_FREECAM);

		break;
	}
	case PLAYERANIMEVENT_JUMP:
	case PLAYERANIMEVENT_KEEPER_JUMP:
	{
		// Play the jump animation.
		if (!m_bJumping)
		{
			m_bJumping = true;
			m_bFirstJumpFrame = true;
			m_flJumpStartTime = gpGlobals->curtime;
			GetSDKPlayer()->m_Shared.SetAction(event);
		}
		break;
	}
	}
}

bool CSDKPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	if ( m_bJumping )
	{
		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
		{
			if ( GetBasePlayer()->GetFlags() & FL_ONGROUND )
			{
				m_bJumping = false;
				GetSDKPlayer()->m_Shared.SetAction(PLAYERANIMEVENT_NONE);
				RestartMainSequence();	// Reset the animation.
				GetSDKPlayer()->m_Shared.m_bWasJumping = true;
			}
		}
	}

	return m_bJumping;
}

//-----------------------------------------------------------------------------
// Purpose: Overriding CMultiplayerAnimState to add prone and sprinting checks as necessary.
// Input  :  - 
// Output : Activity
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
extern ConVar anim_showmainactivity;
#endif

Activity CSDKPlayerAnimState::CalcMainActivity()
{
	float flOuterSpeed = GetOuterXYSpeed();

	CSDKPlayer	*pPlayer = dynamic_cast<CSDKPlayer*>(GetBasePlayer());

	Activity idealActivity = ACT_IDLE;

	if (pPlayer->m_Shared.m_bIsShotCharging && mp_charging_animation_enabled.GetBool())
		return ACT_IOS_RUNCELEB;

	if ( HandleJumping(idealActivity) )
	{
		if (pPlayer->m_Shared.GetAction() == PLAYERANIMEVENT_KEEPER_JUMP)
			return ACT_LEAP;									//keepers jump
		else
			return ACT_HOP;										//normal jump
	}
	else
	{
		if ( flOuterSpeed > MOVING_MINIMUM_SPEED )
		{
			if ( flOuterSpeed > (mp_runspeed.GetInt() + mp_sprintspeed.GetInt()) / 2.0f )
			{
				idealActivity = ACT_SPRINT;
			}
			else if ( flOuterSpeed > (mp_walkspeed.GetInt() + mp_runspeed.GetInt()) / 2.0f )
			{
				idealActivity = ACT_RUN;
			}
			else
			{
				idealActivity = ACT_WALK;
			}
		}
		else
		{
			idealActivity = ACT_IDLE;
		}

		return idealActivity;
	}
}

void CSDKPlayerAnimState::ComputeSequences( CStudioHdr *pStudioHdr )
{
	VPROF( "CBasePlayerAnimState::ComputeSequences" );

	// Lower body (walk/run/idle).
	ComputeMainSequence();

	// The groundspeed interpolator uses the main sequence info.
	UpdateInterpolators();		
	ComputeActionSequence(pStudioHdr);
	ComputeCarrySequence(pStudioHdr);
	ComputeGestureSequence(pStudioHdr);
}

float CSDKPlayerAnimState::GetCurrentMaxGroundSpeed()
{
	Activity currentActivity = 	GetBasePlayer()->GetSequenceActivity( GetBasePlayer()->GetSequence() );
	if ( currentActivity == ACT_WALK || currentActivity == ACT_IDLE )
		return mp_runspeed.GetInt();
	else if ( currentActivity == ACT_RUN )
		return mp_runspeed.GetInt();
	else if ( currentActivity == ACT_SPRINT )			//IOS
		return mp_sprintspeed.GetInt();
	else if ( currentActivity == ACT_IOS_CARRY )		//IOS
		return mp_runspeed.GetInt();
	else if ( currentActivity == ACT_IOS_RUNCELEB )		//IOS
		return mp_runspeed.GetInt();
	else
		return 0;
}

bool CSDKPlayerAnimState::ShouldUpdateAnimState()
{
	// Don't update anim state if we're not visible
	if ( GetBasePlayer()->IsEffectActive( EF_NODRAW ) )
		return false;

	// By default, don't update their animation state when they're dead because they're
	// either a ragdoll or they're not drawn.
#ifdef CLIENT_DLL
	if ( GetBasePlayer()->IsDormant() )
		return false;
#endif

	return (GetBasePlayer()->IsAlive() || m_bDying);
}

bool CSDKPlayerAnimState::SetupPoseParameters( CStudioHdr *pStudioHdr )
{
	// Check to see if this has already been done.
	if ( m_bPoseParameterInit )
		return true;

	// Save off the pose parameter indices.
	if ( !pStudioHdr )
		return false;

	// Look for the movement blenders.
	m_PoseParameterData.m_iMoveX = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "move_x" );
	m_PoseParameterData.m_iMoveY = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "move_y" );
	if ( ( m_PoseParameterData.m_iMoveX < 0 ) || ( m_PoseParameterData.m_iMoveY < 0 ) )
		return false;

	// Look for the aim pitch blender.
	m_PoseParameterData.m_iAimPitch = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "body_pitch" );
	if ( m_PoseParameterData.m_iAimPitch < 0 )
		return false;

	// Look for aim yaw blender.
	m_PoseParameterData.m_iAimYaw = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "body_yaw" );
	if ( m_PoseParameterData.m_iAimYaw < 0 )
		return false;

	m_bPoseParameterInit = true;

	return true;
}

float SnapYawTo( float flValue )
{
	float flSign = 1.0f;
	if ( flValue < 0.0f )
	{
		flSign = -1.0f;
		flValue = -flValue;
	}

	if ( flValue < 23.0f )
	{
		flValue = 0.0f;
	}
	else if ( flValue < 67.0f )
	{
		flValue = 45.0f;
	}
	else if ( flValue < 113.0f )
	{
		flValue = 90.0f;
	}
	else if ( flValue < 157 )
	{
		flValue = 135.0f;
	}
	else
	{
		flValue = 180.0f;
	}

	return ( flValue * flSign );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pStudioHdr - 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr )
{
	// Get the estimated movement yaw.
	EstimateYaw();

	// Get the view yaw.
	float flAngle = AngleNormalize( m_flEyeYaw );

	// Calc side to side turning - the view vs. movement yaw.
	float flYaw = flAngle - m_PoseParameterData.m_flEstimateYaw;
	flYaw = AngleNormalize( -flYaw );

	// Get the current speed the character is running.
	bool bIsMoving;
	float flPlaybackRate = CalcMovementPlaybackRate( &bIsMoving );

	// Setup the 9-way blend parameters based on our speed and direction.
	Vector2D vecCurrentMoveYaw( 0.0f, 0.0f );
	if ( bIsMoving )
	{
		vecCurrentMoveYaw.x = cos( DEG2RAD( flYaw ) ) * flPlaybackRate;
		vecCurrentMoveYaw.y = -sin( DEG2RAD( flYaw ) ) * flPlaybackRate;
	}

	// Set the 9-way blend movement pose parameters.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y );

	m_DebugAnimData.m_vecMoveYaw = vecCurrentMoveYaw;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::EstimateYaw( void )
{
	// Get the frame time.
	float flDeltaTime = gpGlobals->frametime;
	if ( flDeltaTime == 0.0f )
		return;

	// Get the player's velocity and angles.
	Vector vecEstVelocity;
	GetOuterAbsVelocity( vecEstVelocity );
	QAngle angles = GetBasePlayer()->GetLocalAngles();

	// If we are not moving, sync up the feet and eyes slowly.
	if ( vecEstVelocity.x == 0.0f && vecEstVelocity.y == 0.0f )
	{
		float flYawDelta = angles[YAW] - m_PoseParameterData.m_flEstimateYaw;
		flYawDelta = AngleNormalize( flYawDelta );

		if ( flDeltaTime < 0.25f )
		{
			flYawDelta *= ( flDeltaTime * 4.0f );
		}
		else
		{
			flYawDelta *= flDeltaTime;
		}

		m_PoseParameterData.m_flEstimateYaw += flYawDelta;
		AngleNormalize( m_PoseParameterData.m_flEstimateYaw );
	}
	else
	{
		m_PoseParameterData.m_flEstimateYaw = ( atan2( vecEstVelocity.y, vecEstVelocity.x ) * 180.0f / M_PI );
		m_PoseParameterData.m_flEstimateYaw = clamp( m_PoseParameterData.m_flEstimateYaw, -180.0f, 180.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr )
{
	// Get the view pitch.
	float flAimPitch = m_flEyePitch;

	// Set the aim pitch pose parameter and save.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimPitch, -flAimPitch );
	m_DebugAnimData.m_flAimPitch = flAimPitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr )
{
	// Get the movement velocity.
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	// Check to see if we are moving.
	bool bMoving = ( vecVelocity.Length() > 1.0f ) ? true : false;

	// If we are moving or are prone and undeployed.
	if ( bMoving || m_bForceAimYaw )
	{
		// The feet match the eye direction when moving - the move yaw takes care of the rest.
		m_flGoalFeetYaw = m_flEyeYaw;
	}
	// Else if we are not moving.
	else
	{
		// Initialize the feet.
		if ( m_PoseParameterData.m_flLastAimTurnTime <= 0.0f )
		{
			m_flGoalFeetYaw	= m_flEyeYaw;
			m_flCurrentFeetYaw = m_flEyeYaw;
			m_PoseParameterData.m_flLastAimTurnTime = gpGlobals->curtime;
		}
		// Make sure the feet yaw isn't too far out of sync with the eye yaw.
		// TODO: Do something better here!
		else
		{
			float flYawDelta = AngleNormalize(  m_flGoalFeetYaw - m_flEyeYaw );

			if ( fabs( flYawDelta ) > 45.0f/*m_AnimConfig.m_flMaxBodyYawDegrees*/ )
			{
				float flSide = ( flYawDelta > 0.0f ) ? -1.0f : 1.0f;
				m_flGoalFeetYaw += ( 45.0f/*m_AnimConfig.m_flMaxBodyYawDegrees*/ * flSide );
			}
		}
	}

	// Fix up the feet yaw.
	m_flGoalFeetYaw = AngleNormalize( m_flGoalFeetYaw );
	if ( m_flGoalFeetYaw != m_flCurrentFeetYaw )
	{
		if ( m_bForceAimYaw )
		{
			m_flCurrentFeetYaw = m_flGoalFeetYaw;
		}
		else
		{
			ConvergeYawAngles( m_flGoalFeetYaw, /*DOD_BODYYAW_RATE*/3600.0f, gpGlobals->frametime, m_flCurrentFeetYaw );
			m_flLastAimTurnTime = gpGlobals->curtime;
		}
	}

	// Rotate the body into position.
	m_angRender[YAW] = m_flCurrentFeetYaw;

	// Find the aim(torso) yaw base on the eye and feet yaws.
	float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
	flAimYaw = AngleNormalize( flAimYaw );

	// Set the aim yaw and save.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, -flAimYaw );
	m_DebugAnimData.m_flAimYaw	= flAimYaw;

	// Turn off a force aim yaw - either we have already updated or we don't need to.
	m_bForceAimYaw = true; //ios false;

#ifndef CLIENT_DLL
	QAngle angle = GetBasePlayer()->GetAbsAngles();
	angle[YAW] = m_flCurrentFeetYaw;

	GetBasePlayer()->SetAbsAngles( angle );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flGoalYaw - 
//			flYawRate - 
//			flDeltaTime - 
//			&flCurrentYaw - 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::ConvergeYawAngles( float flGoalYaw, float flYawRate, float flDeltaTime, float &flCurrentYaw )
{
#define FADE_TURN_DEGREES 60.0f

	// Find the yaw delta.
	float flDeltaYaw = flGoalYaw - flCurrentYaw;
	float flDeltaYawAbs = fabs( flDeltaYaw );
	flDeltaYaw = AngleNormalize( flDeltaYaw );

	// Always do at least a bit of the turn (1%).
	float flScale = 1.0f;
	flScale = flDeltaYawAbs / FADE_TURN_DEGREES;
	flScale = clamp( flScale, 0.01f, 1.0f );

	float flYaw = flYawRate * flDeltaTime * flScale;
	if ( flDeltaYawAbs < flYaw )
	{
		flCurrentYaw = flGoalYaw;
	}
	else
	{
		float flSide = ( flDeltaYaw < 0.0f ) ? -1.0f : 1.0f;
		flCurrentYaw += ( flYaw * flSide );
	}

	flCurrentYaw = AngleNormalize( flCurrentYaw );

#undef FADE_TURN_DEGREES
}

void CSDKPlayerAnimState::RestartMainSequence( void )
{
	CBaseAnimatingOverlay *pPlayer = GetBasePlayer();
	if ( pPlayer )
	{
		pPlayer->m_flAnimTime = gpGlobals->curtime;
		pPlayer->SetCycle( 0 );
	}
}

float CSDKPlayerAnimState::GetOuterXYSpeed()
{
	Vector vel;
	GetOuterAbsVelocity( vel );
	return vel.Length2D();
}

void CSDKPlayerAnimState::ComputeMainSequence()
{
	VPROF( "CBasePlayerAnimState::ComputeMainSequence" );

	CBaseAnimatingOverlay *pPlayer = GetBasePlayer();

	// Have our class or the mod-specific class determine what the current activity is.
	Activity idealActivity = CalcMainActivity();

#ifdef CLIENT_DLL
	Activity oldActivity = m_eCurrentMainSequenceActivity;
#endif
	
	// Store our current activity so the aim and fire layers know what to do.
	m_eCurrentMainSequenceActivity = idealActivity;

		// Export to our outer class..
	int animDesired = SelectWeightedSequence( TranslateActivity(idealActivity) );

#if !defined( HL1_CLIENT_DLL ) && !defined ( HL1_DLL )
	if ( pPlayer->GetSequenceActivity( pPlayer->GetSequence() ) == pPlayer->GetSequenceActivity( animDesired ) )
		return;
#endif

	if ( animDesired < 0 )
		 animDesired = 0;

	pPlayer->ResetSequence( animDesired );

#ifdef CLIENT_DLL
	// If we went from idle to walk, reset the interpolation history.
	// Kind of hacky putting this here.. it might belong outside the base class.
	if ( (oldActivity == ACT_CROUCHIDLE || oldActivity == ACT_IDLE) && 
		 (idealActivity == ACT_WALK || idealActivity == ACT_RUN_CROUCH) )
	{
		ResetGroundSpeed();
	}
#endif
}

void CSDKPlayerAnimState::UpdateInterpolators()
{
	VPROF( "CBasePlayerAnimState::UpdateInterpolators" );

	// First, figure out their current max speed based on their current activity.
	float flCurMaxSpeed = GetCurrentMaxGroundSpeed();

#ifdef CLIENT_DLL
	float flGroundSpeedInterval = 0.1;

	// Only update this 10x/sec so it has an interval to interpolate over.
	if ( gpGlobals->curtime - m_flLastGroundSpeedUpdateTime >= flGroundSpeedInterval )
	{
		m_flLastGroundSpeedUpdateTime = gpGlobals->curtime;

		m_flMaxGroundSpeed = flCurMaxSpeed;
		m_iv_flMaxGroundSpeed.NoteChanged( gpGlobals->curtime, flGroundSpeedInterval, false );
	}

	m_iv_flMaxGroundSpeed.Interpolate( gpGlobals->curtime, flGroundSpeedInterval );
#else
	m_flMaxGroundSpeed = flCurMaxSpeed;
#endif
}

float CSDKPlayerAnimState::CalcMovementPlaybackRate( bool *bIsMoving )
{
	// Get the player's current velocity and speed.
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );
	float flSpeed = vecVelocity.Length2D();

	// Determine if the player is considered moving or not.
	bool bMoving = ( flSpeed > MOVING_MINIMUM_SPEED );

	// Initialize the return data.
	*bIsMoving = false;
	float flReturn = 1.0f;

	// If we are moving.
	if ( bMoving )
	{
		//		float flGroundSpeed = GetInterpolatedGroundSpeed();
		float flGroundSpeed = GetCurrentMaxGroundSpeed();
		if ( flGroundSpeed < 0.001f )
		{
			flReturn = 0.01;
		}
		else
		{
			// Note this gets set back to 1.0 if sequence changes due to ResetSequenceInfo below
			flReturn = flSpeed / flGroundSpeed;
			flReturn = clamp( flReturn, 0.01f, 10.0f );
		}

		*bIsMoving = true;
	}

	return flReturn;
}

void CSDKPlayerAnimState::GetOuterAbsVelocity( Vector& vel )
{
#if defined( CLIENT_DLL )
	GetBasePlayer()->EstimateAbsVelocity( vel );
#else
	vel = GetBasePlayer()->GetAbsVelocity();
#endif
}

void CSDKPlayerAnimState::ResetGroundSpeed( void )
{
#ifdef CLIENT_DLL
		m_flMaxGroundSpeed = GetCurrentMaxGroundSpeed();
		m_iv_flMaxGroundSpeed.Reset();
		m_iv_flMaxGroundSpeed.NoteChanged( gpGlobals->curtime, 0, false );
#endif
}

const QAngle& CSDKPlayerAnimState::GetRenderAngles()
{
	return m_angRender;
}

void CSDKPlayerAnimState::OnNewModel( void )
{
	m_bPoseParameterInit = false;
	ClearAnimationState();
}

int CSDKPlayerAnimState::SelectWeightedSequence( Activity activity )
{
	return GetSDKPlayer()->SelectWeightedSequence( activity );
}