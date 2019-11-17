#ifndef _PE_Target_NPC_MOVEMENT_SM_H_
#define _PE_Target_NPC_MOVEMENT_SM_H_

#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "../Events/Events.h"

namespace CharacterControl
{
	namespace Events
	{
		struct TargetNPCMovementSM_Event_MOVE_TO : public PE::Events::Event
		{
			PE_DECLARE_CLASS(TargetNPCMovementSM_Event_MOVE_TO);
			TargetNPCMovementSM_Event_MOVE_TO(Vector3 targetPos = Vector3());

			Vector3 m_targetPosition;
			bool m_running;
		};

		// sent by this state machine to its components. probably to behavior state machine
		struct TargetNPCMovementSM_Event_TARGET_REACHED : public PE::Events::Event
		{
			PE_DECLARE_CLASS(TargetNPCMovementSM_Event_TARGET_REACHED);

			TargetNPCMovementSM_Event_TARGET_REACHED()
			{
			}
		};

	};
	namespace Components
	{

		// movement state machine talks to associated animation state machine
		struct TargetNPCMovementSM : public PE::Components::Component
		{
			PE_DECLARE_CLASS(TargetNPCMovementSM);

			enum States
			{
				STANDING,
				RUNNING_TO_TARGET,
				WALKING_TO_TARGET,
			};


			TargetNPCMovementSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself);

			//////////////////////////////////////////////////////////////////////////
			// utility
			//////////////////////////////////////////////////////////////////////////
			PE::Components::SceneNode *getParentsSceneNode();

			//////////////////////////////////////////////////////////////////////////
			// Component API and Event Handlers
			//////////////////////////////////////////////////////////////////////////
			virtual void addDefaultComponents();

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_TargetNPCMovementSM_Event_MOVE_TO)
			virtual void do_TargetNPCMovementSM_Event_MOVE_TO(PE::Events::Event *pEvt);

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
			virtual void do_UPDATE(PE::Events::Event *pEvt);

			//////////////////////////////////////////////////////////////////////////
			// Member Variables
			//////////////////////////////////////////////////////////////////////////
			PE::Handle m_hAnimationSM;
			class TargetNPC *targetNPC;
			Vector3 m_targetPostion;
			States m_state;
		};
	};
};


#endif
