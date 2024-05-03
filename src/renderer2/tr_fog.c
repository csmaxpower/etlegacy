/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file renderer2/tr_fog.c
 */

#include "tr_local.h"


/**
 * @brief RE_SetFog
 * @param[in] fogvar
 * @param[in] var1
 * @param[in] var2
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param[in] density
 *
 * @note
 * if fogvar == FOG_CMD_SWITCHFOG {
 *   fogvar is the command
 *   var1 is the fog to switch to
 *   var2 is the time to transition
 * }
 * else {
 *   fogvar is the fog that's being set
 *   var1 is the near fog z value
 *   var2 is the far fog z value
 *   rgb = color
 *   density is density, and is used to derive the values of 'mode', 'drawsky', and 'clearscreen'
 * }
 *
 * note: If the map loads: skyfogvars, waterfogvars, fogvars    are set.
 *       Global fog is loaded as one of the bsp lumps.
 */
void RE_SetFog(int fogvar, int var1, int var2, float r, float g, float b, float density)
{
	Ren_Developer("RE_SetFog( fogvar = %i, var1 = %i, var2 = %i, r = %f, g = %f, b = %f, density = %f )\n",
	              fogvar, var1, var2, r, g, b, density);

	if (fogvar != FOG_CMD_SWITCHFOG)
	{
		// don't switch to invalid fogs
		if (tr.glfogsettings[var1].registered != qtrue)
		{
			return;
		}

		if (tr.glfogNum == var1) return;

		tr.glfogNum = var1;

		// transitioning to new fog, store the current values as the 'from'

// FOG_CURRENT never gets registered? in old code
//		if (tr.glfogsettings[FOG_CURRENT].registered)
//		{
			Com_Memcpy(&tr.glfogsettings[FOG_LAST], &tr.glfogsettings[FOG_CURRENT], sizeof(glfog_t));
//		}
//		else
//		{
//			// if no current fog fall back to world fog
//			// FIXME: handle transition if there is no FOG_MAP fog
//			Com_Memcpy(&tr.glfogsettings[FOG_LAST], &tr.glfogsettings[FOG_MAP], sizeof(glfog_t));
//		}

		Com_Memcpy(&tr.glfogsettings[FOG_TARGET], &tr.glfogsettings[tr.glfogNum], sizeof(glfog_t));

		// setup transition times
		tr.glfogsettings[FOG_TARGET].startTime = tr.refdef.time;
		tr.glfogsettings[FOG_TARGET].finishTime = tr.refdef.time + var2;
		tr.glfogsettings[FOG_TARGET].registered = qtrue;
		tr.glfogsettings[FOG_CURRENT].registered = qtrue;
	}
	else
	{
		// just set the parameters and return
		if (var1 == 0 && var2 == 0)
		{
			// clear this fog
			tr.glfogsettings[fogvar].registered = qfalse;
			return;
		}

		tr.glfogsettings[fogvar].color[0] = r * tr.identityLight;
		tr.glfogsettings[fogvar].color[1] = g * tr.identityLight;
		tr.glfogsettings[fogvar].color[2] = b * tr.identityLight;
		tr.glfogsettings[fogvar].color[3] = 1;
		tr.glfogsettings[fogvar].start    = var1;
		tr.glfogsettings[fogvar].end      = var2;
		if (density >= 1)
		{
			// distance fog
			tr.glfogsettings[fogvar].mode        = GL_LINEAR;
			tr.glfogsettings[fogvar].drawsky     = qfalse;
			tr.glfogsettings[fogvar].clearscreen = qtrue;
			tr.glfogsettings[fogvar].density     = 1.0;
		}
		else
		{
			// quake sky
			tr.glfogsettings[fogvar].mode        = GL_EXP;
			tr.glfogsettings[fogvar].drawsky     = qtrue;
			tr.glfogsettings[fogvar].clearscreen = qfalse;
			tr.glfogsettings[fogvar].density     = density;
		}

		tr.glfogsettings[fogvar].hint = GL_DONT_CARE;

		tr.glfogsettings[fogvar].registered = qtrue;

		return;
	}
}

/**
 * @brief R_SetFrameFog
 * 
 * This sets the current fog values for this frame.
 * It will handle any transition from one fog to another fog.
 */
void R_SetFrameFog(void)
{
	// new style global fog transitions
	if (tr.world->globalFogTransEndTime)
	{
		if (tr.world->globalFogTransEndTime >= tr.refdef.time)
		{
			int   fadeTime = tr.world->globalFogTransEndTime - tr.world->globalFogTransStartTime;
			float lerpPos;
			if (fadeTime == 0)
			{
				lerpPos = 1.f;
			}
			else
			{
			lerpPos  = (float)(tr.refdef.time - tr.world->globalFogTransStartTime) / (float)fadeTime;
			}

				if (lerpPos > 1)
				{
					lerpPos = 1;
				}
			}
			vec3_t vec;
			VectorSubtract(tr.world->globalTransEndFog, tr.world->globalTransStartFog, vec);
			VectorMA(vec, lerpPos, tr.world->globalTransStartFog, tr.world->fogs[tr.world->globalFog].color);

			tr.world->fogs[tr.world->globalFog].fogParms.colorInt = ColorBytes4(tr.world->fogs[tr.world->globalFog].fogParms.color[0] * tr.identityLight, tr.world->fogs[tr.world->globalFog].fogParms.color[1] * tr.identityLight, tr.world->fogs[tr.world->globalFog].fogParms.color[2] * tr.identityLight, 1.0);
			tr.world->fogs[tr.world->globalFog].fogParms.depthForOpaque = (tr.world->globalTransEndFog[3] - tr.world->globalTransStartFog[3]) * lerpPos + tr.world->globalTransStartFog[3];
			//tr.world->fogs[tr.world->globalFog].fogParms.tcScale = rcp(tr.world->fogs[tr.world->globalFog].fogParms.depthForOpaque * 8.f); // this is a hack..
			tr.world->fogs[tr.world->globalFog].fogParms.tcScale = rcp(tr.world->fogs[tr.world->globalFog].fogParms.depthForOpaque);
		}
		else
		{
			// transition complete
			VectorCopy(tr.world->globalTransEndFog, tr.world->fogs[tr.world->globalFog].color);
			tr.world->fogs[tr.world->globalFog].fogParms.colorInt = ColorBytes4(tr.world->globalTransEndFog[0] * tr.identityLight, tr.world->globalTransEndFog[1] * tr.identityLight, tr.world->globalTransEndFog[2] * tr.identityLight, 1.0);
			tr.world->fogs[tr.world->globalFog].fogParms.depthForOpaque = tr.world->globalTransEndFog[3];
			//tr.world->fogs[tr.world->globalFog].fogParms.density = tr.world->globalTransEndFog[3];
			//tr.world->fogs[tr.world->globalFog].fogParms.tcScale = rcp(tr.world->globalTransEndFog[3] * 8.f); // this is a hack..
			tr.world->fogs[tr.world->globalFog].fogParms.tcScale = rcp(tr.world->globalTransEndFog[3]);
			tr.world->globalFogTransEndTime = 0; // stop any transition
		}

		// do we need to exit this function now?..
		//return; //!!!DEBUG!!!
	}
return; //!!!DEBUG!!!
/*
	if (r_speeds->integer == RSPEEDS_FOG)
	{
		if (!tr.glfogsettings[FOG_TARGET].registered)
		{
			Ren_Print("no fog - calc zFar: %0.1f\n", tr.viewParms.zFar);
			return;
		}
	}

	// If fog is not valid, don't use it
	if (!tr.glfogsettings[FOG_TARGET].registered)
	{
		return;
	}
*/
	// still fading
	if (tr.glfogsettings[FOG_TARGET].finishTime && tr.glfogsettings[FOG_TARGET].finishTime > tr.refdef.time)
	{
		float lerpPos;
		int   fadeTime;

		// transitioning from density to distance
		if (tr.glfogsettings[FOG_LAST].mode == GL_EXP && tr.glfogsettings[FOG_TARGET].mode == GL_LINEAR)
		{
			// for now just fast transition to the target when dissimilar fogs are used
			Com_Memcpy(&tr.glfogsettings[FOG_CURRENT], &tr.glfogsettings[FOG_TARGET], sizeof(glfog_t));
			tr.glfogsettings[FOG_TARGET].finishTime = 0;
		}
		// transitioning from distance to density
		else if (tr.glfogsettings[FOG_LAST].mode == GL_LINEAR && tr.glfogsettings[FOG_TARGET].mode == GL_EXP)
		{
			Com_Memcpy(&tr.glfogsettings[FOG_CURRENT], &tr.glfogsettings[FOG_TARGET], sizeof(glfog_t));
			tr.glfogsettings[FOG_TARGET].finishTime = 0;
		}
		// transitioning like fog modes
		else
		{
			fadeTime = tr.glfogsettings[FOG_TARGET].finishTime - tr.glfogsettings[FOG_TARGET].startTime;
			if (fadeTime <= 0)
			{
				Com_Memcpy(&tr.glfogsettings[FOG_CURRENT], &tr.glfogsettings[FOG_TARGET], sizeof(glfog_t));
				tr.glfogsettings[FOG_TARGET].finishTime = 0;
			}
			else
			{
				lerpPos = (float)(tr.refdef.time - tr.glfogsettings[FOG_TARGET].startTime) / (float)fadeTime;
				if (lerpPos > 1)
				{
					lerpPos = 1;
				}

				// lerp depth
				tr.glfogsettings[FOG_CURRENT].start =
					tr.glfogsettings[FOG_LAST].start + ((tr.glfogsettings[FOG_TARGET].start - tr.glfogsettings[FOG_LAST].start) * lerpPos);
				tr.glfogsettings[FOG_CURRENT].end =
					tr.glfogsettings[FOG_LAST].end + ((tr.glfogsettings[FOG_TARGET].end - tr.glfogsettings[FOG_LAST].end) * lerpPos);

				// lerp color
				tr.glfogsettings[FOG_CURRENT].color[0] =
					tr.glfogsettings[FOG_LAST].color[0] +
					((tr.glfogsettings[FOG_TARGET].color[0] - tr.glfogsettings[FOG_LAST].color[0]) * lerpPos);
				tr.glfogsettings[FOG_CURRENT].color[1] =
					tr.glfogsettings[FOG_LAST].color[1] +
					((tr.glfogsettings[FOG_TARGET].color[1] - tr.glfogsettings[FOG_LAST].color[1]) * lerpPos);
				tr.glfogsettings[FOG_CURRENT].color[2] =
					tr.glfogsettings[FOG_LAST].color[2] +
					((tr.glfogsettings[FOG_TARGET].color[2] - tr.glfogsettings[FOG_LAST].color[2]) * lerpPos);

				tr.glfogsettings[FOG_CURRENT].density    = tr.glfogsettings[FOG_TARGET].density;
				tr.glfogsettings[FOG_CURRENT].mode       = tr.glfogsettings[FOG_TARGET].mode;
				tr.glfogsettings[FOG_CURRENT].registered = qtrue;

				// if either fog in the transition clears the screen, clear the background this frame to avoid hall of mirrors
				tr.glfogsettings[FOG_CURRENT].clearscreen = (tr.glfogsettings[FOG_TARGET].clearscreen || tr.glfogsettings[FOG_LAST].clearscreen);
			}
		}
	}
	else
	{
		if (tr.glfogsettings[FOG_TARGET].finishTime)
		{
			tr.glfogsettings[FOG_TARGET].finishTime = 0;
			// probably usually not necessary to copy the whole thing.
			// potential FIXME: since this is the most common occurance, diff first and only set changes
			Com_Memcpy(&tr.glfogsettings[FOG_CURRENT], &tr.glfogsettings[FOG_TARGET], sizeof(glfog_t));
		}
	}

	// shorten the far clip if the fog opaque distance is closer than the procedural farcip dist
	if (tr.glfogsettings[FOG_CURRENT].mode == GL_LINEAR)
	{
		if (tr.glfogsettings[FOG_CURRENT].end < tr.viewParms.zFar)
		{
			tr.viewParms.zFar = tr.glfogsettings[FOG_CURRENT].end;
		}
	}

	if (r_speeds->integer == RSPEEDS_FOG)
	{
		if (tr.glfogsettings[FOG_CURRENT].mode == GL_LINEAR)
		{
			Ren_Print("farclip fog - den: %0.1f  calc zFar: %0.1f  fog zfar: %0.1f\n", tr.glfogsettings[FOG_CURRENT].density, tr.viewParms.zFar, tr.glfogsettings[FOG_CURRENT].end);
		}
		else
		{
			Ren_Print("density fog - den: %0.4f  calc zFar: %0.1f  fog zFar: %0.1f\n",
			          tr.glfogsettings[FOG_CURRENT].density, tr.viewParms.zFar, tr.glfogsettings[FOG_CURRENT].end);
		}
	}
}

/**
 * @brief RE_SetGlobalFog
 * @param[in] restore flag can be used to restore the original level fog
 * @param[in] duration can be set to fade over a certain period
 * @param[in] r colour
 * @param[in] g colour
 * @param[in] b colour
 * @param[in] depthForOpaque is depth for opaque
 * 
 * NOTE: This gives me an exception on ColorBytes4
 */
void RE_SetGlobalFog(qboolean restore, int duration, float r, float g, float b, float depthForOpaque)
{
	Ren_Developer("RE_SetGlobalFog( restore = %i, duration = %i, r = %f, g = %f, b = %f, depthForOpaque = %f )\n",
	              restore, duration, r, g, b, depthForOpaque);

	if (restore)
	{
		if (duration > 0)
		{
			VectorCopy(tr.world->fogs[tr.world->globalFog].fogParms.color, tr.world->globalTransStartFog);
			tr.world->globalTransStartFog[3] = tr.world->fogs[tr.world->globalFog].fogParms.depthForOpaque;

			Vector4Copy(tr.world->globalOriginalFog, tr.world->globalTransEndFog);

			tr.world->globalFogTransStartTime = tr.refdef.time;
			tr.world->globalFogTransEndTime   = tr.refdef.time + duration;
		}
		else
		{
			VectorCopy(tr.world->globalOriginalFog, tr.world->fogs[tr.world->globalFog].fogParms.color);

			tr.world->fogs[tr.world->globalFog].fogParms.colorInt =
				ColorBytes4(tr.world->globalOriginalFog[0] * tr.identityLight, tr.world->globalOriginalFog[1] * tr.identityLight,
				            tr.world->globalOriginalFog[2] * tr.identityLight, 1.0); // 1.0 alpha?

			tr.world->fogs[tr.world->globalFog].fogParms.depthForOpaque = tr.world->globalOriginalFog[3];
			tr.world->fogs[tr.world->globalFog].tcScale = rcp(tr.world->fogs[tr.world->globalFog].fogParms.depthForOpaque);
		}
	}
	else
	{
		if (duration > 0)
		{
			VectorCopy(tr.world->fogs[tr.world->globalFog].fogParms.color, tr.world->globalTransStartFog);
			tr.world->globalTransStartFog[3] = tr.world->fogs[tr.world->globalFog].fogParms.depthForOpaque;

			VectorSet(tr.world->globalTransEndFog, r, g, b);
			tr.world->globalTransEndFog[3] = depthForOpaque;

			tr.world->globalFogTransStartTime = tr.refdef.time;
			tr.world->globalFogTransEndTime   = tr.refdef.time + duration;
		}
		else
		{
			VectorSet(tr.world->fogs[tr.world->globalFog].fogParms.color, r, g, b);

			tr.world->fogs[tr.world->globalFog].fogParms.colorInt = ColorBytes4(r * tr.identityLight,
			                                                                    g * tr.identityLight,
			                                                                    b * tr.identityLight, 1.0);

			tr.world->fogs[tr.world->globalFog].fogParms.depthForOpaque = depthForOpaque < 1 ? 1 : depthForOpaque;
			tr.world->fogs[tr.world->globalFog].tcScale = rcp(tr.world->fogs[tr.world->globalFog].fogParms.depthForOpaque);
		}
	}
}