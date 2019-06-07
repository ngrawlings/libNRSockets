//
//  Socket.cpp
//  decap
//
//  Created by Nyhl on 18/03/16.
//  Copyright © 2016 Liquidsoft Studio. All rights reserved.
//

#include "Socket.h"

#include <time.h>
#include <stdio.h>
#include <netinet/tcp.h>

#include <libnrthreads/Thread.h>

namespace nrcore {

    EventBase* Socket::event_base;
    
    Socket::Socket(int _fd) : in_buffer(4096), out_buffer(4096), recv_task(this), cb_interface(0) {
        this->fd = _fd;
        enableEvents();
        
        int flag = 1;
        setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
        
        if (cb_interface)
            cb_interface->onConnected(this);
    }

    Socket::Socket(Address address, unsigned short port) : in_buffer(4096), out_buffer(4096), recv_task(this), cb_interface(0) {
        int type = address.getType() == Address::IPV4 ? AF_INET : AF_INET6;
        this->fd = socket(type, SOCK_STREAM, 0);
        if (this->fd == -1)
            throw Exception(-1, "Failed to create socket");
        
        struct sockaddr_in ipa;
        memset(&ipa, 0, sizeof(sockaddr_in));
        ipa.sin_family = type;
        ipa.sin_port = htons(port);
        ipa.sin_len = sizeof(sockaddr_in);
        memcpy(&ipa.sin_addr.s_addr, address.getAddr(), type == AF_INET ? 4 : 16);
        
        int res = connect(this->fd, (const struct sockaddr *)&ipa, sizeof(sockaddr_in));
        if (res == -1)
            throw Exception(errno, "Faield to connect");
        
        enableEvents();
        
        int flag = 1;
        setsockopt(0, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
        
        if (cb_interface)
            cb_interface->onConnected(this);
    }

    Socket::~Socket() {
        send_lock.lock(0, "d");
        recv_lock.lock(0, "d");
        
        if (cb_interface)
            cb_interface->onDestroyed(this);
    }

    size_t Socket::available() {
        return in_buffer.length();
    }

    Memory Socket::read(int max) {
        Memory ret;
        if (in_buffer.length() && recv_lock.isLockedByMe()) {
            int len = in_buffer.length() < max ? (int)in_buffer.length() : max;
            Memory bytes = in_buffer.fetch(len);
            ret = Memory(bytes.getPtr(), len);
        }
        return ret;
    }

    int Socket::send(const char* buffer, size_t len) {
        size_t space = 0, add = 0;
        
        send_lock.lock();
        
        if (event_write) {
            space = out_buffer.freeSpace();
            if (space && event_write) {
                add = space < len ? space : len;
                add = out_buffer.append(buffer, add);
                event_add(event_write, NULL);
            }
        }
        
        send_lock.release();
        
        return (int)add;
    }
    
    void Socket::setCallbackInterface(CallbackInterface *cb) {
        this->cb_interface = cb;
    }
    
    size_t Socket::writeBufferSpace() {
        return out_buffer.freeSpace();
    }

    void Socket::init(EventBase *event_base) {
        Socket::event_base = event_base;
    }

    Socket::ReceiveTask::ReceiveTask(Socket *socket) : Task(false) {
        this->socket = socket;
    }

    Socket::ReceiveTask::~ReceiveTask() {

    }

    void Socket::ReceiveTask::run() {
        lock.lock();
        socket->recv_lock.lock(0, "run");
        try {
            while (socket->in_buffer.length())
                socket->onReceive();
        } catch (...) {
            printf("RT: Error\r\n");
        }
        socket->recv_lock.release();
        finished();
        lock.release();
    }

    bool Socket::ReceiveTask::isLocked() {
        return lock.isLocked();
    }
    
    void Socket::enableEvents() {
        event_write = event_new(event_base->getEventBase(), fd, EV_WRITE, ev_write, (void*)this);
        event_read = event_new(event_base->getEventBase(), fd, EV_READ|EV_PERSIST, ev_read, (void*)this);
        event_add(event_read, NULL);
        event_add(event_write, NULL);
    }

    void Socket::receive() {
        size_t fs = in_buffer.freeSpace();
        
        if (recv_lock.tryLock("receive")) {
            if (fs > 0) {
                char buf[64];
                
                ssize_t s = ::recv(fd, buf, fs > 64 ? 64 :fs , 0);
                if (s == 0) {
                    close();
                    event_del(event_read);
                    event_free(event_read);
                    event_read = 0;
                    fd = 0;
                } else if (s == -1 && errno !=EAGAIN) {
                    close();
                } else {
                    fs -= in_buffer.append(buf, s);
                }
            }
            
            recv_lock.release();
            Thread::runTask(&recv_task);
        }
    }

    void Socket::sendReady() {
        try {
            if (out_buffer.length()) {
                send_lock.lock();
                recv_lock.lock(0, "sendReady");
                recv_lock.release();
                if (out_buffer.length()) {
                    int flags = 0;
                    Memory data = out_buffer.getDataUntilEnd();
                    int s = (int)::send(fd, data.operator char *(), (ssize_t)data.length(), flags);
                    if (!s || (s==-1 && errno!=EAGAIN)) {
                        close();
                    } else if (s) {
                        out_buffer.drop(s);
                    }
                    
                    if (s >= 0 && out_buffer.length())
                        event_add(event_write, NULL);
                }
                
                send_lock.release();
            }
        } catch (...) {
            printf("err\r\n");
        }
        
        onWriteReady();
    }

    void Socket::close() {
        bool locked = false;
        if (!send_lock.isLocked() || !send_lock.isLockedByMe()) {
            send_lock.lock();
            locked = true;
        }

        if (fd)
            ::close(fd);
        
        if (event_write) {
            event_del(event_write);
            event_free(event_write);
            event_write = 0;
        }
        
        if (locked)
            send_lock.release();
        
        if (cb_interface)
            cb_interface->onClosed(this);
    }

    void Socket::ev_read(int fd, short ev, void *arg) {
        ((Socket*)arg)->receive();
    }

    void Socket::ev_write(int fd, short ev, void *arg) {
        ((Socket*)arg)->sendReady();
    }
    
}
