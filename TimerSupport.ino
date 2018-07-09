/*  TimerSupport.ino
 *  
 *  I was originally going to call this TimerSupport.cpp but by calling it a .ino file
 *  the IDE includes the required headers where boolean and millis() are declared.
 *  
 *  This is a collection of timer functions that allow 2 basic types of timer functions
 *  with millisecond resolution:
 *  
 *  - Up Timer, which is started and stopped by the user, then read to determine the
 *    elapsed time. If the up timer is read before it is stopped it will indicate at
 *    that instant in time how long it has been running since it was started. If it 
 *    is read after it has been stopped it will indicate how long it ran until it was
 *    stopped.
 *    Functions that support the Up Timers are: start_up_timer(), stop_up_timer() and
 *    read_up_timer().
 *    A normal use case would be to call start_up_timer() and at some later time call
 *    stop_up_timer(). Then read_up_timer() would be called to get the time it ran.
 *    
 *  - Down Timer, which is initialized to a time-out value and then periodically
 *    checked to determine if the timer has expired. Functions that support the Down 
 *    Timers are: init_down_timer(), check_down_timer(), down_timer_running() and
 *    stop_down_timer(). 
 *    A normal use case would be to initialize the down timer by calling init_down_timer()
 *    and then periodically call down_timer_running() to determine if the timer is still
 *    running. You could also call check_down_timer() if you need to know how much time
 *    is remaining or stop_down_timer() if you need to cancel the timer for some reason.
 *  
 *  All of the timers have an ID that is defined in TimerSupport.h
 *  
 *  First written by David Kanceruk on June 18, 2017
 *  All Rights Reserved.
 */

#include "TimerSupport.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables:
/////////////////////////////////////////////////////////////////////////////

uint32_t startTime_ms[NUM_TIMERS];
uint32_t endTime_ms[NUM_TIMERS];
uint32_t timeOut_ms[NUM_TIMERS];
boolean  timerRunning[NUM_TIMERS];


/////////////////////////////////////////////////////////////////////////////
// Functions:
/////////////////////////////////////////////////////////////////////////////
/*
 *  init_down_timer()
 *  
 *  Input parameters:
 *    timer: timer ID number.
 *    time_ms: number of millisenconds the timer should run.
 *    
 *  Return value:
 *    Time the timer was initialized in ms since power up.
 */
uint32_t init_down_timer(enum Timers timer, uint32_t time_ms)
{
  startTime_ms[timer] = millis();
  timeOut_ms[timer] = time_ms;
  timerRunning[timer] = true;

  return startTime_ms[timer];
}

/*  
 *  check_down_timer()
 *  
 *  Input parameters:
 *    timer: timer ID number.
 *    
 *  Return value:
 *    Number of ms remaining or 0 if the timer expired.
 */
uint32_t check_down_timer(enum Timers timer)
{
  if (timerRunning[timer] == true)
  {
    uint32_t interval = millis() - startTime_ms[timer];

    if(interval >= timeOut_ms[timer])
    {
      endTime_ms[timer] = millis();
      timerRunning[timer] = false;
      return 0;
    }
    else
    {
      return timeOut_ms[timer] - interval;
    }
  }
  else
  {
    return 0; 
  }
}

/*
 *  stop_down_timer()
 *  
 *  Input parameters:
 *    timer: timer ID number.
 *    
 *  Return value:
 *    Time the timer was stopped in ms since power up.
 */
uint32_t stop_down_timer(enum Timers timer)
{
  timeOut_ms[timer] = 0;
  endTime_ms[timer] = millis();
  timerRunning[timer] = false;

  return endTime_ms[timer];
}

/*
 *  down_timer_running()
 *  
 *  Input parameters:
 *    timer: timer ID number.
 *    
 *  Return value:
 *    true if the timer is still running, false if it has stopped.
 */
boolean down_timer_running(enum Timers timer)
{
  check_down_timer(timer);

  return timerRunning[timer];
}

/*
 *  start_up_timer()
 *  
 *  Input parameters:
 *    timer: timer ID number.
 *    
 *  Return value:
 *    Time the timer was started in ms since power up.
 */
uint32_t start_up_timer(enum Timers timer)
{
  startTime_ms[timer] = millis();
  endTime_ms[timer] = startTime_ms[timer];
  timerRunning[timer] = true;

  return startTime_ms[timer];
}

/*
 *  stop_up_timer()
 *  
 *  Input parameters:
 *    timer: timer ID number.
 *    
 *  Return value:
 *    Time the timer was stopped in ms since power up.
 */
uint32_t stop_up_timer(enum Timers timer)
{
  endTime_ms[timer] = millis();
  timerRunning[timer] = false;

  return endTime_ms[timer];
}

/*
 *  read_up_timer()
 *  
 *  Input parameters:
 *    timer: timer ID number.
 *    
 *  Return value:
 *    Time the timer has run in ms since it was started (if it 
 *    is still running) or the time in ms the timer ran (if it
 *    has been stopped).
 */
uint32_t read_up_timer(enum Timers timer)
{
  if (timerRunning[timer] == true)
  {
    return millis() - startTime_ms[timer];    
  }
  else
  {
    return endTime_ms[timer] - startTime_ms[timer];  
  }
}

