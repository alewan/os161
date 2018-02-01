/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
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
#include <machine/spl.h>


/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

//Number of iterations to do
#define NITERATIONS 4

/*
 * 
 * Function Definitions
 * 
 */

//Create pointers for semaphores within the scope of the whole file
typedef struct semaphore * Semaphore;
Semaphore cm_eating;

void* cm_miceCanEat;
void* cm_catsCanEat;
int catsEating;
int bowlEating[2];

int threadsKilled;

//check if other bowl is free

/* who should be "cat" or "mouse" */
static void
sem_eat(const char *who, int num, int bowl, int iteration)
{
        kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
        clocksleep(1);
        kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
}

/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

         

        (void) unusedpointer;

        int eatenThisTurn = 0;
        //Eating NITERATIONS times
    int i = 0;
        for (i = 0; i < NITERATIONS; ++i) {
                //Begin Checks
                //Check if species is eating or already eaten this turn else sleep
                if (!catsEating || eatenThisTurn) {
                        int spl;
                        spl = splhigh();
                        thread_sleep(cm_catsCanEat);
                        splx(spl);
                }

                //Core
                //Request the semaphore with P
                P(cm_eating);

                //Call sem_eat
                int eaten = 0;
                while(!eaten) {
                if (!bowlEating[0] && catsEating) {
                        bowlEating[0] = 1;
                        sem_eat("cat",catnumber,1,i);
                        bowlEating[0] = 0;
                        eaten = 1;
                }
                else if (!bowlEating[1] && catsEating) {
                        bowlEating[1] = 1;
                        sem_eat("cat",catnumber,2,i);
                        bowlEating[1] = 0;
                        eaten = 1;
                }
                else {
                        int spl;
                        spl = splhigh();
                        thread_sleep(cm_catsCanEat);
                        splx(spl);
                        //kprintf("HELLO %d, %d, \n", bowlEating[0], bowlEating[1]);
                }
                }

                //END Checks
                //Release the semaphore with V
                V(cm_eating);
                //Update eaten this turn
                eatenThisTurn = 1;
                if (!bowlEating[0] && !bowlEating[1]) {
                        catsEating = 0;
                        int spl;
                        spl = splhigh();
                        thread_wakeup(cm_miceCanEat);
                        splx(spl);
                }

        }
        //kprintf("KILLING cat");
        threadsKilled++;
        thread_exit();
        return;
        
}
        

/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */

         

        (void) unusedpointer;

        int eatenThisTurn = 0;
        //Eating NITERATIONS times
    int i = 0;
        for (i = 0; i < NITERATIONS; ++i) {
                //Begin Checks
                //Check if species is eating or already eaten this turn else sleep
                if (catsEating || eatenThisTurn) {
                        int spl;
                        spl = splhigh();
                        thread_sleep(cm_miceCanEat);
                        splx(spl);
                }

                //Core
                //Request the semaphore with P
                P(cm_eating);

                //Call sem_eat
                int eaten = 0;
                while(!eaten) {
                if (!bowlEating[0] && !catsEating) {
                        bowlEating[0] = 1;
                        sem_eat("mouse",mousenumber,1,i);
                        bowlEating[0] = 0;
                        eaten = 1;
                }
                else if (!bowlEating[1] && !catsEating) {
                        bowlEating[1] = 1;
                        sem_eat("mouse",mousenumber,2,i);
                        bowlEating[1] = 0;
                        eaten = 1;
                }
                else {
                        //kprintf("HELLO %d, %d, \n", bowlEating[0], bowlEating[1]);
                        int spl;
                        spl = splhigh();
                        thread_sleep(cm_catsCanEat);
                        splx(spl);
                }
                }

                //END Checks
                //Release the semaphore with V
                V(cm_eating);

                //Update eaten this turn
                eatenThisTurn = 1;
                if (!bowlEating[0] && !bowlEating[1]) {
                        catsEating = 1;
                        int spl;
                        spl = splhigh();
                        thread_wakeup(cm_catsCanEat);
                        splx(spl);
                }

        }
        //kprintf("KILLING mouse");
        threadsKilled++;
        thread_exit();
        return;
}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
        //Initialize semaphores created
        cm_eating = sem_create("bowls", 2);
        bowlEating[0] = 0;
        bowlEating[1] = 0;
        catsEating = 1;


        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;
   
        /*
         * Start NCATS catsem() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catsem Thread", 
                                    NULL, 
                                    index, 
                                    catsem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catsem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mousesem Thread", 
                                    NULL, 
                                    index, 
                                    mousesem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mousesem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        while(threadsKilled < (NMICE+NCATS - 1)) 
                thread_yield();

        sem_destroy(cm_eating);
        return 0;
}


/*
 * End of catsem.c
 */
