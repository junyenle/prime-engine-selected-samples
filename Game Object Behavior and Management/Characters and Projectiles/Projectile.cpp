#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/RootSceneNode.h"

#include "Projectile.h"
#include "SoldierNPC.h"
#include "CharacterControl/ClientGameObjectManagerAddon.h"
#include "PrimeEngine/Events/StandardEvents.h"

using namespace PE;
using namespace PE::Components;
using namespace CharacterControl::Events;

namespace CharacterControl
{
	namespace Components
	{
		std::vector<Projectile *> Projectile::projectiles;
		PE_IMPLEMENT_CLASS1(Projectile, Component);
		Projectile::Projectile(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself)
			: Component(context, arena, hMyself)
		{
			projectiles.push_back(this);
			showTimer = 0.1f;
		}

		void Projectile::addDefaultComponents()
		{
			Component::addDefaultComponents();
			PE_REGISTER_EVENT_HANDLER(PE::Events::Event_UPDATE, Projectile::do_UPDATE);
		}


		void Projectile::do_UPDATE(PE::Events::Event *pEvt)
		{
			if (isAlive)
			{
				PE::Events::Event_UPDATE *realEvt = (PE::Events::Event_UPDATE *)pEvt;
				//if (!meshInstance->m_display)
				//{
				//	showTimer -= realEvt->m_frameTime;

				//	if (showTimer <= 0.0f)
				//	{
				//		meshInstance->m_display = true;
				//	}
				//}
				meshInstance->m_display = true;

				startPosition = sceneNode->m_base.getPos();

				float dotTargetStart = (targetPosition - startPosition).dotProduct(targetPosition - sceneNode->m_base.getPos());
				//if (dotTargetStart < 0.0f)
				//{
				//	disable();
				//	return;
				//}
				//else
				//{
				Vector3 forwardVector = (targetPosition - startPosition);
				forwardVector.normalize();
				float velocity = 12.0f;
				sceneNode->m_base.setPos(sceneNode->m_base.getPos() + velocity * forwardVector * realEvt->m_frameTime);
				//m_base = sceneNode->m_base;
				//meshInstance->lifeProgress = 1 - (targetPosition - sceneNode->m_base.getPos()).length() / (targetPosition - startPosition).length();
			//}

			//	for (Component *currentEntity : ClientGameObjectManagerAddon::attackableEntities)
			//	{
			//		Component *target;
			//		SceneNode *currentSN = NULL;
			//		PE::Handle *pHC = currentEntity->m_components.getFirstPtr();

			//		for (PrimitiveTypes::UInt32 i = 0; i < currentEntity->m_components.m_size; i++, pHC++)
			//		{
			//			Component *pC = (*pHC).getObject<Component>();
			//			if (pC->isInstanceOf<SceneNode>())
			//			{
			//				currentSN = (SceneNode *)(pC);
			//				target = currentEntity;
			//				break;
			//			}
			//		}

			//		Vector3 targetPos = currentSN->m_base.getPos();
			//		Vector3 bulletPos = sceneNode->m_base.getPos();
			//		float length = (bulletPos - targetPos).length();
			//		if (length <= 1.5f)
			//		{
			//			SoldierNPC *targetNPC = (SoldierNPC *)target;
			//			if (targetNPC->teamID == this->teamID)
			//			{
			//				continue;
			//			}
			//			targetNPC->health -= 1;
			//			disable();

			//			if (targetNPC->health <= 0.0f)
			//			{
			//				targetNPC->meshInstance->dead = true;
			//				ClientGameObjectManagerAddon::losingTeam = targetNPC->teamID;
			//				ClientGameObjectManagerAddon::gameOver = true;
			//			}
			//		}
				//}

			//using namespace Navigator;
			//initNodes();
			//// TODO(Rohan): this should proably use define from other file
			//extern Poly staticPolys[TOTAL_POLYS];
			//for (Poly poly : staticPolys)
			//{
			//	Vector3 projectilePos = sceneNode->m_base.getPos();
			//	projectilePos.m_y = 0;
			//	if (isInsidePoly(poly, projectilePos))
			//	{
			//		disable();
			//	}
			//}
			}
		}
	}
};