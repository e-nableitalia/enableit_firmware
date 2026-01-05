#pragma once

#include "BoardManager.h"

class BoardAppRegistrar {
public:
    BoardAppRegistrar(BoardApp &app) {
        BoardManager::instance().addApp(&app);
    }
};

#define REGISTER_BOARD_APP(app) \
        static BoardAppRegistrar _registrar_##app(app);
    