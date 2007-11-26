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
#include <confuse.h>
#include <sstream>
#include <algorithm>

#define UPDATE_SNAP_EVENT 1

using namespace ll;
using namespace std;

/**
 * Timer callback function
 */
Uint32 snap_timer_callback(Uint32 interval, void *param);

/**
 * Compares the text property of two item pointers and returns true if the left
 * is less than the right.
 */
bool cmp_item(item* left, item* right)
{ return strcmp(left->text(), right->text()) < 0; }

lemon_menu::lemon_menu(SDL_Surface* screen) :
   _screen(screen), _show_hidden(false), _snap_timer(0),
   _snap_delay(g_opts.get_int(KEY_SNAPSHOT_DELAY))
{
   _layout = new layout(
         g_opts.get_string(KEY_SKIN_FILE),
         g_opts.get_int(KEY_SCREEN_WIDTH),
         g_opts.get_int(KEY_SCREEN_HEIGHT));
   
   load_menus();
   
   /*
    * Should I be using hardware accelerating?  Most docs/guides suggest no..
    * I pass 0 as the alpha mask.  Surfaces don't need an alpha channel to
    * do per-surface alpha and blitting.  In fact I can't seem to get the
    * fadded snapshot blitting to work at all if the alpha channel is set!
    */
   _buffer = SDL_CreateRGBSurface(SDL_SWSURFACE,
      _screen->w, _screen->h, 32, // w,h,bpp
      0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000); // rgba masks, for big-endian
   
   if (!_buffer)
      throw bad_lemon("lemon_menu: unable to create back buffer");

   log << info << "lemon_menu: double buffer created" << endl;
}

lemon_menu::~lemon_menu()
{
   delete _layout;
   delete _top; // delete top menu will propigate to children
   
   SDL_FreeSurface(_buffer); // free back buffer
}

void lemon_menu::load_menus()
{
   cfg_opt_t game_opts[] = {
      CFG_STR("rom", 0, CFGF_NODEFAULT),
      CFG_STR("title", 0, CFGF_NODEFAULT),
      CFG_STR("params", "", CFGF_NONE),
      CFG_END()
   };
   
   cfg_opt_t menu_opts[] = {
      CFG_BOOL("sorted", cfg_true, CFGF_NONE),
      CFG_SEC("game", game_opts, CFGF_MULTI),
      CFG_END()
   };
   
   cfg_opt_t root_opts[] = {
      CFG_SEC("menu", menu_opts, CFGF_TITLE | CFGF_MULTI),
      CFG_END()
   };
   
   //cfg_opt_t opts[] = {
   //   CFG_SEC("root", root_opts, CFGF_TITLE),
   //   CFG_END()
   //};
   
   cfg_t* cfg = cfg_init(root_opts, CFGF_NONE);
   int result = cfg_parse(cfg, g_opts.locate("games.conf"));
   
   if (result == CFG_FILE_ERROR) {
      // file error usually means file not found, warn and load empty menu
      log << warn << "load_menus: file error, using defaults" << endl;
      cfg_parse_buf(cfg, "");
   } else if (result == CFG_PARSE_ERROR) {
      throw bad_lemon("load_menus: parse error");
   }

   // only one root supported for now, should be straight forward to support more
   //cfg_t* root = cfg_getsec(cfg, "root");
   
   //_top = new menu(cfg_title(root));
   _top = new menu("Arcade Games");
   
   // iterate over menu sections   
   int menu_cnt = cfg_size(cfg, "menu");
   for (int i=0; i<menu_cnt; i++) {
      cfg_t* m = cfg_getnsec(cfg, "menu", i);
      
      const char* mtitle = cfg_title(m);
      bool sorted = cfg_getbool(m, "sorted") == cfg_true;
      
      // "should" be safe to assume title is at least one charcter long 
      if (mtitle[0] != '.' || _show_hidden) {

         // create menu and add to root menu
         menu* pmenu = new menu(mtitle);
         _top->add_child(pmenu);
         
         // iterate over the game sections
         int game_cnt = cfg_size(m, "game");
         for (int j=0; j<game_cnt; j++) {
            cfg_t* g = cfg_getnsec(m, "game", j);
            
            char* rom = cfg_getstr(g, "rom");
            char* title = cfg_getstr(g, "title");
            char* params = cfg_getstr(g, "params");
            
            if (title[0] != '.' || _show_hidden)
               pmenu->add_child(new game(rom, title, params));
         }
         
         // sort the menu alphabeticly using game/item name
         if (sorted)
            sort(pmenu->first(), pmenu->last(), cmp_item);
      }
   }
   
   // always sort top menu
   sort(_top->first(), _top->last(), cmp_item);
   
   _current = _top;
   
   cfg_free(cfg);
}

void lemon_menu::render()
{
   // render ui to buffer
   _layout->render(_buffer, _current);
   
   // update the screen
   SDL_BlitSurface(_buffer, NULL, _screen, NULL);
   SDL_UpdateRect(_screen, 0, 0, 0, 0);
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
            load_menus();
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
   
   // this is required when lemon launcher is full screen for some reason
   // otherwise mame freezes and all the processes have to be kill manually
   bool full = g_opts.get_bool(KEY_FULLSCREEN);
   if (full) SDL_WM_ToggleFullScreen(_screen);

   size_t pos = cmd.find("%r");
   if (pos == string::npos)
      throw bad_lemon("mame path missing %r specifier");
   
   cmd.replace(pos, 2, g->rom());
   
   log << debug << "handle_run: " << cmd << endl;

   system(cmd.c_str());

   if (full) SDL_WM_ToggleFullScreen(_screen);

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
   load_menus();
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

Uint32 snap_timer_callback(Uint32 interval, void *param)
{
   SDL_Event evt;
   evt.type = SDL_USEREVENT;
   evt.user.code = UPDATE_SNAP_EVENT;
   
   SDL_PushEvent(&evt);

   return 0;
}
