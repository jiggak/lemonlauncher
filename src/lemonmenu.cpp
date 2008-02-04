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

#ifdef __WIN32__
/* needed for special handling in launch_game function */
#include <windows.h>
#endif

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
 * Asyncronous function for launching a game
 */
int launch_game(void* data);

/**
 * Compares the text property of two item pointers and returns true if the left
 * is less than the right.
 */
bool cmp_item(item* left, item* right)
{ return strcmp(left->text(), right->text()) < 0; }

lemon_menu::lemon_menu(lemonui* ui) :
   _db(NULL), _top(NULL), _current(NULL), _show_hidden(false),
   _snap_timer(0), _snap_delay(g_opts.get_int(KEY_SNAPSHOT_DELAY))
{
   // locate games.db file in confdir
   string db_file("games.db");
   g_opts.resolve(db_file);
   
   if (sqlite3_open(db_file.c_str(), &_db))
      throw bad_lemon(sqlite3_errmsg(_db));
   
   _layout = ui;
   change_view(favorite);
}

lemon_menu::~lemon_menu()
{
   delete _top; // delete top menu will propigate to children
   
   if (_db)
      sqlite3_close(_db);
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
   const int select_key = g_opts.get_int(KEY_KEYCODE_SELECT);
   const int back_key = g_opts.get_int(KEY_KEYCODE_BACK);
   const int alphamod = g_opts.get_int(KEY_KEYCODE_ALPHAMOD);
   const int viewmod = g_opts.get_int(KEY_KEYCODE_VIEWMOD);

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
         }

         break;
      case SDL_KEYDOWN:
         if (key == up_key) {
            handle_up();
         } else if (key == down_key) {
            handle_down();
         } else if (key == pgup_key) {
            if (mod & alphamod)
               handle_alphaup();
            else if (mod & viewmod)
               handle_viewdown();
            else
               handle_pgup();
         } else if (key == pgdown_key) {
            if (mod & alphamod)
               handle_alphadown();
            else if (mod & viewmod)
               handle_viewup();
            else
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

void lemon_menu::handle_viewup()
{
   if (_view != genre) {
      change_view((view_t)(_view+1));
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_viewdown()
{
   if (_view != favorite) {
      change_view((view_t)(_view-1));
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_activate()
{
   // ignore when this isn't any children
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
   log << info << "handle_run: launching game " << g->text() << endl;
   
   string cmd(g_opts.get_string(KEY_MAME_PATH));
   size_t pos = cmd.find("%r");
   if (pos == string::npos)
      throw bad_lemon("mame path missing %r specifier");

   cmd.replace(pos, 2, g->rom());
   ll::log << debug << "handle_run: " << cmd << endl;

   // This bit of code here has been a big pain.  On linux in full screen (X11)
   // lemon launcher has to be minimized before launching mame or else things
   // tend to lock up.  On windows lemon launcher is automagically minimized
   // and must be explicitly foregrounded after mame exits.  And on Mac OS X
   // sdlmame complains it can't open the screen.
   //
   // That said, I think I have it sorted.  Simply destroying lemon launchers
   // screen and then re-creating it after mame exits seems to get rid of the
   // irregularities.

#ifdef __WIN32__
   // get handle of forground window
   HWND hw = GetForegroundWindow();
   
   int exit_code = system(cmd.c_str());
   
   // open iconified window and set it as foreground
   OpenIcon(hw);
   SetForegroundWindow(hw);
   
#else /* all other OS's */
   // destroy buffers and screen
   _layout->destroy_screen();
   
   // launch mame and hope for the best
   int exit_code = system(cmd.c_str());
   
   // create screen and render
   _layout->setup_screen();
   render();
#endif
   
   // only increment the games play counter if emulator returned success
   if (exit_code == 0) {
      
      // locate games.db file in confdir
      string db_file("games.db");
      g_opts.resolve(db_file);
   
      // create query to update number of times game has been played
      string query("UPDATE games SET count = count+1 WHERE rom = ");
      query.append("'").append(g->rom()).append("'");
   
      sqlite3* db = NULL;
      char* error_msg = NULL;
   
      try {
         if (sqlite3_open(db_file.c_str(), &db))
            throw bad_lemon(sqlite3_errmsg(db));
         
         // execute query and throw exception on error
         if (sqlite3_exec(db, query.c_str(), NULL, NULL, &error_msg)
               != SQLITE_OK)
            throw bad_lemon(error_msg);
      } catch (...) {
         sqlite3_free(error_msg);
         throw;
      }
      
      if (db)
         sqlite3_close(db);
   }
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
   
   // recurisvely free top menu / children
   if (_top != NULL)
      delete _top;
   
   // create new top menu
   _current = _top = new menu(view_names[_view]);
   
   string query("SELECT rom, name, params, genre FROM games");
   string where, order;
   
   switch (_view) {
   case favorite:
      order.append("name");
      where.append("fav = 1");
      break;
      
   case most_played:
      order.append("count,name");
      where.append("count > 0");
      break;
      
   case genre:
      order.append("genre,name");
      break;
   }
   
   if (!_show_hidden) {
      if (where.length() != 0) where.append(" AND ");
      where.append("hide = 0");
   }
   
   // assemble query
   query.append(" WHERE ").append(where);
   query.append(" ORDER BY ").append(order);
   
   log << debug << "change_view: " << query.c_str() << endl;
   
   char* error_msg = NULL;

   try {
      if (sqlite3_exec(_db, query.c_str(), &sql_callback,
            (void*)this, &error_msg) != SQLITE_OK)
         throw bad_lemon(error_msg);
   } catch (...) {
      sqlite3_free(error_msg);
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
      menu* m = NULL;
      if (!top->has_children()) {
         // if top menu doesn't have a menu yet, create one for the genre
         m = new menu(argv[3]);
         top->add_child(m);
      } else {
         // get last child of the top level menu
         vector<item*>::iterator i = top->last();
         --i; // move iterator to the last item in list
         
         m = (menu*)*i;
         
         // create new child menu of the genre strings don't match
         if (strcmp(m->text(), argv[3]) != 0) {
            m = new menu(argv[3]);
            top->add_child(m);
         }
      }
      
      m->add_child(g);
      
      break;
   }
   
   return 0;
}

Uint32 snap_timer_callback(Uint32 interval, void *param)
{
   SDL_Event evt;
   evt.type = SDL_USEREVENT;
   evt.user.code = UPDATE_SNAP_EVENT;

   SDL_PushEvent(&evt);

   return 0;
}
