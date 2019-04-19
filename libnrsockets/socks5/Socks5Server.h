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

#include <libnrsockets/Socket.h>

#include "Structures.h"
#include <libnrsockets/Address.h>
#include "ClientSocket.h"

namespace nrcore {

    class Socks5Server : public Socket {
    public:
        typedef enum {
            INIT,
            AUTH,
            REQUEST,
            PROXY
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
        
        
        void selectAuthMethod(CLIENT_INIT *client_init);
        
        void authGSSAPI() {};
        void authUsernamePassword() {};
        
        virtual void customAuthMethod() {};
        
        void processRequest(size_t available);
        void proxy(size_t available);
    };
    
}

#endif /* Socks5Server_hpp */
