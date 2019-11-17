#pragma once


#include "PrimeEngine/Events/Component.h"
#include "CharacterControl/Characters/TargetNPC.h"
//#include "CharacterControl/Characters/SoldierNPC.h"
#include "../Events/Events.h"

namespace CharacterControl
{
	namespace Events
	{
		struct SoldierNPCBehaviorSM_Event_ACQUIRE_TARGET : public PE::Events::Event
		{
			PE_DECLARE_CLASS(SoldierNPCBehaviorSM_Event_ACQUIRE_TARGET);

			SoldierNPCBehaviorSM_Event_ACQUIRE_TARGET(PE::Components::Component *target)
			{
				CharacterControl::Components::TargetNPC *targetNPC = (CharacterControl::Components::TargetNPC *)target;
				if (targetNPC->health > 0.0f)
				{
					this->target = target;
				}
				else	
				{
					this->target = NULL;
				}
			}

			PE::Components::Component *target;
		};
	};

	namespace Components
	{
		

		// movement state machine talks to associated animation state machine
		struct SoldierNPCBehaviorSM : public PE::Components::Component
		{
			PE_DECLARE_CLASS(SoldierNPCBehaviorSM);

			enum States
			{
				IDLE, // stand in place
				WAITING_FOR_WAYPOINT, // have a name of waypoint to go to, but it has not been loaded yet
				PATROLLING_WAYPOINTS,
				SHOOTING_TARGET,
				HAS_TARGET
			};


			SoldierNPCBehaviorSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, PE::Handle hMovementSM);

			void start();


			//////////////////////////////////////////////////////////////////////////
			// Component API and Event handlers
			//////////////////////////////////////////////////////////////////////////
			//
			virtual void addDefaultComponents();
			//
			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_SoldierNPCMovementSM_Event_TARGET_REACHED)
				virtual void do_SoldierNPCMovementSM_Event_TARGET_REACHED(PE::Events::Event *pEvt);
			//
			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
				virtual void do_UPDATE(PE::Events::Event *pEvt);

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PRE_RENDER_needsRC)
				void do_PRE_RENDER_needsRC(PE::Events::Event *pEvt);

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_SoldierNPCBehaviorSM_ACQUIRE_TARGET);
			virtual void do_SoldierNPCBehaviorSM_ACQUIRE_TARGET(PE::Events::Event *pEvt);

			PE::Handle m_hMovementSM;

			// TODO(Rohan): use component
			TargetNPC *targetNPC;
			class SoldierNPC *soldierNPC;
			float attackRange = 5.0f;
			bool m_havePatrolWayPoint;
			char m_curPatrolWayPoint[32];
			States m_state;
		};

	};
};


