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
#include <cmath>
#include <list>
#include <algorithm>

// Falltergeist includes
#include "../Event/MouseEvent.h"
#include "../Exception.h"
#include "../Game/Game.h"
#include "../Game/ContainerItemObject.h"
#include "../Game/Time.h"
#include "../Graphics/Animation.h"
#include "../Graphics/AnimationQueue.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Tile.h"
#include "../Graphics/TileMap.h"
#include "../PathFinding/Hexagon.h"
#include "../PathFinding/HexagonGrid.h"
#include "../Input/Mouse.h"
#include "../LocationCamera.h"
#include "../Logger.h"
#include "../ResourceManager.h"
#include "../Settings.h"
#include "../Game/Defines.h"
#include "../Game/DoorSceneryObject.h"
#include "../Game/DudeObject.h"
#include "../Game/ExitMiscObject.h"
#include "../Game/Object.h"
#include "../Game/ObjectFactory.h"
#include "../Game/WeaponItemObject.h"
#include "../State/CursorDropdown.h"
#include "../State/ExitConfirm.h"
#include "../State/Location.h"
#include "../State/MainMenu.h"
#include "../State/PlayerPanel.h"
#include "../UI/Image.h"
#include "../UI/ImageButton.h"
#include "../UI/SmallCounter.h"
#include "../UI/TextArea.h"
#include "../VM/VM.h"

// Third party includes

namespace Falltergeist
{
namespace State
{

Location::Location() : State()
{
    auto game = Game::getInstance();
    game->mouse()->setState(Mouse::ACTION);

    _camera = new LocationCamera(game->renderer()->width(), game->renderer()->height(), 0, 0);
    _floor = new TileMap();
    _roof = new TileMap();
    _hexagonGrid = new HexagonGrid();

    _hexagonInfo = new TextArea("", game->renderer()->width() - 135, 25);
    _hexagonInfo->setHorizontalAlign(TextArea::HORIZONTAL_ALIGN_RIGHT);

}

Location::~Location()
{
    delete _hexagonGrid;
    delete _camera;
    delete _floor;
    delete _roof;
    delete _locationScript;
    delete _hexagonInfo;
}

void Location::init()
{
    if (initialized()) return;
    State::init();

    setFullscreen(true);
    setModal(true);

    auto game = Game::getInstance();
    setLocation("maps/" + game->settings()->initialLocation() + ".map");
    game->pushState(new PlayerPanel());
}

void Location::setLocation(std::string name)
{
    auto mapFile = ResourceManager::mapFileType(name);

    if (mapFile == nullptr)
    {
        Logger::warning() << "No such map: `" << name << "`; using default map" << std::endl;
        mapFile = ResourceManager::mapFileType("maps/" + Settings::defaultInitLocation() + ".map");
    }

    _currentElevation = mapFile->defaultElevation();

    // Set camera position on default
    camera()->setXPosition(hexagonGrid()->at(mapFile->defaultPosition())->x());
    camera()->setYPosition(hexagonGrid()->at(mapFile->defaultPosition())->y());

    // Initialize MAP vars
    if (mapFile->MVARS()->size() > 0)
    {
        auto name = mapFile->name();
        auto gam = ResourceManager::gamFileType("maps/" + name.substr(0, name.find(".")) + ".gam");
        for (auto mvar : *gam->MVARS())
        {
            _MVARS.push_back(mvar.second);
        }
    }

    auto mapObjects = mapFile->elevations()->at(_currentElevation)->objects();

    // @todo remove old objects from hexagonal grid
    for (auto mapObject : *mapObjects)
    {
        auto object = Game::GameObjectFactory::createObject(mapObject->PID());
        if (!object)
        {
            Logger::error() << "Location::setLocation() - cant create object with PID: " << mapObject->PID() << std::endl;
            continue;
        }

        object->setFID(mapObject->FID());
        object->setElevation(_currentElevation);
        object->setOrientation(mapObject->orientation());

        if (auto exitGrid = dynamic_cast<Game::GameExitMiscObject*>(object))
        {
            exitGrid->setExitMapNumber(mapObject->exitMap());
            exitGrid->setExitElevationNumber(mapObject->exitElevation());
            exitGrid->setExitHexagonNumber(mapObject->exitPosition());
            exitGrid->setExitDirection(mapObject->exitOrientation());
        }

        if (auto container = dynamic_cast<Game::GameContainerItemObject*>(object))
        {
            for (auto child : *mapObject->children())
            {
                auto item = dynamic_cast<Game::GameItemObject*>(Game::GameObjectFactory::createObject(child->PID()));
                if (!item)
                {
                    Logger::error() << "Location::setLocation() - cant create object with PID: " << child->PID() << std::endl;
                    continue;
                }
                item->setAmount(child->ammount());
                container->inventory()->push_back(item);
            }
        }

        if (mapObject->scriptId() > 0)
        {
            auto intFile = ResourceManager::intFileType(mapObject->scriptId());
            if (intFile) object->setScript(new VM(intFile,object));
        }
        if (mapObject->mapScriptId() > 0 && mapObject->mapScriptId() != mapObject->scriptId())
        {
            auto intFile = ResourceManager::intFileType(mapObject->mapScriptId());
            if (intFile) object->setScript(new VM(intFile, object));
        }

        auto hexagon = hexagonGrid()->at(mapObject->hexPosition());
        Location::moveObjectToHexagon(object, hexagon);

        _objects.push_back(object);
    }

    // Adding dude
    {
        auto player = Game::getInstance()->player();
        // Just for testing
        {
            auto armor = Game::GameObjectFactory::createObject(0x00000003); // powered armor
            player->setArmorSlot(dynamic_cast<Game::GameArmorItemObject*>(armor));
            auto leftHand = Game::GameObjectFactory::createObject(0x0000000C); // minigun
            player->setLeftHandSlot(dynamic_cast<Game::GameWeaponItemObject*>(leftHand));
            auto rightHand = Game::GameObjectFactory::createObject(0x00000007); // spear
            player->setRightHandSlot(dynamic_cast<Game::GameWeaponItemObject*>(rightHand));
        }
        player->setPID(0x01000001);
        player->setFID(FID_HERO_MALE);
        player->setOrientation(mapFile->defaultOrientation());

        // Player script
        player->setScript(new VM(ResourceManager::intFileType(0), player));

        auto hexagon = hexagonGrid()->at(mapFile->defaultPosition());
        Location::moveObjectToHexagon(player, hexagon);

        _objects.push_back(player);
    }

    // Location script
    if (mapFile->scriptId() > 0)
    {
        _locationScript = new VM(ResourceManager::intFileType(mapFile->scriptId()-1), Game::getInstance()->locationState());
    }

    // Generates floor and roof images
    {

        for (unsigned int i = 0; i != 100*100; ++i)
        {
            unsigned int tileX = std::ceil(((double)i)/100);
            unsigned int tileY = i%100;
            unsigned int x = (100 - tileY - 1)*48 + 32*(tileX - 1);
            unsigned int y = tileX*24 +(tileY - 1)*12 + 1;

            unsigned int tileNum = mapFile->elevations()->at(_currentElevation)->floorTiles()->at(i);
            if (tileNum > 1)
            {
                auto tile = new Tile(tileNum, x, y);
                _floor->tiles()->push_back(tile);
            }

            tileNum = mapFile->elevations()->at(_currentElevation)->roofTiles()->at(i);
            if (tileNum > 1)
            {
                auto tile = new Tile(tileNum, x, y - 104);
                _roof->tiles()->push_back(tile);
            }
        }
    }
}

void Location::onMouseDown(Event* event, Game::GameObject* object)
{
    if (!object) return;

    std::vector<int> icons;

    if (object->script() && object->script()->hasFunction("use_p_proc"))
    {
        icons.push_back(Mouse::ICON_USE);
    }
    else if (dynamic_cast<Game::GameDoorSceneryObject*>(object))
    {
        icons.push_back(Mouse::ICON_USE);
    }
    else if (dynamic_cast<Game::GameContainerItemObject*>(object))
    {
        icons.push_back(Mouse::ICON_USE);
    }

    switch(object->type())
    {
        case Game::GameObject::TYPE_ITEM:
            break;
        case Game::GameObject::TYPE_DUDE:
            icons.push_back(Mouse::ICON_ROTATE);
            break;
        case Game::GameObject::TYPE_SCENERY:
            break;
        case Game::GameObject::TYPE_CRITTER:
            icons.push_back(Mouse::ICON_TALK);
            break;
        default:
            return;
    }
    icons.push_back(Mouse::ICON_LOOK);
    icons.push_back(Mouse::ICON_INVENTORY);
    icons.push_back(Mouse::ICON_SKILL);
    icons.push_back(Mouse::ICON_CANCEL);

    auto state = new CursorDropdown(icons);
    state->setObject(object);
    auto game = Game::getInstance();
    game->pushState(state);
    event->setHandled(true);
}

void Location::onBackgroundClick(MouseEvent* event)
{
}

void Location::onObjectClick(MouseEvent* event)
{
}

void Location::onKeyUp(std::shared_ptr<KeyboardEvent> event)
{
}

void Location::render()
{
    _floor->render();
    for (auto hexagon : *hexagonGrid()->hexagons())
    {
        hexagon->setInRender(false);
        for (auto object : *hexagon->objects())
        {
            object->render();
            if (object->inRender())
            {
                hexagon->setInRender(true);
            }
        }
    }
    //_roof->render();
    if (active())
    {
        _hexagonInfo->render();
    }
}

void Location::think()
{
    Game::getInstance()->gameTime()->think();

    for (auto object : _objects)
    {
        object->think();
    }

    // location scrolling
    if (_scrollTicks + 10 < SDL_GetTicks())
    {
        _scrollTicks = SDL_GetTicks();
        unsigned int scrollDelta = 5;

        //Game::getInstance()->mouse()->setType(Mouse::ACTION);

        camera()->setXPosition(camera()->xPosition() + (_scrollLeft ? -scrollDelta : (_scrollRight ? scrollDelta : 0)));
        camera()->setYPosition(camera()->yPosition() + (_scrollTop ? -scrollDelta : (_scrollBottom ? scrollDelta : 0)));

        auto mouse = Game::getInstance()->mouse();

        // if scrolling is active
        if (_scrollLeft || _scrollRight || _scrollTop || _scrollBottom)
        {
            if (mouse->scrollState()) mouse->popState();

            unsigned int state;
            if (_scrollLeft)   state = Mouse::SCROLL_W;
            if (_scrollRight)  state = Mouse::SCROLL_E;
            if (_scrollTop)    state = Mouse::SCROLL_N;
            if (_scrollBottom) state = Mouse::SCROLL_S;
            if (_scrollLeft && _scrollTop)     state = Mouse::SCROLL_NW;
            if (_scrollLeft && _scrollBottom)  state = Mouse::SCROLL_SW;
            if (_scrollRight && _scrollTop)    state = Mouse::SCROLL_NE;
            if (_scrollRight && _scrollBottom) state = Mouse::SCROLL_SE;
            mouse->pushState(state);
        }
        // scrolling is not active
        else
        {
            if (mouse->scrollState())
            {
                mouse->popState();
            }
        }

    }
    if (_locationEnter)
    {
        _locationEnter = false;

        if (_locationScript) _locationScript->initialize();

        for (auto object : _objects)
        {
            if (object->script())
            {
                object->script()->initialize();
            }
        }

        if (_locationScript) _locationScript->call("map_enter_p_proc");

        // By some reason we need to use reverse iterator to prevent scripts problems
        // If we use normal iterators, some exported variables are not initialized on the moment
        // when script is called
        for (auto it = _objects.rbegin(); it != _objects.rend(); ++it)
        {
            auto object = *it;
            if (object->script())
            {
                object->script()->call("map_enter_p_proc");
            }
        }
    }
    else
    {
        if (_scriptsTicks + 10000 < SDL_GetTicks())
        {
            _scriptsTicks = SDL_GetTicks();
            if (_locationScript)
            {
                _locationScript->call("map_update_p_proc");
            }
            for (auto object : _objects)
            {
                if (object->script())
                {
                    object->script()->call("map_update_p_proc");
                }
            }
        }
    }
}

void Location::handle(Event* event)
{
    auto game = Game::getInstance();

    if (auto mouseEvent = dynamic_cast<MouseEvent*>(event))
    {
        auto mouse = Game::getInstance()->mouse();

        // Right button pressed
        if (mouseEvent->name() == "mouseup" && mouseEvent->rightButton())
        {
            switch (mouse->state())
            {
                case Mouse::ACTION:
                {
                    auto hexagon = hexagonGrid()->hexagonAt(mouse->x() + camera()->x(), mouse->y() + camera()->y());
                    if (!hexagon)
                    {
                        break;
                    }
                    mouse->pushState(Mouse::HEXAGON_RED);
                    mouse->ui()->setX(hexagon->x() - camera()->x());
                    mouse->ui()->setY(hexagon->y() - camera()->y());
                    break;
                }
                case Mouse::HEXAGON_RED:
                {
                    mouse->popState();
                    break;
                }
            }
            event->setHandled(true);
        }

        // Left button down
        if (mouseEvent->name() == "mousedown" && mouseEvent->leftButton())
        {
            switch (mouse->state())
            {
                case Mouse::HEXAGON_RED:
                    // Preventing dropdown state
                    event->setHandled(true);
                    break;
            }
        }
        // Left button up
        if (mouseEvent->name() == "mouseup" && mouseEvent->leftButton())
        {
            switch (mouse->state())
            {
                case Mouse::HEXAGON_RED:
                {
                    // Here goes the movement
                    auto hexagon = hexagonGrid()->hexagonAt(mouse->x() + camera()->x(), mouse->y() + camera()->y());
                    if (!hexagon)
                    {
                        break;
                    }

                    auto path = hexagonGrid()->findPath(game->player()->hexagon(), hexagon);
                    if (path.size())
                    {
                        game->player()->movementQueue()->clear();
                        game->player()->setRunning(game->settings()->running());
                        for (auto hexagon : path)
                        {
                            game->player()->movementQueue()->push_back(hexagon);
                        }
                        //moveObjectToHexagon(game->player(), hexagon);
                    }
                    event->setHandled(true);
                    break;
                }
            }
        }

        if (mouseEvent->name() == "mousemove")
        {
            auto hexagon = hexagonGrid()->hexagonAt(mouse->x() + camera()->x(), mouse->y() + camera()->y());

            switch (mouse->state())
            {
                case Mouse::HEXAGON_RED:
                {
                    if (!hexagon)
                    {
                        break;
                    }
                    mouse->ui()->setX(hexagon->x() - camera()->x());
                    mouse->ui()->setY(hexagon->y() - camera()->y());
                    break;
                }
            }

            unsigned int scrollArea = 8;
            _scrollLeft = mouseEvent->x() < scrollArea ? true : false;
            _scrollRight = mouseEvent->x() > game->renderer()->width()- scrollArea ? true : false;
            _scrollTop = mouseEvent->y() < scrollArea ? true : false;
            _scrollBottom = mouseEvent->y() > game->renderer()->height() - scrollArea ? true : false;

            if (hexagon)
            {
                std::string text = "Hex number: " + std::to_string(hexagon->number()) + "\n";
                text += "Hex position: " + std::to_string(hexagon->number()%200) + "," + std::to_string((unsigned int)(hexagon->number()/200)) + "\n";
                text += "Hex coords: " + std::to_string(hexagon->x()) + "," + std::to_string(hexagon->y());
                _hexagonInfo->setText(text);
            }
            else
            {
                _hexagonInfo->setText("No hex");
            }
        }
    }

    if (auto keyboardEvent = dynamic_cast<KeyboardEvent*>(event))
    {
        if (event->name() == "keyup")
        {
            switch (keyboardEvent->keyCode())
            {
                // JUST FOR ANIMATION TESTING
                case SDLK_r:
                {
                    game->player()->setRunning(!game->player()->running());
                    break;
                }
            }
        }
    }
    for (auto it = hexagonGrid()->hexagons()->rbegin(); it != hexagonGrid()->hexagons()->rend(); ++it)
    {
        auto hexagon = *it;
        if (!hexagon->inRender()) continue;
        for (auto itt = hexagon->objects()->rbegin(); itt != hexagon->objects()->rend(); ++itt)
        {
            auto object = *itt;
            if (event->handled()) return;
            if (!object->inRender()) continue;
            object->handle(event);
        }
    }
}

void Location::onKeyboardUp(std::shared_ptr<KeyboardEvent> event)
{
    if (event->keyCode() == SDLK_F10)
    {
        Game::getInstance()->pushState(new ExitConfirm());
        //event->setHandled(true);
    }
}

LocationCamera* Location::camera()
{
    return _camera;
}

void Location::setMVAR(unsigned int number, int value)
{
    if (number >= _MVARS.size())
    {
        throw Exception("Location::setMVAR(num, value) - num out of range: " + std::to_string((int)number));
    }
    _MVARS.at(number) = value;
}

int Location::MVAR(unsigned int number)
{
    if (number >= _MVARS.size())
    {
        throw Exception("Location::MVAR(num) - num out of range: " + std::to_string((int)number));
    }
    return _MVARS.at(number);
}

std::map<std::string, VMStackValue*>* Location::EVARS()
{
    return &_EVARS;
}

void Location::moveObjectToHexagon(Game::GameObject* object, Hexagon* hexagon)
{
    auto oldHexagon = object->hexagon();
    if (oldHexagon)
    {
        for (auto it = oldHexagon->objects()->begin(); it != oldHexagon->objects()->end(); ++it)
        {
            if (*it == object)
            {
                oldHexagon->objects()->erase(it);
                break;
            }
        }

        /* JUST FOR EXIT GRIDS TESTING
        for (auto obj : *hexagon->objects())
        {
            if (auto exitGrid = dynamic_cast<GameExitMiscObject*>(obj))
            {
                auto &debug = Logger::critical("LOCATION");
                debug << " PID: 0x" << std::hex << exitGrid->PID() << std::dec << std::endl;
                debug << " name: " << exitGrid->name() << std::endl;
                debug << " exitMapNumber: " << exitGrid->exitMapNumber() << std::endl;
                debug << " exitElevationNumber: " << exitGrid->exitElevationNumber() << std::endl;
                debug << " exitHexagonNumber: " << exitGrid->exitHexagonNumber() << std::endl;
                debug << " exitDirection: " << exitGrid->exitDirection() << std::endl << std::endl;
            }
        }
        */
    }    

    object->setHexagon(hexagon);
    hexagon->objects()->push_back(object);
}

void Location::handleAction(Game::GameObject* object, int action)
{
    switch (action)
    {
        case Mouse::ICON_LOOK:
        {
            object->description_p_proc();
            break;
        }
        case Mouse::ICON_USE:
        {
            auto player = Game::getInstance()->player();
            auto animation = player->setActionAnimation("al");
            animation->addEventHandler("actionFrame", [object, player](Event* event){ object->onUseAnimationActionFrame(event, player); });
            break;
        }
        case Mouse::ICON_ROTATE:
        {
            auto dude = dynamic_cast<Game::GameDudeObject*>(object);
            if (!dude) throw Exception("Location::handleAction() - only Dude can be rotated");

            int orientation = dude->orientation() + 1;
            if (orientation > 5) orientation = 0;
            dude->setOrientation(orientation);

            break;
        }
        case Mouse::ICON_TALK:
        {
            if (auto critter = dynamic_cast<Game::GameCritterObject*>(object))
            {
                critter->talk_p_proc();
            }
            else
            {
                throw Exception("Location::handleAction() - can talk only with critters!");
            }
        }

    }
}

HexagonGrid* Location::hexagonGrid()
{
    return _hexagonGrid;
}

}
}
