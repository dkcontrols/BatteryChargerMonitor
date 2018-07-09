/*  TimerSupport.h
 *   
 *  All of the timers have an ID defined in this file. This file also has
 *  the function prototypes for the functions in TimerSupport.ino
 *  
 *  First written by David Kanceruk on June 18, 2017
 */

#ifndef TIMER_SUPPORT
#define TIMER_SUPPORT

/////////////////////////////////////////////////////////////////////////////
// Timer IDs:
/////////////////////////////////////////////////////////////////////////////

enum Timers
{
  SEND_timer,
  READ_VOLTS_timer,
  NUM_TIMERS
};


/////////////////////////////////////////////////////////////////////////////
// Function prototypes:
/////////////////////////////////////////////////////////////////////////////

uint32_t init_down_timer   (enum Timers timer, uint32_t time_ms);
uint32_t check_down_timer  (enum Timers timer);
uint32_t stop_down_timer   (enum Timers timer);
boolean  down_timer_running(enum Timers timer);
uint32_t start_up_timer    (enum Timers timer);
uint32_t stop_up_timer     (enum Timers timer);
uint32_t read_up_timer     (enum Timers timer);

#endif // TIMER_SUPPORT

