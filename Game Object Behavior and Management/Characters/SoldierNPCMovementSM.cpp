#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "SoldierNPCMovementSM.h"
#include "SoldierNPCAnimationSM.h"
#include "SoldierNPC.h"
#include "TargetNPC.h"
#include "CharacterControl/ClientGameObjectManagerAddon.h"
#include "PrimeEngine/Networking/Tribes/GhostManager/Ghost.h"

using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;

namespace CharacterControl
{

	// Events sent by behavior state machine (or other high level state machines)
	// these are events that specify where a soldier should move
	namespace Events
	{

		PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_MOVE_TO, Event);

		SoldierNPCMovementSM_Event_MOVE_TO::SoldierNPCMovementSM_Event_MOVE_TO(Vector3 targetPos)
			: m_targetPosition(targetPos), m_running(false)
		{
		}

		PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_SHOOTING_TARGET, Event);

		SoldierNPCMovementSM_Event_SHOOTING_TARGET::SoldierNPCMovementSM_Event_SHOOTING_TARGET(CharacterControl::Components::TargetNPC *targetNPC)
			: m_targetNPC(targetNPC)
		{
		}

		PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_STOP, Event);

		PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_TARGET_REACHED, Event);
	}

	namespace Components
	{

		PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM, Component);


		SoldierNPCMovementSM::SoldierNPCMovementSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself)
			: Component(context, arena, hMyself)
			, m_state(STANDING)
		{
		}

		SceneNode *SoldierNPCMovementSM::getParentsSceneNode()
		{
			PE::Handle hParent = getFirstParentByType<Component>();
			if (hParent.isValid())
			{
				// see if parent has scene node component
				return hParent.getObject<Component>()->getFirstComponent<SceneNode>();

			}
			return NULL;
		}

		void SoldierNPCMovementSM::addDefaultComponents()
		{
			Component::addDefaultComponents();

			PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_MOVE_TO, SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_MOVE_TO);
			PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_SHOOTING_TARGET, SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_SHOOTING_TARGET);
			PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_STOP, SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_STOP);
			PE_REGISTER_EVENT_HANDLER(Event_UPDATE, SoldierNPCMovementSM::do_UPDATE);
		}

		void SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_SHOOTING_TARGET(PE::Events::Event *pEvt)
		{
			SoldierNPCMovementSM_Event_SHOOTING_TARGET *pRealEvt = (SoldierNPCMovementSM_Event_SHOOTING_TARGET *)(pEvt);

			m_state = SHOOTING_TARGET;
			PE::Handle h("SoldierNPCAnimSM_Event_SHOOT", sizeof(SoldierNPCAnimSM_Event_SHOOT));
			Events::SoldierNPCAnimSM_Event_SHOOT *pOutEvt = new(h) SoldierNPCAnimSM_Event_SHOOT();

			SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
			pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(pOutEvt);

			SceneNode *pSN = getParentsSceneNode();			
			m_targetNPC = pRealEvt->m_targetNPC;
			Vector3 targetDir = m_targetNPC->m_base.getPos() - pSN->m_base.getPos();
			
			pSN->m_base.turnInDirection(targetDir, 3.1415f);

			// release memory now that event is processed
			h.release();
		}
	

		void SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_MOVE_TO(PE::Events::Event *pEvt)
		{
			SoldierNPCMovementSM_Event_MOVE_TO *pRealEvt = (SoldierNPCMovementSM_Event_MOVE_TO *)(pEvt);
			m_targetPostion = pRealEvt->m_targetPosition;

			OutputDebugStringA("PE: PROGRESS: SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_MOVE_TO() : Recieved Event, Running; ");
			OutputDebugStringA(pRealEvt->m_running ? "True\n" : "False\n");

			//std::vector<Vector3> tempPath = Navigator::findPath(this->soldierNPC->sceneNode->m_base.getPos(), pRealEvt->m_targetPosition);
			//if (tempPath.size() == 0)
			//{
			//	return;
			//}

			//if (tempPath.size() > 0)
			//{
			//	memcpy(this->path, &tempPath[0], tempPath.size() * sizeof(tempPath[0]));
			//	this->pathIterator = 1;
			//	this->m_targetPostion = path[1];
			//}

			//this->path = tempPath;
			//this->pathLen = tempPath.size();

			// change state of this state machine
			//if (pRealEvt->m_running)
			//{
			//	m_state = RUNNING_TO_TARGET;
			//	
			//	// make sure the animations are playing
			//	PE::Handle h("SoldierNPCAnimSM_Event_RUN", sizeof(SoldierNPCAnimSM_Event_RUN));
			//	Events::SoldierNPCAnimSM_Event_RUN *pOutEvt = new(h) SoldierNPCAnimSM_Event_RUN();

			//	SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
			//	pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(pOutEvt);

			//	// release memory now that event is processed
			//	h.release();
			//}
			//else
			//{
			//	m_state = WALKING_TO_TARGET;
			//	
			//	// make sure the animations are playing
			//	PE::Handle h("SoldierNPCAnimSM_Event_WALK", sizeof(SoldierNPCAnimSM_Event_WALK));
			//	Events::SoldierNPCAnimSM_Event_WALK *pOutEvt = new(h) SoldierNPCAnimSM_Event_WALK();

			//	SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
			//	pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(pOutEvt);

			//	// release memory now that event is processed
			//	h.release();
			//}			
		}

		void SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_STOP(PE::Events::Event *pEvt)
		{
			Events::SoldierNPCAnimSM_Event_STOP Evt;

			SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
			pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(&Evt);
		}

		void SoldierNPCMovementSM::do_UPDATE(PE::Events::Event *pEvt)
		{
			return;
			PE::Events::Event_UPDATE *realEvt = (PE::Events::Event_UPDATE *)pEvt;
			if (soldierNPC->componentID == 1000 + CharacterControl::Components::ClientGameObjectManagerAddon::clientID)
			{
				return;
			}

			//if (!GO)
			//{
			//	GO = true;
			//	return;
			//}
			static Vector3 lastForward;

			if (isMoving)
			{
				std::vector<Vector3> corners; // find the dangerous corners
				corners.push_back(Vector3(18.1f, 0.0f, 11.3f));
				corners.push_back(Vector3(11.5f, 0.0f, 11.3f));
				corners.push_back(Vector3(18.1f, 0.0f, 13.4f));
				corners.push_back(Vector3(11.5f, 0.0f, 13.4f));


				for (int i = 0; i < corners.size(); i++)
				{
					if ((soldierNPC->sceneNode->m_base.getPos() - corners[i]).lengthSqr() < 0.1f) // if you're close to corner
					{
						return;
					}
				}

				Vector3 forward = m_targetPostion - soldierNPC->sceneNode->m_base.getPos();

				Vector3 lastWPtoDest = m_targetPostion - lastTargetPosition;
				bool didNotOvershoot = (forward.dotProduct(lastWPtoDest) >= 0);
				if (true)
				{
					soldierNPC->sceneNode->m_base.turnInDirection(forward, 3.14f);
				}

				if (forward.lengthSqr() < 0.01f)
				{
					return;
				}

				if (true)
				{
					forward.normalize();
					soldierNPC->sceneNode->m_base.setPos(soldierNPC->sceneNode->m_base.getPos() + 3.0f * realEvt->m_frameTime * forward);
				}
			}
		}
	}
}




