#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "StreamManager.h"

// Outer-Engine includes

// Inter-Engine includes

#include "../Lua/LuaEnvironment.h"

// additional lua includes needed
extern "C"
{
#include "../../luasocket_dist/src/socket.h"
#include "../../luasocket_dist/src/inet.h"
};

#include "../../../GlobalConfig/GlobalConfig.h"
#include "PrimeEngine/Events/StandardEvents.h"

// Sibling/Children includes
#include "EventManager.h"
#include "Tribes/GhostManager/GhostManager.h"
#include "ConnectionManager.h"
#include "Client/ClientConnectionManager.h"
#include "Server/ServerConnectionManager.h"
#include "CharacterControl/ClientGameObjectManagerAddon.h"
#include "CharacterControl/ServerGameObjectManagerAddon.h"

#if APIABSTRACTION_PS3
#define NET_LITTLE_ENDIAN 1
#else
#define NET_LITTLE_ENDIAN 0
#endif

#ifndef TIMENOW 
#define TIMENOW (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
#endif

using namespace PE::Events;

namespace PE
{
	namespace Components
	{

		PE_IMPLEMENT_CLASS1(StreamManager, Component);

		StreamManager::StreamManager(PE::GameContext &context, PE::MemoryArena arena, PE::NetworkContext &netContext, Handle hMyself, bool isServer)
			: Component(context, arena, hMyself)
			, m_transmissionRecords(1024)
		{
			m_nextIdToTransmit = 0;
			m_pNetContext = &netContext;
			m_isServer = isServer;
		}

		StreamManager::~StreamManager()
		{

		}

		void StreamManager::initialize()
		{

		}

		int StreamManager::delayTime = 0;
		int StreamManager::serverDelayTime = -1;
		int StreamManager::packetDrop = 0;
		void StreamManager::sendNextPackets()
		{
			//if (m_isServer)
			//{
			//	char buf[256];
			//	buf[0] = 'b'; buf[1] = 'y'; buf[2] = 'e'; buf[3] = '\0';
			//	size_t readAmount;

			//	sockaddr_in serverHint = {};
			//	socklen_t serverLen = sizeof(serverHint);
			//	serverHint.sin_family = AF_INET;
			//	serverHint.sin_port = htons(54000);
			//	inet_pton(AF_INET, "127.0.0.1", &serverHint.sin_addr);
			//	t_timeout timeout; // timeout supports managing timeouts of multiple blocking alls by using total.
			//	// but if total is < 0 it just uses block value for each blocking call
			//	timeout.block = PE_SOCKET_SEND_TIMEOUT;
			//	timeout.total = -1.0;
			//	timeout.start = 0;
			//	p_timeout tm = &timeout;

			//	socket_recvfrom(&ServerConnectionManager::udpSocket, buf, 6, &readAmount, (SA *)&serverHint, &serverLen, tm);

			//	//char err_buf[256];
			//	//sprintf(err_buf, "ERROR: %d", WSAGetLastError());
			//	//OutputDebugStringA(err_buf);
			//	if (readAmount > 0)
			//	{
			//		OutputDebugStringA(buf);
			//	}
			//}
			//else
			//{
			//	char buf[] = "hello";
			//	size_t amountSent;
			//	sockaddr_in serverHint = {};
			//	serverHint.sin_family = AF_INET;
			//	serverHint.sin_port = htons(44000);
			//	inet_pton(AF_INET, "127.0.0.1", &serverHint.sin_addr);
			//	t_timeout timeout; // timeout supports managing timeouts of multiple blocking alls by using total.
			//	// but if total is < 0 it just uses block value for each blocking call
			//	timeout.block = PE_SOCKET_SEND_TIMEOUT;
			//	timeout.total = -1.0;
			//	timeout.start = 0;
			//	p_timeout tm = &timeout;
			//	socket_sendto(&ClientConnectionManager::udpSocket, buf, 6, &amountSent, (SA *)&serverHint, sizeof(serverHint), tm);
			//	//char err_buf[256];
			//	//sprintf(err_buf, "ERROR: %d", WSAGetLastError());
			//	//OutputDebugStringA(err_buf);
			//	//int a = 3;
			//}

			int actualDelayTime = m_isServer ? serverDelayTime : delayTime;
			if (!delayedPackets.empty())
			{
				DelayedPacket dp = delayedPackets.front();
				if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - dp.pushTime).count() >= actualDelayTime)
				{
					m_pNetContext->getConnectionManager()->sendPacket(dp.packet, dp.record); // note, we currently have automatic fake acknowldgements so transmission record will be popped in this method
					delayedPackets.pop_front();
					pefree(m_arena, dp.packet);
				}
			}

			while (true)
			{
				int size;
				if (lastPacket)
				{
					size = lastPacketSize;
				}
				else
				{
					size = PE_PACKET_HEADER;
				}

				int sizeLeft = PE_PACKET_TOTAL_SIZE - size;

				// allocate data for next packet

				for (int i = 0; i < EventManager::SendingSlidingWindowSize && !m_isServer; ++i)
				{
					if (EventManager::SendingSlidingWindow[i].isAcked) { continue; }
					else if (TIMENOW - EventManager::SendingSlidingWindow[i].LastTimeSent > 80)
					{
						m_pNetContext->getEventManager()->m_eventsToSend.push_front(EventManager::SendingSlidingWindow[i].data);
						char buf[256];
						if (m_isServer)
						{
							OutputDebugStringA("Server... ");
						}
						else
						{
							char buf[32];
							sprintf(buf, "Client %d...", CharacterControl::Components::ClientGameObjectManagerAddon::clientID);
							OutputDebugStringA(buf);
						}

						sprintf(buf, "resending event %d\n", EventManager::SendingSlidingWindow[i].orderID);
						OutputDebugStringA(buf);
						EventManager::SendingSlidingWindow[i].LastTimeSent = TIMENOW;
					}
				}

				int numEvents = m_pNetContext->getEventManager()->haveEventsToSend();

				//todo: other managers
				int numGhosts = m_pNetContext->getGhostManager()->haveGhostsToSend();
				lastNumGhosts += numGhosts;

				if (numEvents || numGhosts || (lastPacketSize != -1 && lastPacketSize != 0)) //todo: other managers
				{
					m_transmissionRecords.push_back(TransmissionRecord());
					TransmissionRecord &record = m_transmissionRecords.back();

					PE::Packet *pPacket;
					if (lastPacket == NULL)
					{
						pPacket = (PE::Packet *)(pemalloc(m_arena, PE_PACKET_TOTAL_SIZE));
					}
					else
					{
						pPacket = lastPacket;
					}

					bool usefulEventDataSent = false;
					bool wantToSendMoreEvents = false;
					//event manager
					if (numEvents)
					{
						PrimitiveTypes::Int32 isEvent = (PrimitiveTypes::Int32)StreamManager::PACKED_EVENT;
						size += StreamManager::WriteInt32(isEvent, &pPacket->m_data[size]);

						sizeLeft = PE_PACKET_TOTAL_SIZE - size;
						size += m_pNetContext->getEventManager()->fillInNextPacket(&pPacket->m_data[size], &record, sizeLeft, usefulEventDataSent, wantToSendMoreEvents);
					}

					//other managers fillin here

					bool usefulGhostDataSent = false;
					bool wantToSendMoreGhosts = false;

					// ghost manager
					if (numGhosts)
					{
						PrimitiveTypes::Int32 isGhost = (PrimitiveTypes::Int32)StreamManager::PACKED_GHOST;
						size += StreamManager::WriteInt32(isGhost, &pPacket->m_data[size]);

						sizeLeft = PE_PACKET_TOTAL_SIZE - size;
						size += m_pNetContext->getGhostManager()->fillInNextPacket(&pPacket->m_data[size], &record, sizeLeft, usefulGhostDataSent, wantToSendMoreGhosts);
					}

					assert(size > PE_PACKET_HEADER);// we should have filled in something!

					lastPacketSize = size;
					//if (true)
						// RATE LIMIT HERE
					//int dynamicLastNumGhostsLimit = 200;
					//// TUNE THIS
					//if (m_isServer)
					//{
					//	dynamicLastNumGhostsLimit += CharacterControl::Components::ServerGameObjectManagerAddon::RTT;
					//}
					//else
					//{
					//	dynamicLastNumGhostsLimit += CharacterControl::Components::ClientGameObjectManagerAddon::RTT;
					//}

					static int dynamicLastNumGhostsLimit = 250;
					int RTT;
					// TUNE THIS
					if (m_isServer)
					{
						//dynamicLastNumGhostsLimit += CharacterControl::Components::ServerGameObjectManagerAddon::RTT;
						RTT = CharacterControl::Components::ServerGameObjectManagerAddon::RTT;
					}
					else
					{
						RTT = CharacterControl::Components::ClientGameObjectManagerAddon::RTT;
					}

					int delayTime = m_isServer ? StreamManager::serverDelayTime : StreamManager::delayTime;
					if (RTT > 60 + delayTime)
					{
						dynamicLastNumGhostsLimit += 3 * (RTT - 60);
					}
					else if (RTT < 20 + delayTime)
					{
						dynamicLastNumGhostsLimit -= (20 - RTT);
					}

					dynamicLastNumGhostsLimit = min(500, dynamicLastNumGhostsLimit);
					dynamicLastNumGhostsLimit = max(100, dynamicLastNumGhostsLimit);

					//char buf[256];
					//sprintf(buf, "Dynamic Ghost Limit: %d\n", dynamicLastNumGhostsLimit);
					//OutputDebugStringA(buf);
					if (true)
					//if (numEvents || (lastTimeSent == -1 || TIMENOW - lastTimeSent > 30) || (lastNumGhosts >= dynamicLastNumGhostsLimit || lastNumGhosts == 0))
					{
						if (size > PE_PACKET_HEADER)
						{
							StreamManager::WriteInt32(size, &pPacket->m_data[0] /*= &pPacket->m_packetDataSizeInInet*/); // header was allocated in the beginning
							record.m_id = ++m_nextIdToTransmit;
							DelayedPacket delayedPacket = {};
							delayedPacket.packet = pPacket;
							delayedPacket.record = &record;
							delayedPacket.pushTime = std::chrono::system_clock::now();

							int percentValue = (rand() % 100) + 1;
							if (percentValue > packetDrop)
							{
								delayedPackets.push_back(delayedPacket);
							}
						}
						else
						{
							PEASSERT(false, "Though we had something to send yet no useful data was filled in into packet! events expected: %d event data sent: %d ghosts expected: %d ghost data sent :%d", numEvents, (int)(usefulEventDataSent), numGhosts, (int)(usefulGhostDataSent));

							m_transmissionRecords.pop_back(); // cleanup failed transmission record
						}

						lastNumGhosts = 0;
						lastPacket = NULL;
						lastPacketSize = 0;
						lastTimeSent = TIMENOW;
						if (!wantToSendMoreEvents && !wantToSendMoreGhosts)
							return;

					}
					else
					{
						lastPacket = pPacket;
					}
				}
				else
				{
					return;
				}
			}
		}

		void StreamManager::processNotification(bool delivered)
		{
			TransmissionRecord &record = m_transmissionRecords.front();


			m_pNetContext->getEventManager()->processNotification(&record, delivered);

			// todo: other managers here..
			//m_pNetContext->getGhostManager()->processNotification(&record, delivered);

			m_transmissionRecords.pop_front();
		}


		void StreamManager::do_UPDATE(Events::Event *pEvt)
		{
			static bool first = true;
			if (first && m_isServer)
			{
				first = false;
				serverDelayTime = 0;
			}

			sendNextPackets();
		}

		void StreamManager::addDefaultComponents()
		{
			Component::addDefaultComponents();
			PE_REGISTER_EVENT_HANDLER(Events::Event_UPDATE, StreamManager::do_UPDATE);
		}

#if 0 // template
		//////////////////////////////////////////////////////////////////////////
		// ConnectionManager Lua Interface
		//////////////////////////////////////////////////////////////////////////
		//
		void ConnectionManager::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
		{
			/*
			static const struct luaL_Reg l_functions[] = {
				{"l_clientConnectToTCPServer", l_clientConnectToTCPServer},
				{NULL, NULL} // sentinel
			};

			luaL_register(luaVM, 0, l_functions);
			*/

			lua_register(luaVM, "l_clientConnectToTCPServer", l_clientConnectToTCPServer);


			// run a script to add additional functionality to Lua side of Skin
			// that is accessible from Lua
		// #if APIABSTRACTION_IOS
		// 	LuaEnvironment::Instance()->runScriptWorkspacePath("Code/PrimeEngine/Scene/Skin.lua");
		// #else
		// 	LuaEnvironment::Instance()->runScriptWorkspacePath("Code\\PrimeEngine\\Scene\\Skin.lua");
		// #endif

		}

		int ConnectionManager::l_clientConnectToTCPServer(lua_State *luaVM)
		{
			lua_Number lPort = lua_tonumber(luaVM, -1);
			int port = (int)(lPort);

			const char *strAddr = lua_tostring(luaVM, -2);

			GameContext *pContext = (GameContext *)(lua_touserdata(luaVM, -3));

			lua_pop(luaVM, 3);

			pContext->getConnectionManager()->clientConnectToTCPServer(strAddr, port);

			return 0; // no return values
		}
#endif

		// Sending functionality

		void StreamManager::receivePacket(Packet *pPacket)
		{
			int read = 0;

			PrimitiveTypes::Int32 packetSize;
			read += StreamManager::ReadInt32(&pPacket->m_data[read], packetSize);

			// events are packed first

			int ghostOrEvent;
			read += StreamManager::ReadInt32(&pPacket->m_data[read], ghostOrEvent);

			while (packetSize != read)
			{
				bool eventOccured = false;
				if (ghostOrEvent == StreamManager::PACKED_EVENT)
				{
					read += m_pNetContext->getEventManager()->receiveNextPacket(&pPacket->m_data[read]);
					eventOccured = true;
				}

				if (eventOccured)
				{
					if (read != packetSize)
					{
						read += StreamManager::ReadInt32(&pPacket->m_data[read], ghostOrEvent);
					}
				}

				if (ghostOrEvent == StreamManager::PACKED_GHOST)
				{
					read += m_pNetContext->getGhostManager()->receiveNextPacket(&pPacket->m_data[read]);
				}

				if (packetSize != read)
				{
					read += StreamManager::ReadInt32(&pPacket->m_data[read], ghostOrEvent);
				}
			}

			assert(packetSize == read);
		}



		//////////////////////////////////////////////////////////////////////////
		// utils

		void swapEndian32(void *val)
		{
			unsigned int *ival = (unsigned int *)val;
			*ival = ((*ival >> 24) & 0x000000ff) |
				((*ival >> 8) & 0x0000ff00) |
				((*ival << 8) & 0x00ff0000) |
				((*ival << 24) & 0xff000000);
		}


		void swapEndian64(void *val)
		{
			uint64_t *ival = (uint64_t *)val;
			*ival =
				((*ival >> 56) & 0x00000000000000ff) |
				((*ival >> 40) & 0x000000000000ff00) |
				((*ival >> 24) & 0x0000000000ff0000) |
				((*ival >> 8) & 0x00000000ff000000) |
				((*ival << 8) & 0x000000ff00000000) |
				((*ival << 24) & 0x0000ff0000000000) |
				((*ival << 40) & 0x00ff000000000000) |
				((*ival << 56) & 0xff00000000000000);
		}

		int StreamManager::WriteInt64(PrimitiveTypes::UInt64 v, char *pDataStream)
		{
#if !NET_LITTLE_ENDIAN 
			swapEndian64(&v);
#endif

			memcpy(pDataStream, &v, sizeof(PrimitiveTypes::UInt64));
			return sizeof(PrimitiveTypes::UInt64);
		}

		int StreamManager::ReadInt64(char *pDataStream, PrimitiveTypes::UInt64 &out_v)
		{
			PrimitiveTypes::UInt64 v;
			memcpy(&v, pDataStream, sizeof(PrimitiveTypes::UInt64));

#if !NET_LITTLE_ENDIAN 
			swapEndian64(&v);
#endif

			out_v = v;

			return sizeof(PrimitiveTypes::UInt64);
		}

		int StreamManager::WriteInt32(PrimitiveTypes::Int32 v, char *pDataStream)
		{
#if !NET_LITTLE_ENDIAN 
			swapEndian32(&v);
#endif

			memcpy(pDataStream, &v, sizeof(PrimitiveTypes::Int32));
			return sizeof(PrimitiveTypes::Int32);
		}

		int StreamManager::ReadInt32(char *pDataStream, PrimitiveTypes::Int32 &out_v)
		{
			PrimitiveTypes::Int32 v;
			memcpy(&v, pDataStream, sizeof(PrimitiveTypes::Int32));

#if !NET_LITTLE_ENDIAN 
			swapEndian32(&v);
#endif

			out_v = v;

			return sizeof(PrimitiveTypes::Int32);
		}

		int StreamManager::WriteFloat32(PrimitiveTypes::Float32 v, char *pDataStream)
		{
#if !NET_LITTLE_ENDIAN 
			swapEndian32(&v);
#endif

			memcpy(pDataStream, &v, sizeof(PrimitiveTypes::Float32));
			return sizeof(PrimitiveTypes::Float32);
		}

		int StreamManager::ReadFloat32(char *pDataStream, PrimitiveTypes::Float32 &out_v)
		{
			PrimitiveTypes::Float32 v;
			memcpy(&v, pDataStream, sizeof(PrimitiveTypes::Float32));

#if !NET_LITTLE_ENDIAN 
			swapEndian32(&v);
#endif

			out_v = v;

			return sizeof(PrimitiveTypes::Float32);
		}

		int StreamManager::WriteVector4(Vector4 v, char *pDataStream)
		{
			int size = 0;
			size += WriteFloat32(v.m_x, &pDataStream[size]);
			size += WriteFloat32(v.m_y, &pDataStream[size]);
			size += WriteFloat32(v.m_z, &pDataStream[size]);
			size += WriteFloat32(v.m_w, &pDataStream[size]);
			return size;
		}

		int StreamManager::ReadVector4(char *pDataStream, Vector4 &out_v)
		{
			int read = 0;
			read += ReadFloat32(&pDataStream[read], out_v.m_x);
			read += ReadFloat32(&pDataStream[read], out_v.m_y);
			read += ReadFloat32(&pDataStream[read], out_v.m_z);
			read += ReadFloat32(&pDataStream[read], out_v.m_w);
			return read;
		}


		int StreamManager::WriteMatrix4x4(const Matrix4x4 &v, char *pDataStream)
		{
			int size = 0;
			for (int i = 0; i < 16; ++i)
				size += WriteFloat32(v.m16[i], &pDataStream[size]);
			return size;
		}

		int StreamManager::ReadMatrix4x4(char *pDataStream, Matrix4x4 &out_v)
		{
			int read = 0;
			for (int i = 0; i < 16; ++i)
				read += ReadFloat32(&pDataStream[read], out_v.m16[i]);
			return read;
		}

		int StreamManager::WriteNetworkId(Networkable::NetworkId v, char *pDataStream)
		{
			return WriteInt32(v, pDataStream);
		}

		int StreamManager::ReadNetworkId(char *pDataStream, Networkable::NetworkId &out_v)
		{
			PrimitiveTypes::Int32 v;
			int res = ReadInt32(pDataStream, v);
			out_v = v;
			return res;
		}
	}; // namespace Components
}; // namespace PE
