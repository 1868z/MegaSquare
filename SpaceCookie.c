/*
In your program, put the following:

#include "SpaceCookie.c"

void autonomous() {
  // write your autonomous here
}

void driver() {
  // write your driver control here
}


                             ,|
                           //|                              ,|
                         //,/                             -~ |
                       // / |                         _-~   /  ,
                     /'/ / /                       _-~   _/_-~ |
                    ( ( / /'                   _ -~     _-~ ,/'
                     \~\/'/|             __--~~__--\ _-~  _/,
             ,,)))))));, \/~-_     __--~~  --~~  __/~  _-~ /
          __))))))))))))));,>/\   /        __--~~  \-~~ _-~
         -\(((((''''(((((((( >~\/     --~~   __--~' _-~ ~|
--==//////((''  .     `)))))), /     ___---~~  ~~\~~__--~
        ))| @    ;-.     (((((/           __--~~~'~~/
        ( `|    /  )      )))/      ~~~~~__\__---~~__--~~--_
           |   |   |       (/      ---~~~/__-----~~  ,;::'  \         ,
           o_);   ;        /      ----~~/           \,-~~~\  |       /|
                 ;        (      ---~~/         `:::|      |;|      < >
                |   _      `----~~~~'      /      `:|       \;\_____//
          ______/\/~    |                 /        /         ~------~
        /~;;.____/;;'  /          ___----(   `;;;/
       / //  _;______;'------~~~~~    |;;/\    /
      //  | |                        /  |  \;;,\
     (<_  | ;                      /',/-----'  _>
      \_| ||_                     //~;~~~~~~~~~
          `\_|                   (,~~
                                  \~\
                                   ~~
*/

// Declare the functions as they will be defined later.
void autonomous();
void driver();

// True when autonomous completes on its own.
bool autonomous_done = false;

task autonomous_task() {
	autonomous();
	autonomous_done = true;
}

// The driver task is not expected to complete on its own.
task driver_task() {
	while (true) {
	  driver();
  }
}

task main() {
	// During competition, don't start too early.
	while (bIfiRobotDisabled) {
		wait1Msec(1);
	}

	// True while autonomous is running.
	bool autonomous_on = false;
	// True if autonomous was triggered during a competition.
	bool autonomous_competition = false;

	startTask(driver_task);
	// This loop will decide when to switch from the driver task to the
	// autonomous task. It starts assuming we are in driver mode.
	while (true) {
		// Autonomous can be requested in two ways:
	  //  - by pressing Btn 7 & Btn 8 on the remote
	  //  - or automatically during a competition.
		bool autonomous_requested = false;
		if (vexRT[Btn7U] && vexRT[Btn8U]) {
		  autonomous_requested = true;
	  }
		if (bIfiAutonomousMode) {
		  autonomous_requested = true;
			autonomous_competition = true;
	  }
	  // When we detect that autonomous should be on but isn't yet,
	  // we kill the driver control and start the autonomous task.
	  if (autonomous_requested && !autonomous_on) {
  		stopTask(driver_task);
			autonomous_on = true;
			autonomous_done = false;
			startTask(autonomous_task);
	  }
    // This kills the autonomous task during a competition if it
	  // lasts for too long.
	  if (autonomous_competition && !bIfiAutonomousMode) {
			stopTask(autonomous_task);
			autonomous_done = true;
	  }
	  // When autonomous is done, switch back to driver control.
	  if (autonomous_done) {
			autonomous_on = false;
			startTask(driver_task);
	  }

	  // Give more CPU time to the other tasks.
	  wait1Msec(10);
	}
}
