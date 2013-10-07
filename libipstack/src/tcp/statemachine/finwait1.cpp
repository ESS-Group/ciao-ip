// This file is part of Aspect-Oriented-IP.
//
// Aspect-Oriented-IP is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Aspect-Oriented-IP is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Aspect-Oriented-IP.  If not, see <http://www.gnu.org/licenses/>.
// 
// Copyright (C) 2011 Christoph Borchert

#include "tcp/TCP_Socket_Private.h"
#include "demux/Demux.h"
#include "router/sendbuffer/SendBuffer.h"
#include "demux/receivebuffer/ReceiveBuffer.h"
#include "os_integration/Clock.h"

namespace ipstack
{
	void TCP_Socket_Private::finwait1(TCP_Segment* segment, unsigned len) {
		if(segment != 0){
			// new tcp segment received:  
			if(handleRST(segment)){ return; }
			if(handleSYN(segment)){ return; }

			//calculate payload length
			unsigned payload_len = len - (segment->get_header_len()*4);
			uint32_t seqnum = segment->get_seqnum();
			uint32_t acknum = segment->get_acknum();
			
			if(segment->has_ACK()){
			handleACK(acknum);
			}
			
			handleFIN(segment, seqnum, payload_len);
			bool needToFree = handleData(segment, seqnum, payload_len);
			
			//This segment ack'ed our FIN
			bool FIN_acked = (segment->has_ACK()) && (acknum == seqnum_next);
			
			if(needToFree == true){
				freeReceivedSegment(segment);
			}
			
			// **********************************************
			if(FIN_complete()){
			//we have received all data so far      
			if(FIN_acked == true){
				// Our FIN got ack'ed --> connection fully terminated now
		//         printf("FIN+ACK arrived: FINWAIT1 --> TIMEWAIT\n");
				state = TIMEWAIT;
			}
			else{
		//         printf("FIN arrived: FINWAIT1 --> CLOSING (simulaneous close)\n");
				state = CLOSING; //transition: FINWAIT1 --> CLOSING (simulaneous close)
			}
			}
			// **********************************************    
		}
		else{
			// there are no more segments in the input buffer
			updateHistory(); //cleanup packets which are not used anymore (and trigger retransmissions)
			processACK(); //send ACK if requested
			if(history.isEmpty()){ //TODO: is this a good indicator? [ above: if(FIN_acked){ state = FINWAIT2; } ]
			// Our FIN got ack'ed
			//printf("ACK arrived: FINWAIT1 --> FINWAIT2\n");
			state = FINWAIT2;
			}
			else{
			//resend FIN after timeout
			block(history.getNextTimeout());
			}
		}
	}
}