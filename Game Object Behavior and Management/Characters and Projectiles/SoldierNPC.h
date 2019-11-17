#ifndef _CHARACTER_CONTROL_SOLDIER_NPC_
#define _CHARACTER_CONTROL_SOLDIER_NPC_

#include "PrimeEngine/Events/Component.h"
#include "../Events/Events.h"
//#include "SoldierNPCAnimationSM.h"
#include "SoldierNPCBehaviorSM.h"
#include "SoldierNPCMovementSM.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/SceneNode.h"

namespace CharacterControl
{
	namespace Components
	{
		struct SoldierNPC : public PE::Components::Component
		{
			PE_DECLARE_CLASS(SoldierNPC);

			SoldierNPC(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, Events::Event_CreateSoldierNPC *pEvt);

			class SoldierNPCBehaviorSM *behaviorSM;
			class SoldierNPCMovementSM *movementSM;
			class SoldierNPCAnimationSM *animationSM;
			float health = 5.0f;
			float defaultHealth = 5.0f;
			int teamID;
			PE::Components::MeshInstance *meshInstance;
			//Matrix4x4 m_base;
			PE::Components::SceneNode *sceneNode;
			bool selected = false;
			uint64_t hitTime;

			virtual void addDefaultComponents();

			//Todo(Rohan) : proper level reloading
			Matrix4x4 defaultBase;

			//Heart 
			float heartOffsetX = .5f;
			float heartOffsetY = 0.0f;
			float heartOffsetZ = .5f;
			float middleHeartOffsetX = -0.1f;
			float middleHeartOffsetY = 3.0f;
			float middleHeartOffsetZ = 0.1f;
			std::vector<PE::Components::MeshInstance*> heartMIs;
			std::vector<PE::Components::SceneNode*> heartSNs;
			void InitHearts(int& threadOwnershipMas);
			void FixHeartsPos();
			void UpdateHearts();
		};
	}; // namespace Components
}; // namespace CharacterControl
#endif

