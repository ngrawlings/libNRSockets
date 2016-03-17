//
//  ReliableUdp.h
//  libNRCore
//
//  Created by Nyhl on 10/02/15.
//  Copyright (c) 2015 N G Rawlings. All rights reserved.
//

#ifndef __libNRCore__ReliableUdp__
#define __libNRCore__ReliableUdp__

#include "UdpSocket.h"
#include <libnrcore/memory/RingBuffer.h>

#define ACK_PRESENT_BIT     0x80000000
#define ACK_REQUIRED_BIT    0x40000000

namespace nrcore {

    class ReliableUdp : public UdpSocket {
    public:
        ReliableUdp(EventBase *event_base, String interface, Address::ADDRESS_TYPE iptype, unsigned short port);
        virtual ~ReliableUdp();
        
    protected:
        void onReceive();
        
        virtual int received(const char *bytes, const int len) = 0;
        virtual void disconnected() {};
        
    private:
        /*
         L1 [31 ACK Present, 30 ACK Required, ...]
         W2 length
         L3 (Optional) ACK
         */
        
        typedef struct {
            unsigned int offset;
            unsigned short length;
            unsigned int ack;
        } PACKET_HEADER;
        
        
        unsigned int expected;
        unsigned int actual;
        RingBuffer write_buffer;
        RingBuffer read_buffer;
        
        unsigned int incoming_act;
        
        
        
    };
    
};

#endif /* defined(__libNRCore__ReliableUdp__) */
