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
 *
 */

// C++ standard includes
#include <iostream>

// Falltergeist includes
#include "../States/CursorDropdownState.h"
#include "../Engine/Game.h"
#include "../Engine/Location.h"
#include "../Engine/Input/Mouse.h"
#include "../Engine/Surface.h"
#include "../UI/HiddenMask.h"
#include "../Engine/Graphics/Texture.h"
#include "../Game/GameCritterObject.h"
#include "../Game/GameDudeObject.h"

// Third party includes

namespace Falltergeist
{

CursorDropdownState::CursorDropdownState(std::vector<int> icons) : State()
{
    _icons = icons;
    _initialX = _game->mouse()->x();
    _initialY = _game->mouse()->y();
    _initialType = _game->mouse()->type();
}

CursorDropdownState::~CursorDropdownState()
{
}

void CursorDropdownState::init()
{
    if (_initialized) return;
    State::init();
    setFullscreen(false);


    int i = 0;
    for (auto icon : _icons)
    {
        std::string activeSurface;
        std::string inactiveSurface;
        switch (icon)
        {
            case Mouse::ICON_INVENTORY:
                activeSurface = "invenh.frm";
                inactiveSurface = "invenn.frm";
                break;
            case Mouse::ICON_CANCEL:
                activeSurface = "cancelh.frm";
                inactiveSurface = "canceln.frm";
                break;
            case Mouse::ICON_ROTATE:
                activeSurface = "rotateh.frm";
                inactiveSurface = "rotaten.frm";
                break;
            case Mouse::ICON_SKILL:
                activeSurface = "skillh.frm";
                inactiveSurface = "skilln.frm";
                break;
            case Mouse::ICON_LOOK:
                activeSurface = "lookh.frm";
                inactiveSurface = "lookn.frm";
                break;
            case Mouse::ICON_TALK:
                activeSurface = "talkh.frm";
                inactiveSurface = "talkn.frm";
                break;
            case Mouse::ICON_PUSH:
                activeSurface = "pushh.frm";
                inactiveSurface = "pushn.frm";
                break;
            case Mouse::ICON_UNLOAD:
                activeSurface = "unloadh.frm";
                inactiveSurface = "unloadn.frm";
                break;
            case Mouse::ICON_USE:
                activeSurface = "usegeth.frm";
                inactiveSurface = "usegetn.frm";
                break;
            default:
                throw Exception("CursorDropdownState::init() - unknown icon type");

        }
        _activeIcons.push_back(new Image("art/intrface/" + activeSurface));
        _activeIcons.back()->setY(40*i);
        _inactiveIcons.push_back(new Image("art/intrface/" + inactiveSurface));
        _inactiveIcons.back()->setY(40*i);
        i++;
    }

    _game->mouse()->setType(Mouse::NONE);

    _cursor = new Image("art/intrface/actarrow.frm");
    _cursor->setXOffset(0);
    _cursor->setYOffset(0);
    _cursor->setX(_initialX);
    _cursor->setY(_initialY);

    _surface = new Image(40, 40*_icons.size());
    _surface->setX(_initialX + 29);
    _surface->setY(_initialY);

    int deltaY = _surface->y() + _surface->height() - 480;
    int deltaX = _surface->x() + _surface->width() - 640;

    if (deltaY > 0)
    {
        _surface->setY(_surface->y() - deltaY);
        SDL_WarpMouse(_initialX, _surface->y());
    }

    if (deltaX > 0)
    {
        _surface->setX(_surface->x() - 40 - 29 - 29);
        delete _cursor;
        _cursor = new Image("art/intrface/actarrom.frm");
        _cursor->setXOffset(-29);
        _cursor->setYOffset(0);
        _cursor->setX(_initialX);
        _cursor->setY(_initialY);
    }

    _mask = new HiddenMask(640, 480);
    _mask->addEventHandler("mouseleftup", this, (EventRecieverMethod) &CursorDropdownState::onLeftButtonUp);
    _mask->setVisible(true);
    add(_cursor);
    add(_surface);
    add(_mask);

}

void CursorDropdownState::setObject(GameObject* object)
{
    _object = object;
}

void CursorDropdownState::think()
{
    State::think();


    for (UI* ui : _inactiveIcons)
    {
        ui->texture()->copyTo(_surface->texture(), ui->x(), ui->y());
    }

    _currentSurface= (_game->mouse()->y() - _surface->y())/40;
    if (_currentSurface < 0)
    {
        SDL_WarpMouse(_initialX, _surface->y());
        _currentSurface = 0;
    }
    if (_currentSurface >= _icons.size())
    {
        SDL_WarpMouse(_initialX, _surface->y() + _surface->height());
        _currentSurface = _icons.size() - 1;
    }
    auto activeIcon = _activeIcons.at(_currentSurface);

    activeIcon->texture()->copyTo(_surface->texture(), activeIcon->x(), activeIcon->y());

}

void CursorDropdownState::onLeftButtonUp(MouseEvent* event)
{
    CursorDropdownState* state = dynamic_cast<CursorDropdownState*>(event->reciever());
    SDL_WarpMouse(state->_initialX, state->_initialY);
    auto game = &Game::getInstance();
    game->mouse()->setType(state->_initialType);
    game->popState();
    game->location()->handleAction(state->_object, state->_icons.at(state->_currentSurface));
}



}
