#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/DebugRenderer.h"
#include "../ClientGameObjectManagerAddon.h"
#include "../CharacterControlContext.h"
#include "SoldierNPCMovementSM.h"
#include "SoldierNPCAnimationSM.h"
#include "SoldierNPCBehaviorSM.h"
#include "SoldierNPC.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Render/IRenderer.h"
#include <vector>
#include <chrono>

#ifndef TIMENOW 
#define TIMENOW (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
#endif

using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;

namespace CharacterControl
{

	namespace Events
	{
		PE_IMPLEMENT_CLASS1(SoldierNPCBehaviorSM_Event_ACQUIRE_TARGET, Event);
	}

	namespace Components
	{
		PE_IMPLEMENT_CLASS1(SoldierNPCBehaviorSM, Component);

		SoldierNPCBehaviorSM::SoldierNPCBehaviorSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, PE::Handle hMovementSM)
			: Component(context, arena, hMyself), m_hMovementSM(hMovementSM)
		{

		}

		void SoldierNPCBehaviorSM::start()
		{
			if (m_havePatrolWayPoint)
			{
				m_state = WAITING_FOR_WAYPOINT; // will update on next do_UPDATE()
			}
			else
			{
				m_state = IDLE; // stand in place

				PE::Handle h("SoldierNPCMovementSM_Event_STOP", sizeof(SoldierNPCMovementSM_Event_STOP));
				SoldierNPCMovementSM_Event_STOP *pEvt = new(h) SoldierNPCMovementSM_Event_STOP();

				m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
				// release memory now that event is processed
				h.release();

			}
		}

		void SoldierNPCBehaviorSM::addDefaultComponents()
		{
			Component::addDefaultComponents();

			PE_REGISTER_EVENT_HANDLER(SoldierNPCBehaviorSM_Event_ACQUIRE_TARGET, SoldierNPCBehaviorSM::do_SoldierNPCBehaviorSM_ACQUIRE_TARGET);
			PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_TARGET_REACHED, SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED);
			PE_REGISTER_EVENT_HANDLER(Event_UPDATE, SoldierNPCBehaviorSM::do_UPDATE);

			PE_REGISTER_EVENT_HANDLER(Event_PRE_RENDER_needsRC, SoldierNPCBehaviorSM::do_PRE_RENDER_needsRC);
		}

		void SoldierNPCBehaviorSM::do_SoldierNPCBehaviorSM_ACQUIRE_TARGET(PE::Events::Event *pEvt)
		{
			PEINFO("SoldierNPCBehaviorSM:do_SoldierNPCBehaviorSM_Event_ACQUIRE_TARGET\n");

			this->m_state = HAS_TARGET;
			TargetNPC *targetNPC = (TargetNPC *)((SoldierNPCBehaviorSM_Event_ACQUIRE_TARGET *)(pEvt))->target;
			if (targetNPC && targetNPC->health > 0.0f)
			{
				this->targetNPC = (TargetNPC *)((SoldierNPCBehaviorSM_Event_ACQUIRE_TARGET *)(pEvt))->target;
			}

			PE::Handle h("SoldierNPCMovementSM_Event_MOVE_TO", sizeof(SoldierNPCMovementSM_Event_MOVE_TO));
			Events::SoldierNPCMovementSM_Event_MOVE_TO *pEvts = new(h) SoldierNPCMovementSM_Event_MOVE_TO(targetNPC->m_base.getPos());
			pEvts->m_running = true;
			m_hMovementSM.getObject<Component>()->handleEvent(pEvts);
			h.release();
		}

		void SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED(PE::Events::Event *pEvt)
		{
			PEINFO("SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED\n");

			if (m_state == PATROLLING_WAYPOINTS || m_state == HAS_TARGET || soldierNPC->movementSM->m_state == SoldierNPCMovementSM::RUNNING_TO_TARGET)
			{
				ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
				if (pGameObjectManagerAddon)
				{
					// search for waypoint object
					//WayPoint *pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
					//if (!pWP)
					//{
					//	m_state = IDLE;
					//	return;
					//}

					++soldierNPC->movementSM->pathIterator;
					if (soldierNPC->movementSM->pathLen == soldierNPC->movementSM->pathIterator)
					{
						m_state = IDLE;
						return;
					}

					//int nextWaypointIndex = pWP->m_numberOfWaypoints > 0 ? (rand() % pWP->m_numberOfWaypoints) : -1;

					//if (pWP && nextWaypointIndex != -1)
					//{
					//	// have next waypoint to go to
					//	pWP = pGameObjectManagerAddon->getWayPoint(pWP->m_nextWayPointNames[nextWaypointIndex]);
					//	if (pWP)
					//	{
					//		StringOps::writeToString(pWP->m_name, m_curPatrolWayPoint, 32);

					//		m_state = PATROLLING_WAYPOINTS;
					//		PE::Handle h("SoldierNPCMovementSM_Event_MOVE_TO", sizeof(SoldierNPCMovementSM_Event_MOVE_TO));
					//		Events::SoldierNPCMovementSM_Event_MOVE_TO *pEvt = new(h) SoldierNPCMovementSM_Event_MOVE_TO(pWP->m_base.getPos());
					//		pEvt->m_running = pWP->m_needToRunToThisWaypoint;
					//		m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
					//		// release memory now that event is processed
					//		h.release();
					//	}
					//}

					soldierNPC->movementSM->m_targetPostion = soldierNPC->movementSM->path[soldierNPC->movementSM->pathIterator];

					/*else
					{
						m_state = IDLE;
					}*/
					//else
					//{
					//	ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
					//	if (pGameObjectManagerAddon)
					//	{
					//		TargetNPC *targetNPC = pGameObjectManagerAddon->getTarget();
					//		m_state = targetNPC ? SHOOTING_TARGET : IDLE;

					//		if (m_state == SHOOTING_TARGET)
					//		{
					//			PE::Handle h("SoldierNPCMovementSM_Event_SHOOTING_TARGET", sizeof(SoldierNPCMovementSM_Event_SHOOTING_TARGET));
					//			Events::SoldierNPCMovementSM_Event_SHOOTING_TARGET *pEvt = new(h) SoldierNPCMovementSM_Event_SHOOTING_TARGET(targetNPC);
					//			m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
					//		}
					//	}
					//	else
					//	{
					//		m_state = IDLE;
					//	}
					//	// no need to send the event. movement state machine will automatically send event to animation state machine to play idle animation
					//}
				}
			}
		}

		// this event is executed when thread has RC
		void SoldierNPCBehaviorSM::do_PRE_RENDER_needsRC(PE::Events::Event *pEvt)
		{
			Event_PRE_RENDER_needsRC *pRealEvent = (Event_PRE_RENDER_needsRC *)(pEvt);
			//if (soldierNPC->movementSM->m_state == SoldierNPCMovementSM::RUNNING_TO_TARGET)
			//{
			//	Vector3 targetPos = soldierNPC->movementSM->m_targetPostion;
			//	SceneNode *pSN = soldierNPC->movementSM->getParentsSceneNode();
			//	Vector3 curPos = pSN->m_base.getPos();
			//	std::vector<Vector3> path = Navigator::findPath(Vector3(curPos.m_x, 0.0f, curPos.m_z), targetPos);
			//	for (int i = 0; i < path.size() - 1; i++)
			//	{
			//		float posData[12] = { path[i].m_x, path[i].m_y, path[i].m_z, 1, 1, 0, path[i + 1].m_x, path[i + 1].m_y, path[i + 1].m_z, 1, 1, 0 };
			//		DebugRenderer::Instance()->createLineMesh(false, Matrix4x4(), posData, 2, 1, 1);
			//	}


			//}

			if (ClientGameObjectManagerAddon::gameOver)
			{
				char buf[256];
				sprintf(PEString::s_buf, "Team %d Lost!", ClientGameObjectManagerAddon::losingTeam);
				DebugRenderer::Instance()->createTextMesh(
					PEString::s_buf, true, false, false, false, 200.0f,
					Vector3(0.5f, 0.5f, 0), 1.0f, pRealEvent->m_threadOwnershipMask);
				ClientGameObjectManagerAddon::gameOver = false;
			}

			if (m_havePatrolWayPoint)
			{
				char buf[80];
				sprintf(buf, "Patrol Waypoint: %s", m_curPatrolWayPoint);
				SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
				PE::Handle hSoldierSceneNode = pSol->getFirstComponentHandle<PE::Components::SceneNode>();
				Matrix4x4 base = hSoldierSceneNode.getObject<PE::Components::SceneNode>()->m_worldTransform;

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

		void SoldierNPCBehaviorSM::do_UPDATE(PE::Events::Event *pEvt)
		{
			Event_UPDATE *pRealEvt = (Event_UPDATE *)(pEvt);
			this->soldierNPC->UpdateHearts();

			uint64_t timeNow = TIMENOW;
			if (this->soldierNPC->meshInstance->dead && timeNow - soldierNPC->hitTime > 100)
			{
				this->soldierNPC->meshInstance->dead = false;
			}
		}
	}
}




