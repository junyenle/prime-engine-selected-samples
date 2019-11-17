#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "TargetNPCMovementSM.h"
#include "TargetNPC.h"

using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;

namespace CharacterControl
{
	namespace Events
	{
		PE_IMPLEMENT_CLASS1(TargetNPCMovementSM_Event_MOVE_TO, Event);

		TargetNPCMovementSM_Event_MOVE_TO::TargetNPCMovementSM_Event_MOVE_TO(Vector3 targetPos)
			: m_targetPosition(targetPos), m_running(false)
		{
		}

		PE_IMPLEMENT_CLASS1(TargetNPCMovementSM_Event_TARGET_REACHED, Event);
	}

	namespace Components
	{
		PE_IMPLEMENT_CLASS1(TargetNPCMovementSM, Component);

		TargetNPCMovementSM::TargetNPCMovementSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself)
			: Component(context, arena, hMyself)
			, m_state(STANDING)
		{
		}

		SceneNode *TargetNPCMovementSM::getParentsSceneNode()
		{
			PE::Handle hParent = getFirstParentByType<Component>();
			if (hParent.isValid())
			{
				// see if parent has scene node component
				return hParent.getObject<Component>()->getFirstComponent<SceneNode>();

			}
			return NULL;
		}

		void TargetNPCMovementSM::addDefaultComponents()
		{
			Component::addDefaultComponents();

			PE_REGISTER_EVENT_HANDLER(TargetNPCMovementSM_Event_MOVE_TO, TargetNPCMovementSM::do_TargetNPCMovementSM_Event_MOVE_TO);
			PE_REGISTER_EVENT_HANDLER(Event_UPDATE, TargetNPCMovementSM::do_UPDATE);
		}

		void TargetNPCMovementSM::do_TargetNPCMovementSM_Event_MOVE_TO(PE::Events::Event *pEvt)
		{
			TargetNPCMovementSM_Event_MOVE_TO *pRealEvt = (TargetNPCMovementSM_Event_MOVE_TO *)(pEvt);

			m_targetPostion = pRealEvt->m_targetPosition;

			OutputDebugStringA("PE: PROGRESS: TargetNPCMovementSM::do_TargetNPCMovementSM_Event_MOVE_TO() : Recieved Event, Running; ");
			OutputDebugStringA(pRealEvt->m_running ? "True\n" : "False\n");

			// change state of this state machine
			if (pRealEvt->m_running)
			{
				m_state = RUNNING_TO_TARGET;
			}
			else
			{
				m_state = WALKING_TO_TARGET;
			}			
		}

		void TargetNPCMovementSM::do_UPDATE(PE::Events::Event *pEvt)
		{
			if (m_state == WALKING_TO_TARGET || m_state == RUNNING_TO_TARGET)
			{
				SceneNode *pSN = getParentsSceneNode();
				if (pSN)
				{
					Vector3 curPos = pSN->m_base.getPos();
					float dsqr = (m_targetPostion - curPos).lengthSqr();

					bool reached = true;
					if (dsqr > 0.01f)
					{
						// not at the spot yet
						Event_UPDATE *pRealEvt = (Event_UPDATE *)(pEvt);
						float speed = m_state == WALKING_TO_TARGET ? 1.4f :  3.0f ;
						float allowedDisp = speed * pRealEvt->m_frameTime;

						Vector3 dir = (m_targetPostion - curPos);
						dir.normalize();
						float dist = sqrt(dsqr);
						if (dist > allowedDisp)
						{
							dist = allowedDisp; // can move up to allowedDisp
							reached = false; // not reaching destination yet
						}

						// instantaneous turn
						//pSN->m_base.turnInDirection(dir, 3.1415f);

						TargetNPC *pTarget = getFirstParentByTypePtr<TargetNPC>();

						Vector3 newPos = curPos + dir * dist;
						pTarget->m_base.setPos(newPos);
						pSN->m_base.setPos(newPos);
						targetNPC->m_base.setPos(newPos);
					}

					if (reached)
					{
						m_state = STANDING;

						// target has been reached. need to notify all same level state machines (components of parent)
						{
							PE::Handle h("TargetNPCMovementSM_Event_TARGET_REACHED", sizeof(TargetNPCMovementSM_Event_TARGET_REACHED));
							Events::TargetNPCMovementSM_Event_TARGET_REACHED *pOutEvt = new(h) TargetNPCMovementSM_Event_TARGET_REACHED();

							PE::Handle hParent = getFirstParentByType<Component>();
							if (hParent.isValid())
							{
								hParent.getObject<Component>()->handleEvent(pOutEvt);
							}

							// release memory now that event is processed
							h.release();
						}
					}
				}
			}
		}
	}
}



