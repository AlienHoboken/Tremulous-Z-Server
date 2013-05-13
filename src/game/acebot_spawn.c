/*
===========================================================================
Copyright (C) 1998 Steve Yeager
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2008 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
//  acebot_spawn.c - This file contains all of the
//                   spawing support routines for the ACE bot.


#include "g_local.h"
#include "acebot.h"

#if defined(ACEBOT)

int             g_numArenas;

extern gentity_t *podium1;
extern gentity_t *podium2;
extern gentity_t *podium3;

/*static char    *ACESP_GetBotInfoByNumber(int num)
{
	if(num < 0 || num >= g_numBots)
	{
		trap_Printf(va(S_COLOR_RED "Invalid bot number: %i\n", num));
		return NULL;
	}
	return g_botInfos[num];
}*/
// Remove a bot by name or all bots
void ACESP_RemoveBot(char *name)
{
#if 0
	int             i;
	qboolean        freed = false;
	gentity_t      *bot;

	for(i = 0; i < maxclients->value; i++)
	{
		bot = g_edicts + i + 1;
		if(bot->inuse)
		{
			if(bot->is_bot && (strcmp(bot->client->pers.netname, name) == 0 || strcmp(name, "all") == 0))
			{
				bot->health = 0;
				player_die(bot, bot, bot, 100000, vec3_origin);
				// don't even bother waiting for death frames
				bot->deadflag = DEAD_DEAD;
				bot->inuse = false;
				freed = true;
				ACEIT_PlayerRemoved(bot);
				safe_bprintf(PRINT_MEDIUM, "%s removed\n", bot->client->pers.netname);
			}
		}
	}

	if(!freed)
		safe_bprintf(PRINT_MEDIUM, "%s not found\n", name);

	//ACESP_SaveBots();         // Save them again
#endif
}

qboolean ACESP_BotConnect(int clientNum, qboolean restart)
{
#if 1
	char            userinfo[MAX_INFO_STRING];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	//Q_strncpyz(settings.characterfile, Info_ValueForKey(userinfo, "characterfile"), sizeof(settings.characterfile));
	//settings.skill = atof(Info_ValueForKey(userinfo, "skill"));
	//Q_strncpyz(settings.team, Info_ValueForKey(userinfo, "team"), sizeof(settings.team));

	//if(!BotAISetupClient(clientNum, &settings, restart))
	//{
	//  trap_DropClient(clientNum, "BotAISetupClient failed");
	//  return qfalse;
	//}

	/*
	   bot = &g_entities[clientNum];

	   // set bot state
	   bot = g_entities + clientNum;

	   bot->enemy = NULL;
	   bot->bs.moveTarget = NULL;
	   bot->bs.state = STATE_MOVE;

	   // set the current node
	   bot->bs.currentNode = ACEND_FindClosestReachableNode(bot, NODE_DENSITY, NODE_ALL);
	   bot->bs.goalNode = bot->bs.currentNode;
	   bot->bs.nextNode = bot->bs.currentNode;
	   bot->bs.next_move_time = level.time;
	   bot->bs.suicide_timeout = level.time + 15000;
	 */

	return qtrue;
#endif
}


void ACESP_SetupBotState(gentity_t * self)
{
	int             clientNum;
	char            userinfo[MAX_INFO_STRING];
	//char           *team;

	//G_Printf("ACESP_SetupBotState()\n");

	clientNum = self->client - level.clients;
	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	//self->classname = "acebot";
	self->enemy = NULL;

	//self->bs.turnSpeed = 35;	// 100 is deadly fast
	self->bs.moveTarget = NULL;
	self->bs.state = STATE_MOVE;

	// set the current node
	ACEND_setCurrentNode(self, ACEND_FindClosestReachableNode(self, NODE_DENSITY, NODE_MOVE));
	self->bs.goalNode = self->bs.currentNode;
	ACEND_setNextNode(self, self->bs.currentNode);

	//self->bs.nextNode = self->bs.currentNode;
	self->bs.lastNode = INVALID;
	self->bs.next_move_time = level.time;
	self->bs.suicide_timeout = level.time + 15000;

	/*
	   // is the bot part of a team when gameplay has changed?
	   team = Info_ValueForKey(userinfo, "team");
	   if(!team || !*team)
	   {
	   if(g_gametype.integer >= GT_TEAM)
	   {
	   if(PickTeam(clientNum) == TEAM_RED)
	   {
	   team = "red";
	   }
	   else
	   {
	   team = "blue";
	   }
	   }
	   else
	   {
	   team = "red";
	   }
	   //Info_SetValueForKey(userinfo, "team", team);

	   // need to send this or bots will be spectators
	   trap_BotClientCommand(self - g_entities, va("team %s", team));
	   }
	 */

	//if(g_gametype.integer >= GT_TEAM)
	//  trap_BotClientCommand(self - g_entities, va("team %s", Info_ValueForKey(userinfo, "team")));
}



#endif
