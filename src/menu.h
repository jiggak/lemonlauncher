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
#ifndef MENU_H_
#define MENU_H_

#include "item.h"
#include <vector>
#include <string>

using namespace std;

namespace ll {

/**
 * Menu item class
 */
class menu : public item {
private:
   string _name; // menu name
   vector<item*> _children; // array of children
   int _selected; // index of selected child

public:
   menu(const char* name) :
      _name(name), _selected(0) { }
   
   virtual ~menu();

   /** Returns true if there is 1 or more children */
   const bool has_children() const
   { return ! _children.empty(); }
   
   /** Returns currently selected child */
   item* selected()
   { return _children[_selected]; }
   
   /** Returns currently selected child as a bi-directional iterator */
   vector<item*>::iterator selected_begin()
   { return _children.begin()+_selected; }

   /**
    * Attempts to select the child who is 'step' number of children
    * after currently selected child
    * @return true if at least one child was skipped
    */
   const bool select_next(int step = 1);

   /**
    * Attempts to select the child who is 'step' number of children
    * before currently selected child
    * @return true if at least one child was skipped
    */
   const bool select_previous(int step = 1);
   
   /**
    * Attempts to select the child after the currently selected child
    * in alphabetic order
    * @return true if selection has changed
    */
   const bool select_next_alpha();

   /**
    * Attempts to select the child before the currently selected child
    * in alphabetic order
    * @return true if selection has changed
    */
   const bool select_previous_alpha();

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
   
   SDL_Surface* draw(TTF_Font* font, SDL_Color color, SDL_Color hover_color) const;
   
   /* Roland's origional version would iterate through all games in the menu and
    * look for four game snapshots to be placed in a 2x2 grid.  Would be cool if
    * this was not just limited to a 2x2 grid but the performance would be horrible!
    * So, this is skipped until something better can be conjured up. */
   SDL_Surface* snapshot() { return NULL; }
};

} // end namespace

#endif
