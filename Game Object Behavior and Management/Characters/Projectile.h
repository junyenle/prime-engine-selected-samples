#pragma once
#include <vector>
#include "PrimeEngine/Events/Component.h"
#include "../Events/Events.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/SceneNode.h"

namespace CharacterControl
{
	namespace Components
	{
		struct Projectile : public PE::Components::Component
		{
			PE_DECLARE_CLASS(Projectile);

			Projectile(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself);

			static void EnableProjectile(Vector3 startPos, Vector3 targetPos, int teamID) 
			{
				for (Projectile *projectile : projectiles)
				{
					if (!projectile->isAlive)
					{
						startPos.m_y += 0.8f;
						targetPos.m_y += 0.8f;
						projectile->startPosition = startPos;
						projectile->targetPosition = targetPos;
						projectile->teamID = teamID;
						projectile->sceneNode->m_base.setPos(startPos);
						//projectile->m_base.setPos(startPos);
						projectile->isAlive = true;
						projectile->setEnabled(true);
						projectile->showTimer = 0.1f;
						projectile->meshInstance->lifeProgress = 0.0f;
						return;
					}
				}
			}
			
			static void EnableProjectileWithIndex(int index) 
			{
				for (Projectile *projectile : projectiles)
				{
					if (!projectile->componentID == index)
					{
						projectile->disable();
						projectile->isAlive = true;
						projectile->setEnabled(true);
						projectile->showTimer = 0.1f;
						projectile->meshInstance->lifeProgress = 0.0f;
						return;
					}
				}
			}

			void disable()
			{
				isAlive = false;
				setEnabled(false);
				meshInstance->m_display = false;
				meshInstance->lifeProgress = 0.0f;
				showTimer = 0.1f;
			}

			virtual void addDefaultComponents();

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
			virtual void do_UPDATE(PE::Events::Event *pEvt);

			static std::vector<struct Projectile *> projectiles;
			PE::Components::SceneNode *sceneNode;
			//Matrix4x4 m_base;
			Vector3 targetPosition;
			Vector3 startPosition;
			float showTimer;
			int teamID;

			bool isAlive = false;
			PE::Components::MeshInstance *meshInstance;
		};
	}
}








