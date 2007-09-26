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
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include <config.h>

#include "options.h"
#include "log.h"
#include "lemonmenu.h"

using namespace ll;

int main(int argc, char** argv)
{
   options* opts = new options("lemonlauncher.conf");
   
   int level = opts->get_int(KEY_LOGLEVEL);
   log.level((log_level)level);
   log << info << "main: setting log level " << level << endl;
   
   log << info << PACKAGE_STRING << endl;
   
   // initialize sdl
   SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER);
   
   // create a screen to draw on
   SDL_Surface* screen;
   
   int xres = opts->get_int(KEY_SCREEN_WIDTH);
   int yres = opts->get_int(KEY_SCREEN_HEIGHT);
   int bits = opts->get_int(KEY_SCREEN_BPP);
   bool full = opts->get_bool(KEY_FULLSCREEN);
   
   log << info << "main: using graphics mode: " << xres <<'x'<< yres <<'x'<< bits << endl;
   
   screen = SDL_SetVideoMode(xres, yres, bits, SDL_SWSURFACE | (full ? SDL_FULLSCREEN : 0));
   if (!screen) {
      log << error << "main: unable to open screen" << endl;
      return 0;
   }

   // init the font engine
   if (TTF_Init()) {
      log << error << "main: unable to start font engine" << endl;
      return 0;
   }
   
   lemon_menu* menu = new lemon_menu(screen, opts);
   menu->main_loop();
   delete menu;
   
   // shutdown fonts
   TTF_Quit();

   // shutdown sdl
   SDL_Quit();
   
   return 0;
}
