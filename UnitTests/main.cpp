//
//  main.cpp
//  UnitTests
//
//  Created by Nyhl Rawlings on 07/06/2019.
//  Copyright © 2019 Liquidsoft Studio. All rights reserved.
//

#include <libnrcore/memory/Ref.h>
#include <libnrthreads/Task.h>
#include <libnrthreads/Thread.h>

#include "Sock5ProxyServer/Server.h"

#include <libnrdebug/UnitTests.h>

using namespace nrcore;

Ref<Server> server;

bool testLaunchSoxks5ProxyServer() {
    server = Ref<Server>(new Server);
    server.getPtr()->runEventLoop(false);
    
    return true;
}

int main(int argc, const char * argv[]) {
    Task::staticInit();
    Thread::init(4);
    
    UnitTests tests;
    tests.addTest("testLaunchSoxks5ProxyServer", testLaunchSoxks5ProxyServer);
    tests.run();
    
    sleep(120);
    
    server.getPtr()->breakEventLoop();
    server.getPtr()->stop();
    server = Ref<Server>(0);
    
    Thread::stopAllThreads();
    Thread::staticCleanUp();
    Task::staticCleanup();
    
    return 0;
}
