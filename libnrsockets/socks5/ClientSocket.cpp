//
//  ClientSocket.cpp
//  NrSockets
//
//  Created by Nyhl Rawlings on 19/04/2019.
//  Copyright © 2019 Liquidsoft Studio. All rights reserved.
//

#include "ClientSocket.h"

namespace nrcore {
    
    ClientSocket::ClientSocket(Address address, unsigned short port) : Socket(address, port) {
        
    }
    
    ClientSocket::~ClientSocket() {
        
    }
    
    void ClientSocket::onReceive() {
        
    }
    
    void ClientSocket::onWriteReady() {
        
    }
    
}
