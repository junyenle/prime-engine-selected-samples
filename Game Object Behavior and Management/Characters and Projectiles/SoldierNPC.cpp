#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/RootSceneNode.h"

#include "CharacterControl/ClientGameObjectManagerAddon.h"
#include "CharacterControl/CharacterControlContext.h"

#include "PrimeEngine/Scene/SkeletonInstance.h"
#include "PrimeEngine/Scene/MeshInstance.h"

#include "SoldierNPC.h"
#include "SoldierNPCAnimationSM.h"
#include "SoldierNPCMovementSM.h"
#include "SoldierNPCBehaviorSM.h"

#include "PrimeEngine/Networking/Tribes/GhostManager/GhostManager.h"
#include "PrimeEngine/Networking/Tribes/GhostManager/Ghost.h"

#include "PrimeEngine/Networking/Client/ClientNetworkManager.h"
#include "PrimeEngine/Networking/NetworkContext.h"

using namespace PE;
using namespace PE::Components;
using namespace CharacterControl::Events;

namespace CharacterControl
{
	namespace Components
	{
		PE_IMPLEMENT_CLASS1(SoldierNPC, Component);

		SoldierNPC::SoldierNPC(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, Event_CreateSoldierNPC *pEvt) : Component(context, arena, hMyself)
		{
			teamID = pEvt->m_teamID;
			componentID = 1000 + teamID;
			ClientGameObjectManagerAddon::componentMap.insert(std::make_pair(componentID, this));

			// hierarchy of soldier and replated components and variables (note variables are just variables, they are not passed events to)
			// scene
			// +-components
			//   +-soldier scene node
			//   | +-components
			//   |   +-soldier skin
			//   |     +-components
			//   |       +-soldier animation state machine
			//   |       +-soldier weapon skin scene node
			//   |         +-components
			//   |           +-weapon mesh

			// game objects
			// +-components
			//   +-soldier npc
			//     +-variables
			//     | +-m_hMySN = soldier scene node
			//     | +-m_hMySkin = skin
			//     | +-m_hMyGunSN = soldier weapon skin scene node
			//     | +-m_hMyGunMesh = weapon mesh
			//     +-components
			//       +-soldier scene node (restricted to no events. this is for state machines to be able to locate the scene node)
			//       +-movement state machine
			//       +-behavior state machine


			// need to acquire redner context for this code to execute thread-safe
			m_pContext->getGPUScreen()->AcquireRenderContextOwnership(pEvt->m_threadOwnershipMask);

			PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
			SceneNode *pMainSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
			pMainSN->addDefaultComponents();

			pMainSN->m_base.setPos(pEvt->m_pos);
			pMainSN->m_base.setU(pEvt->m_u);
			pMainSN->m_base.setV(pEvt->m_v);
			pMainSN->m_base.setN(pEvt->m_n);
			sceneNode = pMainSN;

			//m_base.setPos(pEvt->m_pos);
			//m_base.setU(pEvt->m_u);
			//m_base.setV(pEvt->m_v);
			//m_base.setN(pEvt->m_n);

			// TODO(Rohan) : actually reload obsj instead of resetting them	
			defaultBase.setPos(pEvt->m_pos);
			defaultBase.setU(pEvt->m_u);
			defaultBase.setV(pEvt->m_v);
			defaultBase.setN(pEvt->m_n);

			RootSceneNode::Instance()->addComponent(hSN);

			// add the scene node as component of soldier without any handlers. this is just data driven way to locate scnenode for soldier's components
			{
				static int allowedEvts[] = { 0 };
				addComponent(hSN, &allowedEvts[0]);
			}

			int numskins = 1; // 8
			for (int iSkin = 0; iSkin < numskins; ++iSkin)
			{
				float z = (iSkin / 4) * 1.5f;
				float x = (iSkin % 4) * 1.5f;
				PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
				SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
				pSN->addDefaultComponents();

				pSN->m_base.setPos(Vector3(x, 0, z));

				// rotation scene node to rotate soldier properly, since soldier from Maya is facing wrong direction
				PE::Handle hRotateSN("SCENE_NODE", sizeof(SceneNode));
				SceneNode *pRotateSN = new(hRotateSN) SceneNode(*m_pContext, m_arena, hRotateSN);
				pRotateSN->addDefaultComponents();

				pSN->addComponent(hRotateSN);

				pRotateSN->m_base.turnLeft(3.1415);

				PE::Handle hSoldierAnimSM("SoldierNPCAnimationSM", sizeof(SoldierNPCAnimationSM));
				SoldierNPCAnimationSM *pSoldierAnimSM = new(hSoldierAnimSM) SoldierNPCAnimationSM(*m_pContext, m_arena, hSoldierAnimSM);
				animationSM = pSoldierAnimSM;
				pSoldierAnimSM->addDefaultComponents();

				pSoldierAnimSM->m_debugAnimIdOffset = 0;// rand() % 3;

				PE::Handle hSkeletonInstance("SkeletonInstance", sizeof(SkeletonInstance));
				addComponent(hSoldierAnimSM);
				SkeletonInstance *pSkelInst = new(hSkeletonInstance) SkeletonInstance(*m_pContext, m_arena, hSkeletonInstance, hSoldierAnimSM);
				//mSkelInst = pSkelInst;
				pSkelInst->addDefaultComponents();

				//TODO(JUN) : this is dumb af
				//pSkelInst->initFromFiles("soldier_Soldier_Skeleton.skela", "Soldier", pEvt->m_threadOwnershipMask);
				pSkelInst->initFromFiles("CastleGuard7_Hips.skela", "CastleGuard", pEvt->m_threadOwnershipMask);

				pSkelInst->setAnimSet("soldier_Soldier_Skeleton.animseta", "Soldier");

				PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
				MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);
				meshInstance = pMeshInstance;
				//mMeshInst = pMeshInstance;
				pMeshInstance->addDefaultComponents();

				pMeshInstance->initFromFile(pEvt->m_meshFilename, pEvt->m_package, pEvt->m_threadOwnershipMask);

				pSkelInst->addComponent(hMeshInstance);

				// add skin to scene node
				pRotateSN->addComponent(hSkeletonInstance);

				//Heart
				for (int i = 0; i < 5; i++)
				{
					InitHearts(pEvt->m_threadOwnershipMask);
				}

				FixHeartsPos();

#if !APIABSTRACTION_D3D11

				{
					PE::Handle hMyGunMesh = PE::Handle("MeshInstance", sizeof(MeshInstance));
					MeshInstance *pGunMeshInstance = new(hMyGunMesh) MeshInstance(*m_pContext, m_arena, hMyGunMesh);

					pGunMeshInstance->addDefaultComponents();
					pGunMeshInstance->initFromFile(pEvt->m_gunMeshName, pEvt->m_gunMeshPackage, pEvt->m_threadOwnershipMask);

					// create a scene node for gun attached to a joint

					PE::Handle hMyGunSN = PE::Handle("SCENE_NODE", sizeof(JointSceneNode));
					JointSceneNode *pGunSN = new(hMyGunSN) JointSceneNode(*m_pContext, m_arena, hMyGunSN, 38);
					pGunSN->addDefaultComponents();

					// add gun to joint
					pGunSN->addComponent(hMyGunMesh);

					// add gun scene node to the skin
					pSkelInst->addComponent(hMyGunSN);
				}

#endif

				pMainSN->addComponent(hSN);
			}

			m_pContext->getGPUScreen()->ReleaseRenderContextOwnership(pEvt->m_threadOwnershipMask);

#if 1
			// add movement state machine to soldier npc
			PE::Handle hSoldierMovementSM("SoldierNPCMovementSM", sizeof(SoldierNPCMovementSM));
			SoldierNPCMovementSM *pSoldierMovementSM = new(hSoldierMovementSM) SoldierNPCMovementSM(*m_pContext, m_arena, hSoldierMovementSM);
			pSoldierMovementSM->soldierNPC = this;
			movementSM = pSoldierMovementSM;
			pSoldierMovementSM->addDefaultComponents();

			// add it to soldier NPC
			addComponent(hSoldierMovementSM);

			// add behavior state machine ot soldier npc
			PE::Handle hSoldierBheaviorSM("SoldierNPCBehaviorSM", sizeof(SoldierNPCBehaviorSM));
			SoldierNPCBehaviorSM *pSoldierBehaviorSM = new(hSoldierBheaviorSM) SoldierNPCBehaviorSM(*m_pContext, m_arena, hSoldierBheaviorSM, hSoldierMovementSM);
			pSoldierBehaviorSM->soldierNPC = this;
			behaviorSM = pSoldierBehaviorSM;
			pSoldierBehaviorSM->addDefaultComponents();

			// add it to soldier NPC
			addComponent(hSoldierBheaviorSM);


			StringOps::writeToString(pEvt->m_patrolWayPoint, pSoldierBehaviorSM->m_curPatrolWayPoint, 32);
			pSoldierBehaviorSM->m_havePatrolWayPoint = StringOps::length(pSoldierBehaviorSM->m_curPatrolWayPoint) > 0;

			// start the soldier
			pSoldierBehaviorSM->start();
#endif

			CharacterControl::Components::ClientGameObjectManagerAddon::targetableEntities.push_back(this);
			CharacterControl::Components::ClientGameObjectManagerAddon::attackableEntities.push_back(this);

			if (teamID == ClientGameObjectManagerAddon::clientID)
			{
				this->selected = true;
				this->meshInstance->selected = true;
				ClientGameObjectManagerAddon::selectedUnit = this;
			}

			PE::Handle hGhost("Ghost", sizeof(Ghost));
			mGhost = new(hGhost) Ghost(*m_pContext);
			mGhost->pComponent = this;
			mGhost->type = PE::Components::Ghost::SoldierNPC;
			mGhost->originalComponentID = componentID;
			mGhost->soldierData.isMoving = false;
			mGhost->soldierData.animationState = SoldierNPCAnimationSM::STAND;
			mGhost->soldierData.desiredPosition = Vector3();
			mGhost->soldierData.actualPosition = sceneNode->m_base.getPos();
			mGhost->soldierData.uBasis = sceneNode->m_base.getU();
			mGhost->soldierData.vBasis = sceneNode->m_base.getV();
			mGhost->soldierData.nBasis = sceneNode->m_base.getN();
			mGhost->soldierData.health = health;
			PE::Components::ClientNetworkManager *pNetworkManager = (ClientNetworkManager *)(m_pContext->getNetworkManager());
			pNetworkManager->getNetworkContext().getGhostManager()->ghostMap.insert(std::make_pair(componentID, mGhost));
			pNetworkManager->getNetworkContext().getGhostManager()->scheduleGhost(mGhost);
		}

		void SoldierNPC::addDefaultComponents()
		{
			Component::addDefaultComponents();

			// custom methods of this component
		}

		void SoldierNPC::InitHearts(int& threadOwnershipMask)
		{
			//Hearts!
			PE::Handle hHeartMI1("MeshInstanceHeart1", sizeof(MeshInstance));
			MeshInstance *pHeartMI1 = new(hHeartMI1) MeshInstance(*m_pContext, m_arena, hHeartMI1);
			pHeartMI1->addDefaultComponents();
			pHeartMI1->initFromFile("heart.mesha", "Default", threadOwnershipMask);
			heartMIs.push_back(pHeartMI1);
			pHeartMI1->m_culledOut = true;
			pHeartMI1->particle = true;

			PE::Handle hHeartSN1("SCENE_NODE_Heart1", sizeof(SceneNode));
			SceneNode *pHeartSN1 = new(hHeartSN1) SceneNode(*m_pContext, m_arena, hHeartSN1);
			heartSNs.push_back(pHeartSN1);

			/*Camera *camera = CameraManager::Instance()->getActiveCamera();
			Matrix4x4 cameraBase = camera->getCamSceneNode()->m_base;
			pHeartSN1->m_base.setU(cameraBase.getU());
			pHeartSN1->m_base.setV(cameraBase.getN());
			pHeartSN1->m_base.setN(cameraBase.getV());
			pHeartSN1->m_base.scaleN(0.05f);
			pHeartSN1->m_base.scaleU(0.05f);*/
			/*pHeartSN1->m_base.scaleV(0.05f);*/

			pHeartSN1->addDefaultComponents();
			pHeartSN1->addComponent(hHeartMI1);

			/*if (heartSNs.size() == 1) {
				RootSceneNode::Instance()->addComponent(hHeartSN1);
			}
			else {
				heartSNs[0]->addComponent(hHeartSN1);
			}*/
			//sceneNode->addComponent(hHeartSN1);
			RootSceneNode::Instance()->addComponent(hHeartSN1);
		}

		void SoldierNPC::FixHeartsPos()
		{
			/*int count = 0;
			for (auto sn : heartSNs) {
				sn->m_base.setPos(Vector3(heartOffsetX * count, heartOffsetY * count, heartOffsetZ * count));
				count++;
			}*/

			//Look like this: 42013
			Vector3 offset = Vector3(-heartOffsetX, heartOffsetY, heartOffsetZ);
			Vector3 middle = Vector3(middleHeartOffsetX, middleHeartOffsetY, middleHeartOffsetZ) + sceneNode->m_base.getPos();
			heartSNs[0]->m_base.setPos(middle);
			heartSNs[1]->m_base.setPos(heartSNs[0]->m_base.getPos() + offset * 1);
			heartSNs[2]->m_base.setPos(heartSNs[0]->m_base.getPos() + offset * -1);
			heartSNs[3]->m_base.setPos(heartSNs[0]->m_base.getPos() + offset * 2);
			heartSNs[4]->m_base.setPos(heartSNs[0]->m_base.getPos() + offset * -2);
		}

		void SoldierNPC::UpdateHearts()
		{
			for (auto mi : heartMIs)
			{
				mi->m_display = false;
				mi->lifeProgress = 0.0f;
			}

			Camera *camera = CameraManager::Instance()->getActiveCamera();
			Matrix4x4 cameraBase = camera->getCamSceneNode()->m_base;

			if (heartMIs.size() > 0)
			{
				//heartSNs[0]->m_base.setPos(sceneNode->m_base.getPos());
				FixHeartsPos();
				for (int i = 0; i < health; i++)
				{
					heartMIs[i]->m_display = true;
					heartSNs[i]->m_base.setU(cameraBase.getU());
					heartSNs[i]->m_base.setV(cameraBase.getN());
					heartSNs[i]->m_base.setN(cameraBase.getV());
					heartSNs[i]->m_base.scaleN(0.025f);
					heartSNs[i]->m_base.scaleU(0.025f);
					heartSNs[i]->m_base.scaleV(0.025f);
				}
			}
		}

	}; // namespace Components
}; // namespace CharacterControl
