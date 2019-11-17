#pragma once

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>
#include <vector>
#include <deque>

// Inter-Engine includes

#include "PrimeEngine/Events/Component.h"

extern "C"
{
#include "luasocket_dist/src/socket.h"
};

#include "PrimeEngine/Networking/NetworkContext.h"
#include "PrimeEngine/Utils/Networkable.h"
#include "PrimeEngine/Networking/Packet.h"

#include "PrimeEngine/Networking/Tribes/GhostManager/Ghost.h"

// Sibling/Children includes

namespace PE
{
	namespace Components
	{
		struct Component;
	};

	namespace Events
	{
		struct Event;
	};

	struct GhostTransmissionData
	{
		int m_size;
		char m_payload[PE_MAX_GHOST_PAYLOAD];
	};

	struct GhostReceptionData
	{
		Components::Component *m_pTargetComponent;
		PE::Components::Ghost *m_Ghost;
	};

}; // namespace PE
