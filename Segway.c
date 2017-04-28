#pragma config(Sensor, in6,    Angle,          sensorAnalog)
#pragma config(Motor,  port1,           Left,          tmotorVex393_HBridge, openLoop, reversed)
#pragma config(Motor,  port2,           Right,         tmotorVex393_MC29, openLoop, reversed)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

int target = 0;
float angRate=0;
float ang=0;
float avg=0;
// Angle < 0 -> display up

task display() {

  // low pass filter awhile to get good initial average
	int N=3000;
	for(int i=0;i<N;i++){
		avg = 0.95*avg + 0.05*SensorValue[Angle];
		wait1Msec(10);
	}


	char msg[32];
	while (true) {
		clearLCDLine(0);
		//snprintf(msg, 31, "%d - %d", SensorValue[Angle], SensorBias[Angle]);
    //displayLCDCenteredString(0, msg);
	  //wait1Msec(1000);
		angRate=SensorValue[Angle]-avg;
		ang += angRate;
    wait1Msec(10);
  }
}

task pid() {
	SensorValue[Angle] = 0;


	int ts = time1[timer1];
	int es = 0;

	while (true) {
		wait1Msec(2);

		int angle = SensorValue[Angle];
		int error = - angle * abs(sinDegrees(angle / 10));

		float dt = time1[timer1] - ts;

		float de = 60000.0 * (error - es) / dt;
		float pe = 60.0 * error;

		writeDebugStreamLine("pe=%f de=%f s=%f", pe, de, sinDegrees(angle/10));

		int power = pe + de;

		if (power > +127) power = +127;
		if (power < -127) power = -127;

		motor[Left] = power;
		motor[Right] = power;

		es = error;
  }
}


task main()
{
	wait1Msec(2000);
  //startTask(pid);
  startTask(display);

  while (true) {
    target = vexRT[Ch3] / 30;
    wait1Msec(50);
  }
}
