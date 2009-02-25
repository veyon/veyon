//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.
// Class was copied from another project...but can find it back
// If someone find it please message me to add it..

#ifndef _TIMER_H

#include <stdio.h>
#include <winsock.h>
#include <time.h>

inline void gettimeofday( struct timeval *t, void* )
{
  t->tv_sec = 0;
  t->tv_usec = clock();
}


class Timer
{

 public:

  Timer();
  virtual ~Timer(){};

  inline double frame()
    {
      if( !stopped )
        gettimeofday( &endT, 0 );
      else
	{
	  frameT.tv_sec = endT.tv_sec;
	  frameT.tv_usec = endT.tv_usec;
	}

      secs  = endT.tv_sec - frameT.tv_sec;
      usecs = endT.tv_usec - frameT.tv_usec;

      frameT.tv_sec = endT.tv_sec;
      frameT.tv_usec = endT.tv_usec;

      return( secs + ( usecs * tScale ) );
    }

  //! Reset the timer to \b newVal.
  inline void reset( float newVal = 0.0f )
    {
      total = newVal;
      if( !stopped )
	gettimeofday( &startT, 0 );
    }

  //! Read the current elapsed time (in seconds).
  inline double read()
    {
      if( !stopped )
	gettimeofday( &endT, 0 );
      else
	{
	  endT.tv_sec = startT.tv_sec;
	  endT.tv_usec = startT.tv_usec;
	}

      secs  = endT.tv_sec - startT.tv_sec;
      usecs = endT.tv_usec - startT.tv_usec;
      return( secs + ( usecs * tScale ) + total );
    }

  //! Start the timer.
  inline void start()
    {
      stopped = false;
      gettimeofday( &startT, 0 );
      frameT.tv_sec = startT.tv_sec;
      frameT.tv_usec = startT.tv_usec;
    }

  //! Stop the timer.
  inline void stop()
    {
      gettimeofday( &endT, 0 );
      secs  = endT.tv_sec - startT.tv_sec;
      usecs = endT.tv_usec - startT.tv_usec;
      total += ( secs + ( usecs * tScale) );
      frameT.tv_sec = endT.tv_sec;
      frameT.tv_usec = endT.tv_usec;
      stopped = true;
    }

 private:

  timeval endT, frameT, startT;
  double secs, usecs, tScale, total;
  bool stopped;

}; /* end class Timer */

#define _TIMER_H
#endif
