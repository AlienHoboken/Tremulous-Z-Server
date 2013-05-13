/*
 ===========================================================================
 Copyright (C) 2007 Amine Haddad

 This file is part of Tremulous.

 The original works of vcxzet (lamebot3) were used a guide to create TremBot.

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

/* Current version: v0.01 */

#include "g_local.h"
#include "acebot.h"

#ifndef RAND_MAX
#define RAND_MAX 32768
#endif

void
G_BotAdd(char *name, int team, int skill, gentity_t * ent)
{
  int i;
  int clientNum;
  char userinfo[MAX_INFO_STRING];
  char model[MAX_QPATH];
  char buffer[MAX_QPATH];
  gentity_t *bot;

  if (level.intermissiontime)
  {
    return;
  }
  if (level.time - level.startTime < 10000)
  {
    return;
  }

  //reservedSlots = trap_Cvar_VariableIntegerValue ("sv_privateclients"); // Found a way to have this var in level.

  // find what clientNum to use for bot
  clientNum = -1;

  for(i = 0;i < level.botslots;i++)
  {
    if (!g_entities[i].inuse)
    {
      clientNum = i;
      break;
    }
  }

  if (clientNum < 0)
  {
    trap_Printf("no more slots for bot\n");
    return;
  }
  if (skill < 1)
    skill = 1;

  bot = &g_entities[clientNum];
  bot->r.svFlags |= SVF_BOT;
  bot->inuse = qtrue;

  //default bot data
  bot->botCommand = BOT_REGULAR;
  if (ent == NULL)
  {
    bot->botFriend = NULL;
  }
  else
  {
    bot->botFriend = ent;
    bot->botFriend->botclientnum[ent->havebot] = clientNum;
  }

  bot->botEnemy = NULL;
  bot->botFriendLastSeen = 0;
  bot->botEnemyLastSeen = 0;
  bot->botSkillLevel = skill;
  bot->botTeam = team;
  bot->turntime = level.time + 3000;

  // register user information
  userinfo[0] = '\0';
  Info_SetValueForKey(userinfo, "name", name);
  Info_SetValueForKey(userinfo, "rate", "8000");
  Info_SetValueForKey(userinfo, "snaps", "20");

  trap_SetUserinfo(clientNum, userinfo);

  // have it connect to the game as a normal client
  if (ClientConnect(clientNum, qtrue) != NULL)
  {
    // won't let us join
    return;
  }

  level.bots++;
  //Since i dont update UserInfo for bots i need to set the netname here.
  Q_strncpyz(bot->client->pers.netname, name, sizeof(bot->client->pers.netname));
  ClientBegin(clientNum);
  G_ChangeTeam(bot, team);

  //Setting initial client information.
  Com_sprintf(
    buffer, MAX_QPATH, "%s/%s", BG_FindModelNameForClass(PCL_HUMAN), BG_FindSkinNameForClass(
      PCL_HUMAN));
  Q_strncpyz(model, buffer, sizeof(model));
  Com_sprintf(
    userinfo, sizeof(userinfo), "t\\%i\\model\\%s\\hmodel\\%s\\", PTE_ALIENS, model, model);
  trap_SetConfigstring(CS_PLAYERS + clientNum, userinfo);
  bot->client->ps.persistant[PERS_STATE] &= ~PS_NONSEGMODEL;
  bot->client->ps.persistant[PERS_STATE] &= ~PS_WALLCLIMBINGFOLLOW;
  bot->client->pers.useUnlagged = qfalse;
  bot->client->pers.predictItemPickup = qfalse;
  bot->client->pers.localClient = qtrue;
}

void
G_BotDel(int clientNum)
{
  gentity_t *bot;

  bot = &g_entities[clientNum];
  if (!(bot->r.svFlags & SVF_BOT))
  {
    trap_Printf(va("'^7%s^7' is not a bot\n", bot->client->pers.netname));
    return;
  }
  if (bot->botFriend != NULL)
  {
    //bot->botFriend->havebot = 0;
  }
  level.bots--;
  ClientDisconnect(clientNum);
}

void
G_BotCmd(gentity_t * master, int clientNum, char *command)
{
  gentity_t *bot;

  bot = &g_entities[clientNum];
  if (!(bot->r.svFlags & SVF_BOT))
  {
    return;
  }

  bot->botFriend = NULL;
  bot->botEnemy = NULL;
  bot->botFriendLastSeen = 0;
  bot->botEnemyLastSeen = 0;

  //ROTAX
  if (g_ambush.integer == 1)
  {
    trap_Printf(va("You can't change aliens behavior in ambush mod\n"));
    return;
  }

  if (!Q_stricmp(command, "regular"))
  {

    bot->botCommand = BOT_REGULAR;
    //trap_SendServerCommand(-1, "print \"regular mode\n\"");

  }
  else if (!Q_stricmp(command, "idle"))
  {

    bot->botCommand = BOT_IDLE;
    //trap_SendServerCommand(-1, "print \"idle mode\n\"");

  }
  else if (!Q_stricmp(command, "attack"))
  {

    bot->botCommand = BOT_ATTACK;
    //trap_SendServerCommand(-1, "print \"attack mode\n\"");

  }
  else if (!Q_stricmp(command, "standground"))
  {

    bot->botCommand = BOT_STAND_GROUND;
    //trap_SendServerCommand(-1, "print \"stand ground mode\n\"");

  }
  else if (!Q_stricmp(command, "defensive"))
  {

    bot->botCommand = BOT_DEFENSIVE;
    //trap_SendServerCommand(-1, "print \"defensive mode\n\"");

  }
  else if (!Q_stricmp(command, "followprotect"))
  {

    bot->botCommand = BOT_FOLLOW_FRIEND_PROTECT;
    bot->botFriend = master;
    //trap_SendServerCommand(-1, "print \"follow-protect mode\n\"");

  }
  else if (!Q_stricmp(command, "followattack"))
  {

    bot->botCommand = BOT_FOLLOW_FRIEND_ATTACK;
    bot->botFriend = master;
    //trap_SendServerCommand(-1, "print \"follow-attack mode\n\"");

  }
  else if (!Q_stricmp(command, "followidle"))
  {

    bot->botCommand = BOT_FOLLOW_FRIEND_IDLE;
    bot->botFriend = master;
    //trap_SendServerCommand(-1, "print \"follow-idle mode\n\"");

  }
  else if (!Q_stricmp(command, "teamkill"))
  {

    bot->botCommand = BOT_TEAM_KILLER;
    //trap_SendServerCommand(-1, "print \"team kill mode\n\"");

  }
  else
  {

    bot->botCommand = BOT_REGULAR;
    //trap_SendServerCommand(-1, "print \"regular (unknown) mode\n\"");

  }

  return;
}

void
Bot_Buy(gentity_t * self)
{

}

qboolean
botBlockedByBot(gentity_t *self)
{
  gentity_t *traceEnt = NULL;
  trace_t tr;
  vec3_t end, forward;

  AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
  VectorMA(self->client->ps.origin, 75, forward, end);

  trap_Trace(
    &tr, self->client->ps.origin, self->r.mins, self->r.maxs, end, self->s.number, MASK_PLAYERSOLID);
  traceEnt = &g_entities[tr.entityNum];

  if (traceEnt && traceEnt->client && traceEnt->client->pers.teamSelection == PTE_ALIENS)
  {
    //So is blocked lets try the other side
    AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
    VectorMA(self->client->ps.origin, -40, forward, end);

    trap_Trace(
      &tr, self->client->ps.origin, self->r.mins, self->r.maxs, end, self->s.number,
      MASK_PLAYERSOLID);

    traceEnt = &g_entities[tr.entityNum];
    if (traceEnt && traceEnt->client && traceEnt->client->pers.teamSelection == PTE_ALIENS)
    {
      return qtrue;
    }
    else if(tr.fraction == 1.0)
    {
      self->client->ps.delta_angles[1] = ANGLE2SHORT(self->client->ps.delta_angles[1] + 180);
      return qfalse;
    }
    else
    {
      //Hit something.
      return qtrue;
    }
    //self->client->ps.delta_angles[1] = ANGLE2SHORT(self->client->ps.delta_angles[1] + 90);
  }
  else
  {
    return qfalse;
  }

}

qboolean
Bot_Stuck(gentity_t * self, int zone)
{
  if (self->turntime < level.time)
  {
    self->turntime = level.time + 800;
    if (abs(self->posX - self->client->ps.origin[0]) < zone && abs(self->posY
        - self->client->ps.origin[1]) < zone)
    {
      self->posX = self->client->ps.origin[0];
      self->posY = self->client->ps.origin[1];
      self->posZ = self->client->ps.origin[2];
      return qtrue;
    }

    self->posX = self->client->ps.origin[0];
    self->posY = self->client->ps.origin[1];
    self->posZ = self->client->ps.origin[2];
  }
  return qfalse;
}

qboolean
WallBlockingNode(gentity_t * self)
{
  vec3_t forward, right, up, muzzle, end;
  vec3_t nextnode;
  vec3_t top =
  { 0, 0, 0 };
  int vh = 0;
  trace_t tr;
  int distanceNode;
  int distance;

  BG_FindViewheightForClass(self->client->ps.stats[STAT_PCLASS], &vh, NULL);
  top[2] = vh;

  AngleVectors(self->client->ps.viewangles, forward, right, up);
  CalcMuzzlePoint(self, forward, right, up, muzzle);
  VectorMA(muzzle, 10000, forward, end);

  trap_Trace(&tr, muzzle, NULL, NULL, end, self->s.number, MASK_SOLID);

  nextnode[0] = ((-(BLOCKSIZE / 2) + level.pathx[self->botnextpath]) * BLOCKSIZE);
  nextnode[1] = ((-(BLOCKSIZE / 2) + level.pathy[self->botnextpath]) * BLOCKSIZE);
  nextnode[2] = self->s.pos.trBase[2];

  distanceNode = Distance(nextnode, self->s.pos.trBase);

  distance = Distance(self->s.pos.trBase, tr.endpos);
  if (distance < distanceNode)
  {
    return qtrue;
  }
  return qfalse;
}
/*
 void
 G_BotFollowPath(gentity_t * self) {
 vec3_t dirToTarget, angleToTarget;
 vec3_t top = {0, 0, 0};
 int vh = 0;
 int tempEntityIndex = -1;
 int cuadrado;

 int x,y;

 if(self->pathfindthink > level.time ) return;

 if(botReachedDestination(self))
 {
 botStopWalk(self);
 trap_SendServerCommand(-1,
 "print \"^1Objetive Reached\n\"");
 return;
 }

 if(self->timedropnodepath > level.time)
 return;

 if(visitedLastNode(self))
 {
 if(nextNode(self))
 {
 trap_SendServerCommand(-1,
 "print \"Moving to next node  \"");
 aimNode(self);
 botWalk(self);
 }


 trap_SendServerCommand(-1,
 va("print \"Distance to node: %d, X:%f , Y:%f, POSx: %f, POSXy: %f, %d\n\"",
 Distance2d(self->s.origin, self->nextnode),
 ((BLOCKSIZE/2)+(self->s.origin[0]/BLOCKSIZE)),
 ((BLOCKSIZE/2)+(self->s.origin[1]/BLOCKSIZE)),
 self->s.origin[0],
 self->s.origin[1],
 self->botnextpath
 ));

 //botStopWalk(self);
 }
 else
 {
 //Posibility of getting stuck here
 //Bot have run out of place.
 if(botLost(self))
 {
 trap_SendServerCommand(-1,
 va("print \"^1BOTLOST: %f %f %f %f.\n\"", self->nextnode[0],self->nextnode[1],self->s.origin[0],self->s.origin[1]));

 if(canMakeWay(self))
 {
 trap_SendServerCommand(-1,
 "print \"i make a way\n\"");
 aimNode(self);
 botWalk(self);
 self->timedropnodepath = level.time +2000;
 }
 else
 {
 //The bot have complety lost him self
 if(findNodeCanSee(self))
 {
 aimNode(self);
 botWalk(self);
 }
 else
 {
 trap_SendServerCommand(-1,
 "print \"Darn it.\n\"");
 }
 }
 }
 else
 {
 trap_SendServerCommand(-1,
 "print \"Looking for next node\n\"");
 if(Bot_Stuck(self,20))
 {
 trap_SendServerCommand(-1,
 "print \"I stuck\n\"");
 if(findNodeCanSee(self))
 {
 trap_SendServerCommand(-1,
 "print \"Finding a node i can see\n\"");
 }
 else
 {
 if(canMakeWay(self))
 {
 aimNode(self);
 botWalk(self);
 self->client->ps.stats[ STAT_STATE ] |= SS_SPEEDBOOST;
 self->pathfindthink = level.time + 3000;
 }
 else
 {
 trap_SendServerCommand(-1,
 "print \"That just damn sucks.\n\"");
 }
 }
 aimNode(self);
 botWalk(self);

 }
 }
 }
 }
 */

void
botSetEnemy(gentity_t *self, gentity_t *enemy, int entityId)
{
  self->lastTimeSawEnemy = level.time;

  if (enemy)
  {
    self->botEnemy = enemy;
  }
  else
  {
    if (entityId >= 0)
    {
      self->botEnemy = &g_entities[entityId];
    }
  }
  if (self->botEnemy)
  {
    self->botEnemy->lastTimeSeen = level.time;
    if (director_debug.integer)
    {
      G_Printf(
        "%s: new enemy = %s", self->client->pers.netname, self->botEnemy->client->pers.netname);
    }
  }
}
void
G_BotThink(gentity_t * self)
{
  int distance = 0;
  int clicksToStopChase = 30; //5 seconds
  int forwardMove = 127; // max speed
  int tempEntityIndex = -1;
  float chompDistance = 0.0f;

  usercmd_t botCmdBuffer = self->client->pers.cmd;

  botCmdBuffer.forwardmove = 0;
  botCmdBuffer.rightmove = 0;
  botCmdBuffer.upmove = 0;
  botCmdBuffer.buttons = 0;

  self->client->pers.cmd.buttons = 0;
  botWalk(self, 0);
  self->client->pers.cmd.rightmove = 0;

  switch(self->botCommand)
  {
    case BOT_FOLLOW_PATH:
      if (self->botEnemy->health <= 0 || self->botEnemy->client->ps.stats[STAT_HEALTH] <= 0
          || self->botEnemy->client->pers.connected == CON_DISCONNECTED)
      {
        self->botCommand = BOT_REGULAR;
        self->botEnemy = NULL;
        memset(&self->client->pers.cmd, 0, sizeof(self->client->pers.cmd));
        break;
      }
      if (botBlockedByBot(self))
      {
    	//Bot was blocked by other bot, lets jump over it lmao..
    	botJump(self, 127);
    	self->botCommand = BOT_REGULAR;
        //This prevent our bot explode to soon
        self->lastTimeSawEnemy = level.time;

//        self->botCommand = BOT_IDLE;
        return;
      }
      switch(self->botMetaMode)
      {
        case ATTACK_RAMBO:
        case ATTACK_CAMPER:
        case ATTACK_ALL:
        default:
          if (nodes[self->bs.currentNode].type == NODE_DUCK && nodes[self->bs.nextNode].type
              == NODE_JUMP)
          {
            ACEAI_Think(self);
            return;
          }

          tempEntityIndex = botFindClosestEnemy(self, qfalse);
          if (tempEntityIndex >= 0)
          {
            if (director_debug.integer)
            {
              G_Printf("Was following path and i found a enemy around so im gonna attack him.\n");
            }
            botSetEnemy(self, NULL, tempEntityIndex);
            //FIXME: THIS was causing bots aim on the wrong direction.
            //whut is this?
            //memset(&self->client->pers.cmd, 0, sizeof(self->client->pers.cmd));
            self->botCommand = BOT_REGULAR;
            ACEND_setCurrentNode(self, INVALID);
          }
          else
          {
            ACEAI_Think(self);
          }
          break;

          //          G_Printf("WHAT THE SHIT\n");
          //          ACEAI_Think(self);
          //          break;
      }
      break;

    case BOT_CHOMP:
      //self->botCommand = BOT_REGULAR;
      //      self->client->ps.viewangles[PITCH] = AngleNormalize360(0.0f);
      //      self->bs.viewAngles[PITCH] = self->client->ps.viewangles[PITCH];
      //      G_SetClientViewAngle(self, self->client->ps.viewangles);

      botAimAtTarget(self, self->botEnemy);
      //G_BotAimAt( self, self->botEnemy->s.origin, &botCmdBuffer);
      botShootIfTargetInRange(self, self->botEnemy);
      //      VectorNormalize(self->client->ps.velocity);
      chompDistance = Distance(self->s.origin, self->botEnemy->s.origin);
      if (chompDistance > 180)
      {
        self->botCommand = BOT_REGULAR;
      }
      botWalk(self, chompDistance);
      botJump(self, chompDistance);
      botShootIfTargetInRange(self, self->botEnemy);
      //VectorScale(self->client->ps.velocity, chompDistance, self->client->ps.velocity);
      break;

    //This is where the magic happends.
    case BOT_REGULAR:
      if (self->botEnemy)
      {
        if (!botTargetInRange(self, self->botEnemy))
        {
          if (self->botEnemyLastSeen > clicksToStopChase)
          {
            self->botEnemy = NULL;
            self->botEnemyLastSeen = 0;
          }
          else
          {
            self->botEnemyLastSeen++;
          }
        }
        else
        {
          self->botEnemyLastSeen = 0;
        }
      }
      else
      {
        tempEntityIndex = botFindClosestEnemy(self, qfalse);
        if (tempEntityIndex >= 0)
        {
          botSetEnemy(self, NULL, tempEntityIndex);
        }
      }
      if (!self->botEnemy)
      {
        if (level.time % 5000 && (self->client->ps.viewangles[PITCH] != 0.0f
            && self->client->ps.viewangles[PITCH] != 360.0f))
        {
          self->client->ps.viewangles[PITCH] = AngleNormalize360(0.0f);
          self->bs.viewAngles[PITCH] = self->client->ps.viewangles[PITCH];
          G_SetClientViewAngle(self, self->client->ps.viewangles);
        }
        if (Bot_Stuck(self, 30))
        {
          self->client->ps.delta_angles[1] = ANGLE2SHORT(self->client->ps.delta_angles[1] - 45);
        }
        else
        {
          if (WallInFront(self))
          {
            selectBetterWay(self);
          }
        }
        botWalk(self, 60);
      }
      else
      {
        /****************************************************************************
         * WE HAVE AN ENEMY
         ****************************************************************************/

        /****************************************************************************
         * Slow down
         ****************************************************************************/
        if (self->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)
        {
          self->client->pers.hyperspeed = 0;
        }

        /****************************************************************************
         * Aim and walk to it
         ****************************************************************************/
        distance = botGetDistanceBetweenPlayer(self, self->botEnemy);
        //G_BotAimAt( self, self->botEnemy->s.origin, &botCmdBuffer);
        botAimAtTarget(self, self->botEnemy);
        if (distance > 50)
        {
          if (g_survival.integer && (level.time - level.startTime) < 340000)
          {
            botWalk(self, 100);
          }
          else
          {
            botWalk(self, forwardMove);
          }

          if (g_survival.integer && (level.time - level.startTime) < 240000)
          {
            botWalk(self, 70);
          }
          if (g_survival.integer && (level.time - level.startTime) < 120000)
          {
            botWalk(self, 60);
          }
          if (g_survival.integer && (level.time - level.startTime) < 60000)
          {
            botWalk(self, 50);
          }

          if (distance < 200)
          {
            self->client->pers.cmd.buttons |= BUTTON_GESTURE;
          }

          if (!g_survival.integer)
          {
            botWalk(self, forwardMove);
          }
          //botShootIfTargetInRange(self, self->botEnemy);
        }
        if (distance < 65)//45
        {
          botWalk(self, 0);
          botShootIfTargetInRange(self, self->botEnemy);
        }
        if (Bot_Stuck(self, 60) && Distance(self->s.pos.trBase, self->botEnemy->s.pos.trBase) > 80)
        {
          botJump(self, 127);
          self->client->ps.stats[STAT_STAMINA] = MAX_STAMINA;
        }
      }
      break;

    case BOT_IDLE:
      // just stand there and look pretty.
      botWalk(self, 0);
      if (self->idletimer < level.time)
      {
        self->idletimer = level.time + 1500;
        if (botBlockedByBot(self))
        {
          //Stay the same
        }
        else
        {
          //Switch to bot follow path
          self->botCommand = BOT_FOLLOW_PATH;
          self->bs.state = STATE_WANDER;
          botWalk(self, 127);
        }
      }
      break;
    default:
      // dunno.
      break;
  }
}

void
G_BotSpectatorThink(gentity_t * self)
{
  if (self->client->ps.pm_flags & PMF_QUEUED)
  {
    //we're queued to spawn, all good
    //G_LogPrintf( "ClientQUEUED: %i\n", self->client->ps.clientNum );
    return;
  }

  if (self->client->sess.sessionTeam == TEAM_SPECTATOR)
  {
    int teamnum = self->client->pers.teamSelection;
    int clientNum = self->client->ps.clientNum;

    if (teamnum == PTE_HUMANS)
    {
      self->client->pers.classSelection = PCL_HUMAN;
      self->client->ps.stats[STAT_PCLASS] = PCL_HUMAN;
      self->client->pers.humanItemSelection = WP_MACHINEGUN;
      G_PushSpawnQueue(&level.humanSpawnQueue, clientNum);
    }
    else if (teamnum == PTE_ALIENS)
    {
      //ROTAX
      if (g_ambush.integer == 1)
      {
        if (ROTACAK_ambush_rebuild_time_temp < level.time && ((level.time - level.startTime)
            > (g_ambush_sec_to_start.integer * 1000)))
        {
          srand(trap_Milliseconds());

          if (ROTACAK_ambush_stage == 1) //adv granger
          {
            self->client->pers.classSelection = PCL_ALIEN_BUILDER0_UPG;
            self->client->ps.stats[STAT_PCLASS] = PCL_ALIEN_BUILDER0_UPG;
            G_PushSpawnQueue(&level.alienSpawnQueue, clientNum);
          }
          else if (ROTACAK_ambush_stage == 2) //dretch
          {
            self->client->pers.classSelection = PCL_ALIEN_LEVEL0;
            self->client->ps.stats[STAT_PCLASS] = PCL_ALIEN_LEVEL0;
            G_PushSpawnQueue(&level.alienSpawnQueue, clientNum);
          }
          else if (ROTACAK_ambush_stage == 3) //basilisk
          {
            self->client->pers.classSelection = PCL_ALIEN_LEVEL1;
            self->client->ps.stats[STAT_PCLASS] = PCL_ALIEN_LEVEL1;
            G_PushSpawnQueue(&level.alienSpawnQueue, clientNum);
          }
          else if (ROTACAK_ambush_stage == 4) //adv basilisk
          {
            self->client->pers.classSelection = PCL_ALIEN_LEVEL1_UPG;
            self->client->ps.stats[STAT_PCLASS] = PCL_ALIEN_LEVEL1_UPG;
            G_PushSpawnQueue(&level.alienSpawnQueue, clientNum);
          }
          else if (ROTACAK_ambush_stage == 5) //marauder
          {
            self->client->pers.classSelection = PCL_ALIEN_LEVEL2;
            self->client->ps.stats[STAT_PCLASS] = PCL_ALIEN_LEVEL2;
            G_PushSpawnQueue(&level.alienSpawnQueue, clientNum);
          }
          else if (ROTACAK_ambush_stage == 6) //adv marauder
          {
            self->client->pers.classSelection = PCL_ALIEN_LEVEL2_UPG;
            self->client->ps.stats[STAT_PCLASS] = PCL_ALIEN_LEVEL2_UPG;
            G_PushSpawnQueue(&level.alienSpawnQueue, clientNum);
          }
          else if (ROTACAK_ambush_stage == 7) //dragon
          {
            self->client->pers.classSelection = PCL_ALIEN_LEVEL3;
            self->client->ps.stats[STAT_PCLASS] = PCL_ALIEN_LEVEL3;
            G_PushSpawnQueue(&level.alienSpawnQueue, clientNum);
          }
          else if (ROTACAK_ambush_stage == 8) //adv dragon
          {
            self->client->pers.classSelection = PCL_ALIEN_LEVEL3_UPG;
            self->client->ps.stats[STAT_PCLASS] = PCL_ALIEN_LEVEL3_UPG;
            G_PushSpawnQueue(&level.alienSpawnQueue, clientNum);
          }
          else if (ROTACAK_ambush_stage == 9) //tyrant
          {
            self->client->pers.classSelection = PCL_ALIEN_LEVEL4;
            self->client->ps.stats[STAT_PCLASS] = PCL_ALIEN_LEVEL4;
            G_PushSpawnQueue(&level.alienSpawnQueue, clientNum);
          }
        }
      }
      else
      {
        if (level.slowdownTime < level.time)
        {
          self->client->pers.classSelection = PCL_HUMAN;
          self->client->ps.stats[STAT_PCLASS] = PCL_HUMAN;
          self->client->pers.humanItemSelection = WP_ZOMBIE_BITE;
          G_PushSpawnQueue(&level.alienSpawnQueue, clientNum);
        }
        else
        {

        }
      }
    }
  }
}
void VectorToAngles(const vec3_t value1, vec3_t angles);
qboolean
botAimAtTarget(gentity_t * self, gentity_t * target)
{
  vec3_t ideal_angles;
  vec3_t direction;
  vec3_t ideal_view;
  vec3_t enemyOrigin;
  float ideal_yaw;
  float ideal_pitch;
  float current_yaw;
  float current_pitch;
  int cmdAngle;
  //TODO: Detect Ducking?


//  if(Distance(target->s.origin, self->oldEnemyOrigin) == 0)
//  {
//    G_printVector(target->s.origin);
//    G_Printf(" %s havent moved\n", target->client->pers.netname);
//    return qfalse;
//  }
  VectorCopy(target->s.origin, self->oldEnemyOrigin);



  VectorCopy(target->s.origin, enemyOrigin);

  if ((target->client->ps.pm_flags & PMF_DUCKED))
  {
    enemyOrigin[2] -= 16; //min-cmin
  }

  VectorSubtract(enemyOrigin, self->client->ps.origin, direction);

  //ADD this if u dont want the zombie head flip
  /*if (Distance(target->client->ps.origin, self->client->ps.origin) < 50)
   {
   G_Printf("Distance to short \n");
   return qfalse;
   }*/

  VectorNormalize(direction);

  current_yaw = AngleNormalize360(self->client->ps.viewangles[YAW]);
  current_pitch = AngleNormalize360(self->client->ps.viewangles[PITCH]);

  VectorToAngles(direction, ideal_angles);

  ideal_yaw = AngleNormalize360(ideal_angles[YAW]);
  ideal_pitch = AngleNormalize360(ideal_angles[PITCH]);

  // yaw
  if (current_yaw != ideal_yaw)
  {
    ideal_view[YAW] = AngleNormalize360(ideal_yaw);
    self->client->pers.cmd.angles[YAW] = ANGLE2SHORT(ideal_view[YAW]);
    cmdAngle = ANGLE2SHORT(ideal_view[ YAW ]);
    self->client->ps.delta_angles[YAW] = cmdAngle - self->client->pers.cmd.angles[YAW];
  }

  // pitch
  if (current_pitch != ideal_pitch)
  {
    ideal_view[PITCH] = AngleNormalize360(ideal_pitch);
    self->client->pers.cmd.angles[PITCH] = ANGLE2SHORT(ideal_view[PITCH]);
    cmdAngle = ANGLE2SHORT(ideal_view[ PITCH ]);
    self->client->ps.delta_angles[PITCH] = cmdAngle - self->client->pers.cmd.angles[PITCH];
  }

  //VectorCopy(ideal_view, self->s.angles);
  //VectorCopy(self->s.angles, self->client->ps.viewangles);


//  if (viewchanged)
//  {
//    for(i = 0;i < 3;i++)
//    {
//      self->client->pers.cmd.angles[i] = ANGLE2SHORT(ideal_view[i]);
//    }
//    for(i = 0;i < 3;i++)
//    {
//      int cmdAngle;
//
//      cmdAngle = ANGLE2SHORT(ideal_view[ i ]);
//      self->client->ps.delta_angles[i] = cmdAngle - self->client->pers.cmd.angles[i];
//    }
//
//    VectorCopy(ideal_view, self->s.angles);
//    VectorCopy(self->s.angles, self->client->ps.viewangles);
//  }
  return qtrue;
}

qboolean
botTargetInRange2(gentity_t * self, gentity_t * target)
{
  return qfalse;
}

qboolean
botTargetInRange(gentity_t * self, gentity_t * target)
{
  trace_t trace;
  gentity_t *traceEnt;
  vec3_t forward, right, up;
  vec3_t muzzle;
  vec3_t maxs;

  VectorSet(maxs, 10, 10, 15);

  AngleVectors(self->client->ps.viewangles, forward, right, up);
  CalcMuzzlePoint(self, forward, right, up, muzzle);
  //int myGunRange;
  //myGunRange = MGTURRET_RANGE * 3;


  if (!self || !target)
  {
    return qfalse;
  }

  if (!self->client)
    return qfalse;

  if (!target->client)
    return qfalse;
  if (target->health <= 0)
    return qfalse;
  if (target->client->ps.stats[STAT_HEALTH] <= 0)
    return qfalse;
  if (target->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)
    return qfalse;

  //To prevent stop following path when isnt need.
  /*if (target->client->ps.origin[2] - self->client->ps.origin[2] >= 50 && self->botCommand == BOT_FOLLOW_PATH)
   return qfalse;*/

  trap_Trace(&trace, muzzle, self->r.mins, maxs, target->s.pos.trBase, self->s.number, MASK_SHOT);
  traceEnt = &g_entities[trace.entityNum];

  //check our target is in LOS
  if (!(traceEnt == target))
    return qfalse;

  //Are we on the same level?
  if (!g_survival.integer)
  {
    if (self->client->ps.origin[2] - target->client->ps.origin[2] < -30
        && (self->client->ps.stats[STAT_PTEAM] == PTE_ALIENS))
    {
      self->client->pers.cmd.upmove = 30;
      self->client->ps.stats[STAT_STAMINA] = MAX_STAMINA;
      return qfalse;
    }

    if (self->client->ps.origin[2] - target->client->ps.origin[2] < -100
        && (self->client->ps.stats[STAT_PTEAM] == PTE_ALIENS))
    {
      self->client->pers.cmd.upmove = 30;
      self->client->ps.stats[STAT_STAMINA] = MAX_STAMINA;
      return qfalse;
    }
  }
  return qtrue;
}

qboolean
WallInFront(gentity_t * self)
{
  vec3_t forward, right, up, muzzle, end;
  vec3_t top =
  { 0, 0, 0 };
  int vh = 0;
  trace_t tr;
  int distance;

  BG_FindViewheightForClass(self->client->ps.stats[STAT_PCLASS], &vh, NULL);
  top[2] = vh;

  //spawn_angles[0] =0;
  //spawn_angles[1] =90;
  //spawn_angles[2] =0;

  //This is to fix the view angle when facing floor.
  //spawn_angles[ YAW ] += 180.0f;
  //AngleNormalize360( spawn_angles[ YAW ] );
  //G_SetClientViewAngle( self, spawn_angles );

  AngleVectors(self->client->ps.viewangles, forward, right, up);
  CalcMuzzlePoint(self, forward, right, up, muzzle);
  VectorMA(muzzle, 10000, forward, end);

  trap_Trace(&tr, muzzle, NULL, NULL, end, self->s.number, MASK_SOLID);

  distance = Distance(self->s.pos.trBase, tr.endpos);
  if (distance < 90)
  {
    return qtrue;
  }
  return qfalse;
}

void
selectBetterWay(gentity_t * self)
{
  vec3_t forward, right, up, muzzle, end, tempangle;
  vec3_t top =
  { 0, 0, 0 };
  int vh = 0;
  trace_t tr;
  int distance1, distance2, distance3;

  BG_FindViewheightForClass(self->client->ps.stats[STAT_PCLASS], &vh, NULL);
  top[2] = vh;

  //self->client->ps.delta_angles[1] =
  //ANGLE2SHORT(self->client->ps.delta_angles[1] - 30);
  VectorCopy(self->client->ps.viewangles, tempangle);

  self->client->ps.viewangles[1] = ANGLE2SHORT(self->client->ps.viewangles[1] - 60);
  AngleVectors(self->client->ps.viewangles, forward, right, up);
  CalcMuzzlePoint(self, forward, right, up, muzzle);
  VectorMA(muzzle, 1000, forward, end);
  trap_Trace(&tr, muzzle, NULL, NULL, end, self->s.number, MASK_SOLID);
  distance1 = Distance(self->s.pos.trBase, tr.endpos);

  VectorCopy(tempangle, self->client->ps.viewangles);
  self->client->ps.viewangles[1] = ANGLE2SHORT(self->client->ps.viewangles[1] - 120);
  AngleVectors(self->client->ps.viewangles, forward, right, up);
  CalcMuzzlePoint(self, forward, right, up, muzzle);
  VectorMA(muzzle, 1000, forward, end);
  trap_Trace(&tr, muzzle, NULL, NULL, end, self->s.number, MASK_SOLID);
  distance2 = Distance(self->s.pos.trBase, tr.endpos);

  VectorCopy(tempangle, self->client->ps.viewangles);
  self->client->ps.viewangles[1] = ANGLE2SHORT(self->client->ps.viewangles[1] - 180);
  AngleVectors(self->client->ps.viewangles, forward, right, up);
  CalcMuzzlePoint(self, forward, right, up, muzzle);
  VectorMA(muzzle, 1000, forward, end);
  trap_Trace(&tr, muzzle, NULL, NULL, end, self->s.number, MASK_SOLID);
  distance3 = Distance(self->s.pos.trBase, tr.endpos);

  VectorCopy(tempangle, self->client->ps.viewangles);

  if (distance1 > distance2)
  {
    self->client->ps.delta_angles[1] = ANGLE2SHORT(self->client->ps.delta_angles[1] - 60);
  }
  else if (distance2 > distance3)
  {
    self->client->ps.delta_angles[1] = ANGLE2SHORT(self->client->ps.delta_angles[1] - 120);
  }
  else
  {
    self->client->ps.delta_angles[1] = ANGLE2SHORT(self->client->ps.delta_angles[1] - 180);
  }
}

int
botFindClosestEnemy(gentity_t * self, qboolean includeTeam)
{
  // return enemy entity index, or -1
  int vectorRange = MGTURRET_RANGE * 5;
  int i;
  int total_entities;
  int entityList[MAX_GENTITIES];
  vec3_t range;
  vec3_t mins, maxs;
  gentity_t *target;

  int currentNodeType = -1;
  int nextNodeType = -1;
  int lastNodeType = -1;

  if (level.numHumanSpawns < 1)
  {
    //Gonna try to chase enemys in long distances.
    vectorRange = MGTURRET_RANGE * 10;
  }
  if (self->botCommand == BOT_FOLLOW_PATH)
  {
    currentNodeType = nodes[self->bs.currentNode].type;
    nextNodeType = nodes[self->bs.nextNode].type;
    lastNodeType = nodes[self->bs.lastNode].type;

    vectorRange = MGTURRET_RANGE * 1.3;
    if (currentNodeType == NODE_JUMP || nextNodeType == NODE_JUMP || lastNodeType == NODE_JUMP)
      vectorRange = MGTURRET_RANGE / 3;
  }

  VectorSet(range, vectorRange, vectorRange, vectorRange);
  VectorAdd(self->client->ps.origin, range, maxs);
  VectorSubtract(self->client->ps.origin, range, mins);

  total_entities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

  // check list for enemies
  for(i = 0;i < total_entities;i++)
  {
    target = &g_entities[entityList[i]];

    if (target == self)
      continue;
    if (!target->client)
      continue;
    if (target->client->ps.stats[STAT_PTEAM] == self->client->ps.stats[STAT_PTEAM])
      continue;

    if (botTargetInRange(self, target))
    {
      return entityList[i];
    }

  }

  if (includeTeam)
  {
    // check list for enemies in team
    for(i = 0;i < total_entities;i++)
    {
      target = &g_entities[entityList[i]];

      if (target->client && self != target && target->client->ps.stats[STAT_PTEAM]
          == self->client->ps.stats[STAT_PTEAM])
      {
        if (botTargetInRange(self, target))
        {
          return entityList[i];
        }
      }
    }
  }

  return -1;
}

// really an int? what if it's too long?

int
botGetDistanceBetweenPlayer(gentity_t * self, gentity_t * player)
{
  return Distance(self->s.pos.trBase, player->s.pos.trBase);
}

qboolean
botShootIfTargetInRange(gentity_t * self, gentity_t * target)
{
  float distance = 0.0f;

  //botAimAtTarget(self, self->botEnemy);

  self->client->pers.cmd.buttons = 0;
  if (self->client->ps.stats[STAT_PTEAM] == PTE_HUMANS) //Human target buildable
  {
      self->client->pers.cmd.buttons |= BUTTON_ATTACK;
  }
  else
  {
    distance = Distance(self->s.origin, self->botEnemy->s.origin);
    if (distance <= ZOMBIE_RANGE)
    {
      self->client->pers.cmd.buttons |= BUTTON_ATTACK;
    }
  }
  return qtrue;
}
void
botWalk(gentity_t *self, int speed)
{
  char validSpeed;

  validSpeed = ClampChar(speed);

  self->client->pers.cmd.forwardmove = validSpeed;
  if(g_survival.integer)
  {
    //self->client->pers.cmd.forwardmove = validSpeed - 30;
  }
  if (speed <= 64)
  {

    self->client->pers.cmd.buttons |= BUTTON_WALKING;
  }
  else
  {
    self->client->pers.cmd.buttons &= ~BUTTON_WALKING;
  }
}
void
botCrouch(gentity_t *self)
{
  G_Printf("Crouch -1\n");
  self->client->pers.cmd.upmove = -1;
}
void
botJump(gentity_t *self, int speed)
{
  if (self->jumpedTime + 1000 > level.time)
  {
    return;
  }

  self->client->pers.cmd.upmove = speed;
  self->jumpedTime = level.time;
}
qboolean
botCanSeeEnemy(gentity_t * self)
{
  trace_t trace;
  gentity_t *traceEnt;
  vec3_t forward, right, up;
  vec3_t muzzle;

  int maxDistance = 1024;
  int distanceToEnemy;

  AngleVectors(self->client->ps.viewangles, forward, right, up);
  CalcMuzzlePoint(self, forward, right, up, muzzle);

  if (!self || !self->botEnemy)
  {
    return qfalse;
  }
  if (!self->client)
    return qfalse;
  if (!self->botEnemy->client)
    return qfalse;
  if (self->botEnemy->health <= 0)
    return qfalse;
  if (self->botEnemy->client->ps.stats[STAT_HEALTH] <= 0)
    return qfalse;
  if (self->botEnemy->client->ps.stats[STAT_PTEAM] != PTE_HUMANS)
    return qfalse;

  distanceToEnemy = Distance(self->client->ps.origin, self->botEnemy->client->ps.origin);

  if (distanceToEnemy > maxDistance)
    return qfalse;

  trap_Trace(
    &trace, self->client->ps.origin, self->r.mins, self->r.maxs, self->botEnemy->client->ps.origin,
    self->s.number, MASK_SHOT | ~MASK_PLAYERSOLID);
  traceEnt = &g_entities[trace.entityNum];

  //check our target is in LOS
  if (traceEnt == self->botEnemy)
    return qtrue;

  return qfalse;
}
void
botForgetEnemy(gentity_t *self)
{
  if (director_debug.integer)
  {
    G_Printf("Forgotten Enemy");
  }
  self->botEnemy = NULL;
}
void
botSelectEnemy(gentity_t *self)
{
  int i, j;
  gentity_t *ent;
  gentity_t *rambo;
  gentity_t *ent2;
  gentity_t *other;

  ent = rambo = ent2 = other = NULL;

  for(i = level.botslots;i < level.botslots + level.numConnectedClients;i++)
  {
    ent = &g_entities[i];
    if (!ent)
      continue;
    if (!ent->client)
      continue;
    if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
      continue;
    if (ent->client->ps.stats[STAT_PTEAM] != PTE_HUMANS)
      continue;
    if (ent->client->ps.stats[STAT_HEALTH] <= 0 || ent->health <= 0)
      continue;

    other = ent;
    rambo = ent;

    for(j = level.botslots;j < level.botslots + level.numConnectedClients;j++)
    {
      ent2 = &g_entities[j];
      if (!ent2)
        continue;
      if (!ent2->client)
        continue;
      if (i == j)
        continue;
      if (ent2->client->sess.sessionTeam == TEAM_SPECTATOR)
        continue;
      if (ent2->client->ps.stats[STAT_PTEAM] != PTE_HUMANS)
        continue;
      if (ent2->client->ps.stats[STAT_HEALTH] <= 0 || ent2->health <= 0)
        continue;

      //G_Printf("Checking visibility from %s to %s\n", ent->client->pers.netname, ent2->client->pers.netname);
      if (G_Visible(ent, ent2))
      {
        rambo = NULL;
        break;
      }
    }
    if (rambo)
      break;
  }

  if (rambo)
  {
    self->botEnemy = rambo;
  }
  else if (other)
  {
    self->botEnemy = other;
  }
}

//Author: f0rqu3
//Trembot code

//*************************
//*
//* AIMING STUFF
//*
//*************************
void G_BotAimAt( gentity_t *self, vec3_t target, usercmd_t *rAngles)
{
        vec3_t aimVec, oldAimVec, aimAngles;
        vec3_t refNormal, grapplePoint, xNormal, viewBase;
        float turnAngle;
        int i;
        vec3_t forward;

        if( ! (self && self->client) )
                return;

        VectorCopy( self->s.pos.trBase, viewBase );
        viewBase[2] += self->client->ps.viewheight;

        VectorSubtract( target, viewBase, aimVec );
        VectorCopy(aimVec, oldAimVec);

        {
        VectorSet(refNormal, 0.0f, 0.0f, 1.0f);
        VectorCopy( self->client->ps.grapplePoint, grapplePoint);
        //cross the reference normal and the surface normal to get the rotation axis
        CrossProduct( refNormal, grapplePoint, xNormal );
        VectorNormalize( xNormal );
        turnAngle = RAD2DEG( acos( DotProduct( refNormal, grapplePoint ) ) );
        //G_Printf("turn angle: %f\n", turnAngle);
        }

       if( turnAngle != 0.0f)
                RotatePointAroundVector( aimVec, xNormal, oldAimVec, -turnAngle);

        vectoangles( aimVec, aimAngles );

        VectorSet(self->client->ps.delta_angles, 0.0f, 0.0f, 0.0f);

        for( i = 0; i < 3; i++ )
        {
                aimAngles[i] = ANGLE2SHORT(aimAngles[i]);
        }

        AngleVectors( self->client->ps.viewangles, forward, NULL, NULL);

        rAngles->angles[0] = aimAngles[0];
        rAngles->angles[1] = aimAngles[1];
        rAngles->angles[2] = aimAngles[2];
}
