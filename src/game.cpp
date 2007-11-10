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

#include <SDL/SDL_image.h>
#include "game.h"
#include "menu.h"
#include "options.h"
#include "error.h"
#include "log.h"

using namespace ll;
using namespace std;

SDL_Surface* game::snapshot()
{
   string img(g_opts.get_string(KEY_MAME_SNAP_PATH));
   
   size_t pos = img.find("%r");
   if (pos == string::npos) {
      log << warn << "game::snapshot: snap option missing %r specifier" << endl;
      return NULL;
   }
   
   img.replace(pos, 2, rom());
   
   log << debug << "game::snapshot: " << img << endl;

   return IMG_Load(img.c_str());
}

SDL_Surface* game::draw(TTF_Font* font, SDL_Color color, SDL_Color hover_color) const
{
   SDL_Color c = this == ((menu*)parent())->selected()? hover_color : color;
   return TTF_RenderText_Blended(font, text(), c);
}
