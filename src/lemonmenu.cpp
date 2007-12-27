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
#include "lemonmenu.h"
#include "game.h"
#include "options.h"
#include "error.h"

#include <cstring>
#include <sqlite3.h>
#include <sstream>
#include <algorithm>
#include <SDL/SDL_rotozoom.h>

#define UPDATE_SNAP_EVENT 1

using namespace ll;
using namespace std;

/**
 * Function executed after a timeout for finding snapshot images
 */
static Uint32 snap_timer_callback(Uint32 interval, void *param);

/**
 * Function executed for each record returned from games list queries
 */
int sql_callback(void *obj, int argc, char **argv, char **colname);

/**
 * Compares the text property of two item pointers and returns true if the left
 * is less than the right.
 */
bool cmp_item(item* left, item* right)
{ return strcmp(left->text(), right->text()) < 0; }

lemon_menu::lemon_menu(lemonui* ui) :
   _top(NULL), _current(NULL), _show_hidden(false), _snap_timer(0),
   _snap_delay(g_opts.get_int(KEY_SNAPSHOT_DELAY))
{
   _layout = ui;
   change_view(genre);
}

lemon_menu::~lemon_menu()
{
   delete _top; // delete top menu will propigate to children
}

void lemon_menu::render()
{
   _layout->render(_current);  // pass off rendering to layout class
}

void lemon_menu::main_loop()
{
   log << info << "main_loop: starting render loop" << endl;

   render();
   reset_snap_timer();

   const int exit_key = g_opts.get_int(KEY_KEYCODE_EXIT);
   const int up_key = g_opts.get_int(KEY_KEYCODE_UP);
   const int down_key = g_opts.get_int(KEY_KEYCODE_DOWN);
   const int pgup_key = g_opts.get_int(KEY_KEYCODE_PGUP);
   const int pgdown_key = g_opts.get_int(KEY_KEYCODE_PGDOWN);
   const int reload_key = g_opts.get_int(KEY_KEYCODE_RELOAD);
   const int toggle_key = g_opts.get_int(KEY_KEYCODE_TOGGLE);
   const int select_key = g_opts.get_int(KEY_KEYCODE_SELECT);
   const int back_key = g_opts.get_int(KEY_KEYCODE_BACK);
   const int alphamod = g_opts.get_int(KEY_KEYCODE_ALPHAMOD);

   _running = true;
   while (_running) {
      SDL_Event event;
      SDL_WaitEvent(&event);

      SDLKey key = event.key.keysym.sym;
      SDLMod mod = event.key.keysym.mod;

      switch (event.type) {
      case SDL_QUIT:
         _running = false;

         break;
      case SDL_KEYUP:
         if (key == exit_key) {
            _running = false;
         } else if (key == select_key) {
            handle_activate();
         } else if (key == back_key) {
            handle_up_menu();
         } else if (key == reload_key) {
            //load_menus();
            //TODO implement this
            render();
         } else if (key == toggle_key) {
            handle_show_hide();
         }

         break;
      case SDL_KEYDOWN:
         if (key == up_key) {
            handle_up();
         } else if (key == down_key) {
            handle_down();
         } else if (key == pgup_key && mod & alphamod) {
            handle_alphadown();
         } else if (key == pgdown_key && mod & alphamod) {
            handle_alphaup();
         } else if (key == pgup_key) {
            handle_pgup();
         } else if (key == pgdown_key) {
            handle_pgdown();
         }

         break;
      case SDL_USEREVENT:
         if (event.user.code == UPDATE_SNAP_EVENT)
            update_snap();

         break;
      }
   }

   reset_snap_timer();
}

void lemon_menu::handle_up()
{
   // ignore event if already at the top of menu
   if (_current->select_previous()) {
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_down()
{
   // ignore event if already at the bottom of menu
   if (_current->select_next()) {
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_pgup()
{
   // ignore event if already at the top of menu
   if (_current->select_previous(_layout->page_size())) {
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_pgdown()
{
   // ignore event if already at the bottom of menu
   if (_current->select_next(_layout->page_size())) {
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_alphaup()
{
   if (_current->select_next_alpha()) {
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_alphadown()
{
   if (_current->select_previous_alpha()) {
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_activate()
{
   if (!_current->has_children()) return;

   item* item = _current->selected();
   if (typeid(menu) == typeid(*item)) {
      handle_down_menu();
   } else if (typeid(game) == typeid(*item)) {
      handle_run();
   }
}

void lemon_menu::handle_run()
{
   game* g = (game*)_current->selected();
   string cmd(g_opts.get_string(KEY_MAME_PATH));

   log << info << "handle_run: launching game " << g->text() << endl;

   bool full = g_opts.get_bool(KEY_FULLSCREEN);
   SDL_Surface* screen = SDL_GetVideoSurface();
   
   // this is required when lemon launcher is full screen for some reason
   // otherwise mame freezes and all the processes have to be kill manually
   if (full) SDL_WM_ToggleFullScreen(screen);

   size_t pos = cmd.find("%r");
   if (pos == string::npos)
      throw bad_lemon("mame path missing %r specifier");

   cmd.replace(pos, 2, g->rom());

   log << debug << "handle_run: " << cmd << endl;

   system(cmd.c_str());

   if (full) SDL_WM_ToggleFullScreen(screen);

   // clear the event queue
   SDL_Event event;
   while (SDL_PollEvent(&event));

   render();
}

void lemon_menu::handle_up_menu()
{
   if (_current != _top) {
      _current = (menu*)_current->parent();
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_down_menu()
{
   _current = (menu*)_current->selected();
   reset_snap_timer();
   render();
}

void lemon_menu::handle_show_hide()
{
   log << info << "handle_show_hide: changing hidden status" << endl;

   _show_hidden = !_show_hidden;
   change_view(genre);
   render();
}

void lemon_menu::update_snap()
{
   if (_current->has_children()) {
      item* item = _current->selected();
      _layout->snap(item->snapshot());
      render();
   }
}

void lemon_menu::reset_snap_timer()
{
   if (_snap_timer)
      SDL_RemoveTimer(_snap_timer);

   // schedule timer to run in 500 milliseconds
   _snap_timer = SDL_AddTimer(_snap_delay, snap_timer_callback, NULL);
}

void lemon_menu::change_view(view_t view)
{
   _view = view;
   
   // free previous menu
   if (_top != NULL)
      delete _top;
   
   // create new top menu
   _current = _top = new menu(view_names[_view]);
   
   string query("SELECT rom, name, params, genre FROM games");
   
   switch (_view) {
   case favorite:
      query.append(" ORDER BY name");
      query.append(" WHERE fav = 1");
      break;
      
   case most_played:
      query.append(" ORDER BY count, name");
      query.append(" WHERE count > 0");
      break;
      
   case genre:
      query.append(" GROUP BY genre");
      query.append(" ORDER BY genre, name");
      break;
   }
   
   string db_file("games.db");
   g_opts.resolve(db_file);
   
   sqlite3 *db = NULL;
   char* error_msg = NULL;
   
   try {
      if (sqlite3_open(db_file.c_str(), &db))
         throw bad_lemon(sqlite3_errmsg(db));
         //throw bad_lemon("can't open database " << sqlite3_errmsg(db) << endl;
      
      if (sqlite3_exec(db, query.c_str(), &sql_callback,
            (void*)this, &error_msg) != SQLITE_OK)
         throw bad_lemon(error_msg);
         //throw bad_lemon("sql error " << error_msg << endl;
   } catch (...) {
      sqlite3_free(error_msg);
      sqlite3_close(db);
      throw;
   }
}

int sql_callback(void* obj, int argc, char **argv, char **colname)
{
   lemon_menu* lm = (lemon_menu*)obj;
   menu* top = lm->top();
   
   game* g = new game(argv[0], argv[1], argv[2]);
   
   switch (lm->view()) {
   case favorite:
   case most_played:
      top->add_child(g);
      
      break;
      
   case genre:
      menu* m;
      if (!top->has_children()) {
         // if top menu doesn't have a menu yet, create one for the genre
         m = new menu(argv[3]);
         top->add_child(m);
      } else {
         // otherwise use the last menu, and create a new one if necessary
         m = (menu*)*top->last();
         
         if (strcmp(m->text(), argv[3]) != 0) {
            m = new menu(argv[3]);
            top->add_child(m);
         }
      }
      
      m->add_child(g);
      
      break;
   }
}

Uint32 snap_timer_callback(Uint32 interval, void *param)
{
   SDL_Event evt;
   evt.type = SDL_USEREVENT;
   evt.user.code = UPDATE_SNAP_EVENT;

   SDL_PushEvent(&evt);

   return 0;
}
