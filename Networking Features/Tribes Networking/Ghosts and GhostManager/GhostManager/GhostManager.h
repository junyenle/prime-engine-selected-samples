#pragma once

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>
#include <vector> // blasphemy
#include <deque>
#include <unordered_map>
#include <chrono>
//#include "CharacterControl/Navigator.h"

// Inter-Engine includes

#include "PrimeEngine/Events/Component.h"

extern "C"
{
#include "luasocket_dist/src/socket.h"
};

#include "PrimeEngine/Networking/NetworkContext.h"
#include "PrimeEngine/Utils/Networkable.h"
#include "PrimeEngine/Networking/Tribes/GhostManager/GhostTransmissionData.h"
#include "PrimeEngine/Networking/Packet.h"

namespace PE
{
	namespace Components
	{
		struct Ghost;
		struct GhostManager : public Component
		{

			PE_DECLARE_CLASS(GhostManager);
			GhostManager(PE::GameContext &context, PE::MemoryArena arena, PE::NetworkContext &netContext, Handle hMyself, bool isServer = false);
			virtual ~GhostManager();
			virtual void addDefaultComponents();
			virtual void initialize();

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
			virtual void do_UPDATE(Events::Event *pEvt);

			//PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PRE_RENDER_needsRC);
			virtual void do_PRE_RENDER_needsRC(PE::Events::Event *pEvt);
			/// called by gameplay code to schedule event transmission to client(s)
			void scheduleGhost(Ghost *pGhost);
			/// called by stream manager to see how many ghosts to send
			int haveGhostsToSend();
			/// called by StreamManager to put queued up ghosts in packet
			int fillInNextPacket(char *pDataStream, TransmissionRecord *pRecord, int packetSizeAllocated, bool &out_usefulDataSent, bool &out_wantToSendMore);
			/// process next packet
			int receiveNextPacket(char *pDataStream);
			/// called by StreamManager to process transmission record deliver notification
			// TODO(Rohan): Implement Process Notification when we do UDP
			//void processNotification(TransmissionRecord *pTransmittionRecord, bool delivered);
			/// render debug information
			void debugRender(int &threadOwnershipMask, float xoffset = 0, float yoffset = 0);

			int m_transmitterNumGhostsNotAcked;
			std::deque<GhostTransmissionData> m_ghostsToSend;
			std::unordered_map<int, Ghost *> ghostMap;
			static std::unordered_map<int, Ghost *> serverGhostMap;
			PE::NetworkContext *m_pNetContext;
			bool m_isServer;
			bool DEBUGPRINT = false;

			struct InputRecord
			{
				InputRecord() {}
				uint64_t timeStamp;
				int inputType; // 0 key 1 mouse
				Vector3 mousePos;
				char keyPressed;
				int selectedID;
			};

			static std::vector<InputRecord> inputHistory;
			void resimulateGhost(Ghost *ghost, uint64_t startTime);
			void resimulateGhostHelper(Ghost *ghost, uint64_t miliDeltaTime, int inputIndex = -1);
			std::unordered_map<int, Ghost> resimGhosts;
			int ghostNum = 0;
			int lastRecieved = -1;

		};
	}; // namespace Components
}; // namespace PE
