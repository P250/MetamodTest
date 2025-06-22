/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Sample Plugin
 * Written by AlliedModders LLC.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 *
 * This sample plugin is public domain.
 */

#include <stdio.h>
#include "plugin.h"
#include "iserver.h"

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK4_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, CPlayerSlot, bool, const char *, uint64);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, ENetworkDisconnectionReason, const char *, uint64, const char *);
SH_DECL_HOOK4_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, CPlayerSlot, char const *, int, uint64);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, CPlayerSlot );
SH_DECL_HOOK6_void(IServerGameClients, OnClientConnected, SH_NOATTRIB, 0, CPlayerSlot, const char*, uint64, const char *, const char *, bool);
SH_DECL_HOOK6(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, CPlayerSlot, const char*, uint64, const char *, bool, CBufferString *);
SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

SH_DECL_HOOK2_void( IServerGameClients, ClientCommand, SH_NOATTRIB, 0, CPlayerSlot, const CCommand & );

MMSPlugin g_ThisPlugin;
IServerGameDLL *server = NULL;
IServerGameClients *gameclients = NULL;
IVEngineServer *engine = NULL;
IGameEventManager2 *gameevents = NULL;
ICvar *icvar = NULL;

// Should only be called within the active game loop (i e map should be loaded and active)
// otherwise that'll be nullptr!
CGlobalVars *GetGameGlobals()
{
	INetworkGameServer *server = g_pNetworkServerService->GetIGameServer();

	if(!server)
		return nullptr;

	return g_pNetworkServerService->GetIGameServer()->GetGlobals();
}

void SampleCvarFChangeCB( CConVar<float> *cvar, CSplitScreenSlot slot, const float *new_val, const float *old_val )
{
	META_CONPRINTF( "Sample convar \"%s\" was changed from %f to %f [%s]\n",
					cvar->GetName(), *old_val, *new_val,
					// When convar is first initialised with a default value, it would have FCVAR_INITIAL_SETVALUE
					// flag set, so you can check for it if needed.
					cvar->IsFlagSet( FCVAR_INITIAL_SETVALUE ) ? "initialised" : "change" );
}

CConVar<int> sample_cvari("sample_cvari", FCVAR_NONE, "help string", 42);
CConVar<float> sample_cvarf("sample_cvarf", FCVAR_NONE, "help string", 69.69f, true, 10.0f, true, 100.0f, SampleCvarFChangeCB );

CON_COMMAND_F(sample_command, "Sample command", FCVAR_NONE)
{
	META_CONPRINTF( "Sample command called by %d. Command: %s\n", context.GetPlayerSlot(), args.GetCommandString() );
}

PLUGIN_EXPOSE(MMSPlugin, g_ThisPlugin);
bool MMSPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);

	// Currently doesn't work from within mm side, use GetGameGlobals() in the mean time instead
	// gpGlobals = ismm->GetCGlobals();

	// Required to get the IMetamodListener events
	g_SMAPI->AddListener( this, this );

	META_CONPRINTF( "Starting plugin.\n" );

	SH_ADD_HOOK(IServerGameDLL, GameFrame, server, SH_MEMBER(this, &MMSPlugin::Hook_GameFrame), true);
	SH_ADD_HOOK(IServerGameClients, ClientActive, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientActive), true);
	SH_ADD_HOOK(IServerGameClients, ClientDisconnect, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientDisconnect), true);
	SH_ADD_HOOK(IServerGameClients, ClientPutInServer, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientPutInServer), true);
	SH_ADD_HOOK(IServerGameClients, ClientSettingsChanged, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientSettingsChanged), false);
	SH_ADD_HOOK(IServerGameClients, OnClientConnected, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_OnClientConnected), false);
	SH_ADD_HOOK(IServerGameClients, ClientConnect, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientConnect), false);
	SH_ADD_HOOK(IServerGameClients, ClientCommand, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientCommand), false);

	META_CONPRINTF( "All hooks started!\n" );
	return true;
}

bool MMSPlugin::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, server, SH_MEMBER(this, &MMSPlugin::Hook_GameFrame), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientActive, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientActive), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientDisconnect), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientPutInServer, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientPutInServer), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientSettingsChanged, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientSettingsChanged), false);
	SH_REMOVE_HOOK(IServerGameClients, OnClientConnected, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_OnClientConnected), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientConnect, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientConnect), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientCommand, gameclients, SH_MEMBER(this, &MMSPlugin::Hook_ClientCommand), false);

	return true;
}

void MMSPlugin::AllPluginsLoaded()
{
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

void MMSPlugin::Hook_ClientActive( CPlayerSlot slot, bool bLoadGame, const char *pszName, uint64 xuid )
{
	META_CONPRINTF( "Hook_ClientActive(%d, %d, \"%s\", %d)\n", slot, bLoadGame, pszName, xuid );
}

void MMSPlugin::Hook_ClientCommand( CPlayerSlot slot, const CCommand &args )
{
	META_CONPRINTF( "Hook_ClientCommand(%d, \"%s\")\n", slot, args.GetCommandString() );
}

void MMSPlugin::Hook_ClientSettingsChanged( CPlayerSlot slot )
{
	META_CONPRINTF( "Hook_ClientSettingsChanged(%d)\n", slot );
}

void MMSPlugin::Hook_OnClientConnected( CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, const char *pszAddress, bool bFakePlayer )
{
	META_CONPRINTF( "Hook_OnClientConnected(%d, \"%s\", %d, \"%s\", \"%s\", %d)\n", slot, pszName, xuid, pszNetworkID, pszAddress, bFakePlayer );
}

bool MMSPlugin::Hook_ClientConnect( CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, bool unk1, CBufferString *pRejectReason )
{
	META_CONPRINTF( "Hook_ClientConnect(%d, \"%s\", %d, \"%s\", %d, \"%s\")\n", slot, pszName, xuid, pszNetworkID, unk1, pRejectReason->Get() );

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void MMSPlugin::Hook_ClientPutInServer( CPlayerSlot slot, char const *pszName, int type, uint64 xuid )
{
	META_CONPRINTF( "Hook_ClientPutInServer(%d, \"%s\", %d, %d)\n", slot, pszName, type, xuid );
}

void MMSPlugin::Hook_ClientDisconnect( CPlayerSlot slot, ENetworkDisconnectionReason reason, const char *pszName, uint64 xuid, const char *pszNetworkID )
{
	META_CONPRINTF( "Hook_ClientDisconnect(%d, %d, \"%s\", %d, \"%s\")\n", slot, reason, pszName, xuid, pszNetworkID );
}

void MMSPlugin::Hook_GameFrame( bool simulating, bool bFirstTick, bool bLastTick )
{
	/**
	 * simulating:
	 * ***********
	 * true  | game is ticking
	 * false | game is not ticking
	 */
}

void MMSPlugin::OnLevelInit( char const *pMapName,
									 char const *pMapEntities,
									 char const *pOldLevel,
									 char const *pLandmarkName,
									 bool loadGame,
									 bool background )
{
	META_CONPRINTF("OnLevelInit(%s)\n", pMapName);
}

void MMSPlugin::OnLevelShutdown()
{
	META_CONPRINTF("OnLevelShutdown()\n");
}
