/* 
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>


/*
 *
 * Constants
 *
 */

/*
 * Number of cars created.
 */

#define NCARS 20

typedef struct lock* lockptr;

//Approaching locks
lockptr lockN, lockS, lockE, lockW;

//Quadrant locks
lockptr lockNW, lockNE, lockSW, lockSE;

int threadskilled;

/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
}

/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
        
        (void) cardirection;
        (void) carnumber;

        int destdirection = (cardirection + 2) % 4;

        message(0, carnumber, cardirection, destdirection);
        message(1, carnumber, cardirection, destdirection);
        message(2, carnumber, cardirection, destdirection);
        message(4, carnumber, cardirection, destdirection);
}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;

        int destdirection = (cardirection + 1) % 4;

        message(0, carnumber, cardirection, destdirection);
        message(1, carnumber, cardirection, destdirection);
        message(2, carnumber, cardirection, destdirection);
        message(3, carnumber, cardirection, destdirection);
        message(4, carnumber, cardirection, destdirection);
 
}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;

        int destdirection = (cardirection + 3) % 4;

        message(0, carnumber, cardirection, destdirection);
        message(1, carnumber, cardirection, destdirection);
        message(4, carnumber, cardirection, destdirection);
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection, direction;

        /*
         * Avoid unused variable and function warnings.
         */

        (void) unusedpointer;
        (void) carnumber;
	(void) gostraight;
	(void) turnleft;
	(void) turnright;

        /*
         * cardirection is set randomly.
         * 0 is from north, 1 is from east, 2 is from south, 3 is from west
         * direction is which turn they want
         * 0 is left, 1 is straight, 2 is right
         */
        cardirection = random() % 4;
        direction = random() % 3;

        //To avoid deadlock, always grab locks in this order: NW, SW, SE, NE
        //NOTE: we only lock the path the car wants to take
        if(cardirection == 0) {
                lock_acquire(lockN);
                if(direction == 0) {
                        lock_acquire(lockNW);
                        lock_acquire(lockSW);
                        lock_acquire(lockSE);
                        turnleft(0, carnumber);
                        lock_release(lockSE);
                        lock_release(lockSW);
                        lock_release(lockNW);                        
                }

                else if(direction == 1) {
                        lock_acquire(lockNW);
                        lock_acquire(lockSW);
                        gostraight(0, carnumber);
                        lock_release(lockSW);
                        lock_release(lockNW);
                }

                else if(direction == 2) {
                        lock_acquire(lockNW);
                        turnright(0, carnumber);
                        lock_release(lockNW);
                }

                lock_release(lockN);

        }

        else if(cardirection == 1) {
                lock_acquire(lockE);
                if(direction == 0) {
                        lock_acquire(lockNW);
                        lock_acquire(lockSW);
                        lock_acquire(lockNE);
                        turnleft(cardirection, carnumber);

                        lock_release(lockNE);
                        lock_release(lockSW);
                        lock_release(lockNW);
                }

                else if(direction == 1) {
                        lock_acquire(lockNW);
                        lock_acquire(lockNE);
                        gostraight(cardirection, carnumber);
                        
                        lock_release(lockNE);
                        lock_release(lockNW);
                }

                else if(direction == 2) {
                        lock_acquire(lockNE);
                        turnright(cardirection, carnumber);
                        lock_release(lockNE);
                }

                lock_release(lockE);
        }

        else if(cardirection == 2) {
                lock_acquire(lockS);
                if(direction == 0) {
                        lock_acquire(lockNW);
                        lock_acquire(lockSE);
                        lock_acquire(lockNE);
                        turnleft(cardirection, carnumber);
                        lock_release(lockNE);
                        lock_release(lockSE);
                        lock_release(lockNW);
                }

                else if(direction == 1) {
                        lock_acquire(lockSE);
                        lock_acquire(lockNE);
                        gostraight(cardirection, carnumber);
                        lock_release(lockNE);
                        lock_release(lockSE);
                }

                else if(direction == 2) {
                        lock_acquire(lockSE);
                        turnright(cardirection, carnumber);
                        lock_release(lockSE);
                }

                lock_release(lockS);
        }

        else if(cardirection == 3) {
                lock_acquire(lockW);
                if(direction == 0) {
                        lock_acquire(lockSW);
                        lock_acquire(lockSE);
                        lock_acquire(lockNE);
                        turnleft(cardirection, carnumber);
                        lock_release(lockNE);
                        lock_release(lockSE);
                        lock_release(lockSW);                                               
                }

                else if(direction == 1) {
                        lock_acquire(lockSW);
                        lock_acquire(lockSE);
                        gostraight(cardirection, carnumber);
                        lock_release(lockSE);
                        lock_release(lockSW);
                }

                else if(direction == 2) {
                        lock_acquire(lockSW);
                        turnright(cardirection, carnumber);
                        lock_release(lockSW);
                }

                lock_release(lockW);
        }
        else {
                panic("boogyoogy");
        }

        threadskilled++;
        thread_exit();
}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;

        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

        threadskilled = 0;

        lockN = lock_create("lockN");
        lockS = lock_create("lockS");
        lockE = lock_create("lockE");
        lockW = lock_create("lockW");

        lockNW = lock_create("lockNW");
        lockSW = lock_create("lockSW");
        lockNE = lock_create("lockNE");
        lockSE = lock_create("lockSE");

        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {

                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {
                        
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }

        while(threadskilled < (NCARS - 1))
                thread_yield();

        lock_destroy(lockN);
        lock_destroy(lockW);
        lock_destroy(lockE);
        lock_destroy(lockS);

        lock_destroy(lockNW);
        lock_destroy(lockSW);
        lock_destroy(lockNE);
        lock_destroy(lockSE);
        
        return 0;
}
