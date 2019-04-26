//
//  Socket.hpp
//  decap
//
//  Created by Nyhl on 18/03/16.
//  Copyright © 2016 Liquidsoft Studio. All rights reserved.
//

#ifndef Socket_hpp
#define Socket_hpp

#include <libnrevent/EventBase.h>
#include <libnrevent/Timer.h>

#include "Address.h"
#include <libnrthreads/Mutex.h>

#include <libnrcore/memory/RingBuffer.h>

#include <errno.h>
#include <libnrcore/exception/Exception.h>

namespace nrcore {

    class Socket {
    public:
        friend class ReceiveTask;
        
        Socket(int _fd);
        Socket(Address address, unsigned short port);
        virtual ~Socket();
        
        size_t available();
        Memory read(int max);
        int send(const char* buffer, size_t len);
        
        static void init(EventBase *event_base);
        static void cleanup();
        
    protected:
        class ReceiveTask : public Task {
        public:
            friend class Socket;
            
            ReceiveTask(Socket *socket);
            virtual ~ReceiveTask();
            
            void run();
            
            bool isLocked();
            
        private:
            Socket *socket;
            Mutex lock;
        };
        
        class CleanUpTimer : public Timer {
        public:
            CleanUpTimer(EventBase *event_base);
            virtual ~CleanUpTimer();
            
        protected:
            void onTick();
        };
        
        class CleanUpTask : public Task {
        public:
            CleanUpTask();
            virtual ~CleanUpTask();
            
        protected:
            void run();
        };
        
    protected:
        int fd;
        static EventBase *event_base;
        struct event *event_read, *event_write;
        
        Mutex recv_lock, send_lock;
        RingBuffer in_buffer, out_buffer;
        
        ReceiveTask recv_task;
        
        virtual void onReceive() = 0;
        virtual void onWriteReady() = 0;
        void receive();
        void sendReady();
        
        void close();
        
    private:
        void enableEvents();
        
        static void ev_read(int fd, short ev, void *arg);
        static void ev_write(int fd, short ev, void *arg);
        
        typedef struct {
            time_t timestamp;
            Socket *socket;
        } SOCKET_CLOSED;
        
        static LinkedList< Ref<SOCKET_CLOSED> > closed_sockets;
        static void addToClosedSockets(Socket *socket);
        static void releaseClosedSockets();
        
        static Ref<CleanUpTimer> cleanup_timer;
    };
    
}

#endif /* Socket_hpp */
