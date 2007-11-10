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

#include "menu.h"
#include "options.h"

using namespace ll;

menu::~menu()
{
   for (vector<item*>::iterator i = _children.begin(); i != _children.end(); i++)
      delete *i;
}

const bool menu::select_next(int step)
{
   int last = _children.size()-1;
   if (_selected < last) {
      _selected = _selected + step <= last? _selected + step : last;
      return true;
   }
   
   return false;
}

const bool menu::select_previous(int step)
{
   if (_selected > 0) {
      _selected = _selected - step >= 0? _selected - step : 0;
      return true;
   }
   
   return false;
}

SDL_Surface* menu::draw(TTF_Font* font, SDL_Color color, SDL_Color hover_color) const
{
   SDL_Color c = parent() && this == ((menu*)parent())->selected()? hover_color : color;
   return TTF_RenderText_Blended(font, text(), c);
}
