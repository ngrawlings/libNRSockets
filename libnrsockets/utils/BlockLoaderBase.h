//
//  BlockLoaderBase.hpp
//  NrSockets
//
//  Created by Nyhl Rawlings on 26/04/2019.
//  Copyright © 2019 Liquidsoft Studio. All rights reserved.
//

#ifndef BlockLoaderBase_hpp
#define BlockLoaderBase_hpp

#include <stdio.h>

#include <libnrcore/memory/ByteArray.h>

namespace nrcore {
    
    class Socket;
    
    class  BlockLoaderBase {
    public:
        BlockLoaderBase();
        virtual ~BlockLoaderBase();
        
        virtual bool appendAvailableData(Socket *socket) = 0;
        
        ByteArray getBuffer();
        
    protected:
        ByteArray buffer;
    };
    
}

#endif /* BlockLoaderBase_hpp */
