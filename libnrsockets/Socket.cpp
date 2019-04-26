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

namespace nrcore {

    LinkedList< Ref<Socket::SOCKET_CLOSED> > Socket::closed_sockets;
    Ref<Socket::CleanUpTask> Socket::cleanup_task;

    Socket::Socket(int _fd) : in_buffer(4096), out_buffer(4096), recv_task(this) {
        this->fd = _fd;
        enableEvents();
        
        int flag = 1;
        setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    }

    Socket::Socket(Address address, unsigned short port) : in_buffer(4096), out_buffer(4096), recv_task(this) {
        int type = address.getType() == Address::IPV4 ? AF_INET : AF_INET6;
        this->fd = socket(type, SOCK_STREAM, 0);
        if (this->fd == -1)
            throw Exception(-1, "Failed to create socket");
        
        struct sockaddr_in ipa;
        ipa.sin_family = type;
        ipa.sin_port = htons(port);
        memcpy(&ipa.sin_addr.s_addr, address.getAddr(), type == AF_INET ? AF_INET : AF_INET6);
        
        int res = connect(this->fd, (const struct sockaddr *)&ipa, sizeof(sockaddr_in));
        if (res == -1)
            throw Exception(errno, "Faield to connect");
        
        int flag = 1;
        setsockopt(0, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    }

    Socket::~Socket() {
        send_lock.lock();
        recv_lock.lock();
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

    void Socket::init(EventBase *event_base) {
        Socket::event_base = event_base;
    }

    void Socket::cleanup() {
        releaseClosedSockets();
    }

    Socket::ReceiveTask::ReceiveTask(Socket *socket) : Task(false) {
        this->socket = socket;
    }

    Socket::ReceiveTask::~ReceiveTask() {

    }

    void Socket::ReceiveTask::run() {
        lock.lock();
        socket->recv_lock.lock();
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
    
    Socket::CleanUpTask::CleanUpTask(EventBase *event_base) : Timer(event_base) {
        start(15, 0);
    }
    
    Socket::CleanUpTask::~CleanUpTask() {
        
    }
    
    void Socket::CleanUpTask::onTick() {
        releaseClosedSockets();
    }

    void Socket::enableEvents() {
        event_write = event_new(event_base->getEventBase(), fd, EV_WRITE, ev_write, (void*)this);
        event_read = event_new(event_base->getEventBase(), fd, EV_READ|EV_PERSIST, ev_read, (void*)this);
        event_add(event_read, NULL);
        event_add(event_write, NULL);
    }

    void Socket::receive() {
        size_t fs = in_buffer.freeSpace();
        
        if (recv_lock.tryLock()) {
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
            
            recv_task.run();
            
            /*if (!Task::taskExists(&recv_task)) {
                Thread::runTask(&recv_task);
            } else if (!recv_task.isFinished() && !recv_task.getAquiredThread()) {
                Thread *thread = Thread::getWaitingThread();
                if (thread) {
                    thread->wake();
                }
            }*/
            
            recv_lock.release();
        }
    }

    void Socket::sendReady() {
        try {
            if (out_buffer.length()) {
                send_lock.lock();
                recv_lock.lock();
                recv_lock.release();
                if (out_buffer.length()) {
                    int flags = 0;
                    Memory data = out_buffer.getDataUntilEnd();
                    int s = (int)::send(fd, data.operator char *(), (ssize_t)data.length(), flags);
                    if (!s || (s==-1 && errno!=EAGAIN)) {
                        printf("closing connection\r\n");
                        close();
                    } else if (s) {
                        printf("sent: %d\r\n", s);
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
            addToClosedSockets(this);
            event_write = 0;
        }
        
        if (locked)
            send_lock.release();
    }

    void Socket::ev_read(int fd, short ev, void *arg) {
        ((Socket*)arg)->receive();
    }

    void Socket::ev_write(int fd, short ev, void *arg) {
        ((Socket*)arg)->sendReady();
    }

    void Socket::addToClosedSockets(Socket *socket) {
        SOCKET_CLOSED *cs = new SOCKET_CLOSED;
        cs->timestamp = time(0);
        cs->socket = socket;
        
        closed_sockets.add(Ref<SOCKET_CLOSED>(cs));
    }

    void Socket::releaseClosedSockets() {
        if (Thread::getThreadInstance() == event_base->getThread()) {
            LinkedListState< Ref<SOCKET_CLOSED> > css(&closed_sockets);
            Ref<SOCKET_CLOSED> entry;
            int cnt = css.length();
            while (cnt--) {
                entry = css.next();
                if (entry.getPtr()->timestamp < time(0)-4) {
                    delete entry.getPtr()->socket;
                    css.remove();
                }
            }
        } else
            throw Exception(-1, "Can only be called on the event base thread");
    }
    
}
