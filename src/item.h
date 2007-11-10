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
#ifndef ITEM_H_
#define ITEM_H_

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

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
    * @param color color to when drawing item
    * @param hover_color color to use when drawing selected item
    * @return newly created surface
    */
   virtual SDL_Surface* draw(TTF_Font* font, SDL_Color color, SDL_Color hover_color)
         const = 0;
   
   /**
    * Generates a snapshot for the item
    * @return newly created surface, or NULL if no snapshot
    */
   virtual SDL_Surface* snapshot() = 0;
};

} // end namespace

#endif
