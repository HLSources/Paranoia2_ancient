/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "rain.h"
#include "studio.h"
#include "r_studioint.h"

//LRC - the fogging fog
float g_fFogColor[3];
float g_fStartDist;
float g_fEndDist;
//int g_iFinalStartDist; //for fading
int g_iFinalEndDist;   //for fading
float g_fFadeDuration; //negative = fading out
extern engine_studio_api_t IEngineStudio;
extern int g_iGunMode;

#define MAX_CLIENTS 32

void EV_HLDM_WaterSplash( float x, float y, float z, float ScaleSplash1, float ScaleSplash2 );

int CHud :: MsgFunc_WaterSplash( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	float X, Y, Z, ScaleSplash1, ScaleSplash2;
    
	X = READ_COORD();
	Y = READ_COORD();
	Z = READ_COORD();
	ScaleSplash1 = READ_COORD();
	ScaleSplash2 = READ_COORD();
    
	EV_HLDM_WaterSplash( X, Y, Z, ScaleSplash1, ScaleSplash2 );
	return 1;
}

void EV_HLDM_NewExplode( float x, float y, float z, float ScaleExplode1 );

int CHud :: MsgFunc_NewExplode( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	float X, Y, Z, ScaleExplode1;
    
	X = READ_COORD();
	Y = READ_COORD();
	Z = READ_COORD();
	ScaleExplode1 = READ_COORD();
    
	EV_HLDM_NewExplode( X, Y, Z, ScaleExplode1 );
	return 1;
}

extern BEAM *pBeam;
extern BEAM *pBeam2;

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	//LRC - reset fog
	g_fStartDist = 0;
	g_fEndDist = 0;
	g_iGunMode = 1;

	return 1;
}

void CAM_ToFirstPerson(void);

void CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	//LRC - clear the fog
	g_fStartDist = 0;
	g_fEndDist = 0;

	//LRC - clear all shiny surfaces
	if (m_pShinySurface)
	{
		delete m_pShinySurface;
		m_pShinySurface = NULL;
	}

	m_iSkyMode = SKY_OFF; //LRC

	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
	g_iGunMode = 1;
}

//LRC
void CHud :: MsgFunc_SetFog( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	for ( int i = 0; i < 3; i++ )
		 g_fFogColor[ i ] = READ_BYTE();

	g_fFadeDuration = READ_SHORT();
	g_fStartDist = READ_SHORT();

	if (g_fFadeDuration > 0)
	{
//		// fading in
//		g_fStartDist = READ_SHORT();
		g_iFinalEndDist = READ_SHORT();
//		g_fStartDist = FOG_LIMIT;
		g_fEndDist = FOG_LIMIT;
	}
	else if (g_fFadeDuration < 0)
	{
//		// fading out
//		g_iFinalStartDist = 
		g_iFinalEndDist = g_fEndDist = READ_SHORT();
	}
	else
	{
//		g_fStartDist = READ_SHORT();
		g_fEndDist = READ_SHORT();
	}
}

//LRC
void CHud :: MsgFunc_KeyedDLight( const char *pszName, int iSize, void *pbuf )
{
//	CONPRINT("MSG:KeyedDLight");
	BEGIN_READ( pbuf, iSize );

// as-yet unused:
//	float	decay;				// drop this each second
//	float	minlight;			// don't add when contributing less
//	qboolean	dark;			// subtracts light instead of adding (doesn't seem to do anything?)

	int iKey = READ_BYTE();
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight( iKey );

	int bActive = READ_BYTE();
	if (!bActive)
	{
		// die instantly
		dl->die = gEngfuncs.GetClientTime();
	}
	else
	{
		// never die
		dl->die = gEngfuncs.GetClientTime() + 1E6;

		dl->origin[0] = READ_COORD();
		dl->origin[1] = READ_COORD();
		dl->origin[2] = READ_COORD();
		dl->radius = READ_BYTE();
		dl->color.r = READ_BYTE();
		dl->color.g = READ_BYTE();
		dl->color.b = READ_BYTE();
	}
}

//LRC
void CHud :: MsgFunc_AddShine( const char *pszName, int iSize, void *pbuf )
{
//	CONPRINT("MSG:AddShine");
	BEGIN_READ( pbuf, iSize );

	float fScale = READ_BYTE();
	float fAlpha = READ_BYTE()/255.0;
	float fMinX = READ_COORD();
	float fMaxX = READ_COORD();
	float fMinY = READ_COORD();
	float fMaxY = READ_COORD();
	float fZ = READ_COORD();
	char *szSprite = READ_STRING();

//	gEngfuncs.Con_Printf("minx %f, maxx %f, miny %f, maxy %f\n", fMinX, fMaxX, fMinY, fMaxY);

	CShinySurface *pSurface = new CShinySurface(fScale, fAlpha, fMinX, fMaxX, fMinY, fMaxY, fZ, szSprite);
	pSurface->m_pNext = m_pShinySurface;
	m_pShinySurface = pSurface;
}

//LRC
void CHud :: MsgFunc_SetSky( const char *pszName, int iSize, void *pbuf )
{
//	CONPRINT("MSG:SetSky");
	BEGIN_READ( pbuf, iSize );

	m_iSkyMode = READ_BYTE();
	m_vecSkyPos.x = READ_COORD();
	m_vecSkyPos.y = READ_COORD();
	m_vecSkyPos.z = READ_COORD();
}

int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	return 1;
}


int CHud :: MsgFunc_Damage(const char *pszName, int iSize, void *pbuf )
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;
	
	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i=0 ; i<3 ; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss",255,160,0);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}

// buz: gasmask message
int CHud :: MsgFunc_GasMask( const char *pszName, int iSize, void *pbuf )
{
	studiohdr_t *pStudioHeader;
	mstudioseqdesc_t *pseq;

	BEGIN_READ( pbuf, iSize );

	gHUD.m_pHeadShieldEnt->model = IEngineStudio.Mod_ForName( "models/v_gasmask.mdl", true );
	
	// 0 is OFF; 1 is ON; 2 is fast switch to ON
	switch( READ_BYTE( ))
	{
	case 0:
		m_iHeadShieldState = SHIELD_TURNING_OFF;
		m_pHeadShieldEnt->curstate.animtime = gEngfuncs.GetClientTime();
		m_pHeadShieldEnt->curstate.sequence = SHIELDANIM_HOLSTER;

		// get animation length in seconds
		pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata( gHUD.m_pHeadShieldEnt->model );
		pseq = (mstudioseqdesc_t *)((byte *)pStudioHeader + pStudioHeader->seqindex) + SHIELDANIM_HOLSTER;
		m_flHeadShieldSwitchTime = gEngfuncs.GetClientTime() + (pseq->numframes / pseq->fps);
		break;
	case 1:
		m_iHeadShieldState = SHIELD_TURNING_ON;
		m_pHeadShieldEnt->curstate.animtime = gEngfuncs.GetClientTime();
		m_pHeadShieldEnt->curstate.sequence = SHIELDANIM_DRAW;

		// get animation length in seconds
		pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata( gHUD.m_pHeadShieldEnt->model );
		pseq = (mstudioseqdesc_t *)((byte *)pStudioHeader + pStudioHeader->seqindex) + SHIELDANIM_DRAW;
		m_flHeadShieldSwitchTime = gEngfuncs.GetClientTime() + (pseq->numframes / pseq->fps);
		break;
	case 2:
		m_iHeadShieldState = SHIELD_ON;
		m_pHeadShieldEnt->curstate.animtime = gEngfuncs.GetClientTime();
		m_pHeadShieldEnt->curstate.sequence = SHIELDANIM_IDLE;
	}

	return 1;
}

int CHud::MsgFunc_HeadShield( const char *pszName, int iSize, void *pbuf )
{
	studiohdr_t *pStudioHeader;
	mstudioseqdesc_t *pseq;

	BEGIN_READ( pbuf, iSize );

	gHUD.m_pHeadShieldEnt->model = IEngineStudio.Mod_ForName( "models/v_headshield.mdl", true );
	
	// 0 is OFF; 1 is ON; 2 is fast switch to ON
	switch( READ_BYTE( ))
	{
	case 0:
		m_iHeadShieldState = SHIELD_TURNING_OFF;
		m_pHeadShieldEnt->curstate.animtime = gEngfuncs.GetClientTime();
		m_pHeadShieldEnt->curstate.sequence = SHIELDANIM_HOLSTER;

		// get animation length in seconds
		pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata( gHUD.m_pHeadShieldEnt->model );
		pseq = (mstudioseqdesc_t *)((byte *)pStudioHeader + pStudioHeader->seqindex) + SHIELDANIM_HOLSTER;
		m_flHeadShieldSwitchTime = gEngfuncs.GetClientTime() + (pseq->numframes / pseq->fps);
		break;
	case 1:
		m_iHeadShieldState = SHIELD_TURNING_ON;
		m_pHeadShieldEnt->curstate.animtime = gEngfuncs.GetClientTime();
		m_pHeadShieldEnt->curstate.sequence = SHIELDANIM_DRAW;

		// get animation length in seconds
		pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata( gHUD.m_pHeadShieldEnt->model );
		pseq = (mstudioseqdesc_t *)((byte *)pStudioHeader + pStudioHeader->seqindex) + SHIELDANIM_DRAW;
		m_flHeadShieldSwitchTime = gEngfuncs.GetClientTime() + (pseq->numframes / pseq->fps);
		break;
	case 2:
		m_iHeadShieldState = SHIELD_ON;
		m_pHeadShieldEnt->curstate.animtime = gEngfuncs.GetClientTime();
		m_pHeadShieldEnt->curstate.sequence = SHIELDANIM_IDLE;
	}

	return 1;
}

// buz: special tank message
int CHud :: MsgFunc_SpecTank( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	m_SpecTank_on = READ_BYTE();
	if (m_SpecTank_on == 0) // turn off
		return 1;
	else if (m_SpecTank_on == 2) // only ammo update
	{
		m_SpecTank_Ammo = READ_LONG();
		m_Ammo.m_fFade = 200.0f;
	}
	else // turn on
	{
		m_SpecTank_point.x = READ_COORD();
		m_SpecTank_point.y = READ_COORD();
		m_SpecTank_point.z = READ_COORD();
		m_SpecTank_defYaw = READ_COORD();
		m_SpecTank_coneHor = READ_COORD();
		m_SpecTank_coneVer = READ_COORD();
		m_SpecTank_distFwd = READ_COORD();
		m_SpecTank_distUp = READ_COORD();
		m_SpecTank_Ammo = READ_LONG();
	}
	return 1;
}

int CHud :: MsgFunc_MusicFade( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	MUSIC_FADE_VOLUME( (float)READ_SHORT() / 100.0f );

	return 1;
}

int CHud :: MsgFunc_Particle( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int entindex = READ_SHORT();
	char *sz = READ_STRING();
	int attachment = READ_BYTE();

	UTIL_CreateAurora( GET_ENTITY( entindex ), sz, attachment );

	return 1;
}

int CHud :: MsgFunc_DelParticle( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int entindex = READ_SHORT();

	UTIL_RemoveAurora( GET_ENTITY( entindex ));

	return 1;
}

// rain
extern rain_properties Rain;

int CHud :: MsgFunc_RainData( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Rain.dripsPerSecond =	READ_SHORT();
	Rain.distFromPlayer =	READ_COORD();
	Rain.windX =		READ_COORD();
	Rain.windY =		READ_COORD();
	Rain.randX =		READ_COORD();
	Rain.randY =		READ_COORD();
	Rain.weatherMode =		READ_SHORT();
	Rain.globalHeight =		READ_COORD();
		
	return 1;
}