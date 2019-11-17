#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/RootSceneNode.h"
#include "TargetNPC.h"
#include "TargetNPCBehaviorSM.h"
#include "TargetNPCMovementSM.h"
#include "CharacterControl/ClientGameObjectManagerAddon.h"

using namespace PE;
using namespace PE::Components;
using namespace CharacterControl::Events;

namespace CharacterControl
{
	namespace Components
	{
		static int runningTargetID = 0;
		PE_IMPLEMENT_CLASS1(TargetNPC, Component);
		TargetNPC::TargetNPC(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, Event_CreateTargetNPC *pEvt) 
			: Component(context, arena, hMyself)
		{
			targetID = runningTargetID++;
			// need to acquire redner context for this code to execute thread-safe
			m_pContext->getGPUScreen()->AcquireRenderContextOwnership(pEvt->m_threadOwnershipMask);

			PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
			SceneNode *pMainSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
			pMainSN->addDefaultComponents();

			pMainSN->m_base.setPos(pEvt->m_pos);
			pMainSN->m_base.setU(pEvt->m_u);
			pMainSN->m_base.setV(pEvt->m_v);
			pMainSN->m_base.setN(pEvt->m_n);

			m_base.setPos(pEvt->m_pos);
			m_base.setU(pEvt->m_u);
			m_base.setV(pEvt->m_v);
			m_base.setN(pEvt->m_n);
			
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

			{
				PE::Handle hTargetMovementSM("TargetNPCMovementSM", sizeof(TargetNPCMovementSM));
				TargetNPCMovementSM *pTargetMovementSM = new(hTargetMovementSM) TargetNPCMovementSM(*m_pContext, m_arena, hTargetMovementSM);
				movementSM = pTargetMovementSM;
				pTargetMovementSM->addDefaultComponents();
				pTargetMovementSM->targetNPC = this;
				addComponent(hTargetMovementSM);

				PE::Handle hTargetBheaviorSM("TargetNPCBehaviorSM", sizeof(TargetNPCBehaviorSM));
				TargetNPCBehaviorSM *pTargetBehaviorSM = new(hTargetBheaviorSM) TargetNPCBehaviorSM(*m_pContext, m_arena, hTargetBheaviorSM, hTargetMovementSM);
				behaviorSM = pTargetBehaviorSM;
				pTargetBehaviorSM->targetNPC = this;
				pTargetBehaviorSM->addDefaultComponents();
				addComponent(hTargetBheaviorSM);

				StringOps::writeToString(pEvt->m_patrolWayPoint, pTargetBehaviorSM->m_curPatrolWayPoint, 32);
				pTargetBehaviorSM->m_havePatrolWayPoint = StringOps::length(pTargetBehaviorSM->m_curPatrolWayPoint) > 0;
				teamID = pEvt->m_teamID;

				// start the Target
				pTargetBehaviorSM->start();
			}

			PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
			MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);
			meshInstance = pMeshInstance;
			pMeshInstance->addDefaultComponents();
			pMeshInstance->initFromFile(pEvt->m_meshFilename, pEvt->m_package, pEvt->m_threadOwnershipMask);
			pMainSN->addComponent(hMeshInstance);
			m_pContext->getGPUScreen()->ReleaseRenderContextOwnership(pEvt->m_threadOwnershipMask);

			// CharacterControl::Components::ClientGameObjectManagerAddon::targetableEntities.push_back(this);
			//CharacterControl::Components::ClientGameObjectManagerAddon::attackableEntities.push_back(this);
		}

		void TargetNPC::addDefaultComponents()
		{
			Component::addDefaultComponents();
		}
	}; 
}; 
