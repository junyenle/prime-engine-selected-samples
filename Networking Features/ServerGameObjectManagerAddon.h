#ifndef _CHARACTER_CONTROL_GAME_SERVER_OBJ_MANAGER_ADDON_
#define _CHARACTER_CONTROL_GAME_SERVER_OBJ_MANAGER_ADDON_

#include "GameObjectMangerAddon.h"
#include "Events/Events.h"
#include "CharacterControl/Navigator.h"

#include "WayPoint.h"

namespace CharacterControl
{
namespace Components
{

// This struct will be added to GameObjectManager as component
// as a result events sent to game object manager will be able to get to this component
// so we can create custom game objects through this class
struct ServerGameObjectManagerAddon : public GameObjectManagerAddon
{
	PE_DECLARE_SINGLETON_CLASS(ServerGameObjectManagerAddon); // creates a static handle and GteInstance*() methods. still need to create construct

	ServerGameObjectManagerAddon(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself) : GameObjectManagerAddon(context, arena, hMyself)
	{}

	// sub-component and event registration
	virtual void addDefaultComponents() ;

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_MoveTank);
	virtual void do_MoveTank(PE::Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_TimeSynch);
	virtual void do_TimeSynch(PE::Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_MoveSoldier);
	virtual void do_MoveSoldier(PE::Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_SpawnProjectile);
	virtual void do_SpawnProjectile(PE::Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_SoldierAcquireTarget);
	virtual void do_SoldierAcquireTarget(PE::Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_SoldierNudgeTarget);
	virtual void do_SoldierNudgeTarget(PE::Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
	virtual void do_UPDATE(PE::Events::Event *pEvt);

	void sendAckTo(int clientID, int orderID);

	static int RTT;

	//////////////////////////////////////////////////////////////////////////
	// Game Specific functionality
	//////////////////////////////////////////////////////////////////////////
	//
	static Navigator serverNavigator;
	static std::vector <std::pair<int, int>> needToAck;
};


}
}

#endif
