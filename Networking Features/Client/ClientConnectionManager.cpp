#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "ClientConnectionManager.h"

// Outer-Engine includes

// Inter-Engine includes

#include "PrimeEngine/Lua/LuaEnvironment.h"

// additional lua includes needed
extern "C"
{
#include "PrimeEngine/../luasocket_dist/src/socket.h"
#include "PrimeEngine/../luasocket_dist/src/inet.h"
};

#include "PrimeEngine/../../GlobalConfig/GlobalConfig.h"

#include "PrimeEngine/Scene/DebugRenderer.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "CharacterControl/ClientGameObjectManagerAddon.h"


// Sibling/Children includes
using namespace PE::Events;

namespace PE
{
	namespace Components
	{
		PE_IMPLEMENT_CLASS1(ClientConnectionManager, ConnectionManager);

		t_socket ClientConnectionManager::udpSocket;
		ClientConnectionManager::ClientConnectionManager(PE::GameContext &context, PE::MemoryArena arena, PE::NetworkContext &netContext, Handle hMyself)
			: ConnectionManager(context, arena, netContext, hMyself)
		{
			m_isServer = false;
			// initialize udp socket
			unsigned short port = 54000;
			bool created = false;
			int numTries = 0;
			while (!created)
			{
				const char *err = inet_trycreate(&ClientConnectionManager::udpSocket, SOCK_DGRAM);
				if (err)
				{
					assert(!"error creating client udp socket occurred");
					break;
				}

				err = inet_trybind(&ClientConnectionManager::udpSocket, "0.0.0.0", port); // leaves socket non-blocking
				numTries++;
				if (err)
				{
					if (numTries >= 10)
					{
						break; // give up
					}

					err = inet_trycreate(&ClientConnectionManager::udpSocket, SOCK_DGRAM);
					port++;
				}
				else
				{
					created = true;
					break;
				}
			}

			if (created)
			{
				udpPort = port;
			}
			else
			{
				assert(!"could not create udp socket on client");
				return;
			}
		}

		ClientConnectionManager::~ClientConnectionManager() { }
		void ClientConnectionManager::initializeConnected(t_socket sock)
		{
			ConnectionManager::initializeConnected(sock);
		}

		void ClientConnectionManager::addDefaultComponents()
		{
			ConnectionManager::addDefaultComponents();
		}

#if 0 //template
		//////////////////////////////////////////////////////////////////////////
		// ConnectionManager Lua Interface
		//////////////////////////////////////////////////////////////////////////
		//

		void ClientConnectionManager::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
		{
			/*
			static const struct luaL_Reg l_functions[] = {
			{"l_clientConnectToTCPServer", l_clientConnectToTCPServer},
			{NULL, NULL} // sentinel
			};
			luaL_register(luaVM, 0, l_functions);

			VS

			lua_register(luaVM, "l_clientConnectToTCPServer", l_clientConnectToTCPServer);

			Note: registering with luaL_register is preferred since it is putting functions into
			table associated with this class
			using lua_register puts functions into global space
			*/
			static const struct luaL_Reg l_functions[] = {
				{"l_clientConnectToTCPServer", l_clientConnectToTCPServer},
				{NULL, NULL} // sentinel
			};

			luaL_register(luaVM, 0, l_functions);

			// run a script to add additional functionality to Lua side of the class
			// that is accessible from Lua
		// #if APIABSTRACTION_IOS
		// 	LuaEnvironment::Instance()->runScriptWorkspacePath("Code/PrimeEngine/Scene/Skin.lua");
		// #else
		// 	LuaEnvironment::Instance()->runScriptWorkspacePath("Code\\PrimeEngine\\Scene\\Skin.lua");
		// #endif

		}

		int ClientConnectionManager::l_clientConnectToTCPServer(lua_State *luaVM)
		{
			lua_Number lPort = lua_tonumber(luaVM, -1);
			int port = (int)(lPort);

			const char *strAddr = lua_tostring(luaVM, -2);

			GameContext *pContext = (GameContext *)(lua_touserdata(luaVM, -3));

			lua_pop(luaVM, 3);

			ClientConnectionManager *pClientConnManager = (ClientConnectionManager *)(pContext->getConnectionManager());
			pClientConnManager->clientConnectToTCPServer(strAddr, port);

			return 0; // no return values
		}
#endif
	}; // namespace Components
}; // namespace PE
