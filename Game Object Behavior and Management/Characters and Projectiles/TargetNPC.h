#pragma once

#include "PrimeEngine/Events/Component.h"
#include "../Events/Events.h"
//#include "TargetNPCBehaviorSM.h"
//#include "TargetNPCMovementSM.h"
#include "PrimeEngine/Scene/MeshInstance.h"

namespace CharacterControl
{
	namespace Components
	{
		struct TargetNPC : public PE::Components::Component
		{
			PE_DECLARE_CLASS(TargetNPC);

			TargetNPC(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, Events::Event_CreateTargetNPC *pEvt);

			virtual void addDefaultComponents();

			Matrix4x4 m_base;
			class TargetNPCMovementSM *movementSM;
			class TargetNPCBehaviorSM *behaviorSM;

			float health = 5.0f;
			int teamID;
			int targetID;

			PE::Components::MeshInstance *meshInstance;

			// hacky defaults
			float defaultHealth = 5.0f;
			Matrix4x4 defaultBase;
			
		};
	}; // namespace Components
}; // namespace CharacterControl
