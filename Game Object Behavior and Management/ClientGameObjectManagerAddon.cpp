#include "ClientGameObjectManagerAddon.h"

#include "PrimeEngine/PrimeEngineIncludes.h"

#include "Characters/SoldierNPC.h"
#include "Characters/TargetNPC.h"
#include "CharacterControl/Audio/SoundManager.h"
#include "WayPoint.h"
#include "Tank/ClientTank.h"
#include "CharacterControl/Client/ClientSpaceShip.h"
#include "Characters/Projectile.h"
#include "PrimeEngine/Scene/CameraManager.h"
#include "PrimeEngine/Networking/Server/ServerNetworkManager.h"
#include "PrimeEngine/Networking/NetworkContext.h"
#include "PrimeEngine/Networking/Tribes/GhostManager/GhostManager.h"



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
		int ClientGameObjectManagerAddon::RTT = 0;
		std::vector<ClientGameObjectManagerAddon::RTTRecord> ClientGameObjectManagerAddon::RTTHistory;

		uint64_t ClientGameObjectManagerAddon::RTTStart = -1;
		long ClientGameObjectManagerAddon::ServerClientTimeDifference = 0;

		bool ClientGameObjectManagerAddon::clientGhostsInitialized = false;
		Navigator ClientGameObjectManagerAddon::clientNavigator;

		float ClientGameObjectManagerAddon::controlDisableTime = 5.0f;
		bool ClientGameObjectManagerAddon::controlDisable = false;

		std::vector<PE::Components::Component *> ClientGameObjectManagerAddon::targetableEntities;
		std::vector<PE::Components::Component *> ClientGameObjectManagerAddon::attackableEntities;
		bool ClientGameObjectManagerAddon::gameOver;
		int ClientGameObjectManagerAddon::losingTeam;
		PE::Components::Component *ClientGameObjectManagerAddon::selectedUnit = NULL;

		std::unordered_map<int, Component *> ClientGameObjectManagerAddon::componentMap;

		PE_IMPLEMENT_CLASS1(ClientGameObjectManagerAddon, Component); // creates a static handle and GteInstance*() methods. still need to create construct

		void ClientGameObjectManagerAddon::addDefaultComponents()
		{
			GameObjectManagerAddon::addDefaultComponents();

			PE_REGISTER_EVENT_HANDLER(Event_CreateSoldierNPC, ClientGameObjectManagerAddon::do_CreateSoldierNPC);
			PE_REGISTER_EVENT_HANDLER(Event_CreateTargetNPC, ClientGameObjectManagerAddon::do_CreateTargetNPC);
			PE_REGISTER_EVENT_HANDLER(Event_CREATE_WAYPOINT, ClientGameObjectManagerAddon::do_CREATE_WAYPOINT);

			// note this component (game obj addon) is added to game object manager after network manager, so network manager will process this event first
			PE_REGISTER_EVENT_HANDLER(PE::Events::Event_SERVER_CLIENT_CONNECTION_ACK, ClientGameObjectManagerAddon::do_SERVER_CLIENT_CONNECTION_ACK);

			PE_REGISTER_EVENT_HANDLER(Event_MoveSoldier_S_to_C, ClientGameObjectManagerAddon::do_MoveSoldier);
			PE_REGISTER_EVENT_HANDLER(Event_TimeSynch_S_to_C, ClientGameObjectManagerAddon::do_TimeSynch);
			PE_REGISTER_EVENT_HANDLER(Event_SoldierAcquireTarget_S_to_C, ClientGameObjectManagerAddon::do_SoldierAcquireTarget);
			PE_REGISTER_EVENT_HANDLER(Event_MoveTank_S_to_C, ClientGameObjectManagerAddon::do_MoveTank);

			PE_REGISTER_EVENT_HANDLER(Event_CreateProjectile, ClientGameObjectManagerAddon::do_CreateProjectile);
			PE_REGISTER_EVENT_HANDLER(Event_InitProjectilePool, ClientGameObjectManagerAddon::do_InitProjectilePool);
			PE_REGISTER_EVENT_HANDLER(Event_Ack_S_to_C, ClientGameObjectManagerAddon::do_Ack);
		}

		void ClientGameObjectManagerAddon::do_CreateTargetNPC(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_CreateTargetNPC>());
			Event_CreateTargetNPC *pTrueEvent = (Event_CreateTargetNPC*)(pEvt);

			createTargetNPC(pTrueEvent);
		}

		void ClientGameObjectManagerAddon::do_CreateProjectile(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_CreateProjectile>());
			Event_CreateProjectile *pTrueEvent = (Event_CreateProjectile*)(pEvt);

			Projectile::EnableProjectile(pTrueEvent->startPosition, pTrueEvent->targetPosition, pTrueEvent->teamID);
			//createProjectile(pTrueEvent);
		}


#include "CharacterControl\Audio\SoundManager.h"
		void ClientGameObjectManagerAddon::do_InitProjectilePool(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_InitProjectilePool>());
			Event_InitProjectilePool *pTrueEvent = (Event_InitProjectilePool *)(pEvt);


			//Projectile::initPool();
		}

		void ClientGameObjectManagerAddon::do_CreateSoldierNPC(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_CreateSoldierNPC>());

			Event_CreateSoldierNPC *pTrueEvent = (Event_CreateSoldierNPC*)(pEvt);

			createSoldierNPC(pTrueEvent);
		}

		void ClientGameObjectManagerAddon::createSoldierNPC(Vector3 pos, int &threadOwnershipMask)
		{
			Event_CreateSoldierNPC evt(threadOwnershipMask);
			evt.m_pos = pos;
			evt.m_u = Vector3(1.0f, 0, 0);
			evt.m_v = Vector3(0, 1.0f, 0);
			evt.m_n = Vector3(0, 0, 1.0f);

			StringOps::writeToString("SoldierTransform.mesha", evt.m_meshFilename, 255);
			StringOps::writeToString("Soldier", evt.m_package, 255);
			StringOps::writeToString("mg34.x_mg34main_mesh.mesha", evt.m_gunMeshName, 64);
			StringOps::writeToString("CharacterControl", evt.m_gunMeshPackage, 64);
			StringOps::writeToString("", evt.m_patrolWayPoint, 32);
			createSoldierNPC(&evt);
		}
		
		void ClientGameObjectManagerAddon::createSoldierNPC(Event_CreateSoldierNPC *pTrueEvent)
		{
			PEINFO("CharacterControl: GameObjectManagerAddon: Creating CreateSoldierNPC\n");

			PE::Handle hSoldierNPC("SoldierNPC", sizeof(SoldierNPC));
			SoldierNPC *pSoldierNPC = new(hSoldierNPC) SoldierNPC(*m_pContext, m_arena, hSoldierNPC, pTrueEvent);
			pSoldierNPC->addDefaultComponents();

			// add the soldier as component to the ObjecManagerComponentAddon
			// all objects of this demo live in the ObjecManagerComponentAddon
			addComponent(hSoldierNPC);
		}

		
		//void ClientGameObjectManagerAddon::createProjectile(Event_CreateProjectile *pTrueEvent)
		//{
		//	PEINFO("CharacterControl: GameObjectManagerAddon: Allocating Projectile\n");

		//	Projectile::EnableProjectile();
		//}

		void ClientGameObjectManagerAddon::createTargetNPC(Event_CreateTargetNPC *pTrueEvent)
		{
			PEINFO("CharacterControl: GameObjectManagerAddon: Creating CreateTargetNPC\n");

			PE::Handle hTargetNPC("TargetNPC", sizeof(TargetNPC));
			TargetNPC *pTargetNPC = new(hTargetNPC) TargetNPC(*m_pContext, m_arena, hTargetNPC, pTrueEvent);
			pTargetNPC->addDefaultComponents();

			// add the soldier as component to the ObjecManagerComponentAddon
			// all objects of this demo live in the ObjecManagerComponentAddon
			addComponent(hTargetNPC);
		}

		void ClientGameObjectManagerAddon::do_CREATE_WAYPOINT(PE::Events::Event *pEvt)
		{
			PEINFO("GameObjectManagerAddon::do_CREATE_WAYPOINT()\n");

			assert(pEvt->isInstanceOf<Event_CREATE_WAYPOINT>());

			Event_CREATE_WAYPOINT *pTrueEvent = (Event_CREATE_WAYPOINT*)(pEvt);

			PE::Handle hWayPoint("WayPoint", sizeof(WayPoint));
			WayPoint *pWayPoint = new(hWayPoint) WayPoint(*m_pContext, m_arena, hWayPoint, pTrueEvent);
			pWayPoint->addDefaultComponents();

			addComponent(hWayPoint);
		}

		TargetNPC *ClientGameObjectManagerAddon::getTarget()
		{
			PE::Handle *pHC = m_components.getFirstPtr();
			for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
			{
				Component *pC = (*pHC).getObject<Component>();

				if (pC->isInstanceOf<TargetNPC>())
				{
					TargetNPC *pTarget = (TargetNPC *)(pC);
					return pTarget;
				}
			}

			return NULL;
		}

		WayPoint *ClientGameObjectManagerAddon::getWayPoint(const char *name)
		{
			PE::Handle *pHC = m_components.getFirstPtr();

			for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
			{
				Component *pC = (*pHC).getObject<Component>();

				if (pC->isInstanceOf<WayPoint>())
				{
					WayPoint *pWP = (WayPoint *)(pC);
					if (StringOps::strcmp(pWP->m_name, name) == 0)
					{
						// equal strings, found our waypoint
						return pWP;
					}
				}
			}
			return NULL;
		}

		void ClientGameObjectManagerAddon::createProjectilePool(int &threadOwnershipMask, PE::GameContext *context, PE::MemoryArena arena)
		{
			for (int i = 0; i < 8; i++)
			{
				createIndividualProjectile(threadOwnershipMask, context, arena, i);
			}
		}

		void ClientGameObjectManagerAddon::createIndividualProjectile(int &threadOwnershipMask, PE::GameContext *m_pContext, PE::MemoryArena m_arena, int projectileID)
		{
			PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
			MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);

			pMeshInstance->addDefaultComponents();
			pMeshInstance->initFromFile("rect.mesha", "Default", threadOwnershipMask);

			// need to create a scene node for this mesh
			PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
			SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
			
			Camera *camera = CameraManager::Instance()->getActiveCamera();
			Matrix4x4 cameraBase = camera->getCamSceneNode()->m_base;
			pSN->m_base.setU(cameraBase.getU());
			pSN->m_base.setV(cameraBase.getN());
			pSN->m_base.setN(cameraBase.getV());
			pSN->m_base.scaleN(0.05f);
			pSN->m_base.scaleU(0.05f);
			pSN->m_base.scaleV(0.05f);
			//Vector3 spawnPos(0, 0, 21.0f);
			//pSN->m_base.setPos(spawnPos);

			pSN->addDefaultComponents();

			pSN->addComponent(hMeshInstance);

			RootSceneNode::Instance()->addComponent(hSN);

			// now add game objects

			PE::Handle hProjectile("Projectile", sizeof(Projectile));
			Projectile *pProjectile = new(hProjectile) Projectile(*m_pContext, m_arena, hProjectile);
			pProjectile->componentID = projectileID;
			ClientGameObjectManagerAddon::componentMap.insert(std::make_pair(projectileID, pProjectile));
			pMeshInstance->particle = true;
			pProjectile->meshInstance = pMeshInstance;
			pProjectile->sceneNode = pSN;
			pProjectile->addDefaultComponents();
			addComponent(hProjectile);
			pProjectile->disable();
			static int allowedEvts[] = { 0 };
			addComponent(hSN, &allowedEvts[0]);

			PE::Handle hGhost("Ghost", sizeof(Ghost));
			pProjectile->mGhost = new(hGhost) Ghost(*m_pContext);
			pProjectile->mGhost->pComponent = pProjectile;
			pProjectile->mGhost->type = PE::Components::Ghost::Projectile;
			pProjectile->mGhost->originalComponentID = projectileID;
			pProjectile->mGhost->projectileData.isAlive = false;
		}

		void ClientGameObjectManagerAddon::createTank(int index, int &threadOwnershipMask)
		{

			//create hierarchy:
			//scene root
			//  scene node // tracks position/orientation
			//    Tank

			//game object manager
			//  TankController
			//    scene node

			PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
			MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);

			pMeshInstance->addDefaultComponents();
			pMeshInstance->initFromFile("kingtiger.x_main_mesh.mesha", "Default", threadOwnershipMask);

			// need to create a scene node for this mesh
			PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
			SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
			pSN->addDefaultComponents();

			Vector3 spawnPos(-36.0f + 6.0f * index, 0, 21.0f);
			pSN->m_base.setPos(spawnPos);

			pSN->addComponent(hMeshInstance);

			RootSceneNode::Instance()->addComponent(hSN);

			// now add game objects

			PE::Handle hTankController("TankController", sizeof(TankController));
			TankController *pTankController = new(hTankController) TankController(*m_pContext, m_arena, hTankController, 0.05f, spawnPos, 0.05f);
			pTankController->addDefaultComponents();

			addComponent(hTankController);

			// add the same scene node to tank controller
			static int alllowedEventsToPropagate[] = { 0 }; // we will pass empty array as allowed events to propagate so that when we add
			// scene node to the square controller, the square controller doesnt try to handle scene node's events
			// because scene node handles events through scene graph, and is child of square controller just for referencing purposes
			pTankController->addComponent(hSN, &alllowedEventsToPropagate[0]);
		}

		void ClientGameObjectManagerAddon::createSpaceShip(int &threadOwnershipMask)
		{

			//create hierarchy:
			//scene root
			//  scene node // tracks position/orientation
			//    SpaceShip

			//game object manager
			//  SpaceShipController
			//    scene node

			PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
			MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);

			pMeshInstance->addDefaultComponents();
			pMeshInstance->initFromFile("space_frigate_6.mesha", "FregateTest", threadOwnershipMask);

			// need to create a scene node for this mesh
			PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
			SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
			pSN->addDefaultComponents();

			Vector3 spawnPos(0, 0, 0.0f);
			pSN->m_base.setPos(spawnPos);

			pSN->addComponent(hMeshInstance);

			RootSceneNode::Instance()->addComponent(hSN);

			// now add game objects

			PE::Handle hSpaceShip("ClientSpaceShip", sizeof(ClientSpaceShip));
			ClientSpaceShip *pSpaceShip = new(hSpaceShip) ClientSpaceShip(*m_pContext, m_arena, hSpaceShip, 0.05f, spawnPos, 0.05f);
			pSpaceShip->addDefaultComponents();

			addComponent(hSpaceShip);

			// add the same scene node to tank controller
			static int alllowedEventsToPropagate[] = { 0 }; // we will pass empty array as allowed events to propagate so that when we add
			// scene node to the square controller, the square controller doesnt try to handle scene node's events
			// because scene node handles events through scene graph, and is child of space ship just for referencing purposes
			pSpaceShip->addComponent(hSN, &alllowedEventsToPropagate[0]);

			pSpaceShip->activate();
		}

		int ClientGameObjectManagerAddon::clientID;
		void ClientGameObjectManagerAddon::do_SERVER_CLIENT_CONNECTION_ACK(PE::Events::Event *pEvt)
		{
			Event_SERVER_CLIENT_CONNECTION_ACK *pRealEvt = (Event_SERVER_CLIENT_CONNECTION_ACK *)(pEvt);
			PE::Handle *pHC = m_components.getFirstPtr();

			int itc = 0;
			ClientGameObjectManagerAddon::clientID = pRealEvt->m_clientId;
			//for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
			//{
			//	Component *pC = (*pHC).getObject<Component>();

			//	if (pC->isInstanceOf<TankController>())
			//	{
			//		if (itc == pRealEvt->m_clientId) //activate tank controller for local client based on local clients id
			//		{
			//			TankController *pTK = (TankController *)(pC);
			//			pTK->activate();
			//			break;
			//		}
			//		++itc;
			//	}
			//}

			for (Projectile *projectile : Projectile::projectiles)
			{
				PE::Components::ClientNetworkManager *pNetworkManager = (ClientNetworkManager *)(m_pContext->getNetworkManager());
				pNetworkManager->getNetworkContext().getGhostManager()->ghostMap.insert(std::make_pair(projectile->componentID, projectile->mGhost));
				pNetworkManager->getNetworkContext().getGhostManager()->scheduleGhost(projectile->mGhost);
			}

			InitXAudio();
			InitSound("AssetsOut\\Audio\\spell_fire.wav", false, 8);
			InitSound("AssetsOut\\Audio\\grunt.wav", false, 4);
			InitSound("AssetsOut\\Audio\\dungeon1.wav", true);
			m_pContext->getLuaEnvironment()->runString("LevelLoader.loadLevel('villagedivided.x_level.levela', 'VillageDivided')");
			m_pContext->getLuaEnvironment()->runString("LevelLoader.loadLevel('basic.x_level.levela', 'Basic')");
			PlaySoundX("AssetsOut\\Audio\\dungeon1.wav");
			ClientGameObjectManagerAddon::clientGhostsInitialized = true;

		}

		void ClientGameObjectManagerAddon::do_TimeSynch(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_TimeSynch_S_to_C>());

			Event_TimeSynch_S_to_C *pTrueEvent = (Event_TimeSynch_S_to_C*)(pEvt);

			static uint64_t lastSeenRTTStartTime = 0;
			uint64_t timenow = TIMENOW;

			if (pTrueEvent->RTTStartTime > lastSeenRTTStartTime)
			{
				int RTT = TIMENOW - pTrueEvent->RTTStartTime;
				RTTRecord record;
				record.time = TIMENOW;
				record.RTT = RTT;
				ClientGameObjectManagerAddon::RTTHistory.push_back(record);

				int thingsToAverage = ClientGameObjectManagerAddon::RTTHistory.size() > 2 ? 2 : ClientGameObjectManagerAddon::RTTHistory.size();
				int total = 0;
				for (int i = ClientGameObjectManagerAddon::RTTHistory.size() - 1; i > ClientGameObjectManagerAddon::RTTHistory.size() - 1 - thingsToAverage; i--)
				{
					total += ClientGameObjectManagerAddon::RTTHistory[i].RTT;
			}
				ClientGameObjectManagerAddon::RTT = total / thingsToAverage;

				float ClientShareOfRTT = 3.0f / 5.0f; // TUNE THIS
				int ClientDelay = ClientShareOfRTT * RTT;
				ClientGameObjectManagerAddon::ServerClientTimeDifference = pTrueEvent->ServerTime - (timenow - ClientDelay);
				//char buf[256];
				//sprintf(buf, "received server time update: %llu\n", pTrueEvent->ServerTime);
				//OutputDebugStringA(buf);
				char rttbuf[256];
				sprintf(rttbuf, "RTT is: %d\n", RTT);
				OutputDebugStringA(rttbuf);
				/*char buftwo[256];
				sprintf(buftwo, "the current server client time difference is: %ld\n\n", ClientGameObjectManagerAddon::ServerClientTimeDifference);
				OutputDebugStringA(buftwo);*/
			}
		}

		void ClientGameObjectManagerAddon::do_Ack(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_Ack_S_to_C>());

			Event_Ack_S_to_C *pTrueEvent = (Event_Ack_S_to_C*)(pEvt);

			int orderID = pTrueEvent->orderID;

			char buf[256];
			sprintf(buf, "received ACK for event: %d\n", orderID);
			OutputDebugStringA(buf);

			if (orderID - EventManager::SendingSlidingWindowStart < 0 || EventManager::SendingSlidingWindow[orderID - EventManager::SendingSlidingWindowStart].isAcked)
			{
				return;
			}

			EventManager::SendingSlidingWindow[orderID - EventManager::SendingSlidingWindowStart].isAcked = true;

			int removeIndex = -1;
			for (int i = 0; i < EventManager::SendingSlidingWindowSize; i++) // see what we can remove
			{
				if (EventManager::SendingSlidingWindow[i].isAcked == false)
				{
					break;
				}
				else
				{
					removeIndex = i + 1;
				}
			}
			if (removeIndex == -1) // nothing to remove
			{
				char buf[256];
				sprintf(buf, "... sliding window now of size: %d\n", EventManager::SendingSlidingWindowSize);
				OutputDebugStringA(buf);
				return;
			}
			EventManager::SendingSlidingWindowSize -= removeIndex;
			memmove(&EventManager::SendingSlidingWindow[0], &EventManager::SendingSlidingWindow[removeIndex],
				sizeof(EventManager::EventTransmissionRecord) * (64 - removeIndex));
			memset(&EventManager::SendingSlidingWindow[EventManager::SendingSlidingWindowSize], 0,
				sizeof(EventManager::EventTransmissionRecord) * (64 - EventManager::SendingSlidingWindowSize));
			EventManager::SendingSlidingWindowStart += removeIndex;


			/*if (numProcessed && numProcessed < PE_EVENT_SLIDING_WINDOW)
			{
				int numLeftoverEvents = PE_EVENT_SLIDING_WINDOW - numProcessed;
				memmove(&m_receivedEvents[0], &m_receivedEvents[numProcessed], sizeof(EventReceptionData) * numLeftoverEvents);
				memset(&m_receivedEvents[numLeftoverEvents], 0, sizeof(EventReceptionData) * (PE_EVENT_SLIDING_WINDOW - numLeftoverEvents));
			}*/


			char buf2[256];
			sprintf(buf2, "... sliding window now of size: %d\n", EventManager::SendingSlidingWindowSize);
			OutputDebugStringA(buf2);

		}

		void ClientGameObjectManagerAddon::do_MoveSoldier(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_MoveSoldier_S_to_C>());

			Event_MoveSoldier_S_to_C *pTrueEvent = (Event_MoveSoldier_S_to_C*)(pEvt);
			PE::Handle *pHC = m_components.getFirstPtr();

			int itc = 0;
			for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
			{
				Component *pC = (*pHC).getObject<Component>();
				if (pC->isInstanceOf<SoldierNPC>())
				{
					SoldierNPC *soldierNPC = (SoldierNPC *)pC;
					if (soldierNPC->componentID == pTrueEvent->id) 
					{
						soldierNPC->behaviorSM->m_state = CharacterControl::Components::SoldierNPCBehaviorSM::PATROLLING_WAYPOINTS;
						PE::Handle h("SoldierNPCMovementSM_Event_MOVE_TO", sizeof(SoldierNPCMovementSM_Event_MOVE_TO));
						Vector3 targetPos = { pTrueEvent->targetPosition.m_x, pTrueEvent->targetPosition.m_y, pTrueEvent->targetPosition.m_z };
						CharacterControl::Events::SoldierNPCMovementSM_Event_MOVE_TO *pEvt = new(h) SoldierNPCMovementSM_Event_MOVE_TO(targetPos);
						pEvt->m_running = pTrueEvent->isRunning;
						soldierNPC->movementSM->handleEvent(pEvt);
						h.release();
						break;
					}

					++itc;
				}
			}
		}

		void ClientGameObjectManagerAddon::do_SoldierAcquireTarget(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_SoldierAcquireTarget_S_to_C>());

			Event_SoldierAcquireTarget_S_to_C *pTrueEvent = (Event_SoldierAcquireTarget_S_to_C*)(pEvt);
			PE::Handle *pHC = m_components.getFirstPtr();

			for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
			{
				Component *pTC = (*pHC).getObject<Component>();
				if (pTC->isInstanceOf<TargetNPC>())
				{
					TargetNPC *targetNPC = (TargetNPC *)pTC;
					if (targetNPC->targetID == pTrueEvent->targetID)
					{
						pHC = m_components.getFirstPtr();
						for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
						{
							Component *pSC = (*pHC).getObject<Component>();
							if (pSC->isInstanceOf<SoldierNPC>())
							{
								SoldierNPC *soldierNPC = (SoldierNPC *)pSC;
								if (soldierNPC->componentID == pTrueEvent->soldierID)
								{
									soldierNPC->behaviorSM->m_state = CharacterControl::Components::SoldierNPCBehaviorSM::HAS_TARGET;
									soldierNPC->behaviorSM->targetNPC = targetNPC;
									break;
								}
							}
						}
					}
				}
			}
		}

		void ClientGameObjectManagerAddon::do_MoveTank(PE::Events::Event *pEvt)
		{
			assert(pEvt->isInstanceOf<Event_MoveTank_S_to_C>());

			Event_MoveTank_S_to_C *pTrueEvent = (Event_MoveTank_S_to_C*)(pEvt);

			PE::Handle *pHC = m_components.getFirstPtr();

			int itc = 0;
			for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
			{
				Component *pC = (*pHC).getObject<Component>();

				if (pC->isInstanceOf<TankController>())
				{
					if (itc == pTrueEvent->m_clientTankId) //activate tank controller for local client based on local clients id
					{
						TankController *pTK = (TankController *)(pC);
						pTK->overrideTransform(pTrueEvent->m_transform);
						break;
					}
					++itc;
				}
			}
		}

	}
}
