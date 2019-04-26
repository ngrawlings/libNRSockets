//
//  ClientSocket.hpp
//  NrSockets
//
//  Created by Nyhl Rawlings on 19/04/2019.
//  Copyright © 2019 Liquidsoft Studio. All rights reserved.
//

#ifndef ClientSocket_hpp
#define ClientSocket_hpp

#include <stdio.h>

#include <libnrsockets/Socket.h>

namespace nrcore {

    class ClientSocket : public Socket {
    public:
        ClientSocket(EventBase *event_base, Address address, unsigned short port);
        virtual ~ClientSocket();
        
    protected:
        
        void onReceive();
        void onWriteReady();
        
    };
    
}

#endif /* ClientSocket_hpp */
