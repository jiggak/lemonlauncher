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
#ifndef LEMONMENU_H_
#define LEMONMENU_H_

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "layout.h"
#include "menu.h"
#include "options.h"
#include "log.h"

namespace ll {

class lemon_menu {
private:
   SDL_Surface* _screen;
   SDL_Surface* _buffer;
   SDL_TimerID  _snap_timer;
   
   layout* _layout;

   bool _running;
   bool _show_hidden;

   menu* _top;
   menu* _current;
   
   const int _snap_delay;

   void load_menus();

   void render();

   void reset_snap_timer();
   void update_snap();

   void handle_up();
   void handle_down();
   void handle_pgup();
   void handle_pgdown();
   void handle_alphaup();
   void handle_alphadown();
   void handle_run();
   void handle_up_menu();
   void handle_down_menu();
   void handle_activate();
   void handle_show_hide();
   
public:
   lemon_menu(SDL_Surface* screen);
   
   ~lemon_menu();

   void main_loop();
};

} // end namespace declaration

#endif /*LEMONMENU_H_*/
