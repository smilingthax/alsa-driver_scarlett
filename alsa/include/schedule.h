#ifndef __SCHEDULE_H
#define __SCHEDULE_H

/*
 *  Wrapper macros for scheduler
 *  Copyright (c) by Jaroslav Kysela <perex@jcu.cz>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* sleep & wakeup state */

#define SND_WK_NONE             0x00
#define SND_WK_SLEEP            0x01	/* process is sleeping */

#define snd_sleep_define( ident ) \
  struct wait_queue *sleeper_##ident; \
  unsigned long sleeper_end_jiffies_##ident; \
  unsigned short sleeper_lock_##ident;
#define snd_sleep_prepare( object, ident ) \
  do { (object) -> sleeper_##ident = NULL; } while ( 0 )
#define snd_sleep( object, ident, time ) \
  do { \
    (object) -> sleeper_end_jiffies_##ident = current -> timeout = jiffies + (time); \
    interruptible_sleep_on( &(object) -> sleeper_##ident ); \
  } while ( 0 )
#define snd_sleep_u( object, ident, time ) \
  do { \
    (object) -> sleeper_end_jiffies_##ident = current -> timeout = jiffies + (time); \
    sleep_on( &(object) -> sleeper_##ident ); \
  } while ( 0 )
#ifdef SND_POLL
#define snd_poll_wait( file, object, ident, table ) \
  poll_wait( file, &(object) -> sleeper_##ident, table )
#else
#define snd_select_wait( object, ident, table ) \
  select_wait( &(object) -> sleeper_##ident, table );
#endif
#if LinuxVersionCode( 2, 1, 71 ) <= LINUX_VERSION_CODE
#define snd_sleep_abort( object, ident ) \
    ( signal_pending( current ) )
#else
#define snd_sleep_abort( object, ident ) \
    ( current -> signal & ~current -> blocked )
#endif
#ifdef LINUX_2_1
#define snd_wakeup( object, ident ) \
  do { \
    wake_up( &(object) -> sleeper_##ident ); \
    current -> need_resched = 1; \
  } while ( 0 )
#else
#define snd_wakeup( object, ident ) \
  do { \
    wake_up( &(object) -> sleeper_##ident ); \
    need_resched = 1; \
  } while ( 0 )
#endif
#define snd_getlock( object, ident ) (object) -> sleeper_lock_##ident
#define snd_timeout( object, ident ) ( (object) -> sleeper_end_jiffies_##ident < jiffies )
#define snd_timeout_value( object, ident ) ( (object) -> sleeper_end_jiffies_##ident - jiffies )
#define snd_mutex_define( ident ) \
  struct semaphore mutex_##ident;
#define snd_mutex_prepare( object, ident ) \
  (object) -> mutex_##ident = MUTEX;
#define snd_mutex_down( object, ident ) down( &(object) -> mutex_##ident )
#define snd_mutex_up( object, ident ) up( &(object) -> mutex_##ident )
#define snd_mutex_define_static( ident ) \
  static struct semaphore local_mutex_##ident = MUTEX;
#define snd_mutex_down_static( ident ) down( &local_mutex_##ident )
#define snd_mutex_up_static( ident ) up( &local_mutex_##ident )

#endif				/* __SCHEDULE_H */
