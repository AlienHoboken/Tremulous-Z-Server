/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
 */

#include "g_local.h"


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
 */

/*
================
G_WriteClientSessionData

Called on game shutdown
================
 */
void G_WriteClientSessionData(gclient_t *client) {
  const char *s;
  const char *var;

  s = va("%i %i %i %i %i %i %i %i %s",
          client->sess.sessionTeam,
          client->sess.restartTeam,
          client->sess.spectatorTime,
          client->sess.spectatorState,
          client->sess.spectatorClient,
          client->sess.wins,
          client->sess.losses,
          client->sess.teamLeader,
          BG_ClientListString(&client->sess.ignoreList)
          );

  var = va("session%i", client - level.clients);

  trap_Cvar_Set(var, s);
}
/*
 ================
 G_EvalPlayerLevel
 
 evalaute player level
 ================
 */
/*int G_EvalPlayerLevel(int kills, int deaths)
{
	int i;
	int kdr;
	kdr = (kills/deaths);
	i = 0;
			if ( kills >= 100 && kills < 200 ) {
				i = 1;
			} else if ( kills >= 200 && kills < 500 ) {
				
				i = 2;
				
			} else if ( kills >= 500 && kills < 1000 ) {
				
				i = 3;
				
			} else if ( kills >= 1000 && kills < 5000 ) {
				
				i = 4;
				
			} else if ( kills >= 5000 && kills < 10000 ) {
				
				i = 5;
				
			} else if ( kills >= 10000 && kills < 17000 ) {
				
				i = 6;
				
			} else if ( kills >= 17000 && kills < 25000 ) {
				
				i = 7;
				
			} else if ( kills >= 25000 && kills < 35000 ) {
				
				i = 8;
				
			} else if ( kills >= 35000 && kills < 47000 ) {
				
				i = 9;
				
			} else if ( kills >= 47000 && kills < 62000 ) {
				
				i = 10;
				
			} else if ( kills >= 62000 && kills < 80000 && kdr > 10) {
				
				i = 11;
				
			} else if ( kills >= 80000 && kills < 100000 && kdr > 20 ) {
				
				i = 12;
				
			} else if ( kills >= 100000 && kills < 150000 && kdr > 30 ) {
				
				i = 13;
				
			} else if ( kills >= 210000 && kills < 280000 && kdr > 40 ) {
				
				i = 14;
				
			} else if ( kills >= 280000 && kills < 360000 && kdr > 50 ) {
				
				i = 15;
				
			} else if ( kills >= 360000 && kills < 450000 && kdr > 60 ) {
				
				i = 16;
				
			} else if ( kills >= 450000 && kills < 550000 && kdr > 70 ) {
				
				i = 17;
				
			} else if ( kills >= 670000 && kills < 8000000 && kdr > 80 ) {
				
				i = 18;
				
			} else if ( kills >= 800000 && kills < 10000000 && kdr > 90 ) {
				
				i = 19;
				
			} else if ( kills >= 1000000  && kdr > 100 ) {
				
				i = 20;
				
			} else {
				
				i = 0;
			}
	return i;
}*/
/*
================
G_ReadSessionData

Called on a reconnect
================
 */
void G_ReadSessionData(gclient_t *client) {
  char s[ MAX_STRING_CHARS ];
  const char *var;

  // bk001205 - format
  int teamLeader;
  int spectatorState;
  int sessionTeam;
  int restartTeam;

  var = va("session%i", client - level.clients);
  trap_Cvar_VariableStringBuffer(var, s, sizeof (s));

  // FIXME: should be using BG_ClientListParse() for ignoreList, but
  //        bg_lib.c's sscanf() currently lacks %s
  sscanf(s, "%i %i %i %i %i %i %i %i %x%x",
          &sessionTeam,
          &restartTeam,
          &client->sess.spectatorTime,
          &spectatorState,
          &client->sess.spectatorClient,
          &client->sess.wins,
          &client->sess.losses,
          &teamLeader,
          &client->sess.ignoreList.hi,
          &client->sess.ignoreList.lo
          );
  // bk001205 - format issues
  client->sess.sessionTeam = (team_t) sessionTeam;
  client->sess.restartTeam = (pTeam_t) restartTeam;
  client->sess.spectatorState = (spectatorState_t) spectatorState;
  client->sess.teamLeader = (qboolean) teamLeader;
}

/*
================
G_InitSessionData

Called on a first-time connect
================
 */
void G_InitSessionData(gclient_t *client, char *userinfo) {
  clientSession_t *sess;
  const char *value;

  sess = &client->sess;

  // initial team determination
  value = Info_ValueForKey(userinfo, "team");
  if (value[ 0 ] == 's') {
    // a willing spectator, not a waiting-in-line
    sess->sessionTeam = TEAM_SPECTATOR;
  } else {
    if (g_maxGameClients.integer > 0 &&
            level.numNonSpectatorClients >= g_maxGameClients.integer)
      sess->sessionTeam = TEAM_SPECTATOR;
    else
      sess->sessionTeam = TEAM_FREE;
  }

  sess->restartTeam = PTE_NONE;
  sess->spectatorState = SPECTATOR_FREE;
  sess->spectatorTime = level.time;
  sess->spectatorClient = -1;
  memset(&sess->ignoreList, 0, sizeof ( sess->ignoreList));

  G_WriteClientSessionData(client);
}

/*
==================
G_WriteSessionData

==================
 */
void G_WriteSessionData(void) {
  char map[ MAX_STRING_CHARS ];
  int i;
  //int record = 0;
  int gameid = 0;
	int j;
	
  char data[ 255 ];

  trap_Cvar_VariableStringBuffer("mapname", map, sizeof ( map));
  //TA: ?
  trap_Cvar_Set("session", va("%i", 0));
  
	trap_SendServerCommand( -1, "print \"^2Syncing with database\n\"" );
	for( i = 0 ; i < level.maxclients ; i++ )
	{
		if( level.clients[ i ].pers.connected == CON_CONNECTED )
		{
			G_WriteClientSessionData( &level.clients[ i ] );
			//Update mysql stuff here to.
			if(level.clients[ i ].pers.mysqlid > 1)
			{
				//level.clients[ i ].pers.playerlevel = G_EvalPlayerLevel( (level.clients[ i ].pers.statscounters.kills + level.clients[ i ].pers.totalkills), (level.clients[ i ].pers.statscounters.deaths + level.clients[ i ].pers.totaldeaths));
				if (!level.clients[ i ].pers.playerlevel)
					level.clients[ i ].pers.playerlevel= 0;
				level.clients[ i ].pers.timeplayed += (level.time - level.clients[ i ].pers.enterTime) / 60000; //Minutes played
				level.clients[ i ].pers.structsbuilt += level.clients[ i ].pers.statscounters.structsbuilt;
				//level.clients[ i ].pers.structskilled += level.clients[ i ].pers.statscounters.structskilled; don't count anything but nodes
				if(level.clients[ i ].pers.teamSelection == PTE_HUMANS)
				{
					level.clients[ i ].pers.credits+=level.clients[ i ].ps.persistant[ PERS_CREDIT ];
				}
				if(level.clients[ i ].pers.teamSelection == PTE_ALIENS)
				{
					level.clients[ i ].pers.evos = 0;
				}
				
				/* Badge related stuff */
				if(level.clients[ i ].pers.teamSelection == level.lastWin)
				{
					level.clients[ i ].pers.gameswin += 1;
				}
				/************************IF MAZE for BADGES************************************/
				//16 Zombie Bait		Die 200 times
				if ( (level.clients[ i ].pers.badges[ 16 ] != 1) && ((level.clients[ i ].pers.totaldeaths + level.clients[ i ].pers.statscounters.deaths) >= 200) )
				{
					level.clients[ i ].pers.badgeupdate[16] = 1;
					level.clients[ i ].pers.badges[16] = 1;
				}
				//17 Champion			Win 500 games	
				if ( (level.clients[ i ].pers.badges[ 17 ] != 1) && (level.clients[ i ].pers.gameswin >= 500) )
				{
					level.clients[ i ].pers.badgeupdate[17] = 1;
					level.clients[ i ].pers.badges[17] = 1;
				}
				//19 Axe Me a Question			Only get axe kills for an entire round	
				if ( (level.clients[ i ].pers.badges[ 19 ] != 1) && (level.clients[ i ].pers.statscounters.kills > 0) && (level.clients[ i ].pers.statscounters.kills == level.clients[ i ].pers.axekills) )
				{
					level.clients[ i ].pers.badgeupdate[19] = 1;
					level.clients[ i ].pers.badges[19] = 1;
				}
				//26 Eradication			Kill 200 zombie nodes	
				if ( (level.clients[ i ].pers.badges[ 26 ] != 1) && (level.clients[ i ].pers.structskilled >= 200) )
				{
					level.clients[ i ].pers.badgeupdate[26] = 1;
					level.clients[ i ].pers.badges[26] = 1;
				}
				//30 Wingman			Get 200 assists in one round	
				if ( (level.clients[ i ].pers.badges[ 30 ] != 1) && (level.clients[ i ].pers.statscounters.assists >= 200) )
				{
					level.clients[ i ].pers.badgeupdate[30] = 1;
					level.clients[ i ].pers.badges[30] = 1;
				}
				//31 Humanitarian		Make it the first 10 minutes in a survival round without killing a single zombie	
				if ( (level.clients[ i ].pers.badges[ 31 ] != 1) && (level.time >= 600000) && (level.clients[ i ].pers.statscounters.kills = 0))
				{
					level.clients[ i ].pers.badgeupdate[31] = 1;
					level.clients[ i ].pers.badges[31] = 1;
				}
				//38 Flawless			Make it an entire round without being hurt, and get at least 20 kills (so we know you aren't just hanging around base. =])
				if ((level.clients[ i ].pers.badges[ 38 ] != 1) && (level.clients[ i ].pers.statscounters.kills >= 20) && !level.clients[ i ].pers.lastDamaged) {
					level.clients[ i ].pers.badgeupdate[38] = 1;
					level.clients[ i ].pers.badges[38] = 1;
				}
				/*************************************************************/
				//Would be better if i runquery just one time instead of one per client.
				if( trap_mysql_runquery( va("UPDATE zplayers SET kills=\"%d\",deaths=\"%d\",pistolkills=\"%d\",timeplayed=\"%d\",adminlevel=\"%d\",playerlevel=\"%d\",lasttime=NOW(),gameswin=\"%d\",structsbuilt=\"%d\",structskilled=\"%d\" WHERE id=\"%d\" LIMIT 1",(level.clients[ i ].pers.statscounters.kills + level.clients[ i ].pers.totalkills), (level.clients[ i ].pers.statscounters.deaths + level.clients[ i ].pers.totaldeaths), level.clients[ i ].pers.pistolkills, level.clients[ i ].pers.timeplayed, level.clients[ i ].pers.adminlevel, level.clients[ i ].pers.playerlevel, level.clients[ i ].pers.gameswin, level.clients[ i ].pers.structsbuilt, level.clients[ i ].pers.structskilled, level.clients[ i ].pers.mysqlid ) ) == qtrue )
				{
					trap_mysql_finishquery();
					//Lets update the badges. //FIX ME: WTF LOL DOUBLE LOOPED UNECESARY
					for(j=1;j<49;j++)
					{
						if(level.clients[ i ].pers.badgeupdate[j] == 1)
						{
							if(trap_mysql_runquery( va("INSERT HIGH_PRIORITY INTO zbadges_player (idplayer,idbadge) VALUES (\"%d\",\"%d\")", level.clients[ i ].pers.mysqlid, j ) ) == qtrue)
							{
								trap_mysql_finishquery();
							}
							else
							{
								trap_mysql_finishquery();
							}
						}
					}
				}
				else
				{
					trap_mysql_finishquery();
				}
			}
		}
	}
	trap_SendServerCommand( -1, "print \"^5Data updated\n\"" );
	
  
  if(g_survival.integer && level.survivalRecordsBroke[0] > 0 && level.survivalRecordTime > 0 && !level.mysqlupdated)
  {
    if(trap_mysql_runquery(va("INSERT HIGH_PRIORITY INTO zgames (map,time) VALUES (\"%s\",\"%d\")",
      map, level.survivalRecordTime)) == qtrue)
      {
        trap_mysql_finishquery();
        if(trap_mysql_runquery("SELECT id FROM zgames ORDER BY id desc LIMIT 1") == qtrue)
        {
          if(trap_mysql_fetchrow() == qtrue)
          {
            trap_mysql_fetchfieldbyName("id", data, sizeof(data));
            gameid = atoi(data);
          }
        }
      }
      trap_mysql_finishquery();
  }
  
  //trap_SendServerCommand( -1, va("print \"^5Game id: %d\n\"",gameid) );

  for (i = 0; i < level.maxclients; i++) {
    if (level.clients[ i ].pers.connected == CON_CONNECTED)
      G_WriteClientSessionData(&level.clients[ i ]);
      if(!g_survival.integer) continue;
      if(level.clients[i].pers.mysqlid > 0 && gameid > 0 && !level.mysqlupdated)
      {
        if(trap_mysql_runquery(va("UPDATE zplayers set lasttime=NOW() WHERE id = \"%d\" LIMIT 1",
          level.clients[i].pers.mysqlid)) == qtrue)
        {
         // trap_SendServerCommand( -1, va("print \"^5Update player date %d\n\"",gameid) );
          trap_mysql_finishquery();
        }
        if(trap_mysql_runquery(va("INSERT HIGH_PRIORITY INTO zplayers_game (idgame,idplayer,timealive) VALUES (%d,%d,%d)",
        gameid, level.clients[i].pers.mysqlid, level.clients[i].pers.lastdeadtime)) == qtrue)
        {
          //trap_SendServerCommand( -1, va("print \"^5Insert players_game relation %d\n\"",gameid) );
          trap_mysql_finishquery();
        }
      }
  }
  
  if(g_survival.integer && level.survivalRecordTime > 0 && !level.mysqlupdated)
  {
    trap_SendServerCommand( -1, "print \"^5Records stored on the server.\n\"" );
  }
  level.mysqlupdated = 1;
}

void G_WinBadge( gentity_t *ent, int id )
{
	//trap_SendServerCommand( ent-g_entities, "print \"^3You have win a new Badge\ntype !badges for more information\n\"" );
	trap_SendServerCommand( ent-g_entities, "print \"^7You won the badge: \"" );
	switch(id)
	{
		case 1:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Get Your Hands Dirty] Kill your first zombie.\n\"" );
			break;
		case 2:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Pro Sniper] Get 200 headshots in a single game.\n\"" );
			break;
		case 3:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Sucks TuBe You] Get 12 kills in a single normal grenade launcher shot.\n\"" );
			break;
		case 4:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Bloodbath] Get 1000 kills in a single survival round.\n\"" );
			break;
		case 5:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Laser Sight] Get 300 kills in a row with the laser gun.\n\"" );
			break;
		case 6:
			trap_SendServerCommand( ent-g_entities, "print \"^3[First Blood] Get the first kill in a survival round.\n\"" );
			break;
		case 7:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Jump Shot] Kill an enemy from the sky, without using any explosives or the lasergun.\n\"" );
			break;
		case 8:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Trap Shoot] Kill an enemy with a shotgun while they are in the air.\n\"" );
			break;
		case 9:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Holey Zombies] Fill 50 zombies with holes from a single chaingun clip.\n\"" );
			break;
		case 10:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Public Enemy No.1] Get 100 Tommy Gun kills in one round.\n\"" );
			break;
		case 11:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Gunslinger] Get 1,000 pistol kills.\n\"" );
			break;
		case 12:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Survivor] Get 1,000 kills.\n\"" );
			break;
		case 13:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Killer] Get 10,000 kills.\n\"" );
			break;
		case 14:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Deadly] Get 100,000 kills.\n\"" );
			break;
		case 15:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Exterminator] Get 1,000,000 kills.\n\"" );
			break;
		case 16:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Zombie Bait] Die 200 times.\n\"" );
			break;
		case 17:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Champion] Win 500 rounds.\n\"" );
			break;
		case 18:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Seen the Light] Go down to 1 hp then recover to at least 75.\n\"" );
			break;
		case 19:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Axe Me a Question] Only get axe kills for an entire round.\n\"" );
			break;
		case 20:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Multikill] Get 5 Mass Driver kills in one shot.\n\"" );
			break;
		case 21:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Rocket Launched] Knock 5 enemies off the ground, killing them, with the rocket launcher.\n\"" );
			break;
		case 22:
			trap_SendServerCommand( ent-g_entities, "print \"^3[It Went Boom!] Kill 10 enemies with one grenade.\n\"" );
			break;
		case 23:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Minefield] Kill 50 enemies with mines in one round.\n\"" );
			break;
		case 24:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Burn Baby Burn!] Light 7 zombies on fire with one incendiary round.\n\"" );
			break;
		case 25:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Mr. Streaky] Kill at least one zombie every 2 seconds for 2 minutes.\n\"" );
			break;
		case 26:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Eradication] Kill 200 zombie nodes.\n\"" );
			break;
		case 27:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Ultimate Sacrifice] Deploy a medical dome that heals a teammate, dying in the process.\n\"" );
			break;
		case 28:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Survivalist] Live for 30minutes in a survival round.\n\"" );
			break;
		case 29:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Hands Tied] Make a zombie commit suicide.\n\"" );
			break;
		case 30:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Wingman] Get 200 assists in one round.\n\"" );
			break;
		case 31:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Humanitarian] Survive the first 10minutes of a survival round without killing a zombie.\n\"" );
			break;
		case 32:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Gibbed] Kill 20+ zombies at once with any weapon.\n\"" );
			break;
		case 33:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Tastes Like Chicken] Eat a human.\n\"" );
			break;
		case 34:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Fore!] Kill 6 zombies with a thrown axe.\n\"" );
			break;
		case 35:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Last Man Standing] Make it to 20 minutes and be the last man alive.\n\"" );
			break;
		case 36:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Guns Blazing] Kill 100 zombies in a row with 20HP or less.\n\"" );
			break;
		case 37:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Ninja] In survival, don't get touched by a zombie for 5minutes.\n\"" );
			break;
		case 38:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Flawless] Go an entire round without getting hurt, and get at least 20 kills.\n\"" );
			break;
		case 39:
			trap_SendServerCommand( ent-g_entities, "print \"^3[Medic] Use 6 medikits on teammates, either domes or heals, in a single round.\n\"" );
			break;
	}
}

