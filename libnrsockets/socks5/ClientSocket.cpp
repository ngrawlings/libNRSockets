//
//  ClientSocket.cpp
//  NrSockets
//
//  Created by Nyhl Rawlings on 19/04/2019.
//  Copyright © 2019 Liquidsoft Studio. All rights reserved.
//

#include "ClientSocket.h"
#include "Socks5Server.h"

namespace nrcore {
    
    ClientSocket::ClientSocket(Socks5Server *server, Address address, unsigned short port) : Socket(address, port) {
        this->server = server;
    }
    
    ClientSocket::~ClientSocket() {
        
    }
    
    void ClientSocket::onReceive() {
        size_t available = this->available();
        
        size_t sz = server->writeBufferSpace();
        
        sz = sz > available ? available : sz;
        Memory data = this->read((int)sz);
        
        server->send(data.operator char *(), data.length());
    }
    
    void ClientSocket::onWriteReady() {
        
    }
    
}
