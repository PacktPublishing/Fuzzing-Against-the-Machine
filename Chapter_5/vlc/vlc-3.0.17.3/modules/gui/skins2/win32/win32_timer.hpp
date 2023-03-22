/*****************************************************************************
 * win32_timer.hpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: 9ee1161d2e6c970b8471fcc5f22064c7ff2097c8 $
 *
 * Authors: Cyril Deguet     <asmax@via.ecp.fr>
 *          Olivier Teulière <ipkiss@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef WIN32_TIMER_HPP
#define WIN32_TIMER_HPP

#include "../src/os_timer.hpp"

class CmdGeneric;

// Win32 specific timer
class Win32Timer: public OSTimer
{
public:
    Win32Timer( intf_thread_t *pIntf, CmdGeneric &rCmd, HWND hWnd );
    virtual ~Win32Timer();

    /// (Re)start the timer with the given delay (in ms). If oneShot is
    /// true, stop it after the first execution of the callback.
    virtual void start( int delay, bool oneShot );

    /// Stop the timer
    virtual void stop();

    /// Execute the callback
    void execute();

private:
    /// Command to execute
    CmdGeneric &m_rCommand;

    /// Delay between two execute
    mtime_t m_interval;

    /// Flag to tell whether the timer must be stopped after the
    /// first execution
    bool m_oneShot;

    /// Handle of the window to which the timer will be attached
    HWND m_hWnd;
};


#endif
