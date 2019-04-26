//
//  Socks5Server.hpp
//  NrSockets
//
//  Created by Nyhl Rawlings on 18/04/2019.
//  Copyright © 2019 Liquidsoft Studio. All rights reserved.
//

#ifndef Socks5Server_hpp
#define Socks5Server_hpp

#include <stdio.h>

#include <libnrcore/memory/ByteArray.h>
#include <libnrcore/memory/RingBuffer.h>
#include <libnrsockets/Socket.h>

#include "Structures.h"
#include <libnrsockets/Address.h>
#include <libnrsockets/utils/BlockLoaderBase.h>
#include "ClientSocket.h"

namespace nrcore {

    class Socks5Server : public Socket {
    public:
        typedef enum {
            INIT,
            AUTH,
            REQUEST,
            PROXY,
            ERROR
        } STATE;
        
    public:
        Socks5Server(EventBase *event_base, int _fd);
        virtual ~Socks5Server();
        
        void setPreferedAuthOrder(Memory auth_sequence);
        
    protected:
        
        void onReceive();
        void onWriteReady();
        
    private:
        STATE state;
        Memory auth_sequence;
        unsigned char selected_auth_method;
        Ref<CLIENT_REQUEST> request;
        Memory address_data;
        Ref<Address> address;
        unsigned short port;
        Ref<ClientSocket> client;
        
        Ref<BlockLoaderBase> block_loader;
        Ref<RingBuffer> proxy_buffer;
        
        void selectAuthMethod(CLIENT_INIT *client_init);
        
        void auth(size_t available);
        void processRequest(size_t available);
        void proxy(size_t available);
        
        bool authGSSAPI() { return false; };
        bool authUsernamePassword(String username, String password) { return false; };
        virtual bool customAuthMethod(size_t available) { return false; };
    };
    
}

#endif /* Socks5Server_hpp */
