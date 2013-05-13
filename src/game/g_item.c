#include "g_local.h"
#include "tremulous.h"

void
G_itemThink(gentity_t *ent)
{
  ent->nextthink = level.time + 3000;
  if (level.spawnedItems >= (10 - ((level.time - level.startTime) / 1000 / 60)))
  {
    G_FreeEntity(ent);
    level.spawnedItems--;
  }
}
void
G_sellAllWeapons(gentity_t *player)
{
  int i;
  for(i = WP_NONE + 1;i < WP_NUM_WEAPONS;i++)
  {
    if (BG_InventoryContainsWeapon(i, player->client->ps.stats) && BG_FindPurchasableForWeapon(i))
    {
      BG_RemoveWeaponFromInventory(i, player->client->ps.stats);
    }
  }
}
void
G_switchWeapon(gentity_t *player, weapon_t weapon)
{
  int maxAmmo, maxClips;

  G_sellAllWeapons(player);

  BG_AddUpgradeToInventory(UP_LIGHTARMOUR, player->client->ps.stats);
  BG_AddUpgradeToInventory(UP_HELMET, player->client->ps.stats);
  BG_AddWeaponToInventory(WP_HBUILD2, player->client->ps.stats);

  BG_AddWeaponToInventory(weapon, player->client->ps.stats);
  BG_FindAmmoForWeapon(weapon, &maxAmmo, &maxClips);
  G_ForceWeaponChange(player, weapon);
  BG_PackAmmoArray(weapon, &player->client->ps.ammo, player->client->ps.powerups, maxAmmo, maxClips);
}
void
G_giveHealth(gentity_t *player)
{
  trap_SendServerCommand(
    player - g_entities, "print \"^3Use construction kit to heal teammates\n\"");
  player->health = player->client->ps.stats[STAT_HEALTH]
      = player->client->ps.stats[STAT_MAX_HEALTH];
  G_AddEvent(player, EV_MEDKIT_USED, 0);

  player->client->ps.persistant[PERS_UNUSED] = 3;
}
void G_giveDomes(gentity_t *ent)
{
	trap_SendServerCommand(ent - g_entities, "print \"^3Use construction kit to heal teammates and deploy healing domes!\nUse fire to deploy a dome, or use secondary fire to heal a teammate!\"");
	ent->health = ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH];
	G_AddEvent(ent, EV_MEDKIT_USED, 0);
	ent->client->ps.persistant[PERS_UNUSED] = 3;
}
void
G_giveMine(gentity_t *player)
{
  BG_AddUpgradeToInventory(UP_MINE, player->client->ps.stats);
}
void
G_itemUse(gentity_t *self, gentity_t *other, gentity_t *activator)
{
  qboolean item = qtrue;

  switch(self->s.modelindex)
  {
    case BA_I_CHAINGUN:
      G_switchWeapon(activator, WP_CHAINGUN);
      break;
    case BA_I_AXE:
      G_switchWeapon(activator, WP_AXE);
      break;
    case BA_I_LASERGUN:
      G_switchWeapon(activator, WP_LAS_GUN);
      break;
    case BA_I_MACHINEGUN:
      G_switchWeapon(activator, WP_MACHINEGUN);
      break;
    case BA_I_MDRIVER:
      G_switchWeapon(activator, WP_MASS_DRIVER);
      break;
    case BA_I_SHOTGUN:
      G_switchWeapon(activator, WP_SHOTGUN);
      break;
    case BA_I_GRENADE_LAUNCHER:
      G_switchWeapon(activator, WP_LAUNCHER);
      break;
    case BA_I_ROCKET_LAUNCHER:
      G_switchWeapon(activator, WP_ROCKET_LAUNCHER);
      break;
    case BA_I_SYRINX:
      //G_giveHealth(activator);
      G_giveDomes(activator);
      break;
    default:
      item = qfalse;
      break;
  }
  ////////////////////////////////////////////////////////////////////////////
  // Special Case for Mines
  ////////////////////////////////////////////////////////////////////////////
  if (self->s.weapon == WP_MINE && self->parent == activator)
  {
    G_giveMine(activator);
    G_FreeEntity(self);
    activator->numMines--;
  }
  else if (self->s.modelindex == BA_I_MINE)
  {
    G_giveMine(activator);
    G_FreeEntity(self);
  }
  else if (item == qtrue)
  {
    G_FreeEntity(self);
    level.spawnedItems--;
  }
}

gentity_t *
spawnItem(gentity_t *ent, buildable_t itemtype)
{
  gentity_t *item;
  vec3_t normal;
  vec3_t origin;

  VectorCopy(ent->s.origin, origin);

  if(G_playerInRange(ent, DOME_RANGE, PTE_HUMANS))
  {
    return NULL;
  }
  if(G_itemInRange(ent, DOME_RANGE/2, PTE_NONE))
  {
    return NULL;
  }
  if (G_doorInRange(ent, DOME_RANGE / 3, PTE_NONE))
  {
    return NULL;
  }

  origin[2] += 40;

  item = G_Spawn();

  BG_FindBBoxForBuildable(itemtype, item->r.mins, item->r.maxs);

  item->s.eType = ET_BUILDABLE;

  item->classname = BG_FindEntityNameForBuildable(itemtype);

  item->s.modelindex = itemtype;

  item->biteam = item->s.modelindex2 = BIT_HUMANS;

  VectorSet(normal, 0.0f, 0.0f, 1.0f);

  VectorMA(origin, 1.0, normal, origin);

  item->health = 10000;

  item->nextthink = level.time + 3000;
  item->think = G_itemThink;

  item->spawned = qtrue;

  item->s.number = item - g_entities;
  item->r.contents = 0;
  item->clipmask = MASK_SOLID;

  item->enemy = NULL;
  item->s.weapon = BG_FindProjTypeForBuildable(itemtype);

  G_SetOrigin(item, origin);

  item->s.pos.trType = TR_STATIONARY;

  item->s.pos.trTime = level.time;
  item->physicsBounce = 0.5;//BG_FindBounceForBuildable(itemtype);
  item->s.groundEntityNum = 1;//-1;
  //item->s.eFlags = EF_BOUNCE_HALF;
  item->use = G_itemUse;
  item->takedamage = qfalse;

  // gently nudge the buildable onto the surface :)
  VectorScale(normal, -50.0f, item->s.pos.trDelta);
  VectorSet(item->s.pos.trDelta,0,0,0);
  item->s.pos.trTime = level.time;

  item->powered = qtrue;

  item->s.generic1 |= B_POWERED_TOGGLEBIT;
  item->s.generic1 |= B_SPAWNED_TOGGLEBIT;
  item->r.svFlags &= ~SVF_BROADCAST;

  VectorCopy(normal, item->s.origin2);

  trap_LinkEntity(item);

  //G_LogPrintf("Item Spawned");
  level.spawnedItems++;
  return item;
}
