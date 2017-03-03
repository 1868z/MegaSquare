#pragma config(I2C_Usage, I2C1, i2cSensors)
#pragma config(Sensor, in1,    zoom,           sensorAccelerometer)
#pragma config(Sensor, dgtl1,  Eye1,           sensorSONAR_cm)
#pragma config(Sensor, dgtl5,  Eye3,           sensorSONAR_cm)
#pragma config(Sensor, dgtl7,  Eye2,           sensorSONAR_cm)
#pragma config(Sensor, dgtl9,  SpinRight,      sensorSONAR_cm)
#pragma config(Sensor, dgtl11, SpinLeft,       sensorSONAR_cm)
#pragma config(Sensor, I2C_1,  ,               sensorQuadEncoderOnI2CPort,    , AutoAssign )
#pragma config(Motor,  port1,           RF,            tmotorVex393HighSpeed_HBridge, openLoop, reversed)
#pragma config(Motor,  port2,           LR,            tmotorVex393HighSpeed_MC29, openLoop)
#pragma config(Motor,  port6,           Spinner,       tmotorVex393_MC29, openLoop, encoderPort, I2C_1)
#pragma config(Motor,  port9,           LF,            tmotorVex393HighSpeed_MC29, openLoop)
#pragma config(Motor,  port10,          RR,            tmotorVex393HighSpeed_HBridge, openLoop, reversed)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#define HISTORY 5
#define ENCODER_RANGE 300
#define SCAN_RANGE 250

#define SAFE 30
#define BUMP 30

// High-pass filter
// y[t] = alpha * (y[t-1] + x[t] - x[t-1])

struct HPF {
  float x;
  float y;
  float alpha;
};

void HPFinit(HPF* hpf, float alpha) {
  hpf->x = 0.0;
  hpf->y = 0.0;
  hpf->alpha = alpha;
}

float HPFupdate(HPF* hpf, float x) {
  hpf->y = hpf->alpha * (hpf->y + x - hpf->x);
  hpf->x = x;

  return hpf->y;
}

enum Direction {
	DIR_E = 0,  // 0 deg
	DIR_NE = 1,
	DIR_N = 2,
	DIR_NW = 3,
	DIR_W = 4,  // 180 deg
	DIR_SW = 5,
	DIR_S = 6,
	DIR_SE = 7,

	DIRECTIONS = 8,
};

// Maintains the median value of a stream of distances.
struct MedianFilter {
	int last;

	int distance[HISTORY];  // ordered distances
	int index[HISTORY];     // time of distance in position p
	int position[HISTORY];  // position of distance at time t
};

// Tracks median distances in the 8 directions
struct World {
	MedianFilter dir[DIRECTIONS];
};

void initialize(World* w) {
	for (int d = 0; d < (long)DIRECTIONS; d++) {
		MedianFilter* o = &w->dir[(Direction)d];
    o->last = 0;
    for (int i = 0; i < HISTORY; i++) {
      o->distance[i] = 100;
      o->index[i] = i;
      o->position[i] = i;
    }
}
}

void swap(MedianFilter* o, int p, int k) {
	int tmp;

	tmp = o->distance[p];
	o->distance[p] = o->distance[k];
	o->distance[k] = tmp;

	int tp = o->index[p];
	int tk = o->index[k];

	o->index[p] = tk;
	o->index[k] = tp;

	tmp = o->position[tp];
	o->position[tp] = o->position[tk];
	o->position[tk] = tmp;
}

void update(World* w, Direction direction, int distance) {
	if (distance <= 0) return;

	MedianFilter* o = &(w->dir[direction]);

	int p = o->position[o->last];  // insertion point
	o->distance[p] = distance;
	if (p < HISTORY - 1 && distance > o->distance[p + 1]) {
		while (p < HISTORY - 1 && distance > o->distance[p + 1]) {
			swap(o, p, p + 1);
		}
	}
	if (p > 0 && distance < o->distance[p - 1]) {
		while (p > 0 && distance < o->distance[p - 1]) {
			swap(o, p, p - 1);
		}
	}
	o->last = (o->last + 1) % HISTORY;
}

int getDistance(World* w, Direction direction) {
	return w->dir[direction].distance[HISTORY / 2];
}

Direction angleToDir(int angle) {
	angle = (angle + 22) % 360;

  return (Direction)(angle / 45);
}

World w;
bool hit = false;

task displayMap() {
	while (true) {
		writeDebugStreamLine("| %.03d | %.03d | %.03d |", getDistance(&w, DIR_NW), getDistance(&w, DIR_N), getDistance(&w, DIR_NE));
		writeDebugStreamLine("| %.03d |     | %.03d |", getDistance(&w, DIR_W),  getDistance(&w, DIR_E));
		writeDebugStreamLine("| %.03d | %.03d | %.03d |", getDistance(&w, DIR_SW), getDistance(&w, DIR_S), getDistance(&w, DIR_SE));
		writeDebugStreamLine("");
  	wait1Msec(1000);
  }
}

task measure() {
	initialize(&w);
	nMotorEncoder[Spinner] = 0;

	int last_angle = -1000;  // ensure we read the first values

	int sign = +1;
  HPF bumper;
  HPFinit(&bumper, 0.05);
  int hittime = time1[T4];

	while (true) {
		int acc = HPFupdate(&bumper, SensorValue[zoom]);
    if (abs(acc) > BUMP) {
    	hit = true;
    	hittime = time1[T4];
		  writeDebugStreamLine("HIT=%d", acc);
	  }
	  if (time1[T4] > hittime + 1000) {
	  	hit = false;
	  }

  	motor[Spinner] = 40 * sign;

		// encoder will cover +/- ENCODER_RANGE, which we map
	  // to 0 - 360 degrees.
		int encoder = nMotorEncoder[Spinner];
    if ((sign < 0 && encoder > SCAN_RANGE) ||
    	  (sign > 0 && encoder < -SCAN_RANGE)) sign = -sign;

		int angle = ( 360 * encoder / (2 * ENCODER_RANGE)) % 360;
		if (angle < 0) angle += 360;

		if (abs(last_angle - angle) < 8) {
		  wait1Msec(1);
		  continue;
	  }
    last_angle = angle;

		// Eye3 -> 0 degrees
		// Eye1 -> 120
		// Eye2 -> 240
		update(&w, angleToDir(angle), SensorValue[Eye3]);
		update(&w, angleToDir(angle+120), SensorValue[Eye1]);
		update(&w, angleToDir(angle+240), SensorValue[Eye2]);
	}
}

void drive(int left, int right) {
  motor[LF] = left;
  motor[LR] = left;
  motor[RF] = right;
  motor[RR] = right;
}

enum State {
  STATE_FORWARD = 0,
  STATE_TURN_LEFT = 1,
  STATE_TURN_RIGHT = 2,
  STATE_BACK_OUT = 3,
};


task main()
{
	wait(2);  // initialize the accelerometer

	startTask(measure);
	startTask(displayMap);

	int fwd_start = time1[T3];
  State s = STATE_FORWARD;
  while (true) {
    switch (s) {
      case STATE_FORWARD:
        if (hit && time1[T3] > fwd_start + 1000) s = STATE_BACK_OUT;
        drive(70, 70);
        if (getDistance(&w, DIR_E) < SAFE ||
        	  getDistance(&w, DIR_NE) < (SAFE+10) ||
        	  getDistance(&w, DIR_SE) < (SAFE+10)) {
        	 s = getDistance(&w, DIR_N) > getDistance(&w, DIR_S) ? STATE_TURN_LEFT : STATE_TURN_RIGHT;
        }
        break;
      case STATE_TURN_LEFT:
        drive(-40, +40);
        if (getDistance(&w, DIR_E) > SAFE &&
        	  getDistance(&w, DIR_NE) > (SAFE+10) &&
        	  getDistance(&w, DIR_SE) > (SAFE+10)) {
          s = STATE_FORWARD;
          fwd_start = time1[T3];
        } else {
          if (hit) s = STATE_BACK_OUT;
        }
        break;
      case STATE_TURN_RIGHT:
        drive(+40, -40);
        if (getDistance(&w, DIR_E) > (SAFE+5) &&
        	  getDistance(&w, DIR_NE) > (SAFE+5) &&
        	  getDistance(&w, DIR_SE) > (SAFE+5)) {
          s = STATE_FORWARD;
          fwd_start = time1[T3];
        } else {
          if (hit) s = STATE_BACK_OUT;
        }
        break;
      case STATE_BACK_OUT:
        drive(-30, -30);
        wait1Msec(1000);
        int turn = getDistance(&w, DIR_N) > getDistance(&w, DIR_S) ? -40 : 40;
        drive(turn, -turn);
        wait1Msec(1000);
        s = STATE_FORWARD;
        fwd_start = time1[T3];
        break;
    }
  }
}
