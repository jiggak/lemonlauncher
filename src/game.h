/*
 * Copyright 2007 Josh Kropf
 * 
 * This file is part of Lemon Launcher.
 * 
 * Lemon Launcher is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Lemon Launcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Lemon Launcher; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef GAME_H_
#define GAME_H_

#include "item.h"
#include <string>

using namespace std;

namespace ll {

/**
 * Game item class
 */
class game : public item {
private:
   string _rom;    // rom name
   string _name;   // game name
   string _params; // game specific mame parameters

public:
   game(const char* rom, const char* name, const char* params) :
      _rom(rom), _name(name), _params(params != NULL? params : "") { }

   virtual ~game() { }
   
   /** Returns the rom name */
   const char* rom() const
   { return _rom.c_str(); }

   /** Returns mame parameters (if any) */
   const char* params() const
   { return _params.c_str(); }

   /** Returns game name as item text */
   const char* text() const
   { return _name.c_str(); }
   
   SDL_Surface* draw(TTF_Font* font, SDL_Color color, SDL_Color hover_color) const;
   SDL_Surface* snapshot();
};

} // end namespace

#endif
