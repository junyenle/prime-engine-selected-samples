#include "PrimeEngine/PrimeEngineIncludes.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "Events.h"
#include "CharacterControl/ClientGameObjectManagerAddon.h"

#ifndef TIMENOW 
#define TIMENOW (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
#endif

using namespace PE;
namespace CharacterControl
{
	namespace Events
	{
		PE_IMPLEMENT_CLASS1(Event_Ack_S_to_C, Event);

		Event_Ack_S_to_C::Event_Ack_S_to_C(PE::GameContext &context)
			: Networkable(context, this)
		{

		}

		void *Event_Ack_S_to_C::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_Ack_S_to_C *pEvt = new (arena) Event_Ack_S_to_C(context);
			return pEvt;
		}
		
		int Event_Ack_S_to_C::packCreationData(char *pDataStream)
		{

			int size = 0;
			size += PE::Components::StreamManager::WriteInt32(orderID, &pDataStream[size]);
			size += PE::Components::StreamManager::WriteInt32(clientID, &pDataStream[size]);
			return size;
		}

		int Event_Ack_S_to_C::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], orderID);
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], clientID);
			return read;
		}
		

		PE_IMPLEMENT_CLASS1(Event_NudgeSoldier_C_to_S, Event);

		Event_NudgeSoldier_C_to_S::Event_NudgeSoldier_C_to_S(PE::GameContext &context)
			: Networkable(context, this)
		{

		}

		void *Event_NudgeSoldier_C_to_S::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_NudgeSoldier_C_to_S *pEvt = new (arena) Event_NudgeSoldier_C_to_S(context);
			return pEvt;
		}

		int Event_NudgeSoldier_C_to_S::packCreationData(char *pDataStream)
		{
			int size = 0;
			size += PE::Components::StreamManager::WriteInt32(direction, &pDataStream[size]);
			size += PE::Components::StreamManager::WriteInt32(soldierID, &pDataStream[size]);
			return size;
		}

		int Event_NudgeSoldier_C_to_S::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], direction);
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], soldierID);
			return read;
		}

		PE_IMPLEMENT_CLASS1(Event_CreateTargetNPC, PE::Events::Event_CREATE_MESH);

		void Event_CreateTargetNPC::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
		{
			static const struct luaL_Reg l_Event_CreateTargetNPC[] = 
			{
				{"Construct", l_Construct},
				{NULL, NULL} // sentinel
			};

			// register the functions in current lua table which is the table for Event_CreateTargetNPC
			luaL_register(luaVM, 0, l_Event_CreateTargetNPC);
		}

		PE_IMPLEMENT_CLASS1(Event_CreateProjectile, PE::Events::Event);

		void Event_CreateProjectile::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
		{
			static const struct luaL_Reg l_Event_CreateProjectile[] = 
			{
				{"Construct", l_Construct},
				{NULL, NULL} // sentinel
			};

			// register the functions in current lua table which is the table for Event_CreateTargetNPC
			luaL_register(luaVM, 0, l_Event_CreateProjectile);
		}

		int Event_CreateProjectile::l_Construct(lua_State* luaVM)
		{
			return 1;
		}

		int Event_CreateTargetNPC::l_Construct(lua_State* luaVM)
		{
			PE::Handle h("EVENT", sizeof(Event_CreateTargetNPC));

			// get arguments from stack
			int numArgs, numArgsConst;
			numArgs = numArgsConst = 18; 

			PE::GameContext *pContext = (PE::GameContext*)(lua_touserdata(luaVM, -numArgs--));
			// this function should only be called frm game thread, so we can use game thread thread owenrship mask
			Event_CreateTargetNPC *pEvt = new(h) Event_CreateTargetNPC(pContext->m_gameThreadThreadOwnershipMask);

			const char* name = lua_tostring(luaVM, -numArgs--);
			const char* package = lua_tostring(luaVM, -numArgs--);

			float positionFactor = 1.0f / 100.0f;
			Vector3 playerPos, u, v, n;
			playerPos.m_x = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
			playerPos.m_y = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
			playerPos.m_z = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
			u.m_x = (float)lua_tonumber(luaVM, -numArgs--); u.m_y = (float)lua_tonumber(luaVM, -numArgs--); u.m_z = (float)lua_tonumber(luaVM, -numArgs--);
			v.m_x = (float)lua_tonumber(luaVM, -numArgs--); v.m_y = (float)lua_tonumber(luaVM, -numArgs--); v.m_z = (float)lua_tonumber(luaVM, -numArgs--);
			n.m_x = (float)lua_tonumber(luaVM, -numArgs--); n.m_y = (float)lua_tonumber(luaVM, -numArgs--); n.m_z = (float)lua_tonumber(luaVM, -numArgs--);
			pEvt->m_peuuid = LuaGlue::readPEUUID(luaVM, -numArgs--);

			const char* wayPointName = NULL;
			if (!lua_isnil(luaVM, -numArgs))
			{
				// have patrol waypoint name
				wayPointName = lua_tostring(luaVM, -numArgs--);
			}
			else
			{
				numArgs--;
			}
			int teamID = lua_tonumber(luaVM, -numArgs--);

			// set data values before popping memory off stack
			StringOps::writeToString(name, pEvt->m_meshFilename, 255);
			StringOps::writeToString(package, pEvt->m_package, 255);
			StringOps::writeToString(wayPointName, pEvt->m_patrolWayPoint, 32);

			lua_pop(luaVM, numArgsConst); 

			pEvt->hasCustomOrientation = true;
			pEvt->m_pos = playerPos;
			pEvt->m_u = u;
			pEvt->m_v = v;
			pEvt->m_n = n;
			pEvt->m_teamID = teamID;

			LuaGlue::pushTableBuiltFromHandle(luaVM, h);

			return 1;
		}

		PE_IMPLEMENT_CLASS1(Event_InitProjectilePool, PE::Events::Event);

		void Event_InitProjectilePool::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
		{
			static const struct luaL_Reg l_Event_InitProjectilePool[] = 
			{
				{"Construct", l_Construct},
				{NULL, NULL} // sentinel
			};

			// register the functions in current lua table which is the table for Event_CreateTargetNPC
			luaL_register(luaVM, 0, l_Event_InitProjectilePool);
		}

		int Event_InitProjectilePool::l_Construct(lua_State* luaVM)
		{
			return 1;
		}

		PE_IMPLEMENT_CLASS1(Event_CreateSoldierNPC, PE::Events::Event_CREATE_SKELETON);

		void Event_CreateSoldierNPC::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
		{
			static const struct luaL_Reg l_Event_CreateSoldierNPC[] = {
				{"Construct", l_Construct},
				{NULL, NULL} // sentinel
			};

			// register the functions in current lua table which is the table for Event_CreateSoldierNPC
			luaL_register(luaVM, 0, l_Event_CreateSoldierNPC);
		}

		int Event_CreateSoldierNPC::l_Construct(lua_State* luaVM)
		{
			PE::Handle h("EVENT", sizeof(Event_CreateSoldierNPC));

			// get arguments from stack
			int numArgs, numArgsConst;
			numArgs = numArgsConst = 20;

			PE::GameContext *pContext = (PE::GameContext*)(lua_touserdata(luaVM, -numArgs--));

			// this function should only be called frm game thread, so we can use game thread thread owenrship mask
			Event_CreateSoldierNPC *pEvt = new(h) Event_CreateSoldierNPC(pContext->m_gameThreadThreadOwnershipMask);

			const char* name = lua_tostring(luaVM, -numArgs--);
			const char* package = lua_tostring(luaVM, -numArgs--);

			const char* gunMeshName = lua_tostring(luaVM, -numArgs--);
			const char* gunMeshPackage = lua_tostring(luaVM, -numArgs--);

			float positionFactor = 1.0f / 100.0f;

			Vector3 playerPos, u, v, n;
			playerPos.m_x = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
			playerPos.m_y = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
			playerPos.m_z = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;

			u.m_x = (float)lua_tonumber(luaVM, -numArgs--); u.m_y = (float)lua_tonumber(luaVM, -numArgs--); u.m_z = (float)lua_tonumber(luaVM, -numArgs--);
			v.m_x = (float)lua_tonumber(luaVM, -numArgs--); v.m_y = (float)lua_tonumber(luaVM, -numArgs--); v.m_z = (float)lua_tonumber(luaVM, -numArgs--);
			n.m_x = (float)lua_tonumber(luaVM, -numArgs--); n.m_y = (float)lua_tonumber(luaVM, -numArgs--); n.m_z = (float)lua_tonumber(luaVM, -numArgs--);

			pEvt->m_peuuid = LuaGlue::readPEUUID(luaVM, -numArgs--);

			const char* wayPointName = NULL;

			if (!lua_isnil(luaVM, -numArgs))
			{
				// have patrol waypoint name
				wayPointName = lua_tostring(luaVM, -numArgs--);
			}
			else
				// ignore
				numArgs--;

			int teamID = lua_tonumber(luaVM, -numArgs--);

			// set data values before popping memory off stack
			StringOps::writeToString(name, pEvt->m_meshFilename, 255);
			StringOps::writeToString(package, pEvt->m_package, 255);

			StringOps::writeToString(gunMeshName, pEvt->m_gunMeshName, 64);
			StringOps::writeToString(gunMeshPackage, pEvt->m_gunMeshPackage, 64);
			StringOps::writeToString(wayPointName, pEvt->m_patrolWayPoint, 32);

			lua_pop(luaVM, numArgsConst); //Second arg is a count of how many to pop

			pEvt->hasCustomOrientation = true;
			pEvt->m_pos = playerPos;
			pEvt->m_u = u;
			pEvt->m_v = v;
			pEvt->m_n = n;
			pEvt->m_teamID = teamID;

			LuaGlue::pushTableBuiltFromHandle(luaVM, h);

			return 1;
		}
		
		PE_IMPLEMENT_CLASS1(Event_MoveSoldier_C_to_S, Event);

		Event_MoveSoldier_C_to_S::Event_MoveSoldier_C_to_S(PE::GameContext &context)
			: Networkable(context, this)
		{

		}

		void *Event_MoveSoldier_C_to_S::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_MoveSoldier_C_to_S *pEvt = new (arena) Event_MoveSoldier_C_to_S(context);
			return pEvt;
		}

		int Event_MoveSoldier_C_to_S::packCreationData(char *pDataStream)
		{
			int size = 0;
			size += PE::Components::StreamManager::WriteVector4(targetPosition, &pDataStream[size]);
			size += PE::Components::StreamManager::WriteInt32(isRunning, &pDataStream[size]);
			size += PE::Components::StreamManager::WriteInt32(id, &pDataStream[size]);
			return size;
		}

		int Event_MoveSoldier_C_to_S::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += PE::Components::StreamManager::ReadVector4(&pDataStream[read], targetPosition);
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], isRunning);
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], id);
			return read;
		}


		PE_IMPLEMENT_CLASS1(Event_SpawnProjectile_C_to_S, Event);

		Event_SpawnProjectile_C_to_S::Event_SpawnProjectile_C_to_S(PE::GameContext &context)
			: Networkable(context, this)
		{
		}

		void *Event_SpawnProjectile_C_to_S::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_SpawnProjectile_C_to_S *pEvt = new (arena) Event_SpawnProjectile_C_to_S(context);
			return pEvt;
		}

		int Event_SpawnProjectile_C_to_S::packCreationData(char *pDataStream)
		{
			int size = 0;
			size += PE::Components::StreamManager::WriteVector4(startPosition, &pDataStream[size]);
			size += PE::Components::StreamManager::WriteVector4(endPosition, &pDataStream[size]);
			size += PE::Components::StreamManager::WriteInt32(teamID, &pDataStream[size]);
			return size;
		}

		int Event_SpawnProjectile_C_to_S::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += PE::Components::StreamManager::ReadVector4(&pDataStream[read], startPosition);
			read += PE::Components::StreamManager::ReadVector4(&pDataStream[read], endPosition);
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], teamID);
			return read;
		}

		PE_IMPLEMENT_CLASS1(Event_TimeSynch_C_to_S, Event);
		Event_TimeSynch_C_to_S::Event_TimeSynch_C_to_S(PE::GameContext &context)
			: Networkable(context, this)
		{
			RTTStartTime = TIMENOW;
			RTT = CharacterControl::Components::ClientGameObjectManagerAddon::RTT;
			clientID = CharacterControl::Components::ClientGameObjectManagerAddon::clientID;
		}

		void *Event_TimeSynch_C_to_S::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_TimeSynch_C_to_S *pEvt = new (arena) Event_TimeSynch_C_to_S(context);
			return pEvt;
		}

		int Event_TimeSynch_C_to_S::packCreationData(char *pDataStream)
		{
			int size = 0;
			size += PE::Components::StreamManager::WriteInt32(RTT, &pDataStream[size]);
			size += PE::Components::StreamManager::WriteInt64(RTTStartTime, &pDataStream[size]);
			size += PE::Components::StreamManager::WriteInt32(clientID, &pDataStream[size]);
			return size;
		}

		int Event_TimeSynch_C_to_S::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], RTT);
			read += PE::Components::StreamManager::ReadInt64(&pDataStream[read], RTTStartTime);
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], clientID);
			return read;
		}

		PE_IMPLEMENT_CLASS1(Event_TimeSynch_S_to_C, Event_TimeSynch_C_to_S);
		Event_TimeSynch_S_to_C::Event_TimeSynch_S_to_C(PE::GameContext &context)
			: Event_TimeSynch_C_to_S(context)
		{
		}

		void *Event_TimeSynch_S_to_C::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_TimeSynch_S_to_C *pEvt = new (arena) Event_TimeSynch_S_to_C(context);
			return pEvt;
		}

		int Event_TimeSynch_S_to_C::packCreationData(char *pDataStream)
		{
			
			int size = 0;
			size += PE::Components::StreamManager::WriteInt64(RTTStartTime, &pDataStream[size]);
			size += PE::Components::StreamManager::WriteInt64(TIMENOW, &pDataStream[size]);
			return size;
		}

		int Event_TimeSynch_S_to_C::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += PE::Components::StreamManager::ReadInt64(&pDataStream[read], RTTStartTime);
			read += PE::Components::StreamManager::ReadInt64(&pDataStream[read], ServerTime);
			return read;
		}

		PE_IMPLEMENT_CLASS1(Event_MoveSoldier_S_to_C, Event_MoveSoldier_C_to_S);
		Event_MoveSoldier_S_to_C::Event_MoveSoldier_S_to_C(PE::GameContext &context)
			: Event_MoveSoldier_C_to_S(context)
		{

		}

		void *Event_MoveSoldier_S_to_C::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_MoveSoldier_S_to_C *pEvt = new (arena) Event_MoveSoldier_S_to_C(context);
			return pEvt;
		}

		int Event_MoveSoldier_S_to_C::packCreationData(char *pDataStream)
		{
			int size = 0;
			size += Event_MoveSoldier_C_to_S::packCreationData(&pDataStream[size]);
			return size;
		}

		int Event_MoveSoldier_S_to_C::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += Event_MoveSoldier_C_to_S::constructFromStream(&pDataStream[read]);
			return read;
		}

		PE_IMPLEMENT_CLASS1(Event_SoldierAcquireTarget_C_to_S, Event);

		Event_SoldierAcquireTarget_C_to_S::Event_SoldierAcquireTarget_C_to_S(PE::GameContext &context)
			: Networkable(context, this)
		{

		}

		void *Event_SoldierAcquireTarget_C_to_S::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_SoldierAcquireTarget_C_to_S *pEvt = new (arena) Event_SoldierAcquireTarget_C_to_S(context);
			return pEvt;
		}

		int Event_SoldierAcquireTarget_C_to_S::packCreationData(char *pDataStream)
		{
			int size = 0;
			size += PE::Components::StreamManager::WriteInt32(targetID, &pDataStream[size]);
			size += PE::Components::StreamManager::WriteInt32(soldierID, &pDataStream[size]);
			return size;
		}

		int Event_SoldierAcquireTarget_C_to_S::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], targetID);
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], soldierID);
			return read;
		}

		PE_IMPLEMENT_CLASS1(Event_SoldierAcquireTarget_S_to_C, Event_SoldierAcquireTarget_C_to_S);
		Event_SoldierAcquireTarget_S_to_C::Event_SoldierAcquireTarget_S_to_C(PE::GameContext &context)
			: Event_SoldierAcquireTarget_C_to_S(context)
		{

		}

		void *Event_SoldierAcquireTarget_S_to_C::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_SoldierAcquireTarget_S_to_C *pEvt = new (arena) Event_SoldierAcquireTarget_S_to_C(context);
			return pEvt;
		}

		int Event_SoldierAcquireTarget_S_to_C::packCreationData(char *pDataStream)
		{
			int size = 0;
			size += Event_SoldierAcquireTarget_C_to_S::packCreationData(&pDataStream[size]);
			return size;
		}

		int Event_SoldierAcquireTarget_S_to_C::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += Event_SoldierAcquireTarget_C_to_S::constructFromStream(&pDataStream[read]);
			return read;
		}

		PE_IMPLEMENT_CLASS1(Event_MoveTank_C_to_S, Event);

		Event_MoveTank_C_to_S::Event_MoveTank_C_to_S(PE::GameContext &context)
			: Networkable(context, this)
		{

		}

		void *Event_MoveTank_C_to_S::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_MoveTank_C_to_S *pEvt = new (arena) Event_MoveTank_C_to_S(context);
			return pEvt;
		}

		int Event_MoveTank_C_to_S::packCreationData(char *pDataStream)
		{
			return PE::Components::StreamManager::WriteMatrix4x4(m_transform, pDataStream);
		}

		int Event_MoveTank_C_to_S::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += PE::Components::StreamManager::ReadMatrix4x4(&pDataStream[read], m_transform);
			return read;
		}


		PE_IMPLEMENT_CLASS1(Event_MoveTank_S_to_C, Event_MoveTank_C_to_S);

		Event_MoveTank_S_to_C::Event_MoveTank_S_to_C(PE::GameContext &context)
			: Event_MoveTank_C_to_S(context)
		{

		}

		void *Event_MoveTank_S_to_C::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Event_MoveTank_S_to_C *pEvt = new (arena) Event_MoveTank_S_to_C(context);
			return pEvt;
		}

		int Event_MoveTank_S_to_C::packCreationData(char *pDataStream)
		{
			int size = 0;
			size += Event_MoveTank_C_to_S::packCreationData(&pDataStream[size]);
			size += PE::Components::StreamManager::WriteInt32(m_clientTankId, &pDataStream[size]);
			return size;
		}

		int Event_MoveTank_S_to_C::constructFromStream(char *pDataStream)
		{
			int read = 0;
			read += Event_MoveTank_C_to_S::constructFromStream(&pDataStream[read]);
			read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], m_clientTankId);
			return read;
		}

		PE_IMPLEMENT_CLASS1(Event_Tank_Throttle, Event);

		PE_IMPLEMENT_CLASS1(Event_Tank_Turn, Event);


	};
};

