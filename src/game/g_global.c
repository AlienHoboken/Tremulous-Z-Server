/*
 ===========================================================================
 Copyright (C) 2011 Diego Michel Rubio Ramirez

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

void G_globalInit()
{
	char data[255];
	global_t *temp = NULL;
	whitelist_t *whitelisttemp = NULL;

	level.globals = NULL;
	level.whitelist = NULL;

	if (trap_mysql_runquery(va("SELECT HIGH_PRIORITY * FROM globals ORDER BY id ASC")) == qtrue)
	{
		while (trap_mysql_fetchrow() == qtrue)
		{
			temp = G_Alloc(sizeof(global_t));

			trap_mysql_fetchfieldbyName("id", data, sizeof(data));
			temp->id = atoi(data);
			trap_mysql_fetchfieldbyName("adminname", temp->adminName, sizeof(temp->adminName));
			trap_mysql_fetchfieldbyName("ip", temp->ip, sizeof(temp->ip));
			trap_mysql_fetchfieldbyName("guid", temp->guid, sizeof(temp->guid));
			trap_mysql_fetchfieldbyName("name", temp->playerName, sizeof(temp->playerName));
			trap_mysql_fetchfieldbyName("reason", temp->reason, sizeof(temp->reason));
			trap_mysql_fetchfieldbyName("server", temp->server, sizeof(temp->server));
			trap_mysql_fetchfieldbyName("subnet", data, sizeof(data));
			temp->subnet = atoi(data);
			trap_mysql_fetchfieldbyName("type", data, sizeof(data));
			temp->type = atoi(data);

			temp->next = level.globals;
			level.globals = temp;
		}
		trap_mysql_finishquery();
	}
	else
	{
		G_LogPrintf("Query Failed on GlobalInit, data base is probably disconnected, try !reconnectdb\n");
	}

	if (trap_mysql_runquery(va("SELECT HIGH_PRIORITY * FROM whitelist ORDER BY id ASC")) == qtrue)
	{
		while (trap_mysql_fetchrow() == qtrue)
		{
			whitelisttemp = G_Alloc(sizeof(whitelist_t));

			trap_mysql_fetchfieldbyName("id", data, sizeof(data));
			whitelisttemp->id = atoi(data);
			trap_mysql_fetchfieldbyName("adminname", whitelisttemp->adminName, sizeof(whitelisttemp->adminName));
			trap_mysql_fetchfieldbyName("ip", whitelisttemp->ip, sizeof(whitelisttemp->ip));
			trap_mysql_fetchfieldbyName("guid", whitelisttemp->guid, sizeof(whitelisttemp->guid));
			trap_mysql_fetchfieldbyName("playername", whitelisttemp->playerName, sizeof(whitelisttemp->playerName));
			trap_mysql_fetchfieldbyName("reason", whitelisttemp->reason, sizeof(whitelisttemp->reason));

			whitelisttemp->next = level.whitelist;
			level.whitelist = whitelisttemp;
		}
		trap_mysql_finishquery();
	}

}

void G_globalExit(void)
{
	global_t *temp = level.globals;
	whitelist_t *whitelist = level.whitelist;

	while (temp != NULL)
	{
		global_t *temp2 = temp->next;
		G_Free(temp);
		temp = temp2;
	}

	while (whitelist != NULL)
	{
		whitelist_t *whitelist2 = whitelist->next;
		G_Free(whitelist);
		whitelist = whitelist2;
	}
}

qboolean G_guidGlobalMatch(char *guid, char *guid2)
{
	if (Q_stricmp(guid, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX") == 0
			|| Q_stricmp(guid2, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX") == 0)
	{
		return qfalse;
	}

	return (Q_stricmp(guid, guid2) == 0);
}
void G_applyGlobal(gentity_t *ent, global_t *global)
{
	switch (global->type)
	{
		case GLOBAL_DENYBUILD:
			ent->client->pers.denyBuild = qtrue;
			break;

		case GLOBAL_FORCESPEC:
			ent->client->pers.forcespec = qtrue;
			break;

		case GLOBAL_HANDICAP:
			ent->client->pers.handicap = qtrue;
			break;

		case GLOBAL_MUTE:
			ent->client->pers.muted = qtrue;
			break;
		case GLOBAL_NONE:
			//huh
			break;
		case GLOBAL_BAN:
			//huh
			break;
	}

	ent->client->pers.globalID = global->id;

	G_AdminsPrintf("^3%s:^7 have been caught by global %s id: %d \n",
		ent->client->pers.netname,
		getGlobalTypeString(global->type),
		global->id);

//	G_LogPrintf("^3%s:^7 have been caught by global %s\n", ent->client->pers.netname,
//		getGlobalTypeString(global->type));

	trap_SendServerCommand(ent->client->ps.clientNum,
		va( "print \"^1You have been caught by global %s id: %d appeals: %s\n\"",
		getGlobalTypeString(global->type),
		global->id,
		GLOBALS_URL));
}

qboolean G_ipGlobalMatch(char *ip, char *ip2, qboolean subnet)
{
	int ipPosition = 1;
	int i;

	for (i = 0; ip[i] != '\0'; i++)
	{
		//3 first blocks match = /24
		if (ipPosition == 4 && subnet == 2)
		{
			return qtrue;
		}
		//First two blocks match = subnet
		if (ipPosition == 3 && subnet == 1)
		{
			return qtrue;
		}
		if (ip[i] == '.')
		{
			ipPosition++;
			continue;
		}
		if (ip[i] != ip2[i])
		{
			return qfalse;
		}
	}

	if(ip[0] == '\0' || ip2[0] == '\0')
		return qfalse;

	return qtrue;
}

int G_globalAdd(gentity_t *adminEnt, gentity_t *victimEnt, char *guid, char *ip, char *playerName, char *reason, int subnet, char *date, globalType_t type)
{
	global_t *temp = G_Alloc(sizeof(global_t));
	char newDate[11];
	char newPlayerName[MAX_NAME_LENGTH];
	char newGuid[33];
	char data[255];

	//DATE
	if (date == NULL)
	{
		qtime_t qt;
		int t;
		t = trap_RealTime(&qt);

		Q_strncpyz(newDate, va("%04i-%02i-%02i", qt.tm_year + 1900, qt.tm_mon
				+ 1, qt.tm_mday), 11);
	}
	else
	{
		Q_strncpyz(newDate, date, 11);
	}

	//PLAYERNAME
	if (playerName == NULL)
	{
		Q_strncpyz(newPlayerName, va("%s", "UnnamedPlayer"), MAX_NAME_LENGTH);
	}
	else
	{
		Q_strncpyz(newPlayerName, playerName, MAX_NAME_LENGTH);
	}

	//GUID
	if (guid == NULL)
	{
		Q_strncpyz(newGuid, va("%s", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"), 33);
	}
	else
	{
		Q_strncpyz(newGuid, guid, 33);
	}

	//Setting
	Q_strncpyz(temp->guid, newGuid, 33);
	Q_strncpyz(temp->ip, ip, 16);
	Q_strncpyz(temp->playerName, newPlayerName, MAX_NAME_LENGTH);
	Q_strncpyz(temp->server, va("%s","z"), 2);
	Q_strncpyz(temp->adminName, (G_isPlayerConnected(adminEnt))
			? adminEnt->client->pers.netname
			: "console", MAX_NAME_LENGTH);
	Q_strncpyz(temp->reason, reason, MAX_STRING_CHARS);

	temp->subnet = subnet;
	temp->type = type;

	if (trap_mysql_runquery(va("INSERT HIGH_PRIORITY INTO globals"
		" (playerid,adminid,adminname,ip,guid,name,reason,subnet,type,server) "
		" VALUES (\"%d\",\"%d\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%d\",\"%d\",\"z\") ", (G_isPlayerConnected(victimEnt)
			? victimEnt->client->pers.mysqlid
			: -1), (G_isPlayerConnected(adminEnt)
			? adminEnt->client->pers.mysqlid
			: -1), (G_isPlayerConnected(adminEnt)
			? adminEnt->client->pers.netname
			: "console"), temp->ip, temp->guid, temp->playerName, temp->reason, temp->subnet, temp->type))
			== qtrue)
	{
		trap_mysql_finishquery();
		if (trap_mysql_runquery(va("SELECT HIGH_PRIORITY id FROM globals ORDER BY id DESC LIMIT 1"))
				== qtrue)
		{
			if (trap_mysql_fetchrow() == qtrue)
			{
				trap_mysql_fetchfieldbyName("id", data, sizeof(data));
				trap_mysql_finishquery();
				temp->id = atoi(data);
			}
			else
			{
				G_LogPrintf("Couldnt insert global to database\n");//va("Hacked client tried to connect guid: %s ip: %s\n", guid, ip) );
				if (level.globals)
				{
					temp->id = level.globals->id + 1;
				}
				else
				{
					temp->id = 1;
				}
			}
		}
	}


	temp->next = level.globals;
	level.globals = temp;

	return temp->id;
}
qboolean G_globalBanCheck(char *userinfo, char *reason, int rlen)
{
	static char lastConnectIP[16] =
	{ "" };
	static int lastConnectTime = 0;
	int t = 0;
	global_t *global = level.globals;
	qboolean ipMatch = qfalse;
	qboolean guidMatch = qfalse;

	char guid[33];
	char ip[16];
	char *value;

	*reason = '\0';

	if (!*userinfo)
		return qfalse;

	value = Info_ValueForKey(userinfo, "ip");
	Q_strncpyz(ip, value, sizeof(ip));
	// strip port
	value = strchr(ip, ':');
	if (value)
		*value = '\0';

	if (!*ip)
		return qfalse;

	value = Info_ValueForKey(userinfo, "cl_guid");
	
	if (!value[0])
	{
		Q_strncpyz(guid, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sizeof(guid));
	} else {
	Q_strncpyz(guid, value, sizeof(guid));
	}
	while (global)
	{
		ipMatch = G_ipGlobalMatch(global->ip, ip, global->subnet);
		guidMatch = G_guidGlobalMatch(guid, global->guid);

		if ((ipMatch != guidMatch)
				&& Q_stricmp(guid, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"))
		{
			//This can cause problems, because if a player is connecting and you delete the global it will recreate again lol..
			//G_globalAdd(guid, ip, global->playerName, global->reason, global->subnet, global->date, global->type);
		}

		if ((ipMatch || guidMatch) && global->type == GLOBAL_BAN)
		{
			// flood protected
			if (t - lastConnectTime >= 3000 || Q_stricmp(lastConnectIP, ip))
			{
				lastConnectTime = t;
				Q_strncpyz(lastConnectIP, ip, sizeof(lastConnectIP));

				G_AdminsPrintf("Banned player %s^7 (%s^7) tried to connect (ban #%d reason: %s^7 )\n", global->playerName, Info_ValueForKey(userinfo, "name"), global->id, global->reason);
			}

			Com_sprintf(reason, rlen, "^3Ban ID:^7%d ^3Reason: ^7%s ^3Appeal: ^7%s", global->id, global->reason, GLOBALS_URL);
			G_LogPrintf("Banned player tried to connect from IP %s, caught by global #%d\n", ip, global->id);
			return qtrue;
		}
		global = global->next;
	}
	return qfalse;
}
void G_whitelistCheck(gentity_t *ent)
{
	whitelist_t *whitelist = level.whitelist;
	qboolean ipMatch = qfalse;
	qboolean guidMatch = qfalse;

	while (whitelist)
	{
		ipMatch
				= G_ipGlobalMatch(whitelist->ip, ent->client->pers.ip, qfalse);
		guidMatch = G_guidGlobalMatch(ent->client->pers.guid, whitelist->guid);

		if (ipMatch || guidMatch)
		{
			G_AdminsPrintf("%s ^7White listed reason: %s ID: %d\n",
					ent->client->pers.netname,
					whitelist->reason,
					whitelist->id);
			ent->client->pers.whiteListed = qtrue;
			return;
		}
		whitelist = whitelist->next;
	}
}
//Check everything except ban.
void G_globalCheck(gentity_t *ent)
{
	global_t *global = level.globals;
	qboolean ipMatch = qfalse;
	qboolean guidMatch = qfalse;

	if(ent->client->pers.whiteListed)
	{
		return;
	}

	while (global)
	{
		ipMatch
				= G_ipGlobalMatch(global->ip, ent->client->pers.ip, global->subnet);
		guidMatch = G_guidGlobalMatch(ent->client->pers.guid, global->guid);

		if (ipMatch || guidMatch)
		{
			G_applyGlobal(ent, global);
		}
		global = global->next;
	}
}
char *getGlobalTypeString(globalType_t type)
{
	char *typeReason;

	switch (type)
	{
		case GLOBAL_DENYBUILD:
			typeReason = "DENYBUILD";
			break;
		case GLOBAL_FORCESPEC:
			typeReason = "FORCESPEC";
			break;
		case GLOBAL_HANDICAP:
			typeReason = "HANDYCAP";
			break;
		case GLOBAL_MUTE:
			typeReason = "MUTE";
			break;
		case GLOBAL_BAN:
			typeReason = "BAN";
			break;
		default:
			typeReason = "OTHER";
			break;
	}
	return typeReason;
}
qboolean G_deleteGlobal(int id)
{
	global_t *global = level.globals;
	global_t *lastGlobal = NULL;

	while (global)
	{
		if (global->id == id)
		{
			if (lastGlobal == NULL)
			{
				level.globals = global->next;
			}
			else
			{
				lastGlobal->next = global->next;
			}
			if (trap_mysql_runquery(va("DELETE QUICK FROM globals WHERE id = %d LIMIT 1", id))
					== qtrue)
			{
				trap_mysql_finishquery();
				G_Free(global);
				return qtrue;
			}
			else
			{
				G_AdminsPrintf("Cant delete %d from database, try !reconnectdb then !gsync\n", id);
				return qfalse;
			}
		}
		lastGlobal = global;
		global = global->next;
	}
	return qfalse;
}
qboolean G_deleteWhite(int id)
{
	whitelist_t *whitelist = level.whitelist;
	whitelist_t *lastWhitelist = NULL;

	while (whitelist)
	{
		if (whitelist->id == id)
		{
			if (lastWhitelist == NULL)
			{
				level.whitelist = whitelist->next;
			}
			else
			{
				lastWhitelist->next = whitelist->next;
			}
			if (trap_mysql_runquery(va("DELETE QUICK FROM whitelist WHERE id = %d LIMIT 1", id))
					== qtrue)
			{
				trap_mysql_finishquery();
				G_Free(whitelist);
				return qtrue;
			}
			else
			{
				G_AdminsPrintf("Cant delete %d from database, try !reconnectdb then !gsync\n",id);
				return qfalse;
			}

		}
		lastWhitelist = whitelist;
		whitelist = whitelist->next;
	}
	return qfalse;
}
void G_adminGlobalSetReason(int skiparg, qboolean subnet, char *reason, int rlen)
{
	if (subnet)
	{
		Com_sprintf(reason, rlen, "%s", G_SayConcatArgs(3 + skiparg));
	}
	else
	{
		Com_sprintf(reason, rlen, "%s", G_SayConcatArgs(2 + skiparg));
	}
}
int G_matchSlot(char *who)
{
	gentity_t *ent;
	ent = &g_entities[atoi(who)];
	if (ent && ent->client && ent->client->pers.connected == CON_CONNECTED)
	{
		return atoi(who);
	}
	return -1;
}
int G_adminGlobalSetSlot(char *who)
{
	int logmatch = -1;

	logmatch = G_matchSlot(who);
	//G_Printf("%d\n", logmatch);
	//	if (logmatch == -1)
	//	{
	//		logmatch = G_matchName(who);
	//	}
	return logmatch;
}
void G_adminGlobalSetWho(char *who, int skiparg)
{
	G_SayArgv(1 + skiparg, who, MAX_STRING_CHARS);
	G_SanitiseString(who, who, MAX_STRING_CHARS);
}
int G_adminGlobalSetSubnet(int skiparg)
{
	char subnet[MAX_NAME_LENGTH];\
	int i;
	G_SayArgv(2 + skiparg, subnet, sizeof(subnet));

	for(i=0;subnet[i] != '\0';i++)
	{
		if( subnet[ i ] < '0' || subnet[ i ] > '9' )
		{
			return 0;
		}
	}
	if(atoi(subnet)>9) return 0;
	return atoi(subnet);
}
void G_globalPrintMsgForAdmins(gentity_t *ent, gentity_t *vic, int type, globalType_t globalID, char *who, char *reason)
{
	char command[MAX_STRING_CHARS];
	char action[MAX_STRING_CHARS];
	switch (type)
	{
		case GLOBAL_BAN:
			Com_sprintf(command, sizeof(command), "%s", "!gban");
			Com_sprintf(action, sizeof(action), "%s", "as been banned by");
			break;
		case GLOBAL_DENYBUILD:
			Com_sprintf(command, sizeof(command), "%s", "!gdenybuild");
			Com_sprintf(action, sizeof(action), "%s", "as lost building rights by");
			break;
		case GLOBAL_FORCESPEC:
			Com_sprintf(command, sizeof(command), "%s", "!gforcespec");
			Com_sprintf(action, sizeof(action), "%s", "as been forced to spec by");
			break;
		case GLOBAL_HANDICAP:
			Com_sprintf(command, sizeof(command), "%s", "!ghandicap");
			Com_sprintf(action, sizeof(action), "%s", "as been handicaped by");
			break;
		case GLOBAL_MUTE:
			Com_sprintf(command, sizeof(command), "%s", "!gmute");
			Com_sprintf(action, sizeof(action), "%s", "as been muted by");
			break;
	}
	AP( va( "print \"^3%s:^7 %s %s %s reason: %s ^3ID:^7#%d \n\"",
					command,
					(G_isPlayerConnected(vic)) ? vic->client->pers.netname : who,
					action,
					(G_isPlayerConnected(ent)) ? ent->client->pers.netname : "console",
					( *reason ) ? reason : "not specified",
					globalID) );
	G_LogPrintf("%s %s %s reason: %s, ID: #%d\n", (G_isPlayerConnected(vic))
			? vic->client->pers.netname
			: who, action, (G_isPlayerConnected(ent))
			? ent->client->pers.netname
			: "console", (*reason) ? reason : "not specified", globalID);
}
void G_globalPrintMsgForPlayer(gentity_t *ent, gentity_t *vic, globalType_t type, int globalID, char *reason)
{
	char action[MAX_STRING_CHARS];
	if (!G_isPlayerConnected(vic))
	{
		return;
	}
	switch (type)
	{
		case GLOBAL_BAN:
			Com_sprintf(action, sizeof(action), "%s", "You have been banned by");
			break;
		case GLOBAL_DENYBUILD:
			Com_sprintf(action, sizeof(action), "%s", "You have been denybuild by");
			break;
		case GLOBAL_FORCESPEC:
			Com_sprintf(action, sizeof(action), "%s", "You have been forced to spec by");
			break;
		case GLOBAL_HANDICAP:
			Com_sprintf(action, sizeof(action), "%s", "You have been handicap by");
			break;
		case GLOBAL_MUTE:
			Com_sprintf(action, sizeof(action), "%s", "You have been muted by");
			break;
		case GLOBAL_NONE:
			//huh?
			break;
	}
	trap_SendServerCommand(vic->client->ps.clientNum, va("print \"^7%s "
		"%s reason: %s ^3ID:^7%d \n\"", action, (G_isPlayerConnected(ent))
			? G_admin_adminPrintName(ent)
			: "console", (*reason) ? reason : "not specified", globalID));
}
void G_globalAction(gentity_t *ent, gentity_t *vic, globalType_t type, char *reason)
{
	if (!G_isPlayerConnected(vic))
		return;

	switch (type)
	{
		case GLOBAL_BAN:
			trap_DropClient(vic->client->ps.clientNum, va("banned by %s^7, reason: %s", (ent)
					? ent->client->pers.netname
					: "console", (*reason) ? reason : "banned by admin"));
			break;
		case GLOBAL_DENYBUILD:
			vic->client->pers.denyBuild = qtrue;
			break;
		case GLOBAL_FORCESPEC:
			vic->client->pers.forcespec = qtrue;
			break;
		case GLOBAL_HANDICAP:
			vic->client->pers.handicap = qtrue;
			break;
		case GLOBAL_MUTE:
			vic->client->pers.muted = qtrue;
			break;
		case GLOBAL_NONE:
			//uh?
			break;
	}
}
qboolean G_adminGlobal(gentity_t *ent, int skiparg, globalType_t type)
{
	gentity_t *vic = NULL;
	char who[MAX_STRING_CHARS];
	char reason[MAX_STRING_CHARS] =
	{ "" };
	qboolean subnetBan = qfalse;
	int playerSlot = -1;
	int minArguments = 2 + skiparg; //Subnet is optional
	int globalID;

	if (G_SayArgc() < minArguments)
	{
		ADMP( "^3!help: ^7!command\n" );
		return qfalse;
	}

	G_adminGlobalSetWho(who, skiparg);
	subnetBan = G_adminGlobalSetSubnet(skiparg);
	G_adminGlobalSetReason(skiparg, subnetBan, reason, sizeof(reason));

	//If isnt a ip check if is a slot, atoi converts ips into ins :(
	if(!G_isValidIpAddress(who))
		playerSlot = G_adminGlobalSetSlot(who);

	if (playerSlot == -1 && !G_isValidIpAddress(who))
	{
		ADMP( "^3No player found by that name, IP, or slot number\n" );
		return qfalse;
	}
	if (playerSlot != -1)
	{
		vic = &g_entities[playerSlot];
		globalID
				= G_globalAdd(ent, vic, vic->client->pers.guid, vic->client->pers.ip, vic->client->pers.netname, reason, subnetBan, NULL, type);
	}
	else
	{
		globalID
				= G_globalAdd(ent, vic, NULL, who, NULL, reason, subnetBan, NULL, type);
	}

	G_globalPrintMsgForAdmins(ent, vic, type, globalID, who, reason);
	G_globalPrintMsgForPlayer(ent, vic, type, globalID, reason);
	G_globalAction(ent, vic, type, reason);

	if (G_isPlayerConnected(vic))
		vic->client->pers.globalID = globalID;

	return qtrue;
}
//WHITELIST FUNCTIONS
void G_globalAddToWhitelist(gentity_t *ent, gentity_t *victim, char *who, char *reason, char *ip, char *guid)
{
	whitelist_t *temp = G_Alloc(sizeof(whitelist_t));
	char data[255];

	Q_strncpyz(temp->adminName, (G_isPlayerConnected(ent))
			? ent->client->pers.netname
			: "console", sizeof(temp->adminName));
	Q_strncpyz(temp->playerName, (G_isPlayerConnected(victim))
			? victim->client->pers.netname
			: "UnnamedPlayer", sizeof(temp->playerName));
	Q_strncpyz(temp->guid, (guid) ? guid : "", sizeof(temp->guid));
	Q_strncpyz(temp->ip, (ip) ? ip : "", sizeof(temp->ip));
	Q_strncpyz(temp->reason, (reason) ? reason : "", sizeof(temp->reason));

	if (trap_mysql_runquery(va("INSERT HIGH_PRIORITY INTO whitelist"
		" (ip,guid,playerid,playername,adminid,adminname,reason) "
		" VALUES (\"%s\",\"%s\",\"%d\",\"%s\",\"%d\",\"%s\",\"%s\") ",
		temp->ip,
		temp->guid,
		(G_isPlayerConnected(victim) ? victim->client->pers.mysqlid : -1),
		(G_isPlayerConnected(victim) ? victim->client->pers.netname : "UnnamedPlayer"),
		(G_isPlayerConnected(ent) ? ent->client->pers.mysqlid : -1),
		(G_isPlayerConnected(ent) ? ent->client->pers.netname : "console"),
		temp->reason))
			== qtrue)
	{
		trap_mysql_finishquery();

		AP( va( "print \"^3%s:^7 have been added to white list by %s reason: %s \n\"",
			(G_isPlayerConnected(victim) ? victim->client->pers.netname : ip),
			(G_isPlayerConnected(ent)) ? ent->client->pers.netname : "console",
			reason));
		G_LogPrintf("^3%s:^7 have been added to white list by %s reason: %s \n",
			(G_isPlayerConnected(victim) ? victim->client->pers.netname : ip),
			(G_isPlayerConnected(ent)) ? ent->client->pers.netname : "console",
			reason);
		if (trap_mysql_runquery(va("SELECT HIGH_PRIORITY id FROM whitelist ORDER BY id DESC LIMIT 1"))
				== qtrue)
		{
			if (trap_mysql_fetchrow() == qtrue)
			{
				trap_mysql_fetchfieldbyName("id", data, sizeof(data));
				trap_mysql_finishquery();
				temp->id = atoi(data);
			}
			else
			{
				G_LogPrintf("WARNING: Couldnt insert whitelist to database\n");
				if (level.whitelist)
				{
					temp->id = level.globals->id + 1;
				}
				else
				{
					temp->id = 1;
				}
			}
		}
	}
	temp->next = level.whitelist;
	level.whitelist = temp;

}
qboolean G_adminWhitelistGlobal(gentity_t *ent, int skiparg)
{
	gentity_t *victim = NULL;
	char type[5];
	char who[16];
	char reason[MAX_STRING_CHARS];
	int playerSlot = -1;

	int minArguments = 2 + skiparg;

	if (G_SayArgc() < minArguments)
	{
		ADMP( "^3!help: ^7!wadd\n" );
		return qfalse;
	}
	G_SayArgv(1 + skiparg, type, sizeof(type));
	G_SayArgv(2 + skiparg, who, sizeof(who));
	Com_sprintf(reason, MAX_STRING_CHARS, "%s", G_SayConcatArgs(3 + skiparg));

	playerSlot = G_adminGlobalSetSlot(who);
	if (playerSlot != -1)
	{
		victim = &g_entities[playerSlot];
	}
	if (strcmp(type, "ip") == 0)
	{
		if (!G_isValidIpAddress(who) && !victim)
		{
			ADMP( "^3!help: ^7INVALID IP\n" );
			return qfalse;
		}
		G_globalAddToWhitelist(ent, victim, who, reason, (G_isValidIpAddress(who))
				? who
				: victim->client->pers.ip, NULL);
		return qtrue;
	}
	else if (strcmp(type, "guid") == 0)
	{
		if (!victim
				|| strcmp(victim->client->pers.guid, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX")
						== 0)
		{
			ADMP( "^3!help: ^7INVALID GUID\n" );
			return qfalse;
		}
		G_globalAddToWhitelist(ent, victim, who, reason, NULL, victim->client->pers.guid);
		return qtrue;
	}
	else
	{
		ADMP( "^3!help: ^7!wadd\n" );
		return qtrue;
	}
}
