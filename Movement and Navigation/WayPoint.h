#ifndef _CHARACTER_CONTROL_WAYPOINT_NPC_
#define _CHARACTER_CONTROL_WAYPOINT_NPC_

#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Math/Matrix4x4.h"

namespace CharacterControl
{
	namespace Events
	{
		struct Event_CREATE_WAYPOINT : public PE::Events::Event
		{
			PE_DECLARE_CLASS(Event_CREATE_WAYPOINT);

			// override SetLuaFunctions() since we are adding custom Lua interface
			static void SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM);

			// Lua interface prefixed with l_
			static int l_Construct(lua_State* luaVM);

			Matrix4x4 m_base;
			char m_name[32];
			char m_nextWaypointNames[3][32];
			int m_numberOfWaypoints;
			bool m_needToRunToNextWaypoint;

			PEUUID m_peuuid; // unique object id
		};
	}

	namespace Components
	{
		struct WayPoint : public PE::Components::Component
		{
			PE_DECLARE_CLASS(WayPoint);

			WayPoint(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, const Events::Event_CREATE_WAYPOINT *pEvt);

			virtual void addDefaultComponents();

			char m_name[32];
			char m_nextWayPointNames[3][32];
			int m_numberOfWaypoints;
			Matrix4x4 m_base;
			int m_needToRunToThisWaypoint;

		};
	}; // namespace Components
}; // namespace CharacterControl
#endif

