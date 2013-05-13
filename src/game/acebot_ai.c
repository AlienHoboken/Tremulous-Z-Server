/*
 ===========================================================================
 Copyright (C) 1998 Steve Yeager
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
//  acebot_ai.c -      This file contains all of the
//                     AI routines for the ACE II bot.


#include "g_local.h"
#include "acebot.h"

#if defined(ACEBOT)

void
ACEAI_CheckServerCommands(gentity_t * self)
{

}

void
ACEAI_CheckSnapshotEntities(gentity_t * self)
{
  /*int             sequence, entnum;

   // parse through the bot's list of snapshot entities and scan each of them
   sequence = 0;
   while((entnum = trap_BotGetSnapshotEntity(self - g_entities, sequence++)) >= 0)	// && (entnum < MAX_CLIENTS))
   ;						//BotScanEntity(bs, &g_entities[entnum], &scan, scan_mode);
   */
}

/*
 =============
 ACEAI_Think

 Main Think function for bot
 =============
 */
void
ACEAI_Think(gentity_t * self)
{
  int i;
  int clientNum;
  char userinfo[MAX_INFO_STRING];

  if (!self->bs.state)
  {
    G_Printf("Setting bot State.\n");
    ACESP_SetupBotState(self);
  }

  clientNum = self->client - level.clients;
  trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

  // set up client movement
  VectorCopy(self->client->ps.viewangles, self->bs.viewAngles);
  VectorSet(self->client->ps.delta_angles, 0, 0, 0);

  // FIXME: needed?

  self->client->pers.cmd.buttons = 0;
  botWalk(self, 0);
  self->client->pers.cmd.upmove = 0;
  self->client->pers.cmd.rightmove = 0;

  self->enemy = NULL;
  self->bs.moveTarget = NULL;

  // force respawn
  if (self->health <= 0)
  {
    self->client->buttons = 0;
    self->client->pers.cmd.buttons = BUTTON_ATTACK;
  }

  if (self->bs.state == STATE_WANDER && self->bs.wander_timeout < level.time)
  {
    // pick a new long range goal
    ACEAI_PickLongRangeGoal(self);
  }

  if (self->bs.state == STATE_WANDER)
  {
    if (director_debug.integer)
    {
      G_Printf("Wandering\n");
    }
    ACEMV_Wander(self);
  }
  else if (self->bs.state == STATE_MOVE)
  {
    if (director_debug.integer)
    {
      G_Printf("ACEMV_Move\n");
    }
    ACEMV_Move(self);
  }

  for(i = 0;i < 3;i++)
  {
    self->client->pers.cmd.angles[i] = ANGLE2SHORT(self->bs.viewAngles[i]);
  }
}

/*
 =============
 ACEAI_InFront

 returns 1 if the entity is in front (in sight) of self
 =============
 */
qboolean
ACEAI_InFront(gentity_t * self, gentity_t * other)
{
  vec3_t vec;
  float angle;
  vec3_t forward;

  AngleVectors(self->bs.viewAngles, forward, NULL, NULL);
  VectorSubtract(other->s.origin, self->client->ps.origin, vec);
  VectorNormalize(vec);
  angle = AngleBetweenVectors(vec, forward);

  if (angle <= 85)
    return qtrue;

  return qfalse;
}

/*
 =============
 ACEAI_Visible

 returns 1 if the entity is visible to self, even if not infront ()
 =============
 */
qboolean
ACEAI_Visible(gentity_t * self, gentity_t * other)
{
  vec3_t spot1;
  vec3_t spot2;
  trace_t trace;

  //if(!self->client || !other->client)
  //  return qfalse;

  VectorCopy(self->client->ps.origin, spot1);
  //spot1[2] += self->client->ps.viewheight;

  VectorCopy(other->client->ps.origin, spot2);
  //spot2[2] += other->client->ps.viewheight;

  trap_Trace(&trace, spot1, NULL, NULL, spot2, self->s.number, MASK_PLAYERSOLID);

  if (trace.entityNum == other->s.number)
    return qtrue;

  return qfalse;
}

// Evaluate the best long range goal and send the bot on
// its way. This is a good time waster, so use it sparingly.
// Do not call it for every think cycle.
void
ACEAI_PickLongRangeGoal(gentity_t * self)
{
  int i;
  int node;
  float weight, bestWeight = 0.0f;
  int currentNode, goalNode;
  gentity_t *goalEnt;
  gclient_t *cl;
  gentity_t *player;
  float cost;

  goalNode = INVALID;
  goalEnt = NULL;

  //////////////////////////////////////////////////////////////////
  // LOOK FOR A TARGET
  //////////////////////////////////////////////////////////////////

  //Free currentNode
  currentNode = ACEND_FindClosestReachableNode(self, NODE_DENSITY, NODE_MOVE);
  ACEND_setCurrentNode(self, currentNode);
  ////////////////////////////////////////////////////////////
  // INVALID TARGET
  ///////////////////////////////////////////////////////////
  if (!ace_pickLongRangeGoal.integer || currentNode == INVALID)
  {
    self->bs.state = STATE_WANDER;
    self->bs.wander_timeout = level.time + 1000;
    self->bs.goalNode = -1;
    //G_Printf("There are not good nodes to follow\n");
    return;
  }

  /*if (!ACEND_pointVisibleFromEntity(nodes[currentNode].origin, self))
   {
   G_Printf("\n\nJACK POINT\n\n");
   self->bs.state = STATE_WANDER;
   self->bs.wander_timeout = level.time + 1000;
   self->bs.goalNode = -1;
   G_Printf("There are not good nodes to follow\n");
   return;
   }*/
  // this should be its own function and is for now just
  // finds a player to set as the goal
  for(i = 0;i < g_maxclients.integer;i++)
  {
    cl = level.clients + i;
    player = level.gentities + cl->ps.clientNum;

    if (player == self)
      continue;

    if (cl->pers.connected != CON_CONNECTED)
      continue;

    if (player->health <= 0)
      continue;

    if (player->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)
      continue;

    node = ACEND_FindClosestReachableNode(player, NODE_DENSITY, NODE_ALL);
    cost = ACEND_FindCost(currentNode, node);

    if (cost == INVALID || cost < 3) // ignore invalid and very short hops
      continue;

    weight = 0.3f;

    weight *= random(); // Allow random variations
    weight /= cost; // Check against cost of getting there

    if (weight > bestWeight)
    {
      bestWeight = weight;
      goalNode = node;
      goalEnt = player;
    }
  }

  // if do not find a goal, go wandering....
  if (bestWeight == 0.0f || goalNode == INVALID)
  {
    self->bs.goalNode = INVALID;
    self->bs.state = STATE_WANDER;
    self->bs.wander_timeout = level.time + 1000;
    ACEND_setCurrentNode(self, INVALID);

    if (ace_debug.integer)
      G_Printf("print \"%s: wandering..n\"", self->client->pers.netname);
    return; // no path?
  }

  // OK, everything valid, let's start moving to our goal
  self->bs.state = STATE_MOVE;
  self->bs.tries = 0; // reset the count of how many times we tried this goal

  if (goalEnt != NULL && ace_debug.integer)
    G_Printf(
      "print \"%s: selected a %s at node %d for LR goal\n\"", self->client->pers.netname,
      goalEnt->classname, goalNode);

  ACEND_SetGoal(self, goalNode, NODE_ALL);
}

// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
void
ACEAI_PickShortRangeGoal(gentity_t * self)
{
  gentity_t *target;
  float bestWeight = 0.0f;
  gentity_t *best;
  float shortRange = 200;

  if (!ace_pickShortRangeGoal.integer)
    return;

  best = NULL;

  // look for a target (should make more efficent later)
  target = G_FindRadius(NULL, self->client->ps.origin, shortRange);

  while(target)
  {
    if (target->classname == NULL)
      return; //goto nextTarget;

    if (target == self)
      goto nextTarget;

    // missile avoidance code
    // set our moveTarget to be the rocket or grenade fired at us.
    if (!Q_stricmp(target->classname, "rocket") || !Q_stricmp(target->classname, "grenade"))
    {
      if (ace_debug.integer)
        trap_SendServerCommand(-1, va("print \"%s: ROCKET ALERT!\n\"", self->client->pers.netname));

      best = target;
      bestWeight++;
      break;
    }

#if 0
    // so players can't sneak RIGHT up on a bot
    if(!Q_stricmp(target->classname, "player"))
    {
      if(target->health && !OnSameTeam(self, target))
      {
        best = target;
        bestWeight++;
        break;
      }
    }
#endif

    if (ACEIT_IsReachable(self, target->s.origin))
    {
      if (ACEAI_InFront(self, target))
      {
      }
    }

    // next target
    nextTarget: target = G_FindRadius(target, self->client->ps.origin, shortRange);
  }

  if (bestWeight)
  {
    self->bs.moveTarget = best;

    if (ace_debug.integer && self->bs.goalEntity != self->bs.moveTarget)
      G_Printf(
        "print \"%s: selected a %s for SR goal\n\"", self->client->pers.netname,
        self->bs.moveTarget->classname);

    self->bs.goalEntity = best;
  }
}

// Scan for enemy (simplifed for now to just pick any visible enemy)
qboolean
ACEAI_FindEnemy(gentity_t * self)
{
  int i;
  gclient_t *cl;
  gentity_t *player;
  float enemyRange;
  float bestRange = 99999;

  if (!ace_attackEnemies.integer)
    return qfalse;

  for(i = 0;i < g_maxclients.integer;i++)
  {
    cl = level.clients + i;
    player = level.gentities + cl->ps.clientNum;

    if (player == self)
      continue;

    if (cl->pers.connected != CON_CONNECTED)
      continue;

    if (player->health <= 0)
      continue;

    // don't attack team mates
    if (OnSameTeam(self, player))
      continue;

    enemyRange = Distance(self->client->ps.origin, player->client->ps.origin);

    if (ACEAI_InFront(self, player) && ACEAI_Visible(self, player) && trap_InPVS(
      self->client->ps.origin, player->client->ps.origin) && enemyRange < bestRange)
    {
      /*
       if(ace_debug.integer && self->enemy != player)
       {
       if(self->enemy == NULL)
       trap_SendServerCommand(-1, va("print \"%s: found enemy %s\n\"", self->client->pers.netname, player->client->pers.netname));
       else
       trap_SendServerCommand(-1, va("print \"%s: found better enemy %s\n\"", self->client->pers.netname, player->client->pers.netname));
       }
       */

      self->enemy = player;
      bestRange = enemyRange;
    }
  }

  // FIXME ? bad design
  return self->enemy != NULL;
}

// Hold fire with RL/BFG?
qboolean
ACEAI_CheckShot(gentity_t * self)
{
  trace_t tr;

  trap_Trace(
    &tr, self->client->ps.origin, tv(-8, -8, -8), tv(8, 8, 8), self->enemy->client->ps.origin,
    self->s.number, MASK_SHOT);

  // if blocked, do not shoot
  if (tr.entityNum == self->enemy->s.number)
    return qtrue;

  return qfalse;
}

// Choose the best weapon for bot (simplified)
void
ACEAI_ChooseWeapon(gentity_t * self)
{
  float range;
  vec3_t v;

  // if no enemy, then what are we doing here?
  if (!self->enemy)
    return;

  // base selection on distance
  VectorSubtract(self->client->ps.origin, self->enemy->client->ps.origin, v);
  range = VectorLength(v);

#ifdef MISSIONPACK
  // only use CG when ammo > 50
  if(self->client->pers.inventory[ITEMLIST_BULLETS] >= 50)
  if(ACEIT_ChangeWeapon(self, FindItem("chaingun")))
  return;
#endif

  if (ACEIT_ChangeWeapon(self, WP_SHOTGUN))
    return;

  if (ACEIT_ChangeWeapon(self, WP_MACHINEGUN))
    return;

  return;
}

#endif
