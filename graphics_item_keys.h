#pragma once
#include <Qt>

namespace Gik {
    static constexpr int KindKey = Qt::UserRole + 1;
    static constexpr int UuidKey = Qt::UserRole + 2;

    enum Kind : int {
        Kind_Node = 1,
        Kind_Edge = 2,
        Kind_Socket = 3
    };
}
