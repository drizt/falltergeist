/*
 * Copyright 2012-2015 Falltergeist Developers.
 *
 * This file is part of Falltergeist.
 *
 * Falltergeist is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Falltergeist is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Falltergeist.  If not, see <http://www.gnu.org/licenses/>.
 */

// C++ standard includes

// Falltergeist includes
#include "../../Logger.h"
#include "../../VM/Handlers/Opcode80D5Handler.h"
#include "../../VM/VM.h"

// Third party includes

namespace Falltergeist
{

Opcode80D5Handler::Opcode80D5Handler(VM* vm) : OpcodeHandler(vm)
{
}

void Opcode80D5Handler::_run()
{
    Logger::debug("SCRIPT") << "[80D5] [*] int tile_num_in_direction(int start_tile, int dir, int distance)" << std::endl;
    auto distance = _vm->popDataInteger();
    auto dir = _vm->popDataInteger();
    auto start_tile = _vm->popDataInteger();
    // dirty hack... should be fixed in future
    _vm->pushDataInteger(start_tile);
}

}
