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
//  acebot_nodes.c -   This file contains all of the 
//                     pathing routines for the ACE bot.


#include "g_local.h"
#include "acebot.h"

#if defined(ACEBOT)

// Total number of nodes
int numNodes;

// array for node data
node_t nodes[MAX_NODES];
short int path_table[MAX_NODES][MAX_NODES];

// Determin cost of moving from one node to another
int
ACEND_FindCost(int from, int to)
{
  int curnode;
  int cost = 1; // Shortest possible is 1

  if (from == INVALID || to == INVALID)
    return INVALID;

  // If we can not get there then return invalid
  if (path_table[from][to] == INVALID)
    return INVALID;

  // Otherwise check the path and return the cost
  curnode = path_table[from][to];

  // Find a path (linear time, very fast)
  while(curnode != to)
  {
    curnode = path_table[curnode][to];
    if (curnode == INVALID) // something has corrupted the path abort
      return INVALID;
    cost++;
  }

  return cost;
}

qboolean
G_canSpawn(gentity_t * ent)
{
  vec3_t cmins, cmaxs, localOrigin;
  trace_t tr;

  BG_FindBBoxForClass(PCL_HUMAN, cmins, cmaxs, NULL, NULL, NULL);
//  { -15, -15, -24 }, //vec3_t  mins;
//  { 15, 15, 32 }, //vec3_t  maxs;
  ent->nextSpawnLocation[2]+=5;
  VectorCopy(ent->nextSpawnLocation, localOrigin);
  //localOrigin[2] += fabs(cmins[2]) + 1.0f;

  trap_Trace(&tr, ent->nextSpawnLocation, cmins, cmaxs, localOrigin, ent->s.number, MASK_SHOT);
  if(tr.startsolid)
  {
    return qfalse;
  }
  if (tr.fraction < 1.0)
  {
    return qfalse;
  }
  return qtrue;
}

qboolean
ACEND_FindClosestSpawnNodeToEnemy(gentity_t * self)
{
  vec3_t v;
  float range = 1024.0f;
  float minrange = 128; // Around the corner :>
  float dist;
  int i;
  int closestNode = -1;
  vec3_t enemyOrigin;
  vec3_t nodeOrigin;
  trace_t tr;

  int cost = -1;
  int bestNode = INVALID;
  int bestCost = 9999;

  VectorCopy(self->botEnemy->client->ps.origin, enemyOrigin);
  enemyOrigin[2]+=26;

  closestNode = ACEND_FindClosestReachableNode(self->botEnemy, 300.0f, NODE_MOVE);

  for(i = numNodes - 1;i >= 0;i--)
  {
    if (nodes[i].type != NODE_MOVE)
      continue;

    VectorSubtract(nodes[i].origin, self->botEnemy->client->ps.origin, v); // subtract first
    dist = VectorLength(v);

    if (dist > minrange && dist < range)
    {
      VectorCopy(nodes[i].origin, nodeOrigin);
      nodeOrigin[2] += 32;

      trap_Trace(
        &tr, enemyOrigin, NULL, NULL, nodeOrigin, self->s.number,
        CONTENTS_SOLID | ~MASK_PLAYERSOLID);

      //Not Visible
      if (tr.fraction != 1.0)
      {
        VectorCopy(nodes[i].origin, self->nextSpawnLocation);
        if(G_canSpawn(self))
        {
          cost = ACEND_FindCost(i,closestNode);
          if(cost > 2 && cost < bestCost)
          {
            bestNode = i;
            bestCost = cost;
          }
          //return qtrue;
        }
        else
        {
        }
      }
    }
  }
  if(bestNode != INVALID)
  {
    VectorCopy(nodes[bestNode].origin, self->nextSpawnLocation);
    return qtrue;
  }
  return qfalse;
}

// Find a close node to the player within dist.
//
// Faster than looking for the closest node, but not very 
// accurate.
int
ACEND_FindCloseReachableNode(gentity_t * self, float range, int type)
{
  vec3_t v;
  int i;
  trace_t tr;
  float dist;

  //  range *= range;

  for(i = 0;i < numNodes;i++)
  {
    if (type == NODE_ALL || type == nodes[i].type) // check node type
    {

      VectorSubtract(nodes[i].origin, self->client->ps.origin, v); // subtract first

      dist = VectorLength(v);

      if (dist < range) // square range instead of sqrt
      {
        // make sure it is visible
        trap_Trace(
          &tr, self->client->ps.origin, self->r.mins, self->r.maxs, nodes[i].origin,
          self->s.number, MASK_PLAYERSOLID);

        if (tr.fraction == 1.0)
          return i;
      }
    }
  }

  return -1;
}

// Find the closest node to the player within a certain range
int
ACEND_FindClosestReachableNode(gentity_t * self, float range, int type)
{
  int i;
  float closest = 99999;
  float dist;
  int node = INVALID;
  vec3_t v;
  trace_t tr;
  //float           rng;
  vec3_t maxs, mins;

  VectorCopy(self->r.mins, mins);
  VectorCopy(self->r.maxs, maxs);
  mins[0] = -10;//{ -15, -15, -24 };
  mins[1] = -10;
  mins[2] = -19;

  maxs[0] = 10;//{ 15, 15, 32 };
  maxs[1] = 10;
  maxs[2] = 27;

  if (self->client->ps.pm_flags & PMF_DUCKED)
  {
    maxs[2] = 15;
  }

  mins[2] += STEPSIZE;

  //  rng = (float)(range * range);   // square range for distance comparison (eliminate sqrt)
  for(i = 0;i < numNodes;i++)
  {
    //FIXME: should not be i?
    if (ACEND_nodeInUse(self->bs.currentNode))
    {
      continue;
    }
    if (type == NODE_ALL || type == nodes[i].type) // check node type
    {
      VectorSubtract(nodes[i].origin, self->client->ps.origin, v); // subtract first

      dist = VectorLength(v);

      if (dist < closest && dist < range)
      {
        // make sure it is visible
        if (range == NODE_DENSITY)
          trap_Trace(
            &tr, self->client->ps.origin, mins, maxs, nodes[i].origin, self->s.number, MASK_SOLID);
        else
          trap_Trace(
            &tr, self->client->ps.origin, mins, maxs, nodes[i].origin, self->s.number,
            MASK_PLAYERSOLID);

        if (tr.fraction == 1.0)
        {
          node = i;
          closest = dist;
        }
      }
    }
  }
  return node;
}

void
ACEND_SetGoal(gentity_t * self, int goalNode, int type)
{
  int node;

  self->bs.goalNode = goalNode;

  node = ACEND_FindClosestReachableNode(self, NODE_DENSITY * 3, type);

  if (node == INVALID)
    return;

  if (ace_debug.integer)
    G_Printf("print \"%s: new start node selected %d\n\"", self->client->pers.netname, node);

  ACEND_setCurrentNode(self, node);

  ACEND_setNextNode(self, self->bs.currentNode);
  self->bs.node_timeout = 0;

  if (ace_showPath.integer)
  {
    // draw path to LR goal
    ACEND_DrawPath(self->bs.currentNode, self->bs.goalNode);
  }
}

// Move closer to goal by pointing the bot to the next node
// that is closer to the goal
qboolean
ACEND_FollowPath(gentity_t * self)
{
  // try again?
  if (self->bs.node_timeout++ > 20)
  {
    //    self->client->pers.cmd.forwardmove = 0;
    //    self->client->pers.cmd.upmove = 0;
    //    self->client->pers.cmd.buttons = 0;
    //    self->client->pers.cmd.rightmove = 0;
    //    self->client->pers.cmd.rightmove = 0;
    //    VectorSet(self->client->ps.velocity,0,0,0);
    //self->client->ps.velocity[2] = 0;
    if (self->bs.tries++ > 3)
      return qfalse;
    else
      ACEND_SetGoal(self, self->bs.goalNode, NODE_MOVE);
  }

  // are we there yet?

  //if(Distance(self->client->ps.origin, nodes[self->bs.nextNode].origin) < 32)
  if (BoundsIntersectPoint(self->r.absmin, self->r.absmax, nodes[self->bs.nextNode].origin))
  {
    // reset timeout
    self->bs.node_timeout = 0;

    if (self->bs.nextNode == self->bs.goalNode)
    {
      if (ace_debug.integer)
        G_Printf("print \"%s: reached goal node!\n\"", self->client->pers.netname);

      //      G_Printf("Is %s visible?", self->botEnemy->client->pers.netname);
      if (Distance(nodes[self->bs.goalNode].origin, self->botEnemy->s.origin) < 200)
      {
        self->botCommand = BOT_CHOMP;
      }
      ACEAI_PickLongRangeGoal(self); // pick a new goal
    }
    else
    {
      ////////////////////////////////////////////////////////////////////////////
      // Special Slow Land Jump
      ////////////////////////////////////////////////////////////////////////////
      if (nodes[self->bs.nextNode].type == NODE_JUMP && nodes[self->bs.lastNode].type == NODE_JUMP
          && nodes[self->bs.currentNode].type == NODE_JUMP)
      {
        self->botPause = level.time + 300;
        // VectorSet(self->client->ps.velocity,0,0,0);
        // G_Printf("Stop a bit.\n");
      }
      else if (nodes[self->bs.lastNode].type == NODE_MOVE && nodes[self->bs.nextNode].type
          == NODE_JUMP)
      {
        self->botPause = level.time + 300;
      }

      /*else if(self->botPause+300 > level.time && nodes[self->bs.lastNode].type == NODE_JUMP)
       {
       G_Printf("\n\n\nHAAA\n");
       }*/

      ACEND_selectNextNode(self);
      //self->bs.currentNode = self->bs.nextNode;
      //self->bs.nextNode = path_table[self->bs.currentNode][self->bs.goalNode];
    }
  }

  if (self->bs.currentNode == INVALID || self->bs.nextNode == INVALID)
    return qfalse;

  // set bot's movement vector
  VectorSubtract(nodes[self->bs.nextNode].origin, self->client->ps.origin, self->bs.moveVector);

  return qtrue;
}
qboolean
ACEND_CheckForDucking(gentity_t *self)
{
  vec3_t crouchingMaxs;
  trace_t trace;
  int closestNode;

  if (self->s.groundEntityNum == ENTITYNUM_NONE)
  {
    return qfalse;
  }

  VectorSet(crouchingMaxs, 15, 15, 32);
  if ((self->client->ps.pm_flags & PMF_DUCKED))
  {
    // try to stand up
    trap_Trace(
      &trace, self->s.origin, self->r.mins, crouchingMaxs, self->s.origin, self->s.clientNum,
      MASK_SOLID);
    if (!trace.allsolid)
    {
      //Well the place is crouching... but...
      return qfalse;
    }
    else
    {
      closestNode = ACEND_FindClosestReachableNode(self, NODE_DENSITY, NODE_DUCK);
      if (closestNode != INVALID)
      {
        if (closestNode != self->bs.lastNode && self->bs.lastNode != INVALID)
        {
          ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);
          if (ace_showLinks.integer)
            ACEND_DrawPath(self->bs.lastNode, closestNode);
        }

        self->bs.lastNode = closestNode; // set visited to last
      }
      else if (closestNode == INVALID)
      {
        closestNode = ACEND_AddNode(self, NODE_DUCK);

        if (self->bs.lastNode != INVALID)
        {
          ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);

          if (ace_showLinks.integer)
            ACEND_DrawPath(self->bs.lastNode, closestNode);
        }
        self->bs.lastNode = closestNode; // set visited to last
      }
      return qtrue;
    }
  }
  else
  {
    return qfalse;
  }
}

qboolean
ACEND_CheckForLadder(gentity_t *self)
{
  int closestNode;
  vec3_t forward, end;
  trace_t trace;

  if (self->s.groundEntityNum == ENTITYNUM_NONE && self->client->ps.velocity[2] > 0)
  {
    AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
    forward[2] = 0.0f;

    VectorMA(self->client->ps.origin, 1.0f, forward, end);

    trap_Trace(
      &trace, self->client->ps.origin, self->r.mins, self->r.maxs, end, self->client->ps.clientNum,
      MASK_PLAYERSOLID);

    if ((trace.fraction < 1.0f) && (trace.surfaceFlags & SURF_LADDER))
    {
      closestNode = ACEND_FindClosestReachableNode(self, NODE_DENSITY, NODE_LADDER);
      if (closestNode != INVALID)
      {
        if (closestNode != self->bs.lastNode && self->bs.lastNode != INVALID)
        {
          ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);
          if (ace_showLinks.integer)
            ACEND_DrawPath(self->bs.lastNode, closestNode);
        }

        self->bs.lastNode = closestNode; // set visited to last
      }
      else if (closestNode == INVALID)
      {
        closestNode = ACEND_AddNode(self, NODE_LADDER);
        // now add link
        if (self->bs.lastNode != INVALID)
        {
          ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);

          if (ace_showLinks.integer)
            ACEND_DrawPath(self->bs.lastNode, closestNode);
        }

        self->bs.lastNode = closestNode; // set visited to last
      }
      return qtrue;
    }
  }
  return qfalse;
}

// This routine is called to hook in the pathing code and sets
// the current node if valid.
void
ACEND_PathMap(gentity_t * self)
{
  int closestNode;
  vec3_t v;

  int currentNodeType = -1;
  int nextNodeType = -1;
  int lastNodeType = -1;

  lastNodeType = nodes[self->bs.lastNode].type;
  currentNodeType = nodes[self->bs.currentNode].type;
  nextNodeType = nodes[self->bs.nextNode].type;

  if (!g_survival.integer)
      return;

  if ((self->r.svFlags & SVF_BOT))
  {
    return;
  }

  // don't add links when you went into a trap
  if (self->health <= 0)
    return;

  ////////////////////////////////////////////////////////
  // LADDER
  ///////////////////////////////////////////////////////
  if (ACEND_CheckForLadder(self))
  {
    return;
  }
  ////////////////////////////////////////////////////////////////////////////
  // DUCKING
  ////////////////////////////////////////////////////////////////////////////
  if (ACEND_CheckForDucking(self))
  {
    return;
  }

  ////////////////////////////////////////////////////////
  // JUMPING
  ///////////////////////////////////////////////////////
  if (self->bs.isJumping && self->s.groundEntityNum != ENTITYNUM_NONE)
  {
    self->bs.isJumping = qfalse;
    closestNode = ACEND_FindClosestReachableNode(self, 30, NODE_JUMP);
    if (closestNode != INVALID)
    {
      if (closestNode != self->bs.lastNode && self->bs.lastNode != INVALID)
      {
        ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);
        if (ace_showLinks.integer)
        {
          ACEND_DrawPath(self->bs.lastNode, closestNode);
        }
      }
      self->bs.lastNode = closestNode;
    }
    else if (closestNode == INVALID)
    {
      closestNode = ACEND_AddNode(self, NODE_JUMP);
      if (self->bs.lastNode != INVALID)
      {
        ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);

        if (ace_showLinks.integer)
          ACEND_DrawPath(self->bs.lastNode, closestNode);
      }
      self->bs.lastNode = closestNode; // set visited to last
    }
    return;
  }

  if ((self->client->ps.pm_flags & PMF_JUMP_HELD) && self->s.groundEntityNum == ENTITYNUM_NONE
      && !self->bs.isJumping)
  {
    closestNode = ACEND_FindClosestReachableNode(self, NODE_DENSITY, NODE_JUMP);

    self->bs.isJumping = qtrue;

    if (closestNode != INVALID)
    {
      if (closestNode != self->bs.lastNode && self->bs.lastNode != INVALID)
      {
        ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);
        if (ace_showLinks.integer)
        {
          ACEND_DrawPath(self->bs.lastNode, closestNode);
        }
      }
      self->bs.lastNode = closestNode;
    }
    else if (closestNode == INVALID)
    {
      closestNode = ACEND_AddNode(self, NODE_JUMP);
      if (self->bs.lastNode != INVALID)
      {
        ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);

        if (ace_showLinks.integer)
          ACEND_DrawPath(self->bs.lastNode, closestNode);
      }
      self->bs.lastNode = closestNode; // set visited to last
    }
    return;
  }
  //  if (self->s.groundEntityNum == ENTITYNUM_NONE
  //  //&& (self->client->ps.pm_flags & PMF_JUMP_HELD)
  //      && (self->client->pers.cmd.upmove >= 1) && !self->waterlevel && !(self->r.svFlags & SVF_BOT))
  //  {
  //    // See if there is a closeby jump landing node (prevent adding too many)
  //    closestNode = ACEND_FindClosestReachableNode(self, 64, NODE_JUMP);
  //
  //    if (closestNode != INVALID)
  //    {
  //      // add automatically some links between nodes
  //      if (closestNode != self->bs.lastNode && self->bs.lastNode != INVALID)
  //      {
  //        ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);
  //        if (ace_showLinks.integer)
  //          ACEND_DrawPath(self->bs.lastNode, closestNode);
  //      }
  //
  //      self->bs.lastNode = closestNode; // set visited to last
  //    }
  //    else if (closestNode == INVALID)
  //    {
  //      closestNode = ACEND_AddNode(self, NODE_JUMP);
  //
  //      // now add link
  //      if (self->bs.lastNode != INVALID)
  //      {
  //        ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);
  //
  //        if (ace_showLinks.integer)
  //          ACEND_DrawPath(self->bs.lastNode, closestNode);
  //      }
  //
  //      self->bs.lastNode = closestNode; // set visited to last
  //    }
  //    return;
  //  }

  ////////////////////////////////////////////////////////
  // FLYING
  ///////////////////////////////////////////////////////
  if (self->s.groundEntityNum == ENTITYNUM_NONE && !self->waterlevel)
    return;

  // lava / slime
  VectorCopy(self->client->ps.origin, v);
  v[2] -= 18;

  if (trap_PointContents(self->client->ps.origin, -1) & (CONTENTS_LAVA | CONTENTS_SLIME))
    return;

  // iterate through all nodes to make sure far enough apart
  closestNode = ACEND_FindClosestReachableNode(self, NODE_DENSITY, NODE_ALL);

  if (closestNode != INVALID)
  {
    // add automatically some links between nodes
    if (closestNode != self->bs.lastNode && self->bs.lastNode != INVALID)
    {
      ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);
      if (ace_showLinks.integer)
        ACEND_DrawPath(self->bs.lastNode, closestNode);
    }

    self->bs.lastNode = closestNode; // set visited to last
  }
  else if (closestNode == INVALID && self->s.groundEntityNum != ENTITYNUM_NONE)
  {
    // add nodes in the water as needed
    if (self->waterlevel)
      closestNode = ACEND_AddNode(self, NODE_WATER);
    else
      closestNode = ACEND_AddNode(self, NODE_MOVE);

    // now add link
    if (self->bs.lastNode != INVALID)
    {
      ACEND_UpdateNodeEdge(self->bs.lastNode, closestNode, self);

      if (ace_showLinks.integer)
        ACEND_DrawPath(self->bs.lastNode, closestNode);
    }

    self->bs.lastNode = closestNode; // set visited to last
  }
}

void
ACEND_InitNodes(void)
{
  // init node array (set all to INVALID)
  numNodes = 0;
  memset(nodes, 0, sizeof(node_t) * MAX_NODES);
  memset(path_table, INVALID, sizeof(short int) * MAX_NODES * MAX_NODES);

}

// show the node for debugging (utility function)
void
ACEND_ShowNode(int node, int type)
{
  gentity_t *ent;

  if (!ace_showNodes.integer)
    return;

  ent = G_Spawn();

  ent->s.eType = ET_AI_NODE;
  ent->r.svFlags |= SVF_BROADCAST;

  ent->classname = "ACEND_node";

  VectorCopy(nodes[node].origin, ent->s.origin);
  VectorCopy(nodes[node].origin, ent->s.pos.trBase);

  // otherEntityNum is transmitted with GENTITYNUM_BITS so enough for 1000 nodes
  ent->s.otherEntityNum = node;
  ent->s.constantLight = type;

  //ent->nextthink = level.time + 200000;
  //ent->think = G_FreeEntity;

  trap_LinkEntity(ent);
}

// draws the current path (utility function)
void
ACEND_DrawPath(int currentNode, int goalNode)
{
  int nextNode;

  if (currentNode == INVALID || goalNode == INVALID)
    return;

  nextNode = path_table[currentNode][goalNode];

  // Now set up and display the path
  while(currentNode != goalNode && currentNode != -1)
  {
    gentity_t *ent;

    ent = G_Spawn();

    ent->s.eType = ET_AI_LINK;
    ent->r.svFlags |= SVF_BROADCAST;

    ent->classname = "ACEND_link";

    VectorCopy(nodes[currentNode].origin, ent->s.origin);
    VectorCopy(nodes[currentNode].origin, ent->s.pos.trBase);
    VectorCopy(nodes[nextNode].origin, ent->s.origin2);

    ent->nextthink = level.time + 3000;
    ent->think = G_FreeEntity;

    trap_LinkEntity(ent);

    currentNode = nextNode;
    nextNode = path_table[currentNode][goalNode];
  }
}

// Turns on showing of the path, set goal to -1 to 
// shut off. (utility function)
void
ACEND_ShowPath(gentity_t * self, int goalNode)
{
  int currentNode;

  currentNode = ACEND_FindClosestReachableNode(self, NODE_DENSITY, NODE_ALL);
  if (currentNode == INVALID)
  {
    trap_SendServerCommand(self - g_entities, "print \"no closest reachable node!\n\"");
    return;
  }

  ACEND_DrawPath(currentNode, goalNode);
}

int
ACEND_AddNode(gentity_t * self, int type)
{
  vec3_t v, v2;
  const char *entityName;

  // block if we exceed maximum
  if (numNodes >= MAX_NODES)
    return INVALID;

  entityName = self->classname;

  ////////////////////////////////////////////////////////////////////////////
  // Initialize node.
  ////////////////////////////////////////////////////////////////////////////
  if (self->client)
  {
    VectorCopy(self->client->ps.origin, nodes[numNodes].origin);
  }
  else
  {
    VectorCopy(self->s.origin, nodes[numNodes].origin);
  }

  nodes[numNodes].type = type;
  nodes[numNodes].inuse = qfalse;
  nodes[numNodes].follower = NULL;

  if (type == NODE_JUMPPAD)
  {
    VectorAdd(self->r.absmin, self->r.absmax, v);
    VectorScale(v, 0.5, v);

    // add jumppad target offset
    VectorNormalize2(self->s.origin2, v2);
    VectorMA(v, 32, v2, v);

    VectorCopy(v, nodes[numNodes].origin);
  }

  SnapVector(nodes[numNodes].origin);

  if (ace_debug.integer)
  {
    G_Printf(
      "node %d added for entity %s type: %s pos: %f %f %f\n", numNodes, entityName,
      ACEND_NodeTypeToString(nodes[numNodes].type), nodes[numNodes].origin[0],
      nodes[numNodes].origin[1], nodes[numNodes].origin[2]);

    if (level.num_entities > 800)
    {
      G_Printf("Entity number is to hight not gonna Draw more nodes.\n");
    }
    else
    {
      ACEND_ShowNode(numNodes, nodes[numNodes].type);
    }
  }

  numNodes++;
  return numNodes - 1; // return the node added
}

const char *
ACEND_NodeTypeToString(int type)
{
  if (type == NODE_MOVE)
  {
    return "move";
  }
  else if (type == NODE_PLATFORM)
  {
    return "platform";
  }
  else if (type == NODE_TRIGGER_TELEPORT)
  {
    return "trigger_teleport";
  }
  /*
   else if(type == NODE_TARGET_TELEPORT)
   {
   return "target_teleport";
   }
   */
  else if (type == NODE_ITEM)
  {
    return "item";
  }
  else if (type == NODE_WATER)
  {
    return "water";
  }
  else if (type == NODE_JUMP)
  {
    return "jump";
  }
  else if (type == NODE_JUMPPAD)
  {
    return "jumppad";
  }

  return "BAD";
}

qboolean
ACEND_nodesVisible(vec3_t from, vec3_t to)
{
  trace_t trace;

  trap_Trace(&trace, from, NULL, NULL, to, -1, MASK_SHOT);

  if (trace.contents & CONTENTS_SOLID)
    return qfalse;

  return qtrue;
}

// add / update node connections (paths)
void
ACEND_UpdateNodeEdge(int from, int to, gentity_t *self)
{
  int i;

  if (from == -1 || to == -1 || from == to)
    return; // safety

  if ((self->r.svFlags & SVF_BOT) && (nodes[from].type == NODE_JUMP || nodes[from].type
      == NODE_JUMP || nodes[from].type == NODE_DUCK))
  {
    return;
  }
  // Add the link
  path_table[from][to] = to;

  // Now for the self-referencing part, linear time for each link added
  for(i = 0;i < numNodes;i++)
  {
    if (path_table[i][from] != INVALID)
    {
      if (i == to)
        path_table[i][to] = INVALID; // make sure we terminate

      else
        path_table[i][to] = path_table[i][from];
    }
  }
  if (ace_showLinks.integer)
    G_Printf("print \"Link %d -> %d\n\"", from, to);
}

// remove a node edge
void
ACEND_RemoveNodeEdge(gentity_t * self, int from, int to)
{
  int i;

  if (ace_showLinks.integer)
    G_Printf("print \"%s: removing link %d -> %d\n\"", self->client->pers.netname, from, to);

  path_table[from][to] = INVALID; // set to invalid

  // Make sure this gets updated in our path array
  for(i = 0;i < numNodes;i++)
  {
    if (path_table[from][i] == to)
      path_table[from][i] = INVALID;
  }
}
void
ACEND_selectNextNode(gentity_t *self)
{
  self->bs.lastNode = self->bs.currentNode;
  ACEND_setCurrentNode(self, self->bs.nextNode);
  ACEND_setNextNode(self, path_table[self->bs.currentNode][self->bs.goalNode]);
  //self->bs.nextNode = path_table[self->bs.currentNode][self->bs.goalNode];
}

qboolean
ACEND_pointVisibleFromEntity(vec3_t point, gentity_t *self)
{
  trace_t trace;

  trap_Trace(&trace, self->s.origin, NULL, NULL, point, self->s.number, MASK_SHOT);

  if (trace.fraction < 1.0)
  {
    return qfalse;
  }

  return qtrue;
}
void
ACEND_setCurrentNode(gentity_t *self, int node)
{
  //  if (self->bs.currentNode != INVALID)
  //  {
  //    nodes[self->bs.currentNode].inuse = qfalse;
  //  }
  self->bs.currentNode = node;
  //  if (self->bs.currentNode != INVALID)
  //  {
  //    nodes[self->bs.currentNode].inuse = qtrue;
  //  }
  //self->bs.nextNode = self->bs.currentNode;
}
void
ACEND_setNextNode(gentity_t *self, int node)
{
  if (self->bs.nextNode != INVALID)
  {
    nodes[self->bs.nextNode].inuse = qfalse;
  }
  self->bs.nextNode = node;
  if (self->bs.nextNode != INVALID)
  {
    nodes[self->bs.nextNode].inuse = qtrue;
  }
}

qboolean
ACEND_nodeInUse(int node)
{
  gentity_t *bot = nodes[node].follower;
  if (node == INVALID)
    return qfalse;

  if (nodes[node].inuse == qfalse)
  {
    return qfalse;
  }
  if (bot)
  {
    if ((bot->r.svFlags & SVF_BOT) && bot->health > 0 && bot->client
        && bot->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)
    {
      return qtrue;
    }
    else
    {
      return qfalse;
    }
  }
  return qfalse;
}
#endif
