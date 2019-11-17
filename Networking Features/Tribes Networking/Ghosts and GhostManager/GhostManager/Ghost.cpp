// APIAbstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
// Sibling/Children includes
#include "Ghost.h"
#include "PrimeEngine/Scene/Skeleton.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/SkeletonInstance.h"
#include "CharacterControl/Characters/SoldierNPCAnimationSM.h"
#include "CharacterControl/Characters/SoldierNPC.h"
#include "CharacterControl/Navigator.h"
#include "CharacterControl/ClientGameObjectManagerAddon.h"
#include "GhostManager.h"
#include "CharacterControl/Navigator.h"
#include "Ghost.h"
#include "CharacterControl/ServerGameObjectManagerAddon.h"
#include <chrono>

#ifndef TIMENOW 
#define TIMENOW (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
#endif

namespace PE
{
	namespace Components
	{
		// using namespace PE::Events;
		PE_IMPLEMENT_CLASS0(Ghost);

		//PE::MemoryArena arena, Handle hMyself
		Ghost::Ghost(PE::GameContext &context)
			: Networkable(context, this, Networkable::s_NetworkId_Invalid) // pre-assigned network id
		{
			this->m_context = &context;
		}

		void Ghost::update(float deltaTime)
		{
			switch (type)
			{
				case Ghost::SoldierNPC:
				{
					if (this->soldierData.health <= 0.0f)
					{
						this->soldierData.health = 5.0f;
						this->possibleDesync[0] = true;
						this->possibleDesync[1] = true;
						for (auto iterator = GhostManager::serverGhostMap.begin(); iterator != GhostManager::serverGhostMap.end(); iterator++)
						{
							PE::Components::Ghost *ghost2 = iterator->second;
							if (ghost2->type == Ghost::Projectile)
							{
								ghost2->projectileData.isAlive = false;
								ghost2->shouldSend[0] = true;
								ghost2->shouldSend[1] = true;
								ghost2->didUpdate[0] = true;
								ghost2->didUpdate[1] = true;
							}

							if (ghost2->type == Ghost::SoldierNPC)
							{
								ghost2->soldierData.actualPosition = ((CharacterControl::Components::SoldierNPC *)(CharacterControl::Components::ClientGameObjectManagerAddon::componentMap[ghost2->originalComponentID]))->defaultBase.getPos();
								ghost2->soldierData.uBasis = ((CharacterControl::Components::SoldierNPC *)(CharacterControl::Components::ClientGameObjectManagerAddon::componentMap[ghost2->originalComponentID]))->defaultBase.getU();
								ghost2->soldierData.vBasis = ((CharacterControl::Components::SoldierNPC *)(CharacterControl::Components::ClientGameObjectManagerAddon::componentMap[ghost2->originalComponentID]))->defaultBase.getV();
								ghost2->soldierData.nBasis = ((CharacterControl::Components::SoldierNPC *)(CharacterControl::Components::ClientGameObjectManagerAddon::componentMap[ghost2->originalComponentID]))->defaultBase.getN();
								ghost2->soldierData.isMoving = false;
								ghost2->soldierData.animationState = CharacterControl::Components::SoldierNPCAnimationSM::AnimId::STAND;
								ghost2->soldierData.health = 5.0f;
								ghost2->shouldSend[0] = true;
								ghost2->shouldSend[1] = true;
								ghost2->didUpdate[0] = true;
								ghost2->didUpdate[1] = true;
								ghost2->possibleDesync[0] = true;
								ghost2->possibleDesync[1] = true;
							}
						}
					}

					if (this->soldierData.isMoving)
					{
						Vector3 nextWP = this->soldierData.path[this->soldierData.pathIterator];
						Vector3 forward = nextWP - this->soldierData.actualPosition;
						float dsqr = forward.lengthSqr();
						forward.normalize();

						if (dsqr > 0.01f)
						{
							// not at the spot yet
							float speed = 3.0f;
							float allowedDisp = speed * deltaTime;

							float dist = sqrt(dsqr);
							if (dist > allowedDisp)
							{
								dist = allowedDisp; // can move up to allowedDisp
							}
							this->soldierData.actualPosition += forward * dist;

							Matrix4x4 base;
							base.setU(this->soldierData.uBasis);
							base.setV(this->soldierData.vBasis);
							base.setN(this->soldierData.nBasis);
							base.turnInDirection(forward, 3.1415f);
							this->soldierData.uBasis = base.getU();
							this->soldierData.vBasis = base.getV();
							this->soldierData.nBasis = base.getN();

						}
						else
						{
							// don't move
							this->soldierData.pathIterator++;
							if (this->soldierData.pathIterator == this->soldierData.pathLen)
							{
								this->soldierData.isMoving = false;
								this->soldierData.animationState = CharacterControl::Components::SoldierNPCAnimationSM::STAND;
							}

							this->shouldSend[0] = true;
							this->shouldSend[1] = true;
						}

						this->didUpdate[0] = true;
						this->didUpdate[1] = true;
					}
				} break;

				case Ghost::Projectile:
				{
					if (this->projectileData.isAlive)
					{
						Vector3 forward = this->projectileData.desiredPosition - this->projectileData.actualPosition;
						float dsqr = forward.lengthSqr();
						forward.normalize();

						float numerator = (this->projectileData.actualPosition - this->projectileData.startPosition).length();
						float denominator = (this->projectileData.desiredPosition - this->projectileData.startPosition).length();
						this->projectileData.lifeProgress = numerator / denominator;

						if (dsqr > 0.05f && this->projectileData.lifeProgress < 0.95f)
						{
							float speed = 12.0f;
							float allowedDisp = speed * deltaTime;

							float dist = sqrt(dsqr);
							if (dist > allowedDisp)
							{
								dist = allowedDisp;
							}

							this->projectileData.actualPosition += forward * dist;

							// collide with people
							for (auto iterator = GhostManager::serverGhostMap.begin(); iterator != GhostManager::serverGhostMap.end(); iterator++)
							{
								PE::Components::Ghost *ghost = iterator->second;
								if (ghost->type == Ghost::SoldierNPC)
								{
									if ((ghost->soldierData.actualPosition - this->projectileData.actualPosition).length() < 1.5f)
									{
										if (ghost->originalComponentID - 1000 != this->projectileData.teamID)
										{
											this->projectileData.isAlive = false;
											--ghost->soldierData.health;
											ghost->shouldSend[0] = true;
											ghost->shouldSend[1] = true;
											ghost->didUpdate[0] = true;
											ghost->didUpdate[1] = true;
											this->shouldSend[0] = true;
											this->shouldSend[1] = true;
											this->didUpdate[0] = true;
											this->didUpdate[1] = true;
											ghost->possibleDesync[0] = true;
											ghost->possibleDesync[1] = true;
											return;
										}
									}
								}
							}

							// collide with walls
							CharacterControl::Components::ServerGameObjectManagerAddon::serverNavigator.initStaticPolys();
							for (Navigator::Poly poly : CharacterControl::Components::ServerGameObjectManagerAddon::serverNavigator.staticPolys)
							{
								Vector3 projectilePos = this->projectileData.actualPosition;
								projectilePos.m_y = 0;
								if (CharacterControl::Components::ServerGameObjectManagerAddon::serverNavigator.isInsidePoly(poly, projectilePos))
								{
									this->projectileData.isAlive = false;
									this->shouldSend[0] = true;
									this->shouldSend[1] = true;
								}
							}
						}
						else
						{
							this->projectileData.isAlive = false;
							this->shouldSend[0] = true;
							this->shouldSend[1] = true;
						}

						this->didUpdate[0] = true;
						this->didUpdate[1] = true;
						//this->shouldSend[0] = true;
						//this->shouldSend[1] = true;
					}
				} break;

				default:
				{

				} break;

			}

			this->lastTimeUpdatedServer = TIMENOW;
		}
		//void Ghost::addDefaultComponents()
		//{
		//	Component::addDefaultComponents();
		//}
	};
};
