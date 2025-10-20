#pragma once

#include <memory>

#include <unistd.h>

namespace own {
    struct FdCloser {
        void operator()(const int* p) const {
            close(*p);
            delete p;
        }
    };

    using owned_fd = std::unique_ptr<int, FdCloser>;
}
