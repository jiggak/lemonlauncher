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

#include "lemonui.h"
#include "menu.h"
#include "options.h"
#include "log.h"

namespace ll {

typedef enum { favorite, most_played, genre } view_t;
static char* view_names[] = {
      "Favorites", "Most Played", "Genres"
};

class lemon_menu {
private:
   lemonui* _layout;

   bool _running;
   bool _show_hidden;

   menu* _top;
   menu* _current;
   view_t _view;
   
   const int _snap_delay;
   SDL_TimerID  _snap_timer;

   void render();

   void reset_snap_timer();
   void update_snap();
   void change_view(view_t view);

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
   lemon_menu(lemonui* ui);
   
   ~lemon_menu();

   void main_loop();
   
   menu* top() const
   { return _top; }
   
   const view_t view() const
   { return _view; }
};

} // end namespace declaration

#endif /*LEMONMENU_H_*/
