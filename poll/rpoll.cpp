#include "rpoll.h"

namespace GNET{

    bool Poll::_running = true;
    std::mutex Poll::_poll_mtx;
#ifdef __linux
    Poll poll;
    struct epoll_event Poll::_ev;
    struct epoll_event Poll::_events[MAX_CONNECT];
    std::vector<BaseNet*> Poll::_deleted_vector;

    int Poll::_eph = 0;
#else
    timeval Poll::_select_timeout;
    fd_set Poll::_read_fds;
    map<int, BaseNet*> Poll::_read_fds_map;
#endif
};
