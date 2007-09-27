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
#include <vector>
#include <string>

#include "options.h"
#include "log.h"

using namespace std;

namespace ll {

/**
 * Base class for all drawable items (game, menu, etc)
 */
class item {
protected:
   item* _parent;

public:
   /** Creates an item with no parent */
   item() : _parent(NULL) { }
   
   /** Make sure those sub-classes get deleted */
   virtual ~item() { }
   
   /** Creats an item and sets the parent */
   item(item* parent) : _parent(parent) { }

   /** Returns true if this item is a child item */
   bool has_parent() const
   { return _parent != NULL; }

   /** Returns pointer to the parent item */
   item* parent() const
   { return _parent; }
   
   /** Sets parent pointer item */
   void parent(item* parent)
   { _parent = parent; }
   
   /** Returns textual representation of this item */
   virtual const char* text() const = 0;
   
   /**
    * Draws the item and returns the result as a surface
    * @param font font used for text drawing
    * @param width available width for drawing
    * @return newly created surface
    */
   virtual SDL_Surface* draw(TTF_Font* font) const = 0;
};

/**
 * Menu item class
 */
class menu : public item {
private:
   string _name; // menu name
   vector<item*> _children; // array of children
   int _selected; // index of selected child

public:
   menu(const char* name) : _name(name), _selected(0) { }
   
   virtual ~menu()
   {
      for (vector<item*>::iterator i = _children.begin(); i != _children.end(); i++) {
         delete *i;
      }
   }

   /**
    * Returns currently selected child
    */
   item* selected()
   { return _children[_selected]; }
   
   /**
    * Returns currently selected child as a bi-directional iterator
    */
   vector<item*>::iterator selected_begin()
   { return _children.begin()+_selected; }

   /**
    * Attempts to select the child who is 'step' number of children ahead of current
    * @return true if at least one child was skipped
    */
   const bool select_next(int step = 1)
   {
      int last = _children.size()-1;
      if (_selected < last) {
         _selected = _selected + step <= last? _selected + step : last;
         return true;
      }
      
      return false;
   }

   /**
    * Attempts to select the child who is 'step' number of children behind of current
    * @return true if at least one child was skipped
    */
   const bool select_previous(int step = 1)
   {
      if (_selected > 0) {
         _selected = _selected - step >= 0? _selected - step : 0;
         return true;
      }
      
      return false;
   }

   /**
    * Returns iterator to first child item
    */
   vector<item*>::iterator first()
   { return _children.begin(); }
   
   /**
    * Returns iterator to one past last child item
    */
   vector<item*>::iterator last()
   { return _children.end(); }
   
   /** Appends the child item to the end of the children list */
   void add_child(item* item)
   {
      item->parent(this);
      _children.push_back(item);
   }

   /** Return menu name as item text */
   const char* text() const
   { return _name.c_str(); }
   
   SDL_Surface* draw(TTF_Font* font) const;
};

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
      _rom(rom), _name(name), _params(params) { }

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
   
   SDL_Surface* draw(TTF_Font* font) const;
};

class lemon_menu {
private:
   options* _opts;

   SDL_Surface* _screen;
   SDL_Surface* _buffer;
   SDL_Surface* _snap;
   SDL_TimerID  _snap_timer;
   
   TTF_Font* _title_font;
   TTF_Font* _list_font;

   bool _running;
   bool _show_hidden;

   menu* _top;
   menu* _current;
   
   const int _page_size;

   void load_menus();

   void render();

   void reset_snap_timer();
   void update_snap();
   void get_game_snap();
   void get_menu_snap();

   void handle_up();
   void handle_down();
   void handle_pgup();
   void handle_pgdown();
   void handle_run();
   void handle_snap();
   void handle_up_menu();
   void handle_down_menu();
   void handle_activate();
   void handle_show_hide();
   
public:
   lemon_menu(SDL_Surface* screen, options* opts);
   
   ~lemon_menu();

   void main_loop();
};

} // end namespace declaration

#endif /*LEMONMENU_H_*/
