// ==================================================
//Copyright (C) 2004-2006 Intel Corporation
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

/**
  * @file  MDIWindow.h
  * @brief 
  */


#ifndef _MDIWINDOW_H
#define _MDIWINDOW_H

#include <q3mainwindow.h>
//Added by qt3to4:
#include <QCloseEvent>
#include "AScrollView.h"

class MDIWindow: public Q3MainWindow
{
    Q_OBJECT
public:
    MDIWindow( QWidget* parent, const char* name, int wflags );
    ~MDIWindow();

    virtual bool init();
    virtual inline AScrollView* getAScrollView();
    
protected:
    void closeEvent( QCloseEvent * );

protected:
    AScrollView *asv;
};

AScrollView* 
MDIWindow::getAScrollView()
{ return asv; } 
    
#endif

