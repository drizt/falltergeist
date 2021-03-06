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

#ifndef FALLTERGEIST_CritterInteract_H
#define FALLTERGEIST_CritterInteract_H

// C++ standard includes

// Falltergeist includes
#include "State.h"

// Third party includes

namespace Falltergeist
{
namespace Game
{
    class GameCritterObject;
}
class VM;

namespace State
{

class CritterInteract : public State
{
protected:
    unsigned int _oldCameraX;
    unsigned int _oldCameraY;
    int _backgroundID = -1;
    int _headID = -1;
    int _mood = 0;
    int _msgFileID = -1;
    VM* _script;
    Game::GameCritterObject* _critter = 0;
public:
    CritterInteract();
    virtual ~CritterInteract();
    virtual void init();

    int backgroundID();
    void setBackgroundID(int backgroundID);

    int headID();
    void setHeadID(int headID);

    int mood();
    void setMood(int mood);

    Game::GameCritterObject* critter();
    void setCritter(Game::GameCritterObject* critter);

    int msgFileID();
    void setMsgFileID(int value);

    VM* script();
    void setScript(VM* script);
};

}
}

#endif // FALLTERGEIST_CritterInteract_H
