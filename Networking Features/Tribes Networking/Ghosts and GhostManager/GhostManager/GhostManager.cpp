#define NOMINMAX

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "GhostManager.h"
#include "Ghost.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"

// additional lua includes needed
extern "C"
{
#include "luasocket_dist/src/socket.h"
#include "luasocket_dist/src/inet.h"
};

#include "../GlobalConfig/GlobalConfig.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Networking/NetworkManager.h"
#include "PrimeEngine/Scene/DebugRenderer.h"
#include "PrimeEngine/Networking/StreamManager.h"
#include "CharacterControl/ClientGameObjectManagerAddon.h"

#include "CharacterControl/Characters/SoldierNPC.h"
#include "CharacterControl/Characters/SoldierNPCMovementSM.h"
#include "CharacterControl/Characters/SoldierNPCAnimationSM.h"
#include "CharacterControl/Characters/Projectile.h"
#include "CharacterControl/Audio/SoundManager.h"
#include "CharacterControl/Navigator.h"
#include "PrimeEngine/Networking/EventManager.h"

#include <chrono>
#ifndef TIMENOW 
#define TIMENOW (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
#endif

extern bool DEBUGBREAK;

using namespace PE::Events;

namespace PE
{
	namespace Components
	{
		std::vector<GhostManager::InputRecord> GhostManager::inputHistory;

		PE_IMPLEMENT_CLASS1(GhostManager, Component);

		std::unordered_map<int, Ghost *> GhostManager::serverGhostMap;

		GhostManager::~GhostManager() {}
		void GhostManager::initialize() {}
		GhostManager::GhostManager(PE::GameContext &context, PE::MemoryArena arena, PE::NetworkContext &netContext, Handle hMyself, bool isServer)
			: Component(context, arena, hMyself), m_transmitterNumGhostsNotAcked(0)
		{
			m_pNetContext = &netContext;
			m_isServer = isServer;
		}

		void GhostManager::addDefaultComponents()
		{
			Component::addDefaultComponents();
			PE_REGISTER_EVENT_HANDLER(Events::Event_UPDATE, GhostManager::do_UPDATE);
			//PE_REGISTER_EVENT_HANDLER(Events::Event_PRE_RENDER_needsRC, GhostManager::do_PRE_RENDER_needsRC);
		}

		void GhostManager::do_PRE_RENDER_needsRC(PE::Events::Event *pEvt)
		{
			Event_PRE_RENDER_needsRC *pRealEvent = (Event_PRE_RENDER_needsRC *)(pEvt);

			if (m_isServer)
			{
				for (auto it = GhostManager::serverGhostMap.begin(); it != GhostManager::serverGhostMap.end(); it++)
				{
					Ghost *ghost = it->second;
					if (ghost->type == Ghost::SoldierNPC)
					{
						Matrix4x4 base;
						base.setPos(ghost->soldierData.actualPosition);
						base.setU(ghost->soldierData.uBasis);
						base.setV(ghost->soldierData.vBasis);
						base.setN(ghost->soldierData.nBasis);
						DebugRenderer::Instance()->createLineMesh(true, base, NULL, 0, 0);// send event while the array is on the stack
					}
				}
			}
		}

		void GhostManager::do_UPDATE(Events::Event *pEvt)
		{
			PE::Events::Event_UPDATE *realEvt = (PE::Events::Event_UPDATE *)pEvt;
			if (m_isServer)
			{
				for (auto iterator = GhostManager::serverGhostMap.begin(); iterator != GhostManager::serverGhostMap.end(); iterator++)
				{
					int ghostID = iterator->first;
					Ghost *ghost = iterator->second;
					if (ghost->changed[0] == 0) { ghost->changed[0] = TIMENOW; }
					if (ghost->changed[1] == 0) { ghost->changed[1] = TIMENOW; }

					if (m_pNetContext->m_clientId == 0)
					{
						//ghost->possibleDesync[0] = false;
						//ghost->possibleDesync[1] = false;
						if (ghost->wasNudged[0])
						{
							ghost->wasNudged[0] = false;
							ghost->wasNudged[1] = false;
							ghost->possibleDesync[0] = true;
							ghost->possibleDesync[1] = true;
						}

						ghost->update(realEvt->m_frameTime);
					}

					int ghostMustSendTimer = 15;

					if (ghost->originalComponentID - 1000 == m_pNetContext->m_clientId)
					{
						if (ghost->possibleDesync[m_pNetContext->m_clientId])
						{
							ghost->possibleDesync[m_pNetContext->m_clientId] = false;
							ghost->changed[m_pNetContext->m_clientId] = TIMENOW;
							ghost->shouldSend[m_pNetContext->m_clientId] = false;
							ghost->didUpdate[m_pNetContext->m_clientId] = false;
							scheduleGhost(ghost);
							++ghost->lastUpdatedServerIndex;
							continue;
						}
						else
						{
							continue;
						}
					}

					//if (ghost->type == Ghost::SoldierNPC)
					//{
					//	if (ghost->originalComponentID - 1000 == m_pNetContext->m_clientId)
					//	{
					//		ghostMustSendTimer = 50;
					//	}
					//}

					//int targetID = CharacterControl::Components::ClientGameObjectManagerAddon::clientID == 0 ? 1001 : 1000; // we're only interested in the other soldier
					//if (ghost->originalcomponentid >= 1000) // found you bitch
					//{
					//	if (ghost->soldierdata.ismoving) // and you're moving
					//	{
					//		std::vector<vector3> corners; // find the dangerous corners
					//		corners.push_back(vector3(18.1f, 0.0f, 11.3f));
					//		corners.push_back(vector3(11.5f, 0.0f, 11.3f));
					//		corners.push_back(vector3(18.1f, 0.0f, 13.4f));
					//		corners.push_back(vector3(11.5f, 0.0f, 13.4f));

					//		for (int i = 0; i < corners.size(); i++)
					//		{
					//			if ((ghost->soldierdata.actualposition - corners[i]).lengthsqr() < 0.8f) // if you're close to corner
					//			{
					//				ghostmustsendtimer = 10; // then you have to send more often
					//				break;
					//			}
					//		}
					//	}
					//}

					if (TIMENOW - ghost->changed[m_pNetContext->m_clientId] < ghostMustSendTimer && !ghost->shouldSend[m_pNetContext->m_clientId]) { continue; }
					else
					{
						if (ghost->didUpdate[m_pNetContext->m_clientId])
						{
							ghost->changed[m_pNetContext->m_clientId] = TIMENOW;
							ghost->shouldSend[m_pNetContext->m_clientId] = false;
							ghost->didUpdate[m_pNetContext->m_clientId] = false;
							scheduleGhost(ghost);
							++ghost->lastUpdatedServerIndex;
						}
						else if (PE::Components::EventManager::READYTOGO && TIMENOW - ghost->changed[m_pNetContext->m_clientId] > 15)
						{
							ghost->changed[m_pNetContext->m_clientId] = TIMENOW;
							ghost->shouldSend[m_pNetContext->m_clientId] = false;
							ghost->didUpdate[m_pNetContext->m_clientId] = false;
							scheduleGhost(ghost);
							++ghost->lastUpdatedServerIndex;
						}
					}
				}
			}
			else
			{
				for (auto iterator = ghostMap.begin(); iterator != ghostMap.end(); iterator++)
				{
					int ghostID = iterator->first;
					Ghost *targetGhost = iterator->second;
					if (targetGhost->lastTimeUpdatedClient == -1)
					{
						// CLIENT TIMING
						targetGhost->lastTimeUpdatedClient = TIMENOW + CharacterControl::Components::ClientGameObjectManagerAddon::ServerClientTimeDifference;
					}

					if (targetGhost->originalComponentID - 1000 == CharacterControl::Components::ClientGameObjectManagerAddon::clientID)
					{
						Component *origComponent = CharacterControl::Components::ClientGameObjectManagerAddon::componentMap[targetGhost->originalComponentID];
						resimulateGhost(targetGhost, targetGhost->lastTimeUpdatedClient);
						targetGhost->lastTimeUpdatedClient = TIMENOW + CharacterControl::Components::ClientGameObjectManagerAddon::ServerClientTimeDifference;
					}
				}
			}
		}

		static bool resim;
		void GhostManager::resimulateGhostHelper(Ghost *ghost, uint64_t miliDeltaTime, int inputIndex)
		{

			using namespace CharacterControl::Components;

			float deltaTime = miliDeltaTime / 1000.0f;
			if (inputIndex != -1)
			{
				// simulate for a particular input
				switch (ghost->type)
				{
				case Ghost::SoldierNPC:
				{
					InputRecord inputRecord = inputHistory[inputIndex];
					if (ghost->originalComponentID == inputRecord.selectedID)
					{
						switch (inputRecord.inputType)
						{
						case 0: {} break;
						case 1:
						{
							// it is a mouse click	
							ghost->soldierData.isMoving = false;
							Vector3 targetPos = Vector3(inputRecord.mousePos.m_x, 0.0f, inputRecord.mousePos.m_z);
							std::vector<Vector3> tempPath = CharacterControl::Components::ClientGameObjectManagerAddon::clientNavigator.findPath(ghost->soldierData.actualPosition, targetPos);
							if (tempPath.size() != 0)
							{
								ghost->soldierData.desiredPosition = targetPos;

								ghost->soldierData.isMoving = true;
								ghost->soldierData.animationState = CharacterControl::Components::SoldierNPCAnimationSM::RUN;
								ghost->soldierData.pathLen = tempPath.size();
								memcpy(ghost->soldierData.path, &tempPath[0], tempPath.size() * sizeof(tempPath[0]));
								ghost->soldierData.pathIterator = 1;
							}
						} break;
						default: {} break;
						}
					}
				} break;
				case Ghost::Projectile: {} break;
				default: {} break;
				}
			}

			// simulate after processing input
			switch (ghost->type)
			{
			case Ghost::SoldierNPC:
			{
				if (ghost->soldierData.isMoving)
				{
					float elapsedTime = 0;

					Vector3 forward;
					Vector3 lastRealForward;
					bool moved = false;
					float speed = 3.0f;
					Matrix4x4 base;
					base.setU(ghost->soldierData.uBasis);
					base.setV(ghost->soldierData.vBasis);
					base.setN(ghost->soldierData.nBasis);
					while (elapsedTime < deltaTime)
					{
						Vector3 nextWP = ghost->soldierData.path[ghost->soldierData.pathIterator];
						forward = nextWP - ghost->soldierData.actualPosition;
						float distanceRemainingToNextWP = (nextWP - ghost->soldierData.actualPosition).length();
						float timeToNextWP = distanceRemainingToNextWP / speed;
						float smallDeltaTime = min(timeToNextWP, (deltaTime - elapsedTime));
						elapsedTime += smallDeltaTime;
						float dsqr = forward.lengthSqr();
						forward.normalize();

						Vector3 meToDest = nextWP - ghost->soldierData.actualPosition;
						Vector3 lastWPtoDest = nextWP - ghost->soldierData.path[ghost->soldierData.pathIterator - 1];
						bool didNotOvershoot = (meToDest.dotProduct(lastWPtoDest) >= 0);

						bool passedWP = false;
						if (dsqr > 0.01f && didNotOvershoot)
						{
							float allowedDisp = speed * smallDeltaTime;
							float dist = sqrt(dsqr);
							dist = dist > allowedDisp ? allowedDisp : dist;
							ghost->soldierData.actualPosition += forward * dist;
							moved = true;
							ghost->soldierData.animationState = SoldierNPCAnimationSM::RUN;

							base.turnInDirection(forward, 3.14f);

							meToDest = nextWP - ghost->soldierData.actualPosition;
							passedWP = (meToDest.dotProduct(lastWPtoDest) <= 0);
						}
						else
						{
							passedWP = true;
						}

						if (passedWP)
						{
							if (++ghost->soldierData.pathIterator == ghost->soldierData.pathLen)
							{
								ghost->soldierData.isMoving = false;
								ghost->soldierData.animationState = SoldierNPCAnimationSM::STAND;
								break;
							}
						}
					}
					
					ghost->soldierData.uBasis = base.getU();
					ghost->soldierData.vBasis = base.getV();
					ghost->soldierData.nBasis = base.getN();

				}
			} break;
			case Ghost::Projectile: {} break;
			default: {} break;
			}
		}


		void GhostManager::resimulateGhost(Ghost *ghost, uint64_t startTime)
		{
			Ghost tempGhost = *ghost;
			// find the index of the input to start from
			int inputIndex = -1;
			for (int i = 0; i < inputHistory.size(); ++i)
			{
				if (inputHistory[i].timeStamp >= startTime)
				{
					inputIndex = i;
					break;
				}
			}

			if (inputIndex != -1)
			{
				uint64_t deltaTime = inputHistory[inputIndex].timeStamp - startTime;
				resimulateGhostHelper(ghost, deltaTime);

				// loop through inputs until we have no more
				for (int i = inputIndex; i < inputHistory.size(); ++i)
				{
					if (i == inputHistory.size() - 1) // last frame
					{
						// CLIENT TIMING
						deltaTime = TIMENOW + CharacterControl::Components::ClientGameObjectManagerAddon::ServerClientTimeDifference - inputHistory[i].timeStamp;
					}
					else
					{
						deltaTime = inputHistory[i + 1].timeStamp - inputHistory[i].timeStamp;
					}

					// simulate based on deltatime, storing in simuGhosts
					resimulateGhostHelper(ghost, deltaTime, i);
				}
			}
			else
			{
				// no inputs, just simulate to current time
				// CLIENT TIMING
				uint64_t deltaTime = TIMENOW + CharacterControl::Components::ClientGameObjectManagerAddon::ServerClientTimeDifference - startTime;
				resimulateGhostHelper(ghost, deltaTime);
			}

			using namespace CharacterControl::Components;
			SoldierNPC *soldierComponent = (SoldierNPC *)ClientGameObjectManagerAddon::componentMap[ghost->originalComponentID];
			// THIS IS A THING
			float len = (soldierComponent->sceneNode->m_base.getPos() - ghost->soldierData.actualPosition).length();
			int count = 0;
			bool didRedo = false;

			if (len > 0.3f)
			{
				memcpy(ghost, &tempGhost, sizeof(Ghost));
			}

			Ghost ghostArray[3] = { tempGhost, tempGhost, tempGhost };
			memcpy(&ghostArray[0], ghost, sizeof(Ghost));
			int minIndex = 0;
			int minLen = len;
			while (count < 2 && len > 0.3f)
			{
				++count;
				didRedo = true;
				inputIndex = inputIndex == -1 ? inputHistory.size() - 1 : inputIndex - 1;
				uint64_t deltaTime;
				for (int i = inputIndex; i < inputHistory.size(); ++i)
				{
					if (i == inputHistory.size() - 1)
					{
						deltaTime = (TIMENOW + CharacterControl::Components::ClientGameObjectManagerAddon::ServerClientTimeDifference) - inputHistory[i].timeStamp;
					}
					else
					{
						deltaTime = inputHistory[i + 1].timeStamp - inputHistory[i].timeStamp;
					}

					resimulateGhostHelper(&ghostArray[count + 1], deltaTime, i);
				}

				len = (soldierComponent->sceneNode->m_base.getPos() - ghost->soldierData.actualPosition).length();
				if (len < minLen)
				{
					minLen = len;
					minIndex = count + 1;
				}
			}

			memcpy(&ghostArray[minIndex], ghost, sizeof(Ghost));

			//float len2 = (soldierComponent->sceneNode->m_base.getPos() - tempGhost.soldierData.actualPosition).length();
			//if (didRedo && len2 < len)
			//{
			//	memcpy(ghost, &tempGhost, sizeof(Ghost));
			//}

			if (ghost->soldierData.isMoving) 
			{
				soldierComponent->movementSM->lastTargetPosition = ghost->soldierData.path[ghost->soldierData.pathIterator - 1];
			}
			soldierComponent->sceneNode->m_base.setPos(ghost->soldierData.actualPosition, true, len);
			Vector3 forward = ghost->soldierData.nBasis;
			soldierComponent->sceneNode->m_base.setU(ghost->soldierData.uBasis);
			soldierComponent->sceneNode->m_base.setV(ghost->soldierData.vBasis);
			soldierComponent->sceneNode->m_base.setN(ghost->soldierData.nBasis);
			soldierComponent->animationSM->m_wantToPlay = (SoldierNPCAnimationSM::AnimId)ghost->soldierData.animationState;
		}

		void GhostManager::scheduleGhost(PE::Components::Ghost *pGhost)
		{
			if (haveGhostsToSend() >= PE_MAX_GHOST_JAM)
			{
				assert(!"sending too many ghosts have to drop, need throttling mechanism here");
				return;
			}

			m_ghostsToSend.push_back(GhostTransmissionData());
			GhostTransmissionData &back = m_ghostsToSend.back();
			int dataSize = 0;

			PrimitiveTypes::Int32 ghostUpdatedIndex = (PrimitiveTypes::Int32)pGhost->lastUpdatedServerIndex;
			dataSize += StreamManager::WriteInt32(ghostUpdatedIndex, &back.m_payload[dataSize]);
			PrimitiveTypes::Int32 ghostType = (PrimitiveTypes::Int32)pGhost->type;
			dataSize += StreamManager::WriteInt32(ghostType, &back.m_payload[dataSize]);
			PrimitiveTypes::Int32 originalComponentID = (PrimitiveTypes::Int32)pGhost->originalComponentID;
			dataSize += StreamManager::WriteInt32(originalComponentID, &back.m_payload[dataSize]);
			PrimitiveTypes::UInt64 timeLastUpdated = (PrimitiveTypes::UInt64)pGhost->lastTimeUpdatedServer;
			dataSize += StreamManager::WriteInt64(timeLastUpdated, &back.m_payload[dataSize]);

			switch (pGhost->type)
			{
			case Ghost::SoldierNPC:
			{
				int isMoving = pGhost->soldierData.isMoving;
				int animationState = pGhost->soldierData.animationState;
				Vector3 desiredPosition = pGhost->soldierData.desiredPosition;
				Vector3 actualPosition = pGhost->soldierData.actualPosition;
				dataSize += StreamManager::WriteInt32(isMoving, &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteInt32(animationState, &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteVector4(Vector4(desiredPosition.m_x, desiredPosition.m_y, desiredPosition.m_z, 1.0f), &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteVector4(Vector4(actualPosition.m_x, actualPosition.m_y, actualPosition.m_z, 1.0f), &back.m_payload[dataSize]);
				Vector3 uBasis = pGhost->soldierData.uBasis;
				Vector3 vBasis = pGhost->soldierData.vBasis;
				Vector3 nBasis = pGhost->soldierData.nBasis;
				dataSize += StreamManager::WriteVector4(Vector4(uBasis.m_x, uBasis.m_y, uBasis.m_z, 0.0f), &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteVector4(Vector4(vBasis.m_x, vBasis.m_y, vBasis.m_z, 0.0f), &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteVector4(Vector4(nBasis.m_x, nBasis.m_y, nBasis.m_z, 0.0f), &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteFloat32(pGhost->soldierData.health, &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteInt32(pGhost->soldierData.isLastLeg, &back.m_payload[dataSize]);
			} break;

			case Ghost::Projectile:
			{
				Vector3 desiredPosition = pGhost->projectileData.desiredPosition;
				Vector3 actualPosition = pGhost->projectileData.actualPosition;
				Vector3 startPosition = pGhost->projectileData.startPosition;
				int isAlive = pGhost->projectileData.isAlive;
				float lifeProgress = pGhost->projectileData.lifeProgress;
				int teamID = pGhost->projectileData.teamID;
				dataSize += StreamManager::WriteVector4(Vector4(desiredPosition.m_x, desiredPosition.m_y, desiredPosition.m_z, 1.0f), &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteVector4(Vector4(actualPosition.m_x, actualPosition.m_y, actualPosition.m_z, 1.0f), &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteVector4(Vector4(startPosition.m_x, startPosition.m_y, startPosition.m_z, 1.0f), &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteInt32(isAlive, &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteFloat32(lifeProgress, &back.m_payload[dataSize]);
				dataSize += StreamManager::WriteInt32(teamID, &back.m_payload[dataSize]);
			} break;

			default:
			{
				assert(!"ghost type is not valid");
			} break;
			}

			back.m_size = dataSize;
		}

		int GhostManager::haveGhostsToSend()
		{
			// TODO(Rohan): this will return 0 or 1, and we probably don't want that
			return m_ghostsToSend.size();
		}

		int GhostManager::fillInNextPacket(char *pDataStream, TransmissionRecord *pRecord, int packetSizeAllocated, bool &out_usefulDataSent, bool &out_wantToSendMore)
		{
			out_usefulDataSent = false;
			out_wantToSendMore = false;

			int ghostsToSend = haveGhostsToSend();
			assert(ghostsToSend);

			int ghostsReallySent = 0;

			int size = 0;
			size += StreamManager::WriteInt32(ghostsToSend, &pDataStream[size]);

			int sizeLeft = packetSizeAllocated - size;

			for (int i = 0; i < ghostsToSend; ++i)
			{
				GhostTransmissionData &ghostTransmissionData = m_ghostsToSend[i];
				if (ghostTransmissionData.m_size > sizeLeft)
				{
					out_wantToSendMore = true;
					break;
				}

				//int checkOrderId = 0;
				//StreamManager::ReadInt32( &evt.m_payload[0], checkOrderId);
				//debug info to show event order id of packet being sent
				//PEINFO("Order id check: %d\n", checkOrderId);

				memcpy(&pDataStream[size], &ghostTransmissionData.m_payload[0], ghostTransmissionData.m_size);
				size += ghostTransmissionData.m_size;
				sizeLeft = packetSizeAllocated - size;

				ghostsReallySent++;
				m_transmitterNumGhostsNotAcked++;
			}

			if (ghostsReallySent > 0)
			{
				m_ghostsToSend.erase(m_ghostsToSend.begin(), m_ghostsToSend.begin() + ghostsReallySent);
			}

			// write real value into the beginning of event chunk
			// first thing in stream is how many ghosts we should send
			StreamManager::WriteInt32(ghostsReallySent, &pDataStream[0]);
			// we are sending useful data only if we are sending ghosts 
			out_usefulDataSent = ghostsReallySent > 0;

			return size;
		}

		void GhostManager::debugRender(int &threadOwnershipMask, float xoffset, float yoffset)
		{
		}

		int GhostManager::receiveNextPacket(char *pDataStream)
		{
			// stuff to resim
			// CLIENT TIMING
			uint64_t nowtime = TIMENOW + CharacterControl::Components::ClientGameObjectManagerAddon::ServerClientTimeDifference;
			nowtime -= (StreamManager::delayTime + 15);

			int read = 0;
			PrimitiveTypes::Int32 numGhosts;

			read += StreamManager::ReadInt32(&pDataStream[read], numGhosts);
			if (numGhosts == 0)
			{
				return read;
			}

			for (int i = 0; i < numGhosts; ++i)
			{
				PrimitiveTypes::Int32 lastUpdatedIndex;
				PrimitiveTypes::Int32 ghostType;
				PrimitiveTypes::Int32 originalComponentID;
				PrimitiveTypes::UInt64 timeLastUpdated;
				read += StreamManager::ReadInt32(&pDataStream[read], lastUpdatedIndex);
				read += StreamManager::ReadInt32(&pDataStream[read], ghostType);
				read += StreamManager::ReadInt32(&pDataStream[read], originalComponentID);
				read += StreamManager::ReadInt64(&pDataStream[read], timeLastUpdated);

				Ghost *targetGhost;
				bool dontUpdate = false;
				if (m_isServer)
				{
					// occurs only when client creates objects
					if (GhostManager::serverGhostMap.find(originalComponentID) == GhostManager::serverGhostMap.end())
					{
						PE::Handle hGhost("Ghost", sizeof(Ghost));
						Ghost *tempGhost = new(hGhost) Ghost(*m_pContext);
						tempGhost->pComponent = NULL;
						GhostManager::serverGhostMap.insert(std::make_pair(originalComponentID, tempGhost));
						targetGhost = GhostManager::serverGhostMap[originalComponentID];
						//targetGhost->changed = true;
					}
					else
					{
						scheduleGhost(GhostManager::serverGhostMap[originalComponentID]);
						targetGhost = &(Ghost(*m_pContext));
						dontUpdate = true;
					}
				}
				else
				{
					// occurs whenever server updates authoritative representation and broadcasts said updates to clients
					if (this->ghostMap.find(originalComponentID) == this->ghostMap.end())
					{
						PE::Handle hGhost("Ghost", sizeof(Ghost));
						Ghost *tempGhost = new(hGhost) Ghost(*m_pContext);
						tempGhost->pComponent = NULL;
						this->ghostMap.insert(std::make_pair(originalComponentID, tempGhost));
					}

					targetGhost = this->ghostMap[originalComponentID];

					if (lastUpdatedIndex > targetGhost->lastUpdatedClientIndex)
					{
						targetGhost->lastUpdatedClientIndex = lastUpdatedIndex;
					}
					else
					{
						if (targetGhost->type == Ghost::SoldierNPC)
						{
							PrimitiveTypes::Int32 isMoving;
							PrimitiveTypes::Int32 animationState;
							Vector4 desiredPosition;
							Vector4 actualPosition;
							Vector4 uBasis;
							Vector4 vBasis;
							Vector4 nBasis;
							float health;
							PrimitiveTypes::Int32 isLastLeg;
							read += StreamManager::ReadInt32(&pDataStream[read], isMoving);
							read += StreamManager::ReadInt32(&pDataStream[read], animationState);
							read += StreamManager::ReadVector4(&pDataStream[read], desiredPosition);
							read += StreamManager::ReadVector4(&pDataStream[read], actualPosition);
							read += StreamManager::ReadVector4(&pDataStream[read], uBasis);
							read += StreamManager::ReadVector4(&pDataStream[read], vBasis);
							read += StreamManager::ReadVector4(&pDataStream[read], nBasis);
							read += StreamManager::ReadFloat32(&pDataStream[read], health);
							read += StreamManager::ReadInt32(&pDataStream[read], isLastLeg);
						}
						else if (targetGhost->type == Ghost::Projectile)
						{
							Vector4 desiredPosition;
							Vector4 actualPosition;
							Vector4 startPosition;
							int isAlive;
							float lifeProgress;
							int teamID;
							read += StreamManager::ReadVector4(&pDataStream[read], desiredPosition);
							read += StreamManager::ReadVector4(&pDataStream[read], actualPosition);
							read += StreamManager::ReadVector4(&pDataStream[read], startPosition);
							read += StreamManager::ReadInt32(&pDataStream[read], isAlive);
							read += StreamManager::ReadFloat32(&pDataStream[read], lifeProgress);
							read += StreamManager::ReadInt32(&pDataStream[read], teamID);
						}

						continue;
					}
				}

				targetGhost->type = (Ghost::GhostType)ghostType;
				targetGhost->originalComponentID = (int)originalComponentID;

				switch (ghostType)
				{
				case Ghost::SoldierNPC:
				{
					PrimitiveTypes::Int32 isMoving;
					PrimitiveTypes::Int32 animationState;
					Vector4 desiredPosition;
					Vector4 actualPosition;
					Vector4 uBasis;
					Vector4 vBasis;
					Vector4 nBasis;
					float health;
					PrimitiveTypes::Int32 isLastLeg;
					read += StreamManager::ReadInt32(&pDataStream[read], isMoving);
					read += StreamManager::ReadInt32(&pDataStream[read], animationState);
					read += StreamManager::ReadVector4(&pDataStream[read], desiredPosition);
					read += StreamManager::ReadVector4(&pDataStream[read], actualPosition);
					read += StreamManager::ReadVector4(&pDataStream[read], uBasis);
					read += StreamManager::ReadVector4(&pDataStream[read], vBasis);
					read += StreamManager::ReadVector4(&pDataStream[read], nBasis);
					read += StreamManager::ReadFloat32(&pDataStream[read], health);
					read += StreamManager::ReadInt32(&pDataStream[read], isLastLeg);

					uint64_t timeToResimFrom = timeLastUpdated - StreamManager::delayTime;
					targetGhost->soldierData.desiredPosition = Vector3(desiredPosition.m_x, desiredPosition.m_y, desiredPosition.m_z);
					targetGhost->soldierData.actualPosition = Vector3(actualPosition.m_x, actualPosition.m_y, actualPosition.m_z);
					targetGhost->soldierData.uBasis = Vector3(uBasis.m_x, uBasis.m_y, uBasis.m_z);
					targetGhost->soldierData.vBasis = Vector3(vBasis.m_x, vBasis.m_y, vBasis.m_z);
					targetGhost->soldierData.nBasis = Vector3(nBasis.m_x, nBasis.m_y, nBasis.m_z);
					targetGhost->soldierData.health = health;
					if (isMoving)
					{
						if (targetGhost->originalComponentID - 1000 == CharacterControl::Components::ClientGameObjectManagerAddon::clientID)
						{
							std::vector<Vector3> tempPath = CharacterControl::Components::ClientGameObjectManagerAddon::clientNavigator.findPath(targetGhost->soldierData.actualPosition, targetGhost->soldierData.desiredPosition);
							if (tempPath.size() != 0)
							{
								targetGhost->soldierData.isMoving = true;
								targetGhost->soldierData.animationState = CharacterControl::Components::SoldierNPCAnimationSM::RUN;
								targetGhost->soldierData.pathLen = tempPath.size();
								memcpy(targetGhost->soldierData.path, &tempPath[0], tempPath.size() * sizeof(tempPath[0]));
								targetGhost->soldierData.pathIterator = 1;
							}
							else
							{
								targetGhost->soldierData.isMoving = false;
								targetGhost->soldierData.animationState = CharacterControl::Components::SoldierNPCAnimationSM::STAND;
								targetGhost->soldierData.pathLen = 0;
							}
						}
						else
						{
							targetGhost->soldierData.isMoving = isMoving;
							targetGhost->soldierData.animationState = CharacterControl::Components::SoldierNPCAnimationSM::RUN;
						}
					}
					else
					{
						targetGhost->soldierData.isMoving = false;
						targetGhost->soldierData.animationState = CharacterControl::Components::SoldierNPCAnimationSM::STAND;
					}


					if (targetGhost->originalComponentID - 1000 == CharacterControl::Components::ClientGameObjectManagerAddon::clientID)
					{
						resimulateGhost(targetGhost, timeToResimFrom);
						// CLIENT TIMING
						targetGhost->lastTimeUpdatedClient = TIMENOW + CharacterControl::Components::ClientGameObjectManagerAddon::ServerClientTimeDifference;
					}

					//#if 0
					//targetGhost->soldierData.isMoving = isMoving;
					//targetGhost->soldierData.animationState = animationState;
					//targetGhost->soldierData.desiredPosition = Vector3(desiredPosition.m_x, desiredPosition.m_y, desiredPosition.m_z);
					//targetGhost->soldierData.actualPosition = Vector3(actualPosition.m_x, actualPosition.m_y, actualPosition.m_z);
					//targetGhost->soldierData.uBasis = Vector3(uBasis.m_x, uBasis.m_y, uBasis.m_z);
					//targetGhost->soldierData.vBasis = Vector3(vBasis.m_x, vBasis.m_y, vBasis.m_z);
					//targetGhost->soldierData.nBasis = Vector3(nBasis.m_x, nBasis.m_y, nBasis.m_z);
					//targetGhost->soldierData.health = health;
					//targetGhost->soldierData.isLastLeg = isLastLeg;
					//#endif

					// make a local ghost that is a copy of the server
					// resim on that ghost from time - delaytime
					// copy to local ghost and set local object
				} break;

				case Ghost::Projectile:
				{
					Vector4 desiredPosition;
					Vector4 actualPosition;
					Vector4 startPosition;
					int isAlive;
					float lifeProgress;
					int teamID;
					read += StreamManager::ReadVector4(&pDataStream[read], desiredPosition);
					read += StreamManager::ReadVector4(&pDataStream[read], actualPosition);
					read += StreamManager::ReadVector4(&pDataStream[read], startPosition);
					read += StreamManager::ReadInt32(&pDataStream[read], isAlive);
					read += StreamManager::ReadFloat32(&pDataStream[read], lifeProgress);
					read += StreamManager::ReadInt32(&pDataStream[read], teamID);
					targetGhost->projectileData.desiredPosition = Vector3(desiredPosition.m_x, desiredPosition.m_y, desiredPosition.m_z);
					targetGhost->projectileData.actualPosition = Vector3(actualPosition.m_x, actualPosition.m_y, actualPosition.m_z);
					targetGhost->projectileData.startPosition = Vector3(startPosition.m_x, startPosition.m_y, startPosition.m_z);
					targetGhost->projectileData.isAlive = isAlive;
					targetGhost->projectileData.lifeProgress = lifeProgress;
					targetGhost->projectileData.teamID = teamID;
				} break;

				default:
				{
					assert(!"read a ghost of unspecified type");
				} break;
				}

				if (!m_isServer)
				{
					// update client-side game objects
					auto itSoldier = CharacterControl::Components::ClientGameObjectManagerAddon::componentMap.find(originalComponentID);
					Component *rawComponent = nullptr;
					if (itSoldier != CharacterControl::Components::ClientGameObjectManagerAddon::componentMap.end())
					{
						rawComponent = CharacterControl::Components::ClientGameObjectManagerAddon::componentMap[originalComponentID];
					}

					switch (targetGhost->type)
					{
					case Ghost::SoldierNPC:
					{
						CharacterControl::Components::SoldierNPC *soldierComponent = (CharacterControl::Components::SoldierNPC *)rawComponent;

						// if it's a self-resim, disable the movement SM
						//if (soldierComponent->componentID == 1000 + CharacterControl::Components::ClientGameObjectManagerAddon::clientID) {
						//	soldierComponent->movementSM->GO = false;
						//}

						if (soldierComponent != nullptr)
						{
							// dont resim health 
							if (soldierComponent->componentID - 1000 != CharacterControl::Components::ClientGameObjectManagerAddon::clientID)
							{
								// THIS IS ANOTHER THING
								soldierComponent->sceneNode->m_base.setPos(targetGhost->soldierData.actualPosition);
								soldierComponent->movementSM->isMoving = targetGhost->soldierData.isMoving;
								//soldierComponent->sceneNode->m_base.turnInDirection(targetGhost->soldierData.nBasis, 0.1);
								soldierComponent->sceneNode->m_base.setU(targetGhost->soldierData.uBasis);
								soldierComponent->sceneNode->m_base.setV(targetGhost->soldierData.vBasis);
								soldierComponent->sceneNode->m_base.setN(targetGhost->soldierData.nBasis);
								Vector3 firstOnPath = CharacterControl::Components::ClientGameObjectManagerAddon::clientNavigator.findPath(soldierComponent->sceneNode->m_base.getPos(), targetGhost->soldierData.desiredPosition)[1];
								soldierComponent->movementSM->m_targetPostion = firstOnPath;
								soldierComponent->animationSM->m_wantToPlay = (CharacterControl::Components::SoldierNPCAnimationSM::AnimId)targetGhost->soldierData.animationState;
							}

							if (soldierComponent->health > targetGhost->soldierData.health)
							{
								soldierComponent->meshInstance->dead = true;
								soldierComponent->hitTime = TIMENOW;
								PlaySoundX("AssetsOut\\Audio\\grunt.wav");
							}

							soldierComponent->health = targetGhost->soldierData.health;
						}
					} break;

					case Ghost::Projectile:
					{
						CharacterControl::Components::Projectile *projectileComponent = (CharacterControl::Components::Projectile *)rawComponent;
						if (targetGhost->projectileData.isAlive && !projectileComponent->isAlive)
						{
							PlaySoundX("AssetsOut\\Audio\\spell_fire.wav");
						}

						if (targetGhost->projectileData.isAlive)
						{
							projectileComponent->sceneNode->m_base.setPos(targetGhost->projectileData.actualPosition);
							projectileComponent->isAlive = true;
							projectileComponent->meshInstance->lifeProgress = targetGhost->projectileData.lifeProgress;
							projectileComponent->setEnabled(true);
							projectileComponent->meshInstance->m_display = true;
							projectileComponent->targetPosition = targetGhost->projectileData.desiredPosition;
						}
						else
						{
							projectileComponent->disable();
						}
					} break;

					default:
					{

					} break;
					}
				}
			}

			return read;
		}; // namespace Components
	}; // namespace PE
};