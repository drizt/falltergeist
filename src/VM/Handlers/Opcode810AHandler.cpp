/*
 * Copyright 2012-2014 Falltergeist Developers.
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
#include "../../Exception.h"
#include "../../Logger.h"
#include "../../ResourceManager.h"
#include "../../Game/Object.h"
#include "../../UI/TextArea.h"
#include "../../VM/Handlers/Opcode810AHandler.h"
#include "../../VM/VM.h"

// Third party includes

namespace Falltergeist
{

Opcode810AHandler::Opcode810AHandler(VM* vm) : OpcodeHandler(vm)
{
}

void Opcode810AHandler::_run()
{
    Logger::debug("SCRIPT") << "[810A] [=] void float_msg(void* who, string* msg, int type) " << std::endl;
    int type = _vm->popDataInteger();
    unsigned int color;
    switch (type)
    {
        case -2:
            //БОЛЬШИЕ КРАСНЫЕ БУКВЫ	FF 00 00
            color = 0xff0000ff;
            break;
        case -1:
            //Самоперебирающиеся цвета @todo
            color = 0xff0000ff; // временно взят из -2
            break;
        case 0:
        case 8:
        case 13:
            color = 0xffff7fff;
            break;
        case 1:
        case 5:
        case 10:
            color = 0x555555ff;
            break;
        case 2:
            color = 0xff0000ff;
            break;
        case 3:
            color = 0x3cfb00ff;
            break;
        case 4:
            color = 0x30598eff;
            break;
        case 6:
            color = 0xa2a2a2ff;
            break;
        case 7:
            color = 0xff4949ff;
            break;
        case 9:
            color = 0xffffffff;
            break;
        case 11:
            color = 0x3c3c3cff;
            break;
        case 12:
            color = 0x757575ff;
            break;
        default:
            throw Exception("Opcode810AHandler - wrong type: " + std::to_string(type));
    }

    auto string = static_cast<std::string*>(_vm->popDataPointer());
    auto object = static_cast<Game::GameObject*>(_vm->popDataPointer());

    auto floatMessage = new TextArea(*string);
    floatMessage->setWidth(200);
    floatMessage->setWordWrap(true);
    floatMessage->setHorizontalAlign(TextArea::HORIZONTAL_ALIGN_CENTER);
    floatMessage->setFont(ResourceManager::font("font1.aaf", color));
    floatMessage->setOutlineColor(0x000000ff);
    object->setFloatMessage(floatMessage);

}

}
