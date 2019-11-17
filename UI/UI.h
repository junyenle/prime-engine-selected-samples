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
		struct UI : public PE::Components::Component
		{
			PE_DECLARE_CLASS(UI);
			enum
			{
				SLIDER,
				HP_BAR
			} type;

			UI(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself);

			static void EnableUI(Vector2 Position) 
			{
				for (UI *ui : StaticUIElements)
				{
					if (!ui->meshInstance->m_display)
					{
						ui->sceneNode->m_base.setPos(Vector3(Position.m_x, Position.m_y, 0.0f));
						ui->setEnabled(true);
						return;
					}
				}
			}
			
			static void EnableUIWithIndex(int index) 
			{
				for (UI *ui : StaticUIElements)
				{
					if (!ui->componentID == index)
					{
						ui->disable();
						ui->setEnabled(true);
						return;
					}
				}
			}

			void disable()
			{
				meshInstance->m_display = false;
				setEnabled(false);
			}

			virtual void addDefaultComponents();

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
			virtual void do_UPDATE(PE::Events::Event *pEvt);

			static std::vector<struct UI *> StaticUIElements;
			PE::Components::SceneNode *sceneNode;
			PE::Components::MeshInstance *meshInstance;
		};
	}
}








