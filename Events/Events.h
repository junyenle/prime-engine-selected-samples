#ifndef _CHARACTER_CONTROL_EVENTS_
#define _CHARACTER_CONTROL_EVENTS_

#include "PrimeEngine/Events/StandardEvents.h"

namespace CharacterControl
{
	namespace Events
	{
		struct Event_Ack_S_to_C : public PE::Events::Event, public PE::Networkable
		{
			PE_DECLARE_CLASS(Event_Ack_S_to_C);
			PE_DECLARE_NETWORKABLE_CLASS;

			Event_Ack_S_to_C(PE::GameContext &context);

			// netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);

			// factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);

			int orderID;
			int clientID;

		};

		// soldier npc nudge
		struct Event_NudgeSoldier_C_to_S : public PE::Events::Event, public PE::Networkable
		{
			PE_DECLARE_CLASS(Event_NudgeSoldier_C_to_S);
			PE_DECLARE_NETWORKABLE_CLASS

				Event_NudgeSoldier_C_to_S(PE::GameContext &context);

			// netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);


			// factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);


			int direction;
			int soldierID;
			// Matrix4x4 m_transform;
		};


		struct Event_CreateSoldierNPC : public PE::Events::Event_CREATE_MESH
		{
			PE_DECLARE_CLASS(Event_CreateSoldierNPC);

			Event_CreateSoldierNPC(int &threadOwnershipMask) : PE::Events::Event_CREATE_MESH(threadOwnershipMask) {}
			// override SetLuaFunctions() since we are adding custom Lua interface
			static void SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM);

			// Lua interface prefixed with l_
			static int l_Construct(lua_State* luaVM);

			int m_npcType;
			int m_teamID;
			char m_gunMeshName[64];
			char m_gunMeshPackage[64];
			char m_patrolWayPoint[32];
		};

		struct Event_CreateTargetNPC : public PE::Events::Event_CREATE_MESH
		{
			PE_DECLARE_CLASS(Event_CreateTargetNPC);

			Event_CreateTargetNPC(int &threadOwnershipMask) : PE::Events::Event_CREATE_MESH(threadOwnershipMask) {}
			// override SetLuaFunctions() since we are adding custom Lua interface
			static void SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM);

			// Lua interface prefixed with l_
			static int l_Construct(lua_State* luaVM);

			int m_teamID;
			char m_meshName[64];
			char m_patrolWayPoint[32];
		};

		struct Event_CreateProjectile : public PE::Events::Event
		{
			PE_DECLARE_CLASS(Event_CreateProjectile);

			Event_CreateProjectile(Vector3 startPos, Vector3 targetPos, int teamID) 
			{
				startPosition = startPos;
				targetPosition = targetPos;
				this->teamID = teamID;
			}

			virtual ~Event_CreateProjectile() {}
			// override SetLuaFunctions() since we are adding custom Lua interface
			static void SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM);

			// Lua interface prefixed with l_
			static int l_Construct(lua_State* luaVM);

			Vector3 startPosition;
			Vector3 targetPosition;
			int teamID;
		};

		struct Event_InitProjectilePool : public PE::Events::Event
		{
			PE_DECLARE_CLASS(Event_InitProjectilePool);

			Event_InitProjectilePool(int &threadOwnershipMask);
			// override SetLuaFunctions() since we are adding custom Lua interface
			static void SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM);

			// Lua interface prefixed with l_
			static int l_Construct(lua_State* luaVM);
		};

		// soldier npc server events
		struct Event_MoveSoldier_C_to_S : public PE::Events::Event, public PE::Networkable
		{
			PE_DECLARE_CLASS(Event_MoveSoldier_C_to_S);
			PE_DECLARE_NETWORKABLE_CLASS

			Event_MoveSoldier_C_to_S(PE::GameContext &context);

			// netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);


			// factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);


			Vector4 targetPosition;
			int isRunning;
			int id;
			// Matrix4x4 m_transform;
		};

		struct Event_TimeSynch_C_to_S : public PE::Events::Event, public PE::Networkable 
		{
			PE_DECLARE_CLASS(Event_TimeSynch_C_to_S);
			PE_DECLARE_NETWORKABLE_CLASS

				Event_TimeSynch_C_to_S(PE::GameContext &context);

			// netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);

			int RTT;
			uint64_t RTTStartTime;
			int clientID;

			// factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);

		};

		struct Event_TimeSynch_S_to_C : public Event_TimeSynch_C_to_S
		{
			PE_DECLARE_CLASS(Event_TimeSynch_S_to_C);
			PE_DECLARE_NETWORKABLE_CLASS

				Event_TimeSynch_S_to_C(PE::GameContext &context);
			// netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);


			// factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);

			uint64_t RTTStartTime;
			uint64_t ServerTime;
		};

		struct Event_SpawnProjectile_C_to_S : public PE::Events::Event, public PE::Networkable
		{
			PE_DECLARE_CLASS(Event_SpawnProjectile_C_to_S);
			PE_DECLARE_NETWORKABLE_CLASS

			Event_SpawnProjectile_C_to_S(PE::GameContext &context);

			// netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);

			// factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);

			Vector4 startPosition;
			Vector4 endPosition;
			int teamID;
		};

		struct Event_MoveSoldier_S_to_C : public Event_MoveSoldier_C_to_S
		{
			PE_DECLARE_CLASS(Event_MoveSoldier_S_to_C);
			PE_DECLARE_NETWORKABLE_CLASS

				Event_MoveSoldier_S_to_C(PE::GameContext &context);
			// netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);


			// factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);
		};

		// soldier npc server events
		struct Event_SoldierAcquireTarget_C_to_S : public PE::Events::Event, public PE::Networkable
		{
			PE_DECLARE_CLASS(Event_SoldierAcquireTarget_C_to_S);
			PE_DECLARE_NETWORKABLE_CLASS

			Event_SoldierAcquireTarget_C_to_S(PE::GameContext &context);

			// netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);
			

			// factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);

			int targetID;
			int soldierID;
			// Matrix4x4 m_transform;
		};


		struct Event_SoldierAcquireTarget_S_to_C : public Event_SoldierAcquireTarget_C_to_S
		{
			PE_DECLARE_CLASS(Event_SoldierAcquireTarget_S_to_C);
			PE_DECLARE_NETWORKABLE_CLASS

				Event_SoldierAcquireTarget_S_to_C(PE::GameContext &context);
			// netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);


			// factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);
		};


		struct Event_MoveTank_C_to_S : public PE::Events::Event, public PE::Networkable
		{
			PE_DECLARE_CLASS(Event_MoveTank_C_to_S);
			PE_DECLARE_NETWORKABLE_CLASS

				Event_MoveTank_C_to_S(PE::GameContext &context);
			// Netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);


			// Factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);


			Matrix4x4 m_transform;
		};


		struct Event_MoveTank_S_to_C : public Event_MoveTank_C_to_S
		{
			PE_DECLARE_CLASS(Event_MoveTank_S_to_C);
			PE_DECLARE_NETWORKABLE_CLASS

				Event_MoveTank_S_to_C(PE::GameContext &context);
			// Netoworkable:
			virtual int packCreationData(char *pDataStream);
			virtual int constructFromStream(char *pDataStream);


			// Factory function used by network
			static void *FactoryConstruct(PE::GameContext&, PE::MemoryArena);

			int m_clientTankId;
		};


		// tank input controls

		struct Event_Tank_Throttle : public PE::Events::Event
		{
			PE_DECLARE_CLASS(Event_Tank_Throttle);

			Event_Tank_Throttle() {}
			virtual ~Event_Tank_Throttle() {}

			Vector3 m_relativeMove;
		};

		struct Event_Tank_Turn : public PE::Events::Event
		{
			PE_DECLARE_CLASS(Event_Tank_Turn);

			Event_Tank_Turn() {}
			virtual ~Event_Tank_Turn() {}

			Vector3 m_relativeRotate;
		};

	}; // namespace Events
}; // namespace CharacterControl

#endif
