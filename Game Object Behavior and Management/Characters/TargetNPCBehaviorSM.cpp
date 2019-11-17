#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/DebugRenderer.h"
#include "../ClientGameObjectManagerAddon.h"
#include "../CharacterControlContext.h"
#include "TargetNPCMovementSM.h"
#include "TargetNPCBehaviorSM.h"
#include "TargetNPC.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Render/IRenderer.h"

using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;

namespace CharacterControl
{
	namespace Components
	{
		PE_IMPLEMENT_CLASS1(TargetNPCBehaviorSM, Component);

		TargetNPCBehaviorSM::TargetNPCBehaviorSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, PE::Handle hMovementSM)
			: Component(context, arena, hMyself), m_hMovementSM(hMovementSM)
		{

		}

		void TargetNPCBehaviorSM::start()
		{
			if (m_havePatrolWayPoint)
			{
				m_state = WAITING_FOR_WAYPOINT; // will update on next do_UPDATE()
			}
			else
			{
				m_state = IDLE; // stand in place
			}
		}

		void TargetNPCBehaviorSM::addDefaultComponents()
		{
			Component::addDefaultComponents();

			PE_REGISTER_EVENT_HANDLER(TargetNPCMovementSM_Event_TARGET_REACHED, TargetNPCBehaviorSM::do_TargetNPCMovementSM_Event_TARGET_REACHED);
			PE_REGISTER_EVENT_HANDLER(Event_UPDATE, TargetNPCBehaviorSM::do_UPDATE);
			PE_REGISTER_EVENT_HANDLER(Event_PRE_RENDER_needsRC, TargetNPCBehaviorSM::do_PRE_RENDER_needsRC);
		}

		void TargetNPCBehaviorSM::do_TargetNPCMovementSM_Event_TARGET_REACHED(PE::Events::Event *pEvt)
		{
			PEINFO("TargetNPCBehaviorSM::do_TargetNPCMovementSM_Event_TARGET_REACHED\n");

			if (m_state == PATROLLING_WAYPOINTS)
			{
				ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
				if (pGameObjectManagerAddon)
				{
					// search for waypoint object
					WayPoint *pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
					int nextWaypointIndex = pWP->m_numberOfWaypoints > 0 ? (rand() % pWP->m_numberOfWaypoints) : -1;

					if (pWP && nextWaypointIndex != -1)
					{
						// have next waypoint to go to
						pWP = pGameObjectManagerAddon->getWayPoint(pWP->m_nextWayPointNames[nextWaypointIndex]);
						if (pWP)
						{
							StringOps::writeToString(pWP->m_name, m_curPatrolWayPoint, 32);

							m_state = PATROLLING_WAYPOINTS;
							PE::Handle h("TargetNPCMovementSM_Event_MOVE_TO", sizeof(TargetNPCMovementSM_Event_MOVE_TO));
							Events::TargetNPCMovementSM_Event_MOVE_TO *pEvt = new(h) TargetNPCMovementSM_Event_MOVE_TO(pWP->m_base.getPos());
							pEvt->m_running = pWP->m_needToRunToThisWaypoint;
							m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
							// release memory now that event is processed
							h.release();
						}
					}
					else
					{
						m_state = IDLE;
					}
				}
			}
		}

		// this event is executed when thread has RC
		void TargetNPCBehaviorSM::do_PRE_RENDER_needsRC(PE::Events::Event *pEvt)
		{
			Event_PRE_RENDER_needsRC *pRealEvent = (Event_PRE_RENDER_needsRC *)(pEvt);
			if (m_havePatrolWayPoint)
			{
				char buf[80];
				sprintf(buf, "Patrol Waypoint: %s", m_curPatrolWayPoint);
				TargetNPC *pSol = getFirstParentByTypePtr<TargetNPC>();
				PE::Handle hTargetSceneNode = pSol->getFirstComponentHandle<PE::Components::SceneNode>();
				Matrix4x4 base = hTargetSceneNode.getObject<PE::Components::SceneNode>()->m_worldTransform;

				DebugRenderer::Instance()->createTextMesh(
					buf, false, false, true, false, 0,
					base.getPos(), 0.01f, pRealEvent->m_threadOwnershipMask);

				{
					//we can also construct points ourself
					bool sent = false;
					ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
					if (pGameObjectManagerAddon)
					{
						WayPoint *pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
						if (pWP)
						{
							Vector3 target = pWP->m_base.getPos();
							Vector3 pos = base.getPos();
							Vector3 color(1.0f, 1.0f, 0);
							Vector3 linepts[] = { pos, color, target, color };

							DebugRenderer::Instance()->createLineMesh(true, base, &linepts[0].m_x, 2, 0);// send event while the array is on the stack
							sent = true;
						}
					}
					if (!sent) // if for whatever reason we didnt retrieve waypoint info, send the event with transform only
						DebugRenderer::Instance()->createLineMesh(true, base, NULL, 0, 0);// send event while the array is on the stack
				}
			}
		}

		void TargetNPCBehaviorSM::do_UPDATE(PE::Events::Event *pEvt)
		{
			if (m_state == WAITING_FOR_WAYPOINT)
			{
				if (m_havePatrolWayPoint)
				{
					ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
					if (pGameObjectManagerAddon)
					{
						// search for waypoint object
						WayPoint *pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
						if (pWP)
						{
							m_state = PATROLLING_WAYPOINTS;
							PE::Handle h("TargetNPCMovementSM_Event_MOVE_TO", sizeof(TargetNPCMovementSM_Event_MOVE_TO));
							Events::TargetNPCMovementSM_Event_MOVE_TO *pEvt = new(h) TargetNPCMovementSM_Event_MOVE_TO(pWP->m_base.getPos());

							m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
							// release memory now that event is processed
							h.release();
						}
					}
				}
				else
				{
					// should not happen, but in any case, set state to idle
					m_state = IDLE;
				}
			}
		}


	}
}



