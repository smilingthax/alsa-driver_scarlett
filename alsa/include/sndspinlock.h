#ifndef __SNDSPINLOCK_H
#define __SNDSPINLOCK_H

/*
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

#if 0
#ifdef SNDCFG_DEBUG
#define SND_DEBUG_SPIN
#endif
#endif

static inline void snd_cli(unsigned long *flags)
{
	save_flags(*(flags));
	cli();
}
static inline void snd_sti(unsigned long *flags)
{
	restore_flags(*(flags));
}

#ifndef SND_DEBUG_SPIN

#if defined( LINUX_2_1 ) && defined( __SMP__ )
#define snd_spin_lock( object, name, flags ) \
  spin_lock_irqsave( &(object) -> spin_lock_##name, *(flags) )
#define snd_spin_unlock( object, name, flags ) \
  spin_unlock_irqrestore( &(object) -> spin_lock_##name, *(flags) )
#define snd_spin_lock_static( name, flags ) \
  spin_lock_irqsave( &snd_static_spin_lock_##name, *(flags) )
#define snd_spin_unlock_static( name, flags ) \
  spin_unlock_irqrestore( &snd_static_spin_lock_##name, *(flags) )
#define snd_spin_define( name ) \
  spinlock_t spin_lock_##name
#define snd_spin_prepare( object, name ) \
  (object) -> spin_lock_##name = SPIN_LOCK_UNLOCKED
#define snd_spin_define_static( name ) \
  static spinlock_t snd_static_spin_lock_##name = SPIN_LOCK_UNLOCKED
#else
#define snd_spin_lock( object, name, flags ) snd_cli( flags )
#define snd_spin_unlock( object, name, flags ) snd_sti( flags )
#define snd_spin_lock_static( name, flags ) snd_cli( flags )
#define snd_spin_unlock_static( name, flags ) snd_sti( flags )
#define snd_spin_define( name )	/* nothing */
#define snd_spin_prepare( object, name ) do { ; } while ( 0 )
#define snd_spin_define_static( name )	/* nothing */
#endif

#else				/* SND_DEBUG_SPIN */

/*
 * This debug implementation doesn't use real spin locks, but shows
 * wrong use of spin locks with both UP and SMP systems.
 */

struct snd_spin_lock {
	unsigned int used;
	char *file;
	int line;
};

#define snd_spin_lock( object, name, flags ) \
  do { \
    snd_cli( flags ); \
    if ( (object) -> spin_lock_##name.used ) { \
      snd_printd( "Oops - " #object " -> spin_lock_" #name " is already locked!!! [" __FILE__ ":%d] <- [%s:%d]\n", (int)__LINE__, (object) -> spin_lock_##name.file, (object) -> spin_lock_##name.line ); \
    } else { \
      (object) -> spin_lock_##name.file = __FILE__; \
      (object) -> spin_lock_##name.line = __LINE__; \
    } \
    (object) -> spin_lock_##name.used++; \
  } while ( 0 )
#define snd_spin_unlock( object, name, flags ) \
  do { \
    (object) -> spin_lock_##name.used--; \
    snd_sti( flags ); \
  } while ( 0 )
#define snd_spin_lock_static( name, flags ) \
  do { \
    snd_cli( flags ); \
    if ( snd_static_spin_lock_##name.used ) { \
      snd_printd( "Oops - snd_static_spin_lock_" #name " is already locked!!! [" __FILE__ ":%d] <- [%s:%d]\n", (int)__LINE__, snd_static_spin_lock_##name.file, snd_static_spin_lock_##name.line ); \
    } else { \
      snd_static_spin_lock_##name.file = __FILE__; \
      snd_static_spin_lock_##name.line = __LINE__; \
    } \
    snd_static_spin_lock_##name.used++; \
  } while ( 0 )
#define snd_spin_unlock_static( name, flags ) \
  do { \
    snd_static_spin_lock_##name.used--; \
    snd_sti( flags ); \
  } while ( 0 )
#define snd_spin_define( name )	\
  struct snd_spin_lock spin_lock_##name
#define snd_spin_prepare( object, name ) \
  (object) -> spin_lock_##name = (struct snd_spin_lock){ 0, NULL, -1 }
#define snd_spin_define_static( name ) \
  static struct snd_spin_lock snd_static_spin_lock_##name = (struct snd_spin_lock){ 0, NULL, -1 }

#endif				/* SND_DEBUG_SPIN */

#endif				/* __SNDSPINLOCK_H */
