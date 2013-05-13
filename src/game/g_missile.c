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

#define MISSILE_PRESTEP_TIME  50

#ifndef RAND_MAX
#define RAND_MAX 32768
#endif

#define GRENADE_SHRAPNEL_COUNT        10
#define GRENADE_SHRAPNEL_RANGE        1200
#define GRENADE_SHRAPNEL_MAG       1000
#define GRENADE_SHRAPNEL_DAMAGE		300

/*
 ================
 G_BounceMissile

 ================
 */
void
G_BounceMissile(gentity_t *ent, trace_t *trace)
{
  vec3_t velocity;
  float dot;
  int hitTime;

  // reflect the velocity on the trace plane
  hitTime = level.previousTime + (level.time - level.previousTime) * trace->fraction;
  BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
  dot = DotProduct(velocity, trace->plane.normal);
  VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

  if (ent->s.eFlags & EF_BOUNCE_HALF)
  {
    VectorScale(ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta);
    // check for stop
    if (trace->plane.normal[2] > 0.2 && VectorLength(ent->s.pos.trDelta) < 40)
    {
      ent->s.eFlags |= EF_NO_BOUNCE_SOUND;
      G_SetOrigin(ent, trace->endpos);
      return;
    }
  }

  VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
  VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
  ent->s.pos.trTime = level.time;
}

/* SEX HAX NADE
 * 
 * */
void
G_ExplodeMissile(gentity_t *ent);

gentity_t *
fire_shrapnel(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "pistol";
  bolt->nextthink = level.time + 10000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_PISTOL;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->r.ownerNum;
  bolt->r.contents = 0;
  bolt->parent = self;
  bolt->damage = GRENADE_SHRAPNEL_DAMAGE;
  bolt->splashDamage = 0;
  bolt->splashRadius = 0;
  bolt->methodOfDeath = MOD_GRENADE;
  bolt->splashMethodOfDeath = MOD_GRENADE;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on

  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(dir, 100, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth

  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;
}

void
G_Explodefragnade(gentity_t *ent, vec3_t origin)
{
  vec3_t pos, dir;
  long i;
  //int damage;
  float mag;
  trace_t tr;
  gentity_t *target;
  srand(trap_Milliseconds());
  for(i = 0;i < GRENADE_SHRAPNEL_COUNT;i++)
  {
    gentity_t *shrapnel;
    dir[0] = (int) (((double) rand() / ((double) (RAND_MAX) + (double) (1))) * GRENADE_SHRAPNEL_MAG) - (GRENADE_SHRAPNEL_MAG / 2);
    dir[1] = (int) (((double) rand() / ((double) (RAND_MAX) + (double) (1))) * GRENADE_SHRAPNEL_MAG) - (GRENADE_SHRAPNEL_MAG / 2);
    dir[2] = (int) (((double) rand() / ((double) (RAND_MAX) + (double) (1))) * GRENADE_SHRAPNEL_MAG) - (GRENADE_SHRAPNEL_MAG / 2);
    //VectorNormalize( dir );
    mag = abs(dir[0]) + abs(dir[1]) + abs(dir[2]);
    dir[0] = dir[0] / mag;
    dir[1] = dir[1] / mag;
    dir[2] = dir[2] / mag;
    VectorMA(origin, GRENADE_SHRAPNEL_RANGE, dir, pos);
    //trap_Trace( &tr, origin, NULL, NULL, pos, ent->r.ownerNum, MASK_SHOT );
    target = &g_entities[tr.entityNum];
    //damage = Distance(tr.endpos,origin) / GRENADE_SHRAPNEL_RANGE * GRENADE_SHRAPNEL_DAMAGE;
    shrapnel = fire_shrapnel(ent, origin, dir);
    //if(target->health > 0 && target->takedamage)
    //{
    //    G_Damage( target, ent, ent, dir, tr.endpos, damage, 0, MOD_GRENADE );
    //}
  }
}

/*
 ================
 G_ExplodeMissile

 Explode a missile without an impact
 ================
 */
void
G_ExplodeMissile(gentity_t *ent)
{
  vec3_t dir;
  vec3_t origin;

  BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
  SnapVector(origin);
  G_SetOrigin(ent, origin);

  // we don't have a valid direction, so just point straight up
  dir[0] = dir[1] = 0;
  dir[2] = 1;

  ent->s.eType = ET_GENERAL;

  //TA: tired... can't be fucked... hack
  if (ent->s.weapon != WP_LOCKBLOB_LAUNCHER)
    G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(dir));

  /*	if( ent->s.weapon == WP_BOMB )
   {
   G_Explodefragnade(ent, origin);
   return;
   }*/

  ent->freeAfterEvent = qtrue;

  // splash damage
  if (ent->splashDamage)
    G_RadiusDamage(ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->splashMethodOfDeath);

  trap_LinkEntity(ent);
}

void
AHive_ReturnToHive(gentity_t *self);

/*
 ================
 G_MissileImpact

 ================
 */
void
G_MissileImpact(gentity_t *ent, trace_t *trace)
{
  gentity_t *other, *attacker;
  vec3_t dir;

  other = &g_entities[trace->entityNum];
  attacker = &g_entities[ent->r.ownerNum];

  if (ent->s.weapon == WP_MINE)
  {
    G_BounceMissile(ent, trace);

    //only play a sound if requested
    if (!(ent->s.eFlags & EF_NO_BOUNCE_SOUND))
      G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);

    return;
  }
  else if (!other->takedamage && (ent->s.eFlags & (EF_BOUNCE | EF_BOUNCE_HALF)))
  {
    G_BounceMissile(ent, trace);

    //only play a sound if requested
    if (!(ent->s.eFlags & EF_NO_BOUNCE_SOUND))
      G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);

    return;
  }

  if (!strcmp(ent->classname, "grenade"))
  {
    //grenade doesn't explode on impact
    G_BounceMissile(ent, trace);

    //only play a sound if requested
    if (!(ent->s.eFlags & EF_NO_BOUNCE_SOUND))
      G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);

    return;
  }
  else if (!strcmp(ent->classname, "lockblob"))
  {
    if (other->client && other->client->ps.stats[STAT_PTEAM] == PTE_HUMANS)
    {
      other->client->ps.stats[STAT_STATE] |= SS_BLOBLOCKED;
      other->client->lastLockTime = level.time;
      AngleVectors(other->client->ps.viewangles, dir, NULL, NULL);
      other->client->ps.stats[STAT_VIEWLOCK] = DirToByte(dir);
    }
  }
  else if (!strcmp(ent->classname, "slowblob"))
  {
    if (other->client && other->client->ps.stats[STAT_PTEAM] == PTE_HUMANS)
    {
      other->client->ps.stats[STAT_STATE] |= SS_SLOWLOCKED;
      other->client->lastSlowTime = level.time;
      AngleVectors(other->client->ps.viewangles, dir, NULL, NULL);
      other->client->ps.stats[STAT_VIEWLOCK] = DirToByte(dir);
    }
  }
  else if (!strcmp(ent->classname, "hive"))
  {
    if (other->s.eType == ET_BUILDABLE && other->s.modelindex == BA_A_HIVE)
    {
      if (!ent->parent)
        G_Printf(S_COLOR_YELLOW "WARNING: hive entity has no parent in G_MissileImpact\n");
      else
        ent->parent->active = qfalse;

      G_FreeEntity(ent);
      return;
    }
    else
    {
      //prevent collision with the client when returning
      ent->r.ownerNum = other->s.number;

      ent->think = AHive_ReturnToHive;
      ent->nextthink = level.time + FRAMETIME;

      //      //only damage humans
      //      if (other->client && other->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS)
      //        returnAfterDamage = qtrue;
      //      else
      //        return;
    }
  }

  // impact damage
  if (other->takedamage)
  {
    // FIXME: wrong damage direction?
    if (ent->damage)
    {
      vec3_t velocity;

      BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
      if (VectorLength(velocity) == 0)
        velocity[2] = 1; // stepped on a grenade

      G_Damage(other, ent, attacker, velocity, ent->s.origin, ent->damage, 0, ent->methodOfDeath);
    }
  }

  //  if (returnAfterDamage)
  //    return;

  // is it cheaper in bandwidth to just remove this ent and create a new
  // one, rather than changing the missile into the explosion?

  if (other->takedamage && other->client)
  {
    G_AddEvent(ent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
    ent->s.otherEntityNum = other->s.number;
  }
  else if (trace->surfaceFlags & SURF_METALSTEPS)
    G_AddEvent(ent, EV_MISSILE_MISS_METAL, DirToByte(trace->plane.normal));
  else
  {
    if (other->r.svFlags & SVF_BOT)
    {
      G_AddEvent(ent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
      if(ent->s.weapon == WP_AXE && ent->s.generic1 == WPM_SECONDARY)
      {
        return;
      }
    }
    else
    {
      G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));
    }
  }

  ent->freeAfterEvent = qtrue;

  // change over to a normal entity right at the point of impact
  ent->s.eType = ET_GENERAL;

  SnapVectorTowards(trace->endpos, ent->s.pos.trBase); // save net bandwidth

  G_SetOrigin(ent, trace->endpos);

  // splash damage (doesn't apply to person directly hit)
  if (ent->splashDamage)
    G_RadiusDamage(trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, other, ent->splashMethodOfDeath);

  trap_LinkEntity(ent);
}

/*
 ================
 G_RunMissile

 ================
 */
void
G_RunMissile(gentity_t *ent)
{
  vec3_t origin;
  trace_t tr;
  int passent;

  // get current position
  BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

  // ignore interactions with the missile owner
  passent = ent->r.ownerNum;

  // trace a line from the previous position to the current position
  trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask);

  if (tr.startsolid || tr.allsolid)
  {
    // make sure the tr.entityNum is set to the entity we're stuck in
    trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask);
    tr.fraction = 0;
  }
else    VectorCopy(tr.endpos, ent->r.currentOrigin);

    ent->r.contents = CONTENTS_SOLID; //trick trap_LinkEntity into...
    trap_LinkEntity(ent);
    ent->r.contents = 0; //...encoding bbox information

    if (tr.fraction != 1)
    {
      // never explode or bounce on sky
      if (tr.surfaceFlags & SURF_NOIMPACT)
      {
        // If grapple, reset owner
        if (ent->parent && ent->parent->client && ent->parent->client->hook == ent)
        ent->parent->client->hook = NULL;

        G_FreeEntity(ent);
        return;
      }

      G_MissileImpact(ent, &tr);
      if (ent->s.eType != ET_MISSILE)
      return; // exploded
    }

    // check think function after bouncing
    G_RunThink(ent);
  }

  //=============================================================================

  /*
   =================
   fire_flamer

   =================
   */
gentity_t *
fire_flamer(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;
  vec3_t pvel;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "flame";
  bolt->nextthink = level.time + FLAMER_LIFETIME;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
//  bolt->s.weapon = WP_FLAMER;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = FLAMER_DMG;
  bolt->splashDamage = FLAMER_DMG;
  bolt->splashRadius = FLAMER_RADIUS;
//  bolt->methodOfDeath = MOD_FLAMER;
//  bolt->splashMethodOfDeath = MOD_FLAMER_SPLASH;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[0] = bolt->r.mins[1] = bolt->r.mins[2] = -15.0f;
  bolt->r.maxs[0] = bolt->r.maxs[1] = bolt->r.maxs[2] = 15.0f;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(self->client->ps.velocity, FLAMER_LAG, pvel);
  VectorMA(pvel, FLAMER_SPEED, dir, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth

  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;
}
/*
 =================
 fire_pulseRifle

 =================
 */
gentity_t *
fire_pulseRifle(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "pulse";
  bolt->nextthink = level.time + 10000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  //bolt->s.weapon = WP_PULSE_RIFLE;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = PRIFLE_DMG;
  bolt->splashDamage = 0;
  bolt->splashRadius = 0;
//  bolt->methodOfDeath = MOD_PRIFLE;
//  bolt->splashMethodOfDeath = MOD_PRIFLE;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(dir, PRIFLE_SPEED, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth

  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;
}

//=============================================================================

/*
 =================
 fire_luciferCannon

 =================
 */
gentity_t *
fire_luciferCannon(gentity_t *self, vec3_t start, vec3_t dir, int damage, int radius)
{
  gentity_t *bolt;
  int localDamage = (int) (ceil(((float) damage / (float) LCANNON_TOTAL_CHARGE) * (float) LCANNON_DAMAGE));

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "lcannon";

  if (damage == LCANNON_TOTAL_CHARGE)
    bolt->nextthink = level.time;
  else
    bolt->nextthink = level.time + 10000;

  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
//  bolt->s.weapon = WP_LUCIFER_CANNON;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = localDamage;
  bolt->splashDamage = localDamage / 2;
  bolt->splashRadius = radius;
//  bolt->methodOfDeath = MOD_LCANNON;
//  bolt->splashMethodOfDeath = MOD_LCANNON_SPLASH;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(dir, LCANNON_SPEED, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth

  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;
}

/*
 =================
 launch_grenade

 =================
 */
gentity_t *
launch_grenade(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "grenade";
  bolt->nextthink = level.time + 5000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_GRENADE;
  bolt->s.eFlags = EF_BOUNCE_HALF;
  bolt->s.generic1 = WPM_PRIMARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = GRENADE_DAMAGE;
  bolt->splashDamage = GRENADE_DAMAGE;
  bolt->splashRadius = GRENADE_RANGE;
  bolt->methodOfDeath = MOD_GRENADE;
  bolt->splashMethodOfDeath = MOD_GRENADE;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[0] = bolt->r.mins[1] = bolt->r.mins[2] = -3.0f;
  bolt->r.maxs[0] = bolt->r.maxs[1] = bolt->r.maxs[2] = 3.0f;
  bolt->s.time = level.time;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(dir, GRENADE_SPEED, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth

  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;
}

gentity_t *
launch_bomb(gentity_t *self, vec3_t start, vec3_t dir)
{
  return NULL;
  /* gentity_t *bolt;

   VectorNormalize( dir );

   bolt = G_Spawn( );
   bolt->classname = "bomb";
   bolt->nextthink = level.time + 5000;
   bolt->think = G_ExplodeMissile;
   bolt->s.eType = ET_MISSILE;
   bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
   bolt->s.weapon = WP_BOMB;
   bolt->s.eFlags = EF_BOUNCE_HALF;
   bolt->s.generic1 = WPM_PRIMARY; //weaponMode
   bolt->r.ownerNum = self->s.number;
   bolt->parent = self;
   bolt->damage = GRENADE_DAMAGE;
   bolt->splashDamage = GRENADE_DAMAGE;
   bolt->splashRadius = GRENADE_RANGE;
   bolt->methodOfDeath = MOD_GRENADE;
   bolt->splashMethodOfDeath = MOD_GRENADE;
   bolt->clipmask = MASK_SHOT;
   bolt->target_ent = NULL;
   bolt->r.mins[ 0 ] = bolt->r.mins[ 1 ] = bolt->r.mins[ 2 ] = -3.0f;
   bolt->r.maxs[ 0 ] = bolt->r.maxs[ 1 ] = bolt->r.maxs[ 2 ] = 3.0f;
   bolt->s.time = level.time;

   bolt->s.pos.trType = TR_GRAVITY;
   bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
   VectorCopy( start, bolt->s.pos.trBase );
   VectorScale( dir, GRENADE_SPEED, bolt->s.pos.trDelta );
   SnapVector( bolt->s.pos.trDelta );      // save net bandwidth

   VectorCopy( start, bolt->r.currentOrigin );

   return bolt;*/
}
//=============================================================================

/*
 ================
 AHive_ReturnToHive

 Adjust the trajectory to point towards the hive
 ================
 */
void
AHive_ReturnToHive(gentity_t *self)
{
  vec3_t dir;
  trace_t tr;

  if (!self->parent)
  {
    G_Printf(S_COLOR_YELLOW "WARNING: AHive_ReturnToHive called with no self->parent\n");
    return;
  }

  trap_UnlinkEntity(self->parent);
  trap_Trace(&tr, self->r.currentOrigin, self->r.mins, self->r.maxs, self->parent->r.currentOrigin, self->r.ownerNum, self->clipmask);
  trap_LinkEntity(self->parent);

  if (tr.fraction < 1.0f)
  {
    //if can't see hive then disperse
    VectorCopy(self->r.currentOrigin, self->s.pos.trBase);
    self->s.pos.trType = TR_STATIONARY;
    self->s.pos.trTime = level.time;

    self->think = G_ExplodeMissile;
    self->nextthink = level.time + 2000;
    self->parent->active = qfalse; //allow the parent to start again
  }
  else
  {
    VectorSubtract(self->parent->r.currentOrigin, self->r.currentOrigin, dir);
    VectorNormalize(dir);

    //change direction towards the hive
    VectorScale(dir, HIVE_SPEED, self->s.pos.trDelta);
    SnapVector(self->s.pos.trDelta); // save net bandwidth
    VectorCopy(self->r.currentOrigin, self->s.pos.trBase);
    self->s.pos.trTime = level.time;

    self->think = G_ExplodeMissile;
    self->nextthink = level.time + 15000;
  }
}

/*
 ================
 AHive_SearchAndDestroy

 Adjust the trajectory to point towards the target
 ================
 */
void
AHive_SearchAndDestroy(gentity_t *self)
{
  vec3_t dir;
  trace_t tr;

  trap_Trace(&tr, self->r.currentOrigin, self->r.mins, self->r.maxs, self->target_ent->r.currentOrigin, self->r.ownerNum, self->clipmask);

  //if there is no LOS or the parent hive is too far away or the target is dead or notargeting, return
  if (tr.entityNum == ENTITYNUM_WORLD || Distance(self->r.currentOrigin, self->parent->r.currentOrigin) > (HIVE_RANGE * 5) || self->target_ent->health <= 0
      || self->target_ent->flags & FL_NOTARGET)
  {
    self->r.ownerNum = ENTITYNUM_WORLD;

    self->think = AHive_ReturnToHive;
    self->nextthink = level.time + FRAMETIME;
  }
  else
  {
    VectorSubtract(self->target_ent->r.currentOrigin, self->r.currentOrigin, dir);
    VectorNormalize(dir);

    //change direction towards the player
    VectorScale(dir, HIVE_SPEED, self->s.pos.trDelta);
    SnapVector(self->s.pos.trDelta); // save net bandwidth
    VectorCopy(self->r.currentOrigin, self->s.pos.trBase);
    self->s.pos.trTime = level.time;

    self->nextthink = level.time + HIVE_DIR_CHANGE_PERIOD;
  }
}

/*
 =================
 fire_hive
 =================
 */
gentity_t *
fire_hive(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "hive";
  bolt->nextthink = level.time + HIVE_DIR_CHANGE_PERIOD;
  bolt->think = AHive_SearchAndDestroy;
  bolt->s.eType = ET_MISSILE;
  bolt->s.eFlags |= EF_BOUNCE | EF_NO_BOUNCE_SOUND;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_HIVE;
  bolt->s.generic1 = WPM_PRIMARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = HIVE_DMG;
  bolt->splashDamage = 0;
  bolt->splashRadius = 0;
  bolt->methodOfDeath = MOD_SWARM;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = self->target_ent;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(dir, HIVE_SPEED, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth
  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;
}

//=============================================================================

/*
 =================
 fire_lockblob
 =================
 */
gentity_t *
fire_lockblob(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "lockblob";
  bolt->nextthink = level.time + 15000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_LOCKBLOB_LAUNCHER;
  bolt->s.generic1 = WPM_PRIMARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = 0;
  bolt->splashDamage = 0;
  bolt->splashRadius = 0;
  bolt->methodOfDeath = MOD_UNKNOWN; //doesn't do damage so will never kill
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(dir, 500, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth
  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;
}

/*
 =================
 fire_slowBlob
 =================
 */
gentity_t *
fire_slowBlob(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "slowblob";
  bolt->nextthink = level.time + 15000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_ABUILD2;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = ABUILDER_BLOB_DMG;
  bolt->splashDamage = 0;
  bolt->splashRadius = 0;
  bolt->methodOfDeath = MOD_SLOWBLOB;
  bolt->splashMethodOfDeath = MOD_SLOWBLOB;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(dir, ABUILDER_BLOB_SPEED, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth
  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;
}

/*
 =================
 fire_paraLockBlob
 =================
 */
gentity_t *
fire_paraLockBlob(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "lockblob";
  bolt->nextthink = level.time + 15000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_LOCKBLOB_LAUNCHER;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = 0;
  bolt->splashDamage = 0;
  bolt->splashRadius = 0;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(dir, LOCKBLOB_SPEED, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth
  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;
}

/*
 =================
 fire_bounceBall
 =================
 */
gentity_t *
fire_bounceBall(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "bounceball";
  bolt->nextthink = level.time + 3000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_ALEVEL3_UPG;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = LEVEL3_BOUNCEBALL_DMG;
  bolt->splashDamage = 0;
  bolt->splashRadius = 0;
//  bolt->methodOfDeath = MOD_LEVEL3_BOUNCEBALL;
//  bolt->splashMethodOfDeath = MOD_LEVEL3_BOUNCEBALL;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(dir, LEVEL3_BOUNCEBALL_SPEED, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth
  VectorCopy(start, bolt->r.currentOrigin);
  /*bolt->s.eFlags |= EF_BOUNCE;*/

  return bolt;
}

gentity_t *
drawRedBall(gentity_t *ent, int x, int y)
{

  gentity_t *bolt;
  vec3_t start;

  start[0] = convertGridToWorld(x);
  start[1] = convertGridToWorld(y);
  start[2] = ent->client->ps.origin[2] + 30;//Over the head.

  bolt = G_Spawn();
  bolt->classname = "pulse";
  bolt->nextthink = level.time + 10000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  //bolt->s.weapon = WP_PULSE_RIFLE;
  bolt->s.generic1 = ent->s.generic1; //weaponMode
  bolt->r.ownerNum = ent->s.number;
  bolt->parent = ent;
  bolt->damage = PRIFLE_DMG;
  bolt->splashDamage = 0;
  bolt->splashRadius = 0;
//  bolt->methodOfDeath = MOD_PRIFLE;
//  bolt->splashMethodOfDeath = MOD_PRIFLE;
  bolt->clipmask = MASK_WATER;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(start, 0, bolt->s.pos.trDelta);

  SnapVector(bolt->s.pos.trDelta); // save net bandwidth

  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;
}
gentity_t *
launch_grenade_primary(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "grenade2";
  bolt->nextthink = level.time + 5000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_GRENADE;
  //bolt->s.eFlags = EF_BOUNCE_HALF;
  bolt->s.generic1 = WPM_PRIMARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = LAUNCHER_DAMAGE;
  bolt->splashDamage = LAUNCHER_DAMAGE;
  bolt->splashRadius = LAUNCHER_RADIUS;
  bolt->methodOfDeath = MOD_GRENADE_LAUNCHER;
  bolt->splashMethodOfDeath = MOD_GRENADE_LAUNCHER;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[0] = bolt->r.mins[1] = bolt->r.mins[2] = -3.0f;
  bolt->r.maxs[0] = bolt->r.maxs[1] = bolt->r.maxs[2] = 3.0f;
  bolt->s.time = level.time;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, LAUNCHER_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta ); // save net bandwidth

  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

gentity_t *
launch_grenade_secondary(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "grenade2";
  bolt->nextthink = level.time + 5000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_GRENADE;
  //bolt->s.eFlags = EF_BOUNCE_HALF;
  bolt->s.generic1 = WPM_SECONDARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = ONFIRE_EXPLOSION_DAMAGE;
  bolt->splashDamage = ONFIRE_EXPLOSION_DAMAGE;
  bolt->splashRadius = INCENDIARY_GRENADE_RANGE;
  bolt->methodOfDeath = MOD_GRENADE_LAUNCHER_INCENDIARY;
  bolt->splashMethodOfDeath = MOD_GRENADE_LAUNCHER_INCENDIARY;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[0] = bolt->r.mins[1] = bolt->r.mins[2] = -3.0f;
  bolt->r.maxs[0] = bolt->r.maxs[1] = bolt->r.maxs[2] = 3.0f;
  bolt->s.time = level.time;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, LAUNCHER_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta ); // save net bandwidth

  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

gentity_t *
fire_axe(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "axe";
  bolt->nextthink = level.time + 50000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_AXE;
  //bolt->s.eFlags = EF_BOUNCE_HALF;
  bolt->s.generic1 = WPM_SECONDARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = AXE_DAMAGE;
  bolt->splashDamage = 0;
  bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_AXE;
	bolt->splashMethodOfDeath = MOD_AXE;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[0] = bolt->r.mins[1] = bolt->r.mins[2] = -10.0f;
  bolt->r.maxs[0] = bolt->r.maxs[1] = bolt->r.maxs[2] = 10.0f;
  bolt->s.time = level.time;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, AXE_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta ); // save net bandwidth

  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}


void
G_domethink(gentity_t *ent)
{
  vec3_t origin, mins ,maxs;
  gentity_t *otherEnt;
  int radius, numListedEntities;
  int entityList[MAX_GENTITIES];
  int e,i;
  float dist;
  vec3_t v;

  ent->nextthink = level.time + 1000;

  if(level.time > ent->dieTime)
  {
    ent->freeAfterEvent = qtrue;
    ent->parent->numDomes -= 1;
    return;
  }

  VectorCopy(ent->s.origin, origin);
  radius = DOME_RANGE;

  for(i = 0;i < 3;i++)
  {
    mins[i] = origin[i] - radius;
    maxs[i] = origin[i] + radius;
  }

  numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

  for(e = 0;e < numListedEntities;e++)
  {
    otherEnt = &g_entities[entityList[e]];

    if(!otherEnt)
      continue;
    if(!otherEnt->client)
      continue;
    if (otherEnt->client->sess.sessionTeam == TEAM_SPECTATOR)
      continue;
    if(otherEnt->health <= 0)
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


    if(otherEnt->client->ps.stats[STAT_PTEAM] != PTE_HUMANS)
    {
      otherEnt->client->ps.stats[STAT_STATE] |= SS_CREEPSLOWED;
      continue;
    }

    if (otherEnt->health < 100)
    {
      otherEnt->health = otherEnt->client->ps.stats[STAT_HEALTH]
          = otherEnt->health + 3;
		if ( (otherEnt->client->pers.badges[ 18 ] != 1) && (otherEnt->health >= 75) && otherEnt->client->pers.onehp)
		{
			G_WinBadge( otherEnt, 18 );
			otherEnt->client->pers.badgeupdate[18] = 1;
			otherEnt->client->pers.badges[18] = 1;
		}
      if(otherEnt->health >=100)
      {
        otherEnt->health = otherEnt->client->ps.stats[STAT_HEALTH] = 100;
        //G_AddEvent(otherEnt, EV_MEDKIT_USED, 0);
      }
    }
  }
}

void
G_minethink(gentity_t *ent)
{
  trace_t tr;
  vec3_t end, origin, dir;
  gentity_t *traceEnt;

  ent->nextthink = level.time + 100;

  BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
  SnapVector(origin);
  G_SetOrigin(ent, origin);

  // set aiming directions
  VectorCopy(origin,end);
  end[2] += 10;//aim up

  trap_Trace(&tr, origin, NULL, NULL, end, ent->s.number, MASK_SHOT);
  if (tr.surfaceFlags & SURF_NOIMPACT)
    return;

  traceEnt = &g_entities[tr.entityNum];

  dir[0] = dir[1] = 0;
  dir[2] = 1;

  if (traceEnt->client && (traceEnt->r.svFlags & SVF_BOT)
      && traceEnt->health > 0 && traceEnt->client->ps.stats[STAT_PTEAM] == PTE_ALIENS)//FIRE IN ZE HOLE!
  {//Might want to check team too
    ent->s.eType = ET_GENERAL;
    G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(dir));
    ent->freeAfterEvent = qtrue;
    G_RadiusDamage(ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->splashMethodOfDeath);
    ent->parent->numMines -= 1;
    trap_LinkEntity(ent);
  }
}

gentity_t *
plant_mine(gentity_t *self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "mine";
  bolt->nextthink = level.time + 2000;
  bolt->think = G_minethink;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_MINE;
  bolt->s.eFlags = EF_BOUNCE_HALF;
  bolt->s.generic1 = WPM_PRIMARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
 if(g_survival.integer)
 {
	 bolt->damage = MINE_DAMAGE;
	 bolt->splashDamage = MINE_DAMAGE;
 } else {
	 bolt->damage = (1.25 * MINE_DAMAGE);
	 bolt->splashDamage = (1.25 * MINE_DAMAGE);
 }
  bolt->splashRadius = MINE_RANGE;
  bolt->methodOfDeath = MOD_MINE;
  bolt->splashMethodOfDeath = MOD_MINE;
  bolt->clipmask = MASK_DEADSOLID;
  bolt->target_ent = NULL;
  bolt->r.mins[0] = bolt->r.mins[1] = -5.0f;
  bolt->r.mins[2] = 0.0f;
  bolt->r.maxs[0] = bolt->r.maxs[1] = 5.0f;
  bolt->r.maxs[2] = 2.0f;
  bolt->s.time = level.time;

  bolt->biteam = bolt->s.modelindex2 = BIT_HUMANS;
  bolt->use = G_itemUse;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, GRENADE_SPEED/3, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta ); // save net bandwidth

  VectorCopy( start, bolt->r.currentOrigin );
  return bolt;
}
/*
=================
fire_rocket
=================
*/
gentity_t      *fire_rocket(gentity_t * self, vec3_t start, vec3_t dir)
{
  gentity_t *bolt;

  VectorNormalize(dir);

  bolt = G_Spawn();
  bolt->classname = "rocket_launcher";
  bolt->nextthink = level.time + 5000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_ROCKET_LAUNCHER;
  bolt->s.generic1 = WPM_PRIMARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = ROCKET_LAUNCHER_DAMAGE;
  bolt->splashDamage = ROCKET_LAUNCHER_DAMAGE;
  bolt->splashRadius = ROCKET_LAUNCHER_RANGE;
  bolt->methodOfDeath = MOD_ROCKET_LAUNCHER;
  bolt->splashMethodOfDeath = MOD_ROCKET_LAUNCHER;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[0] = bolt->r.mins[1] = bolt->r.mins[2] = -3.0f;
  bolt->r.maxs[0] = bolt->r.maxs[1] = bolt->r.maxs[2] = 3.0f;
  bolt->s.time = level.time;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
  VectorCopy(start, bolt->s.pos.trBase);
  VectorScale(dir, ROCKET_LAUNCHER_SPEED, bolt->s.pos.trDelta);
  SnapVector(bolt->s.pos.trDelta); // save net bandwidth

  VectorCopy(start, bolt->r.currentOrigin);

  return bolt;

}

/*
=================
fire_dome
=================
*/
gentity_t      *fire_dome(gentity_t * self, vec3_t start, vec3_t dir)
{
    gentity_t *bolt;

    VectorNormalize(dir);

    bolt = G_Spawn();
    bolt->classname = "dome";
    bolt->nextthink = level.time + 1000;
    bolt->dieTime = level.time + 60000; //60 Second stand.
    bolt->think = G_domethink;
    bolt->s.eType = ET_MISSILE;
    bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
    bolt->s.weapon = WP_DOME;
    bolt->s.eFlags = EF_BOUNCE_HALF;
    bolt->s.generic1 = WPM_PRIMARY; //weaponMode
    bolt->r.ownerNum = self->s.number;
    bolt->parent = self;
    bolt->damage = MINE_DAMAGE;
    bolt->splashDamage = MINE_DAMAGE;
    bolt->splashRadius = MINE_RANGE;
    bolt->methodOfDeath = MOD_MINE;
    bolt->splashMethodOfDeath = MOD_MINE;
    bolt->clipmask = MASK_DEADSOLID;
    bolt->target_ent = NULL;
    bolt->r.mins[0] = bolt->r.mins[1] = -3.0f;
    bolt->r.mins[2] = 0.0f;
    bolt->r.maxs[0] = bolt->r.maxs[1] = 3.0f;
    bolt->r.maxs[2] = 2.0f;
    bolt->s.time = level.time;

    bolt->biteam = bolt->s.modelindex2 = BIT_HUMANS;
    //bolt->use = G_itemUse;

    bolt->s.pos.trType = TR_GRAVITY;
    bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
    VectorCopy( start, bolt->s.pos.trBase );
    VectorScale( dir, GRENADE_SPEED/3, bolt->s.pos.trDelta );
    SnapVector( bolt->s.pos.trDelta ); // save net bandwidth

    VectorCopy( start, bolt->r.currentOrigin );
    return bolt;
}
