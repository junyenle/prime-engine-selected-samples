#pragma once
#include <vector> 
#include "PrimeEngine/Math/Vector3.h"

#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Utils/Array/Array.h"
#include "PrimeEngine/Scene/RootSceneNode.h"
#include "PrimeEngine/Scene/CameraManager.h"

#include "PrimeEngine/Utils/Networkable.h"

// Sibling/Children includes
namespace PE
{
	namespace Components
	{
		// public Component?
		struct Ghost : public PEClass, public Networkable
		{
			Ghost(const Ghost &g2)
				: Networkable(*g2.m_context, this, Networkable::s_NetworkId_Invalid)
			{
				switch (g2.type)
				{
					case SoldierNPC:
					{
						this->type = SoldierNPC;
						this->originalComponentID = g2.originalComponentID;

						this->soldierData.isMoving = g2.soldierData.isMoving;
						this->soldierData.animationState = g2.soldierData.animationState;

						this->soldierData.desiredPosition = g2.soldierData.desiredPosition;
						this->soldierData.actualPosition = g2.soldierData.actualPosition;
						this->soldierData.uBasis = g2.soldierData.uBasis;
						this->soldierData.vBasis = g2.soldierData.vBasis;
						this->soldierData.nBasis = g2.soldierData.nBasis;
						this->soldierData.health = g2.soldierData.health;
						this->soldierData.isLastLeg = g2.soldierData.isLastLeg;
						this->soldierData.pathIterator = g2.soldierData.pathIterator;

						this->soldierData.pathLen = 0;
						if (g2.soldierData.pathLen > 0)
						{
							for (int i = 0; i < g2.soldierData.pathLen; ++i)
							{
								this->soldierData.path[this->soldierData.pathLen++] = g2.soldierData.path[i];
							}
						}

					} break;

					case Projectile:
					{
						this->type = Projectile;
						this->originalComponentID = g2.originalComponentID;
						this->projectileData.desiredPosition = g2.projectileData.desiredPosition;
						this->projectileData.actualPosition = g2.projectileData.actualPosition;
						this->projectileData.startPosition = g2.projectileData.startPosition;
						this->projectileData.isAlive = g2.projectileData.isAlive;
						this->projectileData.lifeProgress = g2.projectileData.lifeProgress;
						this->projectileData.teamID = g2.projectileData.teamID;
					} break;

					default:
					{

					} break;
				}
			}

			enum GhostType
			{
				SoldierNPC,
				Projectile
			};

			PE_DECLARE_CLASS(Ghost);
			PE_DECLARE_NETWORKABLE_CLASS

			Ghost(PE::GameContext &context);
			virtual ~Ghost() {}

			// Component ------------------------------------------------------------
			//virtual void addDefaultComponents();

			GhostType type;
			int originalComponentID;
			uint64_t lastTimeUpdatedClient = -1;
			uint64_t lastTimeUpdatedServer = -1;
			uint64_t rateLimitTimer = -1;
			bool needsToResim = false;
			uint64_t resimTime;
			int lastUpdatedServerIndex = 0;
			int lastUpdatedClientIndex = -1;

			union
			{
				struct
				{
					int isMoving;
					int animationState;
					Vector3 desiredPosition;
					Vector3 actualPosition;
					Vector3 uBasis;
					Vector3 vBasis;
					Vector3 nBasis;
					float health;
					int isLastLeg;
					Vector3 path[16];
					int pathLen = 0;
					int pathIterator;
					bool turningCorner = false;
					Vector3 lastPos = Vector3(0.0f, 0.0f, 0.0f);
				} soldierData;

				struct
				{
					Vector3 desiredPosition;
					Vector3 actualPosition;
					Vector3 startPosition;
					int isAlive;
					float lifeProgress;
					int teamID;
				} projectileData;
			};

			void update(float deltaTime);

			uint64_t changed[2] = { 0, 0 }; // last sent time 
			bool shouldSend[2] = { false, false }; // send right now (imperative)
			bool didUpdate[2] = { false, false }; // did anything even happen?
			bool dead[2] = { false, false };
			//bool changed = false;
			Component *pComponent;
			GameContext *m_context;
			bool possibleDesync[2] = { false, false };
			bool wasNudged[2] = { false, false };
		};
	}; // namespace Components
}; // namespace PE
