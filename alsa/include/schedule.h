#ifndef __SCHEDULE_H
#define __SCHEDULE_H

/*
 *  Wrapper macros for scheduler
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
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

#ifdef LINUX_2_1

#define snd_sleep_define( ident ) \
	struct wait_queue *sleeper_##ident; \
	long sleeper_timeout_##ident; \
	unsigned short sleeper_lock_##ident;
#define snd_sleep_prepare( object, ident ) \
	do { (object)->sleeper_##ident = NULL; \
             (object)->sleeper_timeout_##ident = -1; } while ( 0 )
#define snd_sleep( object, ident, time ) \
	do { \
    		(object)->sleeper_timeout_##ident = \
      			interruptible_sleep_on_timeout( \
      				&(object)->sleeper_##ident, time ); \
  	} while ( 0 )
#define snd_poll_wait( file, object, ident, table ) \
	poll_wait( file, &(object)->sleeper_##ident, table )
#define snd_sleep_abort( object, ident ) \
	( signal_pending( current ) )
#define snd_wakeup( object, ident ) \
	do { \
		wake_up( &(object)->sleeper_##ident ); \
		current->need_resched = 1; \
	} while ( 0 )
#define snd_getlock( object, ident ) (object)->sleeper_lock_##ident
#define snd_timeout( object, ident ) \
			( (object)->sleeper_timeout_##ident == 0 )
#define snd_timevalue( object, ident ) (object)->sleeper_timeout_##ident
#define snd_schedule( object, ident, time ) \
	do { \
		current->state = TASK_INTERRUPTIBLE; \
		(object)->sleeper_timeout_##ident = schedule_timeout( time ); \
	} while ( 0 )

#else /* Linux 2.0.x */

#define snd_sleep_define( ident ) \
	struct wait_queue *sleeper_##ident; \
	unsigned long sleeper_end_jiffies_##ident; \
	unsigned short sleeper_lock_##ident;
#define snd_sleep_prepare( object, ident ) \
	do { (object)->sleeper_##ident = NULL; \
	     (object)->sleeper_end_jiffies_##ident = -1; } while ( 0 )
#define snd_sleep( object, ident, time ) \
	do { \
		(object)->sleeper_end_jiffies_##ident = \
			current->timeout = jiffies + (time); \
		interruptible_sleep_on( &(object)->sleeper_##ident ); \
	} while ( 0 )
#define snd_select_wait( object, ident, table ) \
	select_wait( &(object)->sleeper_##ident, table );
#define snd_sleep_abort( object, ident ) \
	( current->signal & ~current->blocked )
#define snd_wakeup( object, ident ) \
	do { \
		wake_up( &(object)->sleeper_##ident ); \
		need_resched = 1; \
	} while ( 0 )
#define snd_getlock( object, ident ) (object)->sleeper_lock_##ident
#define snd_timeout( object, ident ) \
			( (object)->sleeper_end_jiffies_##ident < jiffies )
#define snd_timevalue( object, ident ) \
			( (signed long)((object)->sleeper_end_jiffies_##ident - jiffies) )
#define snd_schedule( object, ident, time ) \
	do { \
		(object)->sleeper_end_jiffies_##ident = \
			current->timeout = jiffies + (time); \
		current->state = TASK_INTERRUPTIBLE; \
		schedule(); \
	} while ( 0 )

#endif /* LINUX_2_1 */

/* mutex */

#define snd_mutex_define( ident ) \
  struct semaphore mutex_##ident;
#define snd_mutex_prepare( object, ident ) \
  (object)->mutex_##ident = MUTEX;
#define snd_mutex_down( object, ident ) down( &(object)->mutex_##ident )
#define snd_mutex_up( object, ident ) up( &(object)->mutex_##ident )
#define snd_mutex_define_static( ident ) \
  static struct semaphore local_mutex_##ident = MUTEX;
#define snd_mutex_down_static( ident ) down( &local_mutex_##ident )
#define snd_mutex_up_static( ident ) up( &local_mutex_##ident )

#endif				/* __SCHEDULE_H */
