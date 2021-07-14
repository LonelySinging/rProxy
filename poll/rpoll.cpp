#include "rpoll.h"

namespace GNET{
    Poll poll;
    struct epoll_event Poll::_ev;
    struct epoll_event Poll::_events[MAX_CONNECT];

    int Poll::_eph = 0;
    bool Poll::_running = true;

};
