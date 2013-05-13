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
//#include "acebot.h"

damageRegion_t g_damageRegions[PCL_NUM_CLASSES][MAX_LOCDAMAGE_REGIONS];
int g_numDamageRegions[PCL_NUM_CLASSES];

armourRegion_t g_armourRegions[UP_NUM_UPGRADES][MAX_ARMOUR_REGIONS];
int g_numArmourRegions[UP_NUM_UPGRADES];

/*
 ============
 AddScore

 Adds score to both the client and his team
 ============
 */
void
AddScore(gentity_t *ent, int score)
{
  if (!ent->client)
    return;
  if (ent->r.svFlags & SVF_BOT)
    return;

  ent->client->ps.persistant[PERS_SCORE] += score;
  CalculateRanks();
}

/*
 ==================
 LookAtKiller
 ==================
 */
void
LookAtKiller(gentity_t *self, gentity_t *inflictor, gentity_t *attacker)
{
  vec3_t dir;

  if (attacker && attacker != self)
    VectorSubtract(attacker->s.pos.trBase, self->s.pos.trBase, dir);
  else if (inflictor && inflictor != self)
    VectorSubtract(inflictor->s.pos.trBase, self->s.pos.trBase, dir);
  else
  {
    self->client->ps.stats[STAT_VIEWLOCK] = self->s.angles[YAW];
    return;
  }

  self->client->ps.stats[STAT_VIEWLOCK] = vectoyaw(dir);
}

// these are just for logging, the client prints its own messages
char *modNames[] =
{ "MOD_UNKNOWN", "MOD_SHOTGUN", "MOD_PAINSAW", "MOD_MACHINEGUN", "MOD_CHAINGUN",
    "MOD_PRIFLE", "MOD_MDRIVER", "MOD_LASGUN", "MOD_LCANNON", "MOD_LCANNON_SPLASH", "MOD_FLAMER",
    "MOD_FLAMER_SPLASH", "MOD_GRENADE", "MOD_WATER", "MOD_SLIME", "MOD_LAVA", "MOD_CRUSH",
    "MOD_TELEFRAG", "MOD_FALLING", "MOD_SUICIDE", "MOD_TARGET_LASER", "MOD_TRIGGER_HURT",

    "MOD_ABUILDER_CLAW", "MOD_LEVEL0_BITE", "MOD_LEVEL1_CLAW", "MOD_LEVEL1_PCLOUD",
    "MOD_LEVEL3_CLAW", "MOD_LEVEL3_POUNCE", "MOD_LEVEL3_BOUNCEBALL", "MOD_LEVEL2_CLAW",
    "MOD_LEVEL2_ZAP", "MOD_LEVEL4_CLAW", "MOD_LEVEL4_CHARGE",

    "MOD_SLOWBLOB", "MOD_POISON", "MOD_SWARM",

    "MOD_HSPAWN", "MOD_TESLAGEN", "MOD_MGTURRET", "MOD_REACTOR",

    "MOD_ASPAWN", "MOD_ATUBE", "MOD_OVERMIND" };

/*
 ==================
 player_die
 ==================
 */
void
player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath)
{
  gentity_t *ent;
  int anim;
  int killer;
  int i, j;
	int k, l;
  char *killerName, *obit;
  float totalDamage = 0.0f;
  float percentDamage = 0.0f;
  gentity_t *player;
  qboolean tk = qfalse;
  qboolean aliensuicide = qfalse;
  qboolean survivalkill = qfalse;

	 /************************IF MAZE for BADGES************************************/
	if(attacker->client && self->client)
	{
		if(attacker->client->pers.mysqlid > 1)
		{
			//1  	Get Your Hands Dirty 	Get first kill
			if(attacker->client->pers.badges[ 1 ] != 1)
			{
				G_WinBadge( attacker, 1 );
				attacker->client->pers.badgeupdate[1] = 1;
				attacker->client->pers.badges[1] = 1;
			}
			//3		Sucks TuBe You	Get 12 kills in one grenade launcher shot
			if((attacker->client->pers.badges[ 3 ] != 1) && (attacker->comboKills >= 11) && (attacker->comboMod == MOD_GRENADE_LAUNCHER ) )
			{
				G_WinBadge( attacker, 3 );
				attacker->client->pers.badgeupdate[3] = 1;
				attacker->client->pers.badges[3] = 1;
			}
			//5		Laser Sight		Get 300 lasergun kills in a row
			if( (attacker->client->pers.badges[ 5 ] != 1) && (meansOfDeath == MOD_LASERGUN) )
			{
				if (attacker->client->pers.laserkills >= 300)
				{
					G_WinBadge( attacker, 5 );
					attacker->client->pers.badgeupdate[5] = 1;
					attacker->client->pers.badges[5] = 1;
				} else {
					attacker->client->pers.laserkills++;
				}
			} else if (meansOfDeath != MOD_LASERGUN)
			{
				attacker->client->pers.laserkills = 0;
			}
			//6		First Blood		Kill the first enemy in a survival round.
			if(g_survival.integer && (attacker->client->pers.badges[ 6 ] != 1) && (g_humanKills.integer == 0) )
			{
				G_WinBadge( attacker, 6 );
				attacker->client->pers.badgeupdate[6] = 1;
				attacker->client->pers.badges[6] = 1;
			}
			//7		Jump Shot	Kill an enemy while in the air. No explosive weapons or laser gun allowed.
			if( (attacker->client->pers.badges[ 7 ] != 1) && (attacker->s.groundEntityNum == ENTITYNUM_NONE) && (meansOfDeath != MOD_GRENADE_LAUNCHER)
			   && (meansOfDeath != MOD_GRENADE_LAUNCHER_INCENDIARY) && (meansOfDeath != MOD_ROCKET_LAUNCHER) && (meansOfDeath != MOD_GRENADE)
			   && (meansOfDeath != MOD_LASERGUN) && (meansOfDeath != MOD_MINE) )
			{
				G_WinBadge( attacker, 7 );
				attacker->client->pers.badgeupdate[7] = 1;
				attacker->client->pers.badges[7] = 1;
			}
			//8		Trap Shoot	Kill an enemy with the shotgun while they are in the air.
			if( (attacker->client->pers.badges[ 8 ] != 1) && (self->s.groundEntityNum == ENTITYNUM_NONE) && (meansOfDeath == MOD_SHOTGUN) )
			{
				G_WinBadge( attacker, 8 );
				attacker->client->pers.badgeupdate[8] = 1;
				attacker->client->pers.badges[8] = 1;
			}
			//9		Holey Zombies	Kill 50 zombies with a single chaingun clip
			if ( (attacker->client->pers.badges[ 9 ] != 1) && (attacker->client->pers.chaingunkills >= 50) )
			{
				G_WinBadge( attacker, 9 );
				attacker->client->pers.badgeupdate[9] = 1;
				attacker->client->pers.badges[9] = 1;
			} else if ((attacker->client->pers.badges[ 9 ] != 1) && (meansOfDeath == MOD_CHAINGUN))
			{
				if(attacker->client->ps.ammo <= attacker->client->pers.chaingunlastkillammo )
				{
					attacker->client->pers.chaingunlastkillammo = attacker->client->ps.ammo;
					attacker->client->pers.chaingunkills++;
				} else {
					attacker->client->pers.chaingunkills = 0;
					attacker->client->pers.chaingunlastkillammo = attacker->client->ps.ammo;
				}
				
			}
			//10 Public Enemy No. 1		Get 100 thompson kills in one round
			if ( (attacker->client->pers.badges[ 10 ] != 1) && (attacker->client->pers.tommykills >= 100) )
			{
				G_WinBadge( attacker, 10 );
				attacker->client->pers.badgeupdate[10] = 1;
				attacker->client->pers.badges[10] = 1;
			} else if ((attacker->client->pers.badges[ 10 ] != 1) && (meansOfDeath == MOD_MACHINEGUN))
			{
				attacker->client->pers.tommykills++;
			}
			//11 Gunslinger		Get 1,000 pistol kills
			if ( (attacker->client->pers.badges[ 11 ] != 1) && (attacker->client->pers.pistolkills >=1000) )
			{
				G_WinBadge( attacker, 11 );
				attacker->client->pers.badgeupdate[11] = 1;
				attacker->client->pers.badges[11] = 1;
			} else if ((attacker->client->pers.badges[ 11 ] != 1) && (meansOfDeath == MOD_PISTOL))
			{
				attacker->client->pers.pistolkills++;
			}
			//12 Survivor		Get 1,000 Kills
			if ( (attacker->client->pers.badges[ 12 ] != 1) && ((attacker->client->pers.totalkills + attacker->client->pers.statscounters.kills) >= 1000) )
			{
				G_WinBadge( attacker, 12 );
				attacker->client->pers.badgeupdate[12] = 1;
				attacker->client->pers.badges[12] = 1;
			}
			//13 Killer		Get 10,000 Kills
			if ( (attacker->client->pers.badges[ 13 ] != 1) && ((attacker->client->pers.totalkills + attacker->client->pers.statscounters.kills) >= 10000) )
			{
				G_WinBadge( attacker, 13 );
				attacker->client->pers.badgeupdate[13] = 1;
				attacker->client->pers.badges[13] = 1;
			}
			//14 Deadly		Get 100,000 Kills
			if ( (attacker->client->pers.badges[ 14 ] != 1) && ((attacker->client->pers.totalkills + attacker->client->pers.statscounters.kills) >= 100000) )
			{
				G_WinBadge( attacker, 14 );
				attacker->client->pers.badgeupdate[14] = 1;
				attacker->client->pers.badges[14] = 1;
			}
			//15 Exterminator		Get 1,000,000 Kills
			if ( (attacker->client->pers.badges[ 15 ] != 1) && ((attacker->client->pers.totalkills + attacker->client->pers.statscounters.kills) >= 1000000) )
			{
				G_WinBadge( attacker, 15 );
				attacker->client->pers.badgeupdate[15] = 1;
				attacker->client->pers.badges[15] = 1;
			}
			//19 Axe Me a Question		Only get axe kills for an entire round
			if ((attacker->client->pers.badges[ 19 ] != 1) && (meansOfDeath == MOD_AXE))
			{
				attacker->client->pers.axekills++;
			}
			//20 Multikill				Get 5 mass driver kills in 1 shot
			if((attacker->client->pers.badges[ 20 ] != 1) && (attacker->comboKills >= 4) && (attacker->comboMod == MOD_MDRIVER ) )
			{
				G_WinBadge( attacker, 20 );
				attacker->client->pers.badgeupdate[20] = 1;
				attacker->client->pers.badges[20] = 1;
			}
			//21 Rocket Launched				Knock 5 enemies off the ground, killing them, with the rocket launcher within one round.
			if((attacker->client->pers.badges[ 21 ] != 1) && (attacker->client->pers.launcherkills >= 5 ))
			{
				G_WinBadge( attacker, 21 );
				attacker->client->pers.badgeupdate[21] = 1;
				attacker->client->pers.badges[21] = 1;
			} else if ((attacker->client->pers.badges[ 21 ] != 1) && (self->s.groundEntityNum == ENTITYNUM_NONE) && (meansOfDeath == MOD_ROCKET_LAUNCHER ))
			{
				attacker->client->pers.launcherkills++;
			}
			//22 It went boom!	Get 10 kills in one grenade 
			if((attacker->client->pers.badges[ 22 ] != 1) && (attacker->comboKills >= 9) && (attacker->comboMod == MOD_GRENADE) )
			{
			    G_WinBadge( attacker, 22 );
			    attacker->client->pers.badgeupdate[22] = 1;
			    attacker->client->pers.badges[22] = 1;
			}
			   //23 Minefield		Kill 50 enemies with mines in one round
			   if ( (attacker->client->pers.badges[ 23 ] != 1) && (attacker->client->pers.minekills >=50) )
			   {
				   G_WinBadge( attacker, 23 );
				   attacker->client->pers.badgeupdate[23] = 1;
				   attacker->client->pers.badges[23] = 1;
			   } else if ((attacker->client->pers.badges[ 23 ] != 1) && (meansOfDeath == MOD_MINE))
			   {
				   attacker->client->pers.minekills++;
			   }
			   //25 Mr. Streaky		Get one kill every 2 seconds for 2 minutes
				   if (attacker->client->pers.badges[ 25 ] != 1)
				   {
					   if( !((level.time - attacker->client->lastKillTime) <= 2000))
					   {
						   attacker->client->pers.streakstart = level.time;
					   } else if((level.time - attacker->client->pers.streakstart) >=120000)
					   {
						   G_WinBadge( attacker, 25 );
						   attacker->client->pers.badgeupdate[25] = 1;
						   attacker->client->pers.badges[25] = 1;
					   }
				   }
			   
			//31 Humanitarian	Make it the first 10 minutes in a survival round without killing a single zombie
			if((attacker->client->pers.badges[ 31 ] != 1) && (level.time >= 600000) && (attacker->client->pers.statscounters.kills = 0) )
			{
				G_WinBadge( attacker, 31 );
				attacker->client->pers.badgeupdate[31] = 1;
				attacker->client->pers.badges[31] = 1;
			}
			//32 Gibbed		Get a 20+ multikill
			if((attacker->client->pers.badges[ 32 ] != 1) && (attacker->comboKills >= 19) )
			{
			    G_WinBadge( attacker, 32 );
			    attacker->client->pers.badgeupdate[32] = 1;
			    attacker->client->pers.badges[32] = 1;
			}
			//33 Tastes Like Chicken		Eat a human
			if((attacker->client->pers.badges[ 33 ] != 1) && (attacker->client->pers.teamSelection == PTE_ALIENS) )
			{
			    G_WinBadge( attacker, 33 );
			    attacker->client->pers.badgeupdate[33] = 1;
			    attacker->client->pers.badges[33] = 1;
			}
			//34 Fore!		Get a 6 multikill with the axe throw
			if((attacker->client->pers.badges[ 34 ] != 1) && (meansOfDeath == MOD_AXE) )
			{
				attacker->client->pers.axethrowkills++;
				if((level.time - attacker->client->pers.lastaxethrowkill) >= 599)
				{
					attacker->client->pers.lastaxethrowkill = level.time;
					attacker->client->pers.axethrowkills = 1;
				} else if(attacker->client->pers.axethrowkills >= 6)
				{
				G_WinBadge( attacker, 34 );
			    attacker->client->pers.badgeupdate[34] = 1;
			    attacker->client->pers.badges[34] = 1;
				}
				attacker->client->pers.lastaxethrowkill = level.time;
			}
			//36 Last Stand			Kill 100 zombies with 20 or less health without healing
			if((attacker->client->pers.badges[ 36 ] != 1) && g_survival.integer && (attacker->client->ps.stats[STAT_HEALTH] <=20))
			{
				attacker->client->pers.hurtkills++;
				if(attacker->client->pers.hurtkills >= 100)
				{
					G_WinBadge( attacker, 36 );
					attacker->client->pers.badgeupdate[36] = 1;
					attacker->client->pers.badges[36] = 1;
				}
			} else {
				attacker->client->pers.hurtkills = 0;
			}
		}
		if(self->client->pers.mysqlid > 1)
		{
			//27 Ultimate Sacrifice		Deploy a dome for a teammate, dying in the process.
			if((self->client->pers.badges[ 27 ] != 1) && g_survival.integer && ((level.time - self->client->pers.deploytime) <= 2000 ) )
			{
				l = 0;
				for( k = 0 ; k < level.maxclients ; k++ )
				{
					if((level.clients[ k ].pers.teamSelection == PTE_HUMANS) && (level.clients[ k ].ps.stats[STAT_HEALTH] > 0))
					{
						l = 1;
						break;
					}
				}
				if(l)
				{
					G_WinBadge( self, 27 );
					self->client->pers.badgeupdate[27] = 1;
					self->client->pers.badges[27] = 1;
				}
			}
			//28 Survivalist Live for 30min in a survival round
			if((self->client->pers.badges[ 28 ] != 1) && g_survival.integer && (level.time >= 1800000))
			{
				G_WinBadge( self, 28 );
				self->client->pers.badgeupdate[28] = 1;
				self->client->pers.badges[28] = 1;
			}
			//35 Last man standing		Make it to 20 minutes in survival and be the last man alive.
			if((self->client->pers.badges[ 35 ] != 1) && g_survival.integer && (level.time >= 1200000))
			{
				l = 0;
				for( k = 0 ; k < level.maxclients ; k++ )
				{
					if((level.clients[ k ].pers.teamSelection == PTE_HUMANS) && (level.clients[ k ].ps.stats[STAT_HEALTH] > 0))
					{
						l = 1;
						break;
					}
				}
				if(!l)
				{
					G_WinBadge( self, 35 );
					self->client->pers.badgeupdate[35] = 1;
					self->client->pers.badges[35] = 1;
				}
			}
		}
	}
	

	//29 Hands tied		Make a zombie commit suicide
	if(self->client->pers.lastattacker->client && (self->client->pers.lastattacker->client->pers.mysqlid > 1) && (self->client->pers.lastattacker->client->pers.badges[ 29 ] != 1) && ((meansOfDeath == MOD_WATER) || (meansOfDeath == MOD_SLIME) || (meansOfDeath == MOD_LAVA) || (meansOfDeath == MOD_CRUSH) 
		|| (meansOfDeath == MOD_FALLING) || (meansOfDeath == MOD_SUICIDE) 
		|| (meansOfDeath == MOD_TARGET_LASER) || (meansOfDeath == MOD_TRIGGER_HURT)) && ((level.time - self->lastDamageTime) <= 2000))
	{
		self->client->pers.lastattacker->client->pers.badgeupdate[29] = 1;
		self->client->pers.lastattacker->client->pers.badges[29] = 1;
		G_WinBadge( self->client->pers.lastattacker, 29 );
	}
			
	
	
  if (self->client->ps.pm_type == PM_DEAD)
    return;

  if (level.intermissiontime)
    return;

  //Some how this loop... Fixed on g_damage( Remove this comment if tests go well.

  if (self->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)
  {
    if(g_survival.integer && attacker && attacker->client && attacker->client->ps.stats[STAT_PTEAM] == PTE_HUMANS)
    {
      if(level.time > level.levelUpTime + LEVEL_PAUSE)
      {
        level.levelKills++;
      }
    }
//    self->botnextpath = 0;
//    self->botlostpath = qtrue;
//    //trap_SendServerCommand( -1, "print \"shit\n\"");
//    VectorCopy(self->s.origin2, dir);
//    //G_AddEvent( self, EV_HUMAN_BUILDABLE_EXPLOSION, DirToByte( dir ) );
//    tent = G_TempEntity(self->s.origin, EV_ALIEN_BUILDABLE_EXPLOSION);
//    tent->s.eventParm = DirToByte(dir);
//    tent->s.otherEntityNum = self->s.number;
   
	  //zombie bomb was here
  }
  if (g_survival.integer)
  {
    if (self && self->client)
    {
      if (self->r.svFlags & SVF_BOT)
      {
        self->botCommand = BOT_REGULAR;
        //        self->botEnemy = NULL;
      }
      self->client->pers.lastdeadtime = level.time - level.startTime;
    }
  }
	//G_SelectiveRadiusDamage( self->s.pos.trBase, self, (float)100, (float)500, self, MOD_SLOWBLOB, self->client->ps.stats[ STAT_PTEAM ] );
    if (g_survival.integer)
    {
		//G_RadiusDamage( self->s.pos.trBase, self, (float)100, (float)50, self, MOD_SLOWBLOB);
    }
    else
	{
		if (self && self->client)
		{
			if (self->r.svFlags & SVF_BOT)
			{
		G_RadiusDamage(self->s.pos.trBase, self, (float) 75, (float) 250, self, MOD_SLOWBLOB);
			}
		}
	}

  self->client->ps.pm_type = PM_DEAD;
  self->suicideTime = 0;

  if (attacker)
  {
    killer = attacker->s.number;

    if (attacker->client)
    {
      killerName = attacker->client->pers.netname;
      tk = (attacker != self && attacker->client->ps.stats[STAT_PTEAM]
          == self->client->ps.stats[STAT_PTEAM]);

      if (attacker != self && attacker->client->ps.stats[STAT_PTEAM]
          == self->client->ps.stats[STAT_PTEAM])
      {
        attacker->client->pers.statscounters.teamkills++;
        if (attacker->client->pers.teamSelection == PTE_ALIENS)
        {
          level.alienStatsCounters.teamkills++;
        }
        else if (attacker->client->pers.teamSelection == PTE_HUMANS)
        {
          level.humanStatsCounters.teamkills++;
        }
      }

    }
    else
      killerName = "<non-client>";
  }
  else
  {
    killer = ENTITYNUM_WORLD;
    killerName = "<world>";
  }

  if (killer < 0 || killer >= MAX_CLIENTS)
  {
    killer = ENTITYNUM_WORLD;
    killerName = "<world>";
  }

  if (meansOfDeath < 0 || meansOfDeath >= sizeof(modNames) / sizeof(modNames[0]))
    obit = "<bad obituary>";
  else
    obit = modNames[meansOfDeath];

  G_LogPrintf(
    "Kill: %i %i %i: %s^7 killed %s^7 by %s\n", killer, self->s.number, meansOfDeath, killerName,
    self->client->pers.netname, obit);

  //TA: close any menus the client has open
  G_CloseMenus(self->client->ps.clientNum);

  //TA: deactivate all upgrades
  for(i = UP_NONE + 1;i < UP_NUM_UPGRADES;i++)
    BG_DeactivateUpgrade(i, self->client->ps.stats);

  if (self->client)
  {
    aliensuicide = self->client->ps.stats[STAT_PTEAM] == PTE_ALIENS && meansOfDeath == MOD_SUICIDE;
    survivalkill = self->client->ps.stats[STAT_PTEAM] == PTE_ALIENS && g_survival.integer;
  }
  // broadcast the death event to everyone
  if (aliensuicide)// || survivalkill)
  {
    //Dont do shit.
  }
  else if (!tk)
  {
    if(self && self->client && self->client->ps.stats[STAT_PTEAM] == PTE_ALIENS
        && attacker && attacker->client && attacker->client->ps.stats[STAT_PTEAM] == PTE_HUMANS)
    {

      if(attacker->comboKills > 0 && attacker->comboMod != meansOfDeath)
      {
        if(attacker->comboKills > 1)
        {
          trap_SendServerCommand(-1, va("print \"[x^%d%d ^7%s] %s\n\"", attacker->comboKills%10, attacker->comboKills, modString(attacker->comboMod),
            attacker->client->pers.netname));
        }
        attacker->comboKills = 1;
        attacker->comboMod = meansOfDeath;
      }
      else
      {
        attacker->comboKills++;
		attacker->comboMod = meansOfDeath;
      }
    }
    else if(!(self->client->ps.stats[STAT_PTEAM] == PTE_ALIENS && meansOfDeath == MOD_UNKNOWN))
    {
      ent = G_TempEntity(self->r.currentOrigin, EV_OBITUARY);
      ent->s.eventParm = meansOfDeath;
      ent->s.otherEntityNum = self->s.number;
      ent->s.otherEntityNum2 = killer;
      ent->r.svFlags = SVF_BROADCAST; // send to everyone
    }
  }
  else if (attacker && attacker->client)
  {
    // tjw: obviously this is a hack and belongs in the client, but
    //      this works as a temporary fix.
    if (attacker->client->ps.stats[STAT_PTEAM] == self->client->ps.stats[STAT_PTEAM])
    {
      trap_SendServerCommand(-1, va(
        "print \"%s^7 was killed by ^1TEAMMATE^7 %s^7 (Did %d damage to %d max)\n\"",
        self->client->pers.netname, attacker->client->pers.netname,
        self->client->tkcredits[attacker->s.number], self->client->ps.stats[STAT_MAX_HEALTH]));
      trap_SendServerCommand(attacker - g_entities, va(
        "cp \"You killed ^1TEAMMATE^7 %s\"", self->client->pers.netname));
      G_LogOnlyPrintf(
        "%s^7 was killed by ^1TEAMMATE^7 %s^7 (Did %d damage to %d max)\n",
        self->client->pers.netname, attacker->client->pers.netname,
        self->client->tkcredits[attacker->s.number], self->client->ps.stats[STAT_MAX_HEALTH]);
    }
  }
  self->enemy = attacker;

  //self->client->ps.persistant[PERS_KILLED]++;
  //self->client->pers.statscounters.deaths++; move it down to humans, so zombie deaths don't count against player
  if (self->client->pers.teamSelection == PTE_ALIENS)
  {
    level.alienStatsCounters.deaths++;
  }
  else if (self->client->pers.teamSelection == PTE_HUMANS)
  {
	self->client->ps.persistant[PERS_KILLED]++;
	  self->client->pers.statscounters.deaths++;
    level.humanStatsCounters.deaths++;
  }

  if (attacker && attacker->client)
  {
    attacker->client->lastkilled_client = self->s.number;

    if (g_devmapKillerHP.integer && g_cheats.integer)
    {
      trap_SendServerCommand(self - g_entities, va(
        "print \"Your killer, %s, had %3i HP.\n\"", killerName, attacker->health));
    }

    if (attacker == self || OnSameTeam(self, attacker))
    {
      AddScore(attacker, -1);

      // Retribution: transfer value of player from attacker to victim
      if (g_retribution.integer)
      {
        if (attacker != self)
        {
          int max = ALIEN_MAX_KILLS, tk_value = 0;
          char *type = "evos";

          if (attacker->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)
          {
            tk_value = BG_ClassCanEvolveFromTo(
              PCL_ALIEN_LEVEL0, self->client->ps.stats[STAT_PCLASS], ALIEN_MAX_KILLS, 0);
          }
          else
          {
            tk_value = BG_GetValueOfEquipment(&self->client->ps);
            max = HUMAN_MAX_CREDITS;
            type = "credits";
          }

          if (attacker->client->ps.persistant[PERS_CREDIT] < tk_value)
            tk_value = attacker->client->ps.persistant[PERS_CREDIT];
          if (self->client->ps.persistant[PERS_CREDIT] + tk_value > max)
            tk_value = max - self->client->ps.persistant[PERS_CREDIT];

          if (tk_value > 0)
          {

            // adjust using the retribution cvar (in percent)
            tk_value = tk_value * g_retribution.integer / 100;

            G_AddCreditToClient(self->client, tk_value, qtrue);
            G_AddCreditToClient(attacker->client, -tk_value, qtrue);

            trap_SendServerCommand(self->client->ps.clientNum, va(
              "print \"Received ^3%d %s ^7from %s ^7in retribution.\n\"", tk_value, type,
              attacker->client->pers.netname));
            trap_SendServerCommand(attacker->client->ps.clientNum, va(
              "print \"Transfered ^3%d %s ^7to %s ^7in retribution.\n\"", tk_value, type,
              self->client->pers.netname));
          }
        }
      }
      // Normal teamkill penalty
      else
      {
        if (attacker->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)
          G_AddCreditToClient(attacker->client, -FREEKILL_ALIEN, qtrue);
        else if (attacker->client->ps.stats[STAT_PTEAM] == PTE_HUMANS)
          G_AddCreditToClient(attacker->client, -FREEKILL_HUMAN, qtrue);
      }
    }
    else
    {
      AddScore(attacker, 1);
	
	if ( (attacker->client->pers.badges[ 4 ] != 1) && (attacker->client->pers.statscounters.kills >= 1000) && g_survival.integer)	
	{
		G_WinBadge( attacker, 4 );
		attacker->client->pers.badgeupdate[4] = 1;
		attacker->client->pers.badges[4] = 1;
	}
      attacker->client->lastKillTime = level.time;

      if (self && self->client)
      {
        self->client->lastdietime = level.time;
      }

      attacker->client->pers.statscounters.kills++;
      if (attacker->client->pers.teamSelection == PTE_ALIENS)
      {
        level.alienStatsCounters.kills++;
      }
      else if (attacker->client->pers.teamSelection == PTE_HUMANS)
      {
        level.humanStatsCounters.kills++;
      }
    }
    if (attacker == self)
    {
      attacker->client->pers.statscounters.suicides++;
      if (attacker->client->pers.teamSelection == PTE_ALIENS)
      {
        level.alienStatsCounters.suicides++;
      }
      else if (attacker->client->pers.teamSelection == PTE_HUMANS)
      {
        level.humanStatsCounters.suicides++;
      }
    }
  }

  else if (attacker->s.eType != ET_BUILDABLE)
    AddScore(self, -1);
  else if (attacker->s.eType == ET_BUILDABLE && attacker->builder && attacker->builder->client)
  {
    G_AddCreditToClient(attacker->builder->client, 250, qtrue);
  }

  //total up all the damage done by every client
  for(i = 0;i < MAX_CLIENTS;i++)
    totalDamage += (float) self->credits[i];

  // if players did more than DAMAGE_FRACTION_FOR_KILL increment the stage counters
  if (!OnSameTeam(self, attacker) && totalDamage >= (self->client->ps.stats[STAT_MAX_HEALTH]
      * DAMAGE_FRACTION_FOR_KILL))
  {
    if (self->client->ps.stats[STAT_PTEAM] == PTE_HUMANS)
    {
      trap_Cvar_Set("g_alienKills", va("%d", g_alienKills.integer + 1));
      trap_Cvar_Set("g_humanKills", va("%d", g_humanKills.integer + 1));
      if (g_alienStage.integer < 2)
      {
        self->client->pers.statscounters.feeds++;
        level.humanStatsCounters.feeds++;
      }
    }
    else if (self->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)
    {
      trap_Cvar_Set("g_humanKills", va("%d", g_humanKills.integer + 1));
      trap_Cvar_Set("g_alienKills", va("%d", g_alienKills.integer + 1));
      if (g_humanStage.integer < 2)
      {
        self->client->pers.statscounters.feeds++;
        level.alienStatsCounters.feeds++;
      }
    }
  }
  if (totalDamage > 0.0f)
  {
    /* if( self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS )
     {
     //nice simple happy bouncy human land
     float classValue = BG_FindValueOfClass( self->client->ps.stats[ STAT_PCLASS ] );

     for( i = 0; i < MAX_CLIENTS; i++ )
     {
     player = g_entities + i;

     if( !player->client )
     continue;

     if( player->client->ps.stats[ STAT_PTEAM ] != PTE_HUMANS )
     continue;

     if( !self->credits[ i ] )
     continue;

     percentDamage = (float)self->credits[ i ] / totalDamage;
     if( percentDamage > 0 && percentDamage < 1)
     {
     player->client->pers.statscounters.assists++;
     level.humanStatsCounters.assists++;
     }

     //add credit
     G_AddCreditToClient( player->client,
     (int)( classValue * percentDamage ), qtrue );
     }
     }
     else*/if (self->client->ps.stats[STAT_PTEAM] == PTE_HUMANS || self->client->ps.stats[STAT_PTEAM]
        == PTE_ALIENS)
    {
      //horribly complex nasty alien land
      float humanValue = 120.0f; //BG_GetValueOfHuman( &self->client->ps );
      int frags;
      int unclaimedFrags = (int) humanValue;

      for(i = 0;i < MAX_CLIENTS;i++)
      {
        player = g_entities + i;

        if (!player->client)
          continue;

        /*if( player->client->ps.stats[ STAT_PTEAM ] != PTE_ALIENS )
         continue;*/

        //this client did no damage
        if (!self->credits[i])
          continue;

        //nothing left to claim
        if (!unclaimedFrags)
          break;

        percentDamage = (float) self->credits[i] / totalDamage;
        if (percentDamage > 0 && percentDamage < 1)
        {
          player->client->pers.statscounters.assists++;
          level.alienStatsCounters.assists++;
        }

        frags = (int) floor(humanValue * percentDamage);

        if (frags > 0)
        {
          //add kills
          G_AddCreditToClient(player->client, frags, qtrue);

          //can't revist this account later
          self->credits[i] = 0;

          //reduce frags left to be claimed
          unclaimedFrags -= frags;
        }
      }

      //there are frags still to be claimed
      if (unclaimedFrags)
      {
        //the clients remaining at this point do not
        //have enough credit to claim even one frag
        //so simply give the top <unclaimedFrags> clients
        //a frag each

        for(i = 0;i < unclaimedFrags;i++)
        {
          int maximum = 0;
          int topClient = 0;

          for(j = 0;j < MAX_CLIENTS;j++)
          {
            //this client did no damage
            if (!self->credits[j])
              continue;

            if (self->credits[j] > maximum)
            {
              maximum = self->credits[j];
              topClient = j;
            }
          }

          if (maximum > 0)
          {
            player = g_entities + topClient;

            //add kills
            G_AddCreditToClient(player->client, 1, qtrue);

            //can't revist this account again
            self->credits[topClient] = 0;
          }
        }
      }
    }
  }

  if (!(self->r.svFlags & SVF_BOT))
  {
    ScoreboardMessage(self); // show scores
  }
  // send updated scores to any clients that are following this one,
  // or they would get stale scoreboards
  for(i = level.botslots;i < level.botslots + level.numConnectedClients;i++)
  {
    gclient_t *client;

    client = &level.clients[i];
    if (client->pers.connected != CON_CONNECTED)
      continue;

    if (client->sess.sessionTeam != TEAM_SPECTATOR)
      continue;

    if (client->sess.spectatorClient == self->s.number)
      ScoreboardMessage(g_entities + i);
  }

  VectorCopy(self->s.origin, self->client->pers.lastDeathLocation);

  self->takedamage = qfalse; // can still be gibbed

  self->s.weapon = WP_NONE;
  self->r.contents = CONTENTS_CORPSE;

  self->s.angles[PITCH] = 0;
  self->s.angles[ROLL] = 0;
  self->s.angles[YAW] = self->s.apos.trBase[YAW];

  //FIXME: THIS OVERFLOW SCORES
  //LookAtKiller(self, inflictor, attacker);

  VectorCopy(self->s.angles, self->client->ps.viewangles);

  self->s.loopSound = 0;

  self->r.maxs[2] = -8;

  // don't allow respawn until the death anim is done
  // g_forcerespawn may force spawning at some later time
  self->client->respawnTime = level.time + 1700;

  // remove powerups
  memset(self->client->ps.powerups, 0, sizeof(self->client->ps.powerups));

  {
    // normal death
    static int i;

    if (!(self->client->ps.persistant[PERS_STATE] & PS_NONSEGMODEL))
    {
      switch(i)
      {
        case 0:
          anim = BOTH_DEATH1;
          break;
        case 1:
          anim = BOTH_DEATH2;
          break;
        case 2:
        default:
          anim = BOTH_DEATH3;
          break;
      }
    }
    else
    {
      switch(i)
      {
        case 0:
          anim = NSPA_DEATH1;
          break;
        case 1:
          anim = NSPA_DEATH2;
          break;
        case 2:
        default:
          anim = NSPA_DEATH3;
          break;
      }
    }
    self->client->ps.legsAnim = ((self->client->ps.legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT)
        | anim;

    if (!(self->client->ps.persistant[PERS_STATE] & PS_NONSEGMODEL))
    {
      self->client->ps.torsoAnim = ((self->client->ps.torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT)
          | anim;
    }

    // use own entityid if killed by non-client to prevent uint8_t overflow
    G_AddEvent(self, EV_DEATH1 + i, (killer < MAX_CLIENTS) ? killer : self - g_entities);

    // globally cycle through the different death animations
    i = (i + 1) % 3;
  }
  trap_LinkEntity(self);
}

////////TA: locdamage

/*
 ===============
 G_ParseArmourScript
 ===============
 */
void
G_ParseArmourScript(char *buf, int upgrade)
{
  char *token;
  int count;

  count = 0;

  while(1)
  {
    token = COM_Parse(&buf);

    if (!token[0])
      break;

    if (strcmp(token, "{"))
    {
      G_Printf("Missing { in armour file\n");
      break;
    }

    if (count == MAX_ARMOUR_REGIONS)
    {
      G_Printf("Max armour regions exceeded in locdamage file\n");
      break;
    }

    //default
    g_armourRegions[upgrade][count].minHeight = 0.0;
    g_armourRegions[upgrade][count].maxHeight = 1.0;
    g_armourRegions[upgrade][count].minAngle = 0;
    g_armourRegions[upgrade][count].maxAngle = 360;
    g_armourRegions[upgrade][count].modifier = 1.0;
    g_armourRegions[upgrade][count].crouch = qfalse;

    while(1)
    {
      token = COM_ParseExt(&buf, qtrue);

      if (!token[0])
      {
        G_Printf("Unexpected end of armour file\n");
        break;
      }

      if (!Q_stricmp(token, "}"))
      {
        break;
      }
      else if (!strcmp(token, "minHeight"))
      {
        token = COM_ParseExt(&buf, qfalse);

        if (!token[0])
          strcpy(token, "0");

        g_armourRegions[upgrade][count].minHeight = atof(token);
      }
      else if (!strcmp(token, "maxHeight"))
      {
        token = COM_ParseExt(&buf, qfalse);

        if (!token[0])
          strcpy(token, "100");

        g_armourRegions[upgrade][count].maxHeight = atof(token);
      }
      else if (!strcmp(token, "minAngle"))
      {
        token = COM_ParseExt(&buf, qfalse);

        if (!token[0])
          strcpy(token, "0");

        g_armourRegions[upgrade][count].minAngle = atoi(token);
      }
      else if (!strcmp(token, "maxAngle"))
      {
        token = COM_ParseExt(&buf, qfalse);

        if (!token[0])
          strcpy(token, "360");

        g_armourRegions[upgrade][count].maxAngle = atoi(token);
      }
      else if (!strcmp(token, "modifier"))
      {
        token = COM_ParseExt(&buf, qfalse);

        if (!token[0])
          strcpy(token, "1.0");

        g_armourRegions[upgrade][count].modifier = atof(token);
      }
      else if (!strcmp(token, "crouch"))
      {
        g_armourRegions[upgrade][count].crouch = qtrue;
      }
    }

    g_numArmourRegions[upgrade]++;
    count++;
  }
}

/*
 ===============
 G_ParseDmgScript
 ===============
 */
void
G_ParseDmgScript(char *buf, int class)
{
  char *token;
  int count;

  count = 0;

  while(1)
  {
    token = COM_Parse(&buf);

    if (!token[0])
      break;

    if (strcmp(token, "{"))
    {
      G_Printf("Missing { in locdamage file\n");
      break;
    }

    if (count == MAX_LOCDAMAGE_REGIONS)
    {
      G_Printf("Max damage regions exceeded in locdamage file\n");
      break;
    }

    //default
    g_damageRegions[class][count].minHeight = 0.0;
    g_damageRegions[class][count].maxHeight = 1.0;
    g_damageRegions[class][count].minAngle = 0;
    g_damageRegions[class][count].maxAngle = 360;
    g_damageRegions[class][count].modifier = 1.0;
    g_damageRegions[class][count].crouch = qfalse;

    while(1)
    {
      token = COM_ParseExt(&buf, qtrue);

      if (!token[0])
      {
        G_Printf("Unexpected end of locdamage file\n");
        break;
      }

      if (!Q_stricmp(token, "}"))
      {
        break;
      }
      else if (!strcmp(token, "minHeight"))
      {
        token = COM_ParseExt(&buf, qfalse);

        if (!token[0])
          strcpy(token, "0");

        g_damageRegions[class][count].minHeight = atof(token);
      }
      else if (!strcmp(token, "maxHeight"))
      {
        token = COM_ParseExt(&buf, qfalse);

        if (!token[0])
          strcpy(token, "100");

        g_damageRegions[class][count].maxHeight = atof(token);
      }
      else if (!strcmp(token, "minAngle"))
      {
        token = COM_ParseExt(&buf, qfalse);

        if (!token[0])
          strcpy(token, "0");

        g_damageRegions[class][count].minAngle = atoi(token);
      }
      else if (!strcmp(token, "maxAngle"))
      {
        token = COM_ParseExt(&buf, qfalse);

        if (!token[0])
          strcpy(token, "360");

        g_damageRegions[class][count].maxAngle = atoi(token);
      }
      else if (!strcmp(token, "modifier"))
      {
        token = COM_ParseExt(&buf, qfalse);

        if (!token[0])
          strcpy(token, "1.0");

        g_damageRegions[class][count].modifier = atof(token);
      }
      else if (!strcmp(token, "crouch"))
      {
        g_damageRegions[class][count].crouch = qtrue;
      }
    }

    g_numDamageRegions[class]++;
    count++;
  }
}

/*
 ============
 G_CalcDamageModifier
 ============
 */
static float
G_CalcDamageModifier(vec3_t point, gentity_t *targ, gentity_t *attacker, int class, int dflags)
{
  vec3_t targOrigin;
  vec3_t bulletPath;
  vec3_t bulletAngle;
  vec3_t pMINUSfloor, floor, normal;

  float clientHeight, hitRelative, hitRatio;
  int bulletRotation, clientRotation, hitRotation;
  float modifier = 1.0f;
  int i, j;

  if (point == NULL)
    return 1.0f;

  if (g_unlagged.integer && targ->client && targ->client->unlaggedCalc.used)
    VectorCopy(targ->client->unlaggedCalc.origin, targOrigin);
else    VectorCopy(targ->r.currentOrigin, targOrigin);

    clientHeight = targ->r.maxs[ 2 ] - targ->r.mins[ 2 ];

    if (targ->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING)
    VectorCopy(targ->client->ps.grapplePoint, normal);
    else
    VectorSet(normal, 0, 0, 1);

    VectorMA(targOrigin, targ->r.mins[ 2 ], normal, floor);
    VectorSubtract(point, floor, pMINUSfloor);

    hitRelative = DotProduct(normal, pMINUSfloor) / VectorLength(normal);

    if (hitRelative < 0.0f)
    hitRelative = 0.0f;

    if (hitRelative > clientHeight)
    hitRelative = clientHeight;

    hitRatio = hitRelative / clientHeight;

    VectorSubtract(targOrigin, point, bulletPath);
    vectoangles(bulletPath, bulletAngle);

    clientRotation = targ->client->ps.viewangles[ YAW ];
    bulletRotation = bulletAngle[ YAW ];

    hitRotation = abs(clientRotation - bulletRotation);

    hitRotation = hitRotation % 360; // Keep it in the 0-359 range

    if (dflags & DAMAGE_NO_LOCDAMAGE)
    {
      for (i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++)
      {
        float totalModifier = 0.0f;
        float averageModifier = 1.0f;

        //average all of this upgrade's armour regions together
        if (BG_InventoryContainsUpgrade(i, targ->client->ps.stats))
        {
          for (j = 0; j < g_numArmourRegions[ i ]; j++)
          totalModifier += g_armourRegions[ i ][ j ].modifier;

          if (g_numArmourRegions[ i ])
          averageModifier = totalModifier / g_numArmourRegions[ i ];
          else
          averageModifier = 1.0f;
        }

        modifier *= averageModifier;
      }
    }
    else
    {
      if (attacker && attacker->client)
      {
        attacker->client->pers.statscounters.hitslocational++;
        level.alienStatsCounters.hitslocational++;
      }
      for (i = 0; i < g_numDamageRegions[ class ]; i++)
      {
        qboolean rotationBound;

        if (g_damageRegions[ class ][ i ].minAngle >
            g_damageRegions[ class ][ i ].maxAngle)
        {
          rotationBound = (hitRotation >= g_damageRegions[ class ][ i ].minAngle &&
              hitRotation <= 360) || (hitRotation >= 0 &&
              hitRotation <= g_damageRegions[ class ][ i ].maxAngle);
        }
        else
        {
          rotationBound = (hitRotation >= g_damageRegions[ class ][ i ].minAngle &&
              hitRotation <= g_damageRegions[ class ][ i ].maxAngle);
        }

        if (rotationBound &&
            hitRatio >= g_damageRegions[ class ][ i ].minHeight &&
            hitRatio <= g_damageRegions[ class ][ i ].maxHeight &&
            (g_damageRegions[ class ][ i ].crouch ==
                (targ->client->ps.pm_flags & PMF_DUCKED)))
        modifier *= g_damageRegions[ class ][ i ].modifier;
      }

      if (attacker && attacker->client && modifier == 2)
      {
        attacker->client->pers.statscounters.headshots++;
		  if ((attacker->client->pers.badges[ 2 ] != 1) && (attacker->client->pers.statscounters.headshots >= 200))
		  {
			  G_WinBadge( attacker, 2 );
			  attacker->client->pers.badgeupdate[2] = 1;
			  attacker->client->pers.badges[2] = 1;
		  }
        level.alienStatsCounters.headshots++;
      }
      if (attacker && attacker->client
          && modifier != 2
          && targ->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS)
      {
        /*if (!g_survival.integer)
         {
         return 0.0f;
         }*/
      }

      for (i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++)
      {
        if (BG_InventoryContainsUpgrade(i, targ->client->ps.stats))
        {
          for (j = 0; j < g_numArmourRegions[ i ]; j++)
          {
            qboolean rotationBound;

            if (g_armourRegions[ i ][ j ].minAngle >
                g_armourRegions[ i ][ j ].maxAngle)
            {
              rotationBound = (hitRotation >= g_armourRegions[ i ][ j ].minAngle &&
                  hitRotation <= 360) || (hitRotation >= 0 &&
                  hitRotation <= g_armourRegions[ i ][ j ].maxAngle);
            }
            else
            {
              rotationBound = (hitRotation >= g_armourRegions[ i ][ j ].minAngle &&
                  hitRotation <= g_armourRegions[ i ][ j ].maxAngle);
            }

            if (rotationBound &&
                hitRatio >= g_armourRegions[ i ][ j ].minHeight &&
                hitRatio <= g_armourRegions[ i ][ j ].maxHeight &&
                (g_armourRegions[ i ][ j ].crouch ==
                    (targ->client->ps.pm_flags & PMF_DUCKED)))
            modifier *= g_armourRegions[ i ][ j ].modifier;
          }
        }
      }
    }

    return modifier;
  }

  /*
   ============
   G_InitDamageLocations
   ============
   */
void
G_InitDamageLocations(void)
{
  char *modelName;
  char filename[MAX_QPATH];
  int i;
  int len;
  fileHandle_t fileHandle;
  char buffer[MAX_LOCDAMAGE_TEXT];

  for(i = PCL_NONE + 1;i < PCL_NUM_CLASSES;i++)
  {
    modelName = BG_FindModelNameForClass(i);
    Com_sprintf(filename, sizeof(filename), "models/players/%s/locdamage.cfg", modelName);

    len = trap_FS_FOpenFile(filename, &fileHandle, FS_READ);
    if (!fileHandle)
    {
      G_Printf(va(S_COLOR_RED "file not found: %s\n", filename));
      continue;
    }

    if (len >= MAX_LOCDAMAGE_TEXT)
    {
      G_Printf(va(
        S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len,
        MAX_LOCDAMAGE_TEXT));
      trap_FS_FCloseFile(fileHandle);
      continue;
    }

    trap_FS_Read(buffer, len, fileHandle);
    buffer[len] = 0;
    trap_FS_FCloseFile(fileHandle);

    G_ParseDmgScript(buffer, i);
  }

  for(i = UP_NONE + 1;i < UP_NUM_UPGRADES;i++)
  {
    modelName = BG_FindNameForUpgrade(i);
    Com_sprintf(filename, sizeof(filename), "armour/%s.armour", modelName);

    len = trap_FS_FOpenFile(filename, &fileHandle, FS_READ);

    //no file - no parsage
    if (!fileHandle)
      continue;

    if (len >= MAX_LOCDAMAGE_TEXT)
    {
      G_Printf(va(
        S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len,
        MAX_LOCDAMAGE_TEXT));
      trap_FS_FCloseFile(fileHandle);
      continue;
    }

    trap_FS_Read(buffer, len, fileHandle);
    buffer[len] = 0;
    trap_FS_FCloseFile(fileHandle);

    G_ParseArmourScript(buffer, i);
  }
}

////////TA: locdamage


/*
 ============
 T_Damage

 targ    entity that is being damaged
 inflictor entity that is causing the damage
 attacker  entity that caused the inflictor to damage targ
 example: targ=monster, inflictor=rocket, attacker=player

 dir     direction of the attack for knockback
 point   point at which the damage is being inflicted, used for headshots
 damage    amount of damage being inflicted
 knockback force to be applied against targ as a result of the damage

 inflictor, attacker, dir, and point can be NULL for environmental effects

 dflags    these flags are used to control how T_Damage works
 DAMAGE_RADIUS     damage was indirect (from a nearby explosion)
 DAMAGE_NO_ARMOR     armor does not protect from this damage
 DAMAGE_NO_KNOCKBACK   do not affect velocity, just view angles
 DAMAGE_NO_PROTECTION  kills godmode, armor, everything
 ============
 */

//TA: team is the team that is immune to this damage

void
G_SelectiveDamage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir,
  vec3_t point, int damage, int dflags, int mod, int team)
{
  if (targ->client && (team != targ->client->ps.stats[STAT_PTEAM]))
    G_Damage(targ, inflictor, attacker, dir, point, damage, dflags, mod);
}

void
G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point,
  int damage, int dflags, int mod)
{
  gclient_t *client;
  int take;
  int save;
  int asave = 0;
  int knockback;
  float damagemodifier = 0.0;
  int takeNoOverkill;

	if ((attacker->client->ps.stats[STAT_PTEAM] == PTE_ALIENS) && (mod != MOD_ZOMBIE_BITE))
		return;
  if (!targ->takedamage)
    return;

  if(targ == attacker) //no ff?
      return;

  if(targ && (mod == MOD_GRENADE_LAUNCHER_INCENDIARY)
      && !(targ->client->ps.stats[STAT_STATE] & SS_ONFIRE))
  {
    targ->client->ps.stats[STAT_STATE] |= SS_ONFIRE;
    targ->client->lastOnFireTime = level.time;
    targ->client->lastOnFireClient = attacker;
	  if(level.time > attacker->client->pers.flametimer)
	  {
		  attacker->client->pers.flames =0;
		  
	  }
	  attacker->client->pers.flames++;
	  if(attacker->client->pers.flames == 1)
	  {
		  attacker->client->pers.flametimer = (level.time + 2000);
	  }
	  if(attacker->client->pers.flames >=7)
	  {
		 // G_WinBadge( attacker, 24 );
		  attacker->client->pers.badgeupdate[24] = 1;
		  attacker->client->pers.badges[24] = 1;
	  }
  }
	
  //Hax to reduce power of lasergun when killing buildables.
  if(mod == MOD_LASERGUN && targ->s.eType == ET_BUILDABLE)
  {
    damage = damage/10;
  }

  if (targ == attacker && g_survival.integer)
  {
    //Do the damage
    take = 10;
    targ->health = targ->health - take;

    if (targ->client)
      targ->client->ps.stats[STAT_HEALTH] = targ->health;

    targ->lastDamageTime = level.time;

    if (targ->health <= 0)
    {
      if (targ->client)
        targ->flags |= FL_NO_KNOCKBACK;

      if (targ->health < -999)
        targ->health = -999;

      targ->enemy = attacker;
      targ->die(targ, inflictor, attacker, take, mod);
      return;
    }
    else if (targ->pain)
	{
		if (targ->health == 1)
		{
			targ->client->pers.onehp = 1;
		}
		targ->pain(targ, attacker, take);
	}
    return;
  }

  if (g_survival.integer && attacker && attacker->client && targ->s.eType == ET_BUILDABLE)
  {
    return;
  }

  if (targ->health <= 0)
    return;

  if (g_survival.integer && attacker && attacker->client && targ && targ->client
      && targ->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)
  {
    if (targ->botEnemy == NULL)
    {
      targ->botEnemy = attacker;
    }
    else if (targ->botCommand == BOT_FOLLOW_PATH)
    {
      switch(targ->botMetaMode)
      {
        case ATTACK_RAMBO:
          if (targ->health < 85)
          {
            targ->botEnemy = attacker;
            targ->botCommand = BOT_REGULAR;
            memset(&targ->client->pers.cmd, 0, sizeof(targ->client->pers.cmd));
          }
          break;
        case ATTACK_CAMPER:
          if (targ->health < 95)
          {
            targ->botEnemy = attacker;
            targ->botCommand = BOT_REGULAR;
            memset(&targ->client->pers.cmd, 0, sizeof(targ->client->pers.cmd));
          }
          break;
        case ATTACK_ALL:
          if (targ->health < 100)
          {
            targ->botEnemy = attacker;
            targ->botCommand = BOT_REGULAR;
            memset(&targ->client->pers.cmd, 0, sizeof(targ->client->pers.cmd));
          }
          break;
      }
    }
  }

  if (targ->client)
  {
    if ((targ->r.svFlags & SVF_BOT) && targ->client->ps.pm_flags & PMF_QUEUED)
      return;

    if (targ->client->sess.sessionTeam == TEAM_SPECTATOR && targ->client->ps.stats[STAT_PTEAM]
        == PTE_ALIENS)
      return;
  }

  // the intermission has allready been qualified for, so don't
  // allow any extra scoring
  if (level.intermissionQueued)
    return;

  if (!inflictor)
    inflictor = &g_entities[ENTITYNUM_WORLD];

  if (!attacker)
    attacker = &g_entities[ENTITYNUM_WORLD];

  // shootable doors / buttons don't actually have any health
  if (targ->s.eType == ET_MOVER)
  {
    if (targ->use && (targ->moverState == MOVER_POS1 || targ->moverState == ROTATOR_POS1))
      targ->use(targ, inflictor, attacker);

    return;
  }

  client = targ->client;

  if (client)
  {
    if (targ && targ->client && targ->client->ps.stats[STAT_PTEAM] == PTE_HUMANS && level.time
        - targ->antispawncamp < g_antispawncamp.integer)
      return;
    if (client->noclip && !g_devmapNoGod.integer)
      return;
  }

  if (g_ctn.integer > 0 && targ->s.eType == ET_BUILDABLE && targ->s.modelindex == BA_H_SPAWN)
  {
    return;
  }

  if (!dir)
    dflags |= DAMAGE_NO_KNOCKBACK;
  else
    VectorNormalize(dir);

  knockback = damage;

  if (inflictor->s.weapon != WP_NONE)
  {
    knockback = (int) ((float) knockback * BG_FindKnockbackScaleForWeapon(inflictor->s.weapon));
  }

  if (targ->client)
  {
    knockback = (int) ((float) knockback * BG_FindKnockbackScaleForClass(
      targ->client->ps.stats[STAT_PCLASS]));
  }

  if (knockback > 200)
    knockback = 200;

  if (targ->flags & FL_NO_KNOCKBACK)
    knockback = 0;

  if (dflags & DAMAGE_NO_KNOCKBACK)
    knockback = 0;

  // figure momentum add, even if the damage won't be taken
  // Humans will not feel knockback to prevent ppl pushing each other.
  if (knockback && targ->client && targ->client->ps.stats[STAT_PTEAM] != PTE_HUMANS)
  {
    vec3_t kvel;
    float mass;

    mass = 200;

    VectorScale(dir, g_knockback.value * (float) knockback / mass, kvel);
    VectorAdd(targ->client->ps.velocity, kvel, targ->client->ps.velocity);

    // set the timer so that the other client can't cancel
    // out the movement immediately
    if (!targ->client->ps.pm_time)
    {
      int t;

      t = knockback * 2;
      if (t < 50)
        t = 50;

      if (t > 200)
        t = 200;

      targ->client->ps.pm_time = t;
      targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
    }
  }

  // check for completely getting out of the damage
  if (!(dflags & DAMAGE_NO_PROTECTION))
  {

    // if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
    // if the attacker was on the same team
    if (targ != attacker && OnSameTeam(targ, attacker))
    {
      if (g_dretchPunt.integer && targ->client->ps.stats[STAT_PCLASS] == PCL_ALIEN_LEVEL0)
      {
        vec3_t dir, push;

        VectorSubtract(targ->r.currentOrigin, attacker->r.currentOrigin, dir);
        VectorNormalizeFast(dir);
        VectorScale(dir, (damage * 10.0f), push);
        push[2] = 64.0f;
        VectorAdd(targ->client->ps.velocity, push, targ->client->ps.velocity);
        return;
      }
      else if (g_friendlyFire.value <= 0)
      {
        if (targ->client->ps.stats[STAT_PTEAM] == PTE_HUMANS)
        {
          if (g_friendlyFireHumans.value <= 0)
            return;
          else if (g_friendlyFireHumans.value > 0 && g_friendlyFireHumans.value < 1)
            damage = (int) (0.5 + g_friendlyFireHumans.value * (float) damage);
        }
        if (targ->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)
        {
          if (g_friendlyFireAliens.value == 0)
            return;
          else if (g_friendlyFireAliens.value > 0 && g_friendlyFireAliens.value < 1)
            damage = (int) (0.5 + g_friendlyFireAliens.value * (float) damage);
        }
      }
      else if (g_friendlyFire.value > 0 && g_friendlyFire.value < 1)
      {
        damage = (int) (0.5 + g_friendlyFire.value * (float) damage);
      }
    }

    // If target is buildable on the same team as the attacking client
    if (targ->s.eType == ET_BUILDABLE && attacker->client && targ->biteam
        == attacker->client->pers.teamSelection)
    {
      if (g_friendlyBuildableFire.value <= 0)
      {
        return;
      }
      else if (g_friendlyBuildableFire.value > 0 && g_friendlyBuildableFire.value < 1)
      {
        damage = (int) (0.5 + g_friendlyBuildableFire.value * (float) damage);
      }
    }

    // check for godmode
    if (targ->flags & FL_GODMODE && !g_devmapNoGod.integer)// && !g_survival.integer)
      return;

    if (targ->s.eType == ET_BUILDABLE && g_cheats.integer && g_devmapNoStructDmg.integer)
      return;
  }

  // add to the attacker's hit counter
  if (attacker->client && targ != attacker && targ->health > 0 && targ->s.eType != ET_MISSILE
      && targ->s.eType != ET_GENERAL)
  {
    if (OnSameTeam(targ, attacker))
      attacker->client->ps.persistant[PERS_HITS]--;
    else
      attacker->client->ps.persistant[PERS_HITS]++;
  }

  take = damage;
  save = 0;

  // add to the damage inflicted on a player this frame
  // the total will be turned into screen blends and view angle kicks
  // at the end of the frame
  if (client)
  {
    if (attacker)
      client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
    else
      client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;

    client->damage_armor += asave;
    client->damage_blood += take;
    client->damage_knockback += knockback;

    if (dir)
    {
      VectorCopy(dir, client->damage_from);
      client->damage_fromWorld = qfalse;
    }
    else
    {
      VectorCopy(targ->r.currentOrigin, client->damage_from);
      client->damage_fromWorld = qtrue;
    }

    // set the last client who damaged the target
    targ->client->lasthurt_client = attacker->s.number;
    targ->client->lasthurt_mod = mod;

    damagemodifier = G_CalcDamageModifier(
      point, targ, attacker, client->ps.stats[STAT_PCLASS], dflags);
    take = (int) ((float) take * damagemodifier);

    //if boosted poison every attack
    if (attacker->client && attacker->client->ps.stats[STAT_STATE] & SS_BOOSTED)
    {
      if (targ->client->ps.stats[STAT_PTEAM] == PTE_HUMANS && !(targ->client->ps.stats[STAT_STATE]
          & SS_POISONED) && targ->client->poisonImmunityTime < level.time)
      {
        targ->client->ps.stats[STAT_STATE] |= SS_POISONED;
        targ->client->lastPoisonTime = level.time;
        targ->client->lastPoisonClient = attacker;
        attacker->client->pers.statscounters.repairspoisons++;
        level.alienStatsCounters.repairspoisons++;
      }
    }
  }

  if (take < 1)
    take = 1;

  if (g_debugDamage.integer)
  {
    G_Printf(
      "%i: client:%i health:%i damage:%i armor:%i\n", level.time, targ->s.number, targ->health,
      take, asave);
  }

  takeNoOverkill = take;
  if (takeNoOverkill > targ->health)
  {
    if (targ->health > 0)
      takeNoOverkill = targ->health;
    else
      takeNoOverkill = 0;
  }

  if (take)
  {
    //Increment some stats counters
    if (attacker && attacker->client)
    {
      if (targ->biteam == attacker->client->pers.teamSelection || OnSameTeam(targ, attacker))
      {
        attacker->client->pers.statscounters.ffdmgdone += takeNoOverkill;
        if (attacker->client->pers.teamSelection == PTE_ALIENS)
        {
          level.alienStatsCounters.ffdmgdone += takeNoOverkill;
        }
        else if (attacker->client->pers.teamSelection == PTE_HUMANS)
        {
          level.humanStatsCounters.ffdmgdone += takeNoOverkill;
        }
      }
      else if (targ->s.eType == ET_BUILDABLE)
      {
        attacker->client->pers.statscounters.structdmgdone += takeNoOverkill;

        if (attacker->client->pers.teamSelection == PTE_ALIENS)
        {
          level.alienStatsCounters.structdmgdone += takeNoOverkill;
        }
        else if (attacker->client->pers.teamSelection == PTE_HUMANS)
        {
          level.humanStatsCounters.structdmgdone += takeNoOverkill;
        }

        if (targ->health > 0 && (targ->health - take) <= 0)
        {
          attacker->client->pers.statscounters.structskilled++;
          if (targ->s.modelindex == BA_H_SPAWN)
		  {
			  attacker->client->pers.structskilled++;
		  }
		  if (attacker->client->pers.teamSelection == PTE_ALIENS)
          {
            level.alienStatsCounters.structskilled++;
          }
          else if (attacker->client->pers.teamSelection == PTE_HUMANS)
          {
            level.humanStatsCounters.structskilled++;
          }
        }
      }
      else if (targ->client)
      {
        attacker->client->pers.statscounters.dmgdone += takeNoOverkill;
        attacker->client->pers.statscounters.hits++;
        if (attacker->client->pers.teamSelection == PTE_ALIENS)
        {
          level.alienStatsCounters.dmgdone += takeNoOverkill;
        }
        else if (attacker->client->pers.teamSelection == PTE_HUMANS)
        {
          level.humanStatsCounters.dmgdone += takeNoOverkill;
        }
      }
    }

    //Do the damage
    targ->health = targ->health - take;
	  
    if (targ->client)
      targ->client->ps.stats[STAT_HEALTH] = targ->health;
	  //37 Once you go human...		Taste your first human blood
	  if(g_survival.integer && (targ->r.svFlags & SVF_BOT) && (attacker->client->pers.badges[ 37 ] != 1))
	  {
		  G_WinBadge( attacker, 37 );
		  attacker->client->pers.badgeupdate[37] = 1;
		  attacker->client->pers.badges[37] = 1;
	  }
	  //40 Ninja		Don't get touched by a survival zombie for 5 minutes
	  if(g_survival.integer && !(targ->r.svFlags & SVF_BOT) && (targ->client->pers.badges[ 40 ] != 1) && ((level.time - targ->lastDamageTime) >= 300000))
	  {
		  G_WinBadge( targ, 40 );
		  targ->client->pers.badgeupdate[40] = 1;
		  targ->client->pers.badges[40] = 1;
	  }
    targ->lastDamageTime = level.time;
	  targ->client->pers.lastDamaged = level.time;
	  targ->client->pers.lastattacker = attacker;
	  
	  if (targ->health == 1)
	  {
		  targ->client->pers.onehp = 1;
	  }

    //TA: add to the attackers "account" on the target
    if (targ->client && attacker->client)
    {
      if (attacker != targ && !OnSameTeam(targ, attacker))
        targ->credits[attacker->client->ps.clientNum] += take;
      else if (attacker != targ && OnSameTeam(targ, attacker))
        targ->client->tkcredits[attacker->client->ps.clientNum] += takeNoOverkill;
    }

    if (targ->health <= 0)
    {
      if (client)
        targ->flags |= FL_NO_KNOCKBACK;

      if (targ->health < -999)
        targ->health = -999;

      targ->enemy = attacker;
      targ->die(targ, inflictor, attacker, take, mod);
      return;
    }
    else if (targ->pain)
	{
		if (targ->health == 1)
		{
			targ->client->pers.onehp = 1;
		}
      targ->pain(targ, attacker, take);
	}
  }
}

/*
 ============
 CanDamage

 Returns qtrue if the inflictor can directly damage the target.  Used for
 explosions and melee attacks.
 ============
 */
qboolean
CanDamage(gentity_t *targ, vec3_t origin)
{
  vec3_t dest;
  trace_t tr;
  vec3_t midpoint;

  // use the midpoint of the bounds instead of the origin, because
  // bmodels may have their origin is 0,0,0
  VectorAdd(targ->r.absmin, targ->r.absmax, midpoint);
  VectorScale(midpoint, 0.5, midpoint);

  VectorCopy(midpoint, dest);
  trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
  if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
    return qtrue;

  // this should probably check in the plane of projection,
  // rather than in world coordinate, and also include Z
  VectorCopy(midpoint, dest);
  dest[0] += 15.0;
  dest[1] += 15.0;
  trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
  if (tr.fraction == 1.0)
    return qtrue;

  VectorCopy(midpoint, dest);
  dest[0] += 15.0;
  dest[1] -= 15.0;
  trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
  if (tr.fraction == 1.0)
    return qtrue;

  VectorCopy(midpoint, dest);
  dest[0] -= 15.0;
  dest[1] += 15.0;
  trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
  if (tr.fraction == 1.0)
    return qtrue;

  VectorCopy(midpoint, dest);
  dest[0] -= 15.0;
  dest[1] -= 15.0;
  trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
  if (tr.fraction == 1.0)
    return qtrue;

  return qfalse;
}

//TA:

/*
 ============
 G_SelectiveRadiusDamage
 ============
 */
qboolean
G_SelectiveRadiusDamage(vec3_t origin, gentity_t *attacker, float damage, float radius,
  gentity_t *ignore, int mod, int team)
{
  float points, dist;
  gentity_t *ent;
  int entityList[MAX_GENTITIES];
  int numListedEntities;
  vec3_t mins, maxs;
  vec3_t v;
  vec3_t dir;
  int i, e;
  qboolean hitClient = qfalse;

  if (radius < 1)
    radius = 1;

  for(i = 0;i < 3;i++)
  {
    mins[i] = origin[i] - radius;
    maxs[i] = origin[i] + radius;
  }

  numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

  for(e = 0;e < numListedEntities;e++)
  {
    ent = &g_entities[entityList[e]];

    if (ent == ignore)
      continue;

    if (!ent->takedamage)
      continue;

    // find the distance from the edge of the bounding box
    for(i = 0;i < 3;i++)
    {
      if (origin[i] < ent->r.absmin[i])
        v[i] = ent->r.absmin[i] - origin[i];
      else if (origin[i] > ent->r.absmax[i])
        v[i] = origin[i] - ent->r.absmax[i];
      else
        v[i] = 0;
    }

    dist = VectorLength(v);
    if (dist >= radius)
      continue;

    points = damage * (1.0 - dist / radius);

    if (CanDamage(ent, origin))
    {
      VectorSubtract(ent->r.currentOrigin, origin, dir);
      // push the center of mass higher than the origin so players
      // get knocked into the air more
      dir[2] += 24;
      G_SelectiveDamage(ent, NULL, attacker, dir, origin, (int) points, DAMAGE_RADIUS
          | DAMAGE_NO_LOCDAMAGE, mod, team);
    }
  }

  return hitClient;
}

/*
 ============
 G_RadiusDamage
 ============
 */
qboolean
G_RadiusDamage(vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore,
  int mod)
{
  float points, dist;
  gentity_t *ent;
  int entityList[MAX_GENTITIES];
  int numListedEntities;
  vec3_t mins, maxs;
  vec3_t v;
  vec3_t dir;
  int i, e;
  qboolean hitClient = qfalse;

  if (radius < 1)
    radius = 1;

  for(i = 0;i < 3;i++)
  {
    mins[i] = origin[i] - radius;
    maxs[i] = origin[i] + radius;
  }

  numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

  for(e = 0;e < numListedEntities;e++)
  {
    ent = &g_entities[entityList[e]];

    if (ent == ignore)
      continue;

    if (!ent->takedamage)
      continue;

    // find the distance from the edge of the bounding box
    for(i = 0;i < 3;i++)
    {
      if (origin[i] < ent->r.absmin[i])
        v[i] = ent->r.absmin[i] - origin[i];
      else if (origin[i] > ent->r.absmax[i])
        v[i] = origin[i] - ent->r.absmax[i];
      else
        v[i] = 0;
    }

    dist = VectorLength(v);
    if (dist >= radius)
      continue;

    points = damage * (1.0 - dist / radius);

    if (CanDamage(ent, origin))
    {
      VectorSubtract(ent->r.currentOrigin, origin, dir);
      // push the center of mass higher than the origin so players
      // get knocked into the air more
      dir[2] += 24;
      G_Damage(
        ent, NULL, attacker, dir, origin, (int) points, DAMAGE_RADIUS | DAMAGE_NO_LOCDAMAGE, mod);
    }
  }

  return hitClient;
}
