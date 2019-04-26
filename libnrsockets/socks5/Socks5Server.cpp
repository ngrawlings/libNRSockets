//
//  Socks5Server.cpp
//  NrSockets
//
//  Created by Nyhl Rawlings on 18/04/2019.
//  Copyright © 2019 Liquidsoft Studio. All rights reserved.
//

#include "Socks5Server.h"

#include <libnrsockets/socks5/BlockLoaders/UsernamePasswordLoader.h>

namespace nrcore {
    
    Socks5Server::Socks5Server(int _fd) : Socket(_fd) {
        state = INIT;
        port = 0;
    }
    
    Socks5Server::~Socks5Server() {
    }
    
    void Socks5Server::setPreferedAuthOrder(Memory auth_sequence) {
        this->auth_sequence = auth_sequence;
    }
    
    void Socks5Server::onReceive() {
        size_t available = this->available();
        
        switch (state) {
            case INIT:
                if (available >= sizeof(CLIENT_INIT)) {
                    Memory data = this->read(sizeof(CLIENT_INIT));
                    selectAuthMethod((CLIENT_INIT *)data.operator char *());
                }
                break;
                    
            case AUTH:
                auth(available);
                break;
                    
            case REQUEST:
                processRequest(available);
                break;
                
            case PROXY:
                proxy(available);
                break;
                
            default:
                this->close();
                break;
        }
    }
    
    void Socks5Server::onWriteReady() {
        
    }
    
    void Socks5Server::selectAuthMethod(CLIENT_INIT *client_init) {
        SERVER_AUTH_SELECT reply;
        size_t len = auth_sequence.length();
        
        for (size_t i=0; i<len; i++) {
            for (int x=0; x<client_init->auth_method_cnt; x++) {
                if (auth_sequence[i] == client_init->auth_methouds[x]) {
                    
                    selected_auth_method = client_init->auth_methouds[x];
                    
                    reply.version = 0x5;
                    reply.version = selected_auth_method;
                    
                    this->send((const char*)&reply, sizeof(SERVER_AUTH_SELECT));
                    
                    if (client_init->auth_methouds[x])
                        state = AUTH;
                    else
                        state = REQUEST;
                    
                    return;
                }
            }
        }
        
        reply.version = 0x5;
        reply.version = 0xFF;
        
        this->send((const char*)&reply, sizeof(SERVER_AUTH_SELECT));
    }
    
    void Socks5Server::auth(size_t available) {
        bool result = false;
        switch (selected_auth_method) {
            case NO_AUTH:
                throw Exception(-1, "Code excetion should not be here");
                
            case GSSAPI:
                {
                    result = authGSSAPI();
                }
                break;
                
            case USERNAME_PASSWORD:
                {
                    if (!block_loader.getPtr())
                        block_loader = Ref<BlockLoaderBase>(new UsernamePasswordLoader());
                    
                    if (block_loader.getPtr()->appendAvailableData(this)) {
                        result = authUsernamePassword(
                                ((UsernamePasswordLoader*)block_loader.getPtr())->getUsername(),
                                ((UsernamePasswordLoader*)block_loader.getPtr())->getPassword());
                        block_loader = Ref<BlockLoaderBase>();
                    }
                    
                }
                break;
                
            default:
                {
                    result = customAuthMethod(available);
                }
        }
        
        AUTH_RESULT reply;
        reply.version = 0x05;
        
        if (result) {
            state = REQUEST;
            reply.result = 0;
        } else {
            state = ERROR;
            reply.result = 1;
        }
        
        this->send((const char*)&reply, sizeof(AUTH_RESULT));
        
        if (!result)
            this->close();
    }
    
    void Socks5Server::processRequest(size_t available) {
        if (!request.getPtr()) {
            if (available >= sizeof(CLIENT_REQUEST)) {
                Memory data = this->read(sizeof(CLIENT_REQUEST));
                CLIENT_REQUEST *client_request = new CLIENT_REQUEST;
                memcpy((unsigned char*)client_request, data.operator char *(), sizeof(CLIENT_REQUEST));
                request = Ref<CLIENT_REQUEST>(client_request);
            }
        }
        
        if (request.getPtr()->address_type == IPV4 && this->available() >= sizeof(ADDRESS_IPV4)) {
            
            address_data = this->read(sizeof(ADDRESS_IPV4));
            address = Ref<Address>(new Address(Address::IPV4, (const char*)((ADDRESS_IPV4*)address_data.getPtr())->ip));
            memcpy(&port, &((ADDRESS_IPV4*)address_data.getPtr())->port, 2);
            
        } else if (request.getPtr()->address_type == IPV6 && this->available() >= sizeof(ADDRESS_IPV4)) {
            
            address_data = this->read(sizeof(ADDRESS_IPV6));
            address = Ref<Address>(new Address(Address::IPV6, (const char*)((ADDRESS_IPV6*)address_data.getPtr())->ip));
            memcpy(&port, &((ADDRESS_IPV6*)address_data.getPtr())->port, 2);
            
        } else if (request.getPtr()->address_type == DOMAIN) {
            
            if (address_data.length() == 0) {
                ByteArray ba(this->read(1).getPtr(), 1);
                Memory data = this->read(ba[0]+2);
                ba.append(ByteArray(data.getPtr(), (int)data.length()));
                address_data = ba;
            } else if (address_data.length() < address_data[0]+2) {
                ByteArray ba(address_data.getPtr(), (int)address_data.length());
                Memory data = this->read((ba[0]+2)-(int)address_data.length());
                ba.append(ByteArray(data.getPtr(), (int)data.length()));
                address_data = ba;
            }
            
            if (address_data.length() == address_data[0]+2) {
                address = Ref<Address>(new Address(Address::DOMAIN, &address_data[1]));
                memcpy(&port, &address_data[address_data.length()-2], 2);
            }
            
        }
        
        if (address.getPtr() && port) {
            // Launch Connection
            
            if (request.getPtr()->cmd == CONNECT) {
                try {
                    client = Ref<ClientSocket>(new ClientSocket(address, port));
                    
                    SERVER_RESPONSE reply;
                    
                    reply.version = 0x5;
                    
                    this->send((const char*)&reply, sizeof(SERVER_RESPONSE));
                    
                } catch (Exception e) {
                    return;
                }
            } else if (request.getPtr()->cmd == BIND) {
                
            } else if (request.getPtr()->cmd == UDP_ASSOCIATE) {
                
            }
            
            proxy_buffer = Ref<RingBuffer>(new RingBuffer(4096));
            state = PROXY;
        }
    }
    
    void Socks5Server::proxy(size_t available) {
        
    }
    
}
