#include "ServerGameObjectManagerAddon.h"

#include "PrimeEngine/Lua/Server/ServerLuaEnvironment.h"
#include "PrimeEngine/Networking/Server/ServerNetworkManager.h"
#include "PrimeEngine/GameObjectModel/GameObjectManager.h"
#include "PrimeEngine/Networking/Tribes/GhostManager/GhostManager.h"

#include "Characters/SoldierNPC.h"
#include "CharacterControl/Characters/SoldierNPCAnimationSM.h"
#include "Characters/TargetNPC.h"
#include "WayPoint.h"
#include "Tank/ClientTank.h"
#include "Navigator.h"


#ifndef TIMENOW 
#define TIMENOW (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
#endif

using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;
using namespace CharacterControl::Components;

namespace CharacterControl
{
	namespace Components
	{

		std::vector <std::pair<int, int>> ServerGameObjectManagerAddon::needToAck;
		int ServerGameObjectManagerAddon::RTT = 0;

		Navigator ServerGameObjectManagerAddon::serverNavigator;

		PE_IMPLEMENT_CLASS1(ServerGameObjectManagerAddon, GameObjectManagerAddon); // creates a static handle and GteInstance*() methods. still need to create construct

		void ServerGameObjectManagerAddon::addDefaultComponents()
		{
			GameObjectManagerAddon::addDefaultComponents();

			PE_REGISTER_EVENT_HANDLER(Event_MoveTank_C_to_S, ServerGameObjectManagerAddon::do_MoveTank);
			PE_REGISTER_EVENT_HANDLER(Event_MoveSoldier_C_to_S, ServerGameObjectManagerAddon::do_MoveSoldier);
			PE_REGISTER_EVENT_HANDLER(Event_TimeSynch_C_to_S, ServerGameObjectManagerAddon::do_TimeSynch);
			PE_REGISTER_EVENT_HANDLER(Event_SpawnProjectile_C_to_S, ServerGameObjectManagerAddon::do_SpawnProjectile);
			PE_REGISTER_EVENT_HANDLER(Event_SoldierAcquireTarget_C_to_S, ServerGameObjectManagerAddon::do_SoldierAcquireTarget);
			PE_REGISTER_EVENT_HANDLER(Event_NudgeSoldier_C_to_S, ServerGameObjectManagerAddon::do_SoldierNudgeTarget);
			PE_REGISTER_EVENT_HANDLER(PE::Events::Event_UPDATE, ServerGameObjectManagerAddon::do_UPDATE);
		}

		void ServerGameObjectManagerAddon::do_UPDATE(PE::Events::Event *pEvt)
		{
			for (std::pair<int, int> x : needToAck)
			{
				int evtOrderID = x.first;
				int clientID = x.second;

				sendAckTo(clientID, evtOrderID);
			}

			needToAck.clear();
		}

		void ServerGameObjectManagerAddon::sendAckTo(int clientID, int orderID)
		{
			Event_Ack_S_to_C fwdEvent(*m_pContext);
			fwdEvent.orderID = orderID;
			fwdEvent.clientID = clientID;
			ServerNetworkManager *pNM = (ServerNetworkManager *)(m_pContext->getNetworkManager());

			int otherID = clientID == 0 ? 1 : 0;
			pNM->scheduleEventToAllExcept(&fwdEvent, m_pContext->getGameObjectManager(), otherID);
		}

		void ServerGameObjectManagerAddon::do_TimeSynch(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_TimeSynch_C_to_S>());

			Event_TimeSynch_C_to_S *pTrueEvent = (Event_TimeSynch_C_to_S *)(pEvt);

			ServerGameObjectManagerAddon::RTT = pTrueEvent->RTT;

			Event_TimeSynch_S_to_C fwdEvent(*m_pContext);
			fwdEvent.RTTStartTime = pTrueEvent->RTTStartTime;
			fwdEvent.ServerTime = TIMENOW;
			//char buf[2556];
			//sprintf(buf, "server now forwarding time of: %llu\n", fwdEvent.ServerTime);
			//OutputDebugStringA(buf);

			ServerNetworkManager *pNM = (ServerNetworkManager *)(m_pContext->getNetworkManager());

			int otherID = pTrueEvent->clientID == 0 ? 1 : 0;
			pNM->scheduleEventToAllExcept(&fwdEvent, m_pContext->getGameObjectManager(), otherID);

		}

		void ServerGameObjectManagerAddon::do_MoveTank(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_MoveTank_C_to_S>());

			Event_MoveTank_C_to_S *pTrueEvent = (Event_MoveTank_C_to_S*)(pEvt);

			// need to send this event to all clients except the client it came from

			Event_MoveTank_S_to_C fwdEvent(*m_pContext);
			fwdEvent.m_transform = pTrueEvent->m_transform;
			fwdEvent.m_clientTankId = pTrueEvent->m_networkClientId; // need to tell cleints which tank to move

			ServerNetworkManager *pNM = (ServerNetworkManager *)(m_pContext->getNetworkManager());
			pNM->scheduleEventToAllExcept(&fwdEvent, m_pContext->getGameObjectManager(), pTrueEvent->m_networkClientId);

		}


		void ServerGameObjectManagerAddon::do_SpawnProjectile(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_SpawnProjectile_C_to_S>());
			Event_SpawnProjectile_C_to_S *pTrueEvent = (Event_SpawnProjectile_C_to_S *)(pEvt);

			Vector4 startPosition = pTrueEvent->startPosition;
			Vector4 endPosition = pTrueEvent->endPosition;
			int teamID = pTrueEvent->teamID;

			for (auto it = GhostManager::serverGhostMap.begin(); it != GhostManager::serverGhostMap.end(); it++)
			{
				int componentID = it->first;
				Ghost *ghost = it->second;

				if (ghost->type == Ghost::Projectile && !ghost->projectileData.isAlive)
				{
					ghost->projectileData.isAlive = true;
					ghost->projectileData.startPosition = Vector3(startPosition.m_x, startPosition.m_y += 0.8f, startPosition.m_z);
					ghost->projectileData.desiredPosition = Vector3(endPosition.m_x, endPosition.m_y += 0.8f, endPosition.m_z);
					ghost->projectileData.actualPosition.m_y += 0.8f;
					ghost->projectileData.actualPosition = ghost->projectileData.startPosition;
					ghost->projectileData.lifeProgress = 0.0f;
					ghost->projectileData.teamID = teamID;
					ghost->changed[0] = true;
					ghost->changed[1] = true;
					//ghost->projectileData.ghostIndex = ghost->originalComponentID;
					break;
				}
			}
			char buf[256];
			sprintf(buf, "projectile teamid: %d\n", teamID);
			OutputDebugStringA(buf);
			sendAckTo(teamID, pTrueEvent->orderID);
		}

		void ServerGameObjectManagerAddon::do_MoveSoldier(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_MoveSoldier_C_to_S>());
			Event_MoveSoldier_C_to_S *pTrueEvent = (Event_MoveSoldier_C_to_S *)(pEvt);
			int componentID = pTrueEvent->id;
			Vector4 targetPosition = pTrueEvent->targetPosition;
			Vector3 polySafePos = Vector3(targetPosition.m_x, 0, targetPosition.m_z);
			std::vector<Vector3> tempPath = ServerGameObjectManagerAddon::serverNavigator.findPath(GhostManager::serverGhostMap[componentID]->soldierData.actualPosition, polySafePos);
			if (tempPath.size() == 0)
			{
				return;
			}

			GhostManager::serverGhostMap[componentID]->soldierData.desiredPosition = Vector3(targetPosition.m_x, targetPosition.m_y, targetPosition.m_z);
			GhostManager::serverGhostMap[componentID]->soldierData.isMoving = true;
			GhostManager::serverGhostMap[componentID]->soldierData.animationState = CharacterControl::Components::SoldierNPCAnimationSM::RUN;

			memcpy(GhostManager::serverGhostMap[componentID]->soldierData.path, &tempPath[0], tempPath.size() * sizeof(tempPath[0]));
			GhostManager::serverGhostMap[componentID]->soldierData.pathLen = tempPath.size();
			//GhostManager::serverGhostMap[componentID]->soldierData.path = tempPath;
			GhostManager::serverGhostMap[componentID]->soldierData.pathIterator = 1;

			sendAckTo(pTrueEvent->id - 1000, pTrueEvent->orderID);

		}

		void ServerGameObjectManagerAddon::do_SoldierAcquireTarget(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_SoldierAcquireTarget_C_to_S>());

			Event_SoldierAcquireTarget_C_to_S *pTrueEvent = (Event_SoldierAcquireTarget_C_to_S*)(pEvt);

			// need to send this event to all clients except the client it came from

			Event_SoldierAcquireTarget_S_to_C fwdEvent(*m_pContext);
			fwdEvent.targetID = pTrueEvent->targetID;
			fwdEvent.soldierID = pTrueEvent->soldierID;

			ServerNetworkManager *pNM = (ServerNetworkManager *)(m_pContext->getNetworkManager());
			pNM->scheduleEventToAllExcept(&fwdEvent, m_pContext->getGameObjectManager(), -1);

		}
		void ServerGameObjectManagerAddon::do_SoldierNudgeTarget(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_NudgeSoldier_C_to_S>());

			float moveDist = 0.1;

			Event_NudgeSoldier_C_to_S *pTrueEvent = (Event_NudgeSoldier_C_to_S*)(pEvt);

			Ghost* targetGhost = GhostManager::serverGhostMap[pTrueEvent->soldierID];
			Vector3 forward = targetGhost->soldierData.nBasis;
			forward.normalize();
			Vector3 right = targetGhost->soldierData.uBasis;
			right.normalize();

			switch (pTrueEvent->direction)
			{
				case 0: // forwards
				{
					targetGhost->soldierData.actualPosition += moveDist * forward;
				} break;

				case 1: // right
				{
					targetGhost->soldierData.actualPosition += moveDist * right;
				} break;

				case 2: // back
				{
					targetGhost->soldierData.actualPosition -= moveDist * forward;
				} break;

				case 3: // left
				{
					targetGhost->soldierData.actualPosition -= moveDist * right;
				} break;

				default:
				{
				}
			}

			targetGhost->shouldSend[0] = true;
			targetGhost->shouldSend[1] = true;
			targetGhost->didUpdate[0] = true;
			targetGhost->didUpdate[1] = true;
			targetGhost->possibleDesync[0] = true;
			targetGhost->possibleDesync[1] = true;
			targetGhost->wasNudged[0] = true;
			targetGhost->wasNudged[1] = true;

			sendAckTo(pTrueEvent->soldierID - 1000, pTrueEvent->orderID);
		}
	}
}
