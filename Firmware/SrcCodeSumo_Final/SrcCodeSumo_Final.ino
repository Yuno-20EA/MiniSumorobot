#include <NewPing.h>
#include <Arduino.h>
const unsigned long TIME_FORWARD = 100;
const unsigned long TIME_LEFT    = 180;
const unsigned long TIME_RIGHT   = 180;

const unsigned long COAST_MS = 20;

const int INA = 2, INB = 3;  
const int INC = 4, IND = 5; 

const int TRIG_FRONT = A0, ECHO_FRONT = A1;
const int TRIG_RIGHT = A4, ECHO_RIGHT = A5;
const int TRIG_LEFT  = 10, ECHO_LEFT  = 11;
const int MAX_DISTANCE = 50;

NewPing sonarFront(TRIG_FRONT, ECHO_FRONT, MAX_DISTANCE);
NewPing sonarRight(TRIG_RIGHT, ECHO_RIGHT, MAX_DISTANCE);
NewPing sonarLeft (TRIG_LEFT,  ECHO_LEFT,  MAX_DISTANCE);

int  distFront = 0, distRight = 0, distLeft = 0;
int  cntFront = 0,  cntRight = 0,  cntLeft = 0;
bool enemyFront = false, enemyRight = false, enemyLeft = false;
bool isTimeUp = false;

const unsigned long PING_INTERVAL_MS = 30;
unsigned long lastPingTime = 0;
uint8_t sensorIndex = 0;  

enum ActionPhase { AP_DECIDE, AP_COAST, AP_WAIT_TURN, AP_WAIT_FORWARD };
enum Move { MOVE_FORWARD, MOVE_LEFT, MOVE_RIGHT };

ActionPhase actionPhase = AP_DECIDE;
unsigned long phaseEndTime = 0;
Move currentMove = MOVE_FORWARD;        
Move pendingMove = MOVE_FORWARD;     
unsigned long pendingDuration = 0;
ActionPhase pendingResultPhase = AP_DECIDE;

void setup() {
  pinMode(INA, OUTPUT); pinMode(INB, OUTPUT);
  pinMode(INC, OUTPUT); pinMode(IND, LOW);
}
//  ĐỘNG CƠ 
void forward() {
  digitalWrite(INA, HIGH); digitalWrite(INB, LOW);
  digitalWrite(INC, HIGH); digitalWrite(IND, LOW);
}
void left() {
  digitalWrite(INA, LOW);  digitalWrite(INB, HIGH);
  digitalWrite(INC, HIGH); digitalWrite(IND, LOW);
}
void right() {
  digitalWrite(INA, HIGH); digitalWrite(INB, LOW);
  digitalWrite(INC, LOW);  digitalWrite(IND, HIGH);
}
void stopMotors() {
  digitalWrite(INA, LOW); digitalWrite(INB, LOW);
  digitalWrite(INC, LOW); digitalWrite(IND, LOW);
}

void applyMove(Move m) {
  switch (m) {
    case MOVE_FORWARD: forward(); break;
    case MOVE_LEFT:    left();    break;
    case MOVE_RIGHT:   right();   break;
  }
}

void updateFlag(int dist, int &counter, bool &flag) {
  if (dist > 0 && dist < MAX_DISTANCE) counter++; else counter = 0;
  flag = (counter >= 2);
}

void updateSensors() {

  
  if (actionPhase == AP_COAST) return;
  if (millis() - lastPingTime < PING_INTERVAL_MS) return;
  lastPingTime = millis();
  switch (sensorIndex) {
    case 0: distFront = sonarFront.ping_cm(); updateFlag(distFront, cntFront, enemyFront); break;
    case 1: distRight = sonarRight.ping_cm(); updateFlag(distRight, cntRight, enemyRight); break;
    case 2: distLeft  = sonarLeft.ping_cm();  updateFlag(distLeft,  cntLeft,  enemyLeft);  break;
  }
  sensorIndex = (sensorIndex + 1) % 3;
}
void queueMove(Move m, unsigned long duration, ActionPhase resultPhase) {
  if (m == currentMove) {
    actionPhase = resultPhase;
    phaseEndTime = millis() + duration;
    return;
  }
  stopMotors();
  pendingMove = m;
  pendingDuration = duration;
  pendingResultPhase = resultPhase;
  currentMove = m;
  actionPhase = AP_COAST;
  phaseEndTime = millis() + COAST_MS;
}

void runAttackStateMachine() {
  if (millis() < phaseEndTime) return;
  if (actionPhase == AP_COAST) {
    applyMove(pendingMove);
    actionPhase = pendingResultPhase;
    phaseEndTime = millis() + pendingDuration;
    return;
  }
  if (actionPhase == AP_WAIT_TURN) { 
    queueMove(MOVE_FORWARD, TIME_FORWARD, AP_WAIT_FORWARD);
    return;
  }
  if (enemyFront) {
    queueMove(MOVE_FORWARD, TIME_FORWARD, AP_WAIT_FORWARD);
  } else if (enemyRight) {
    queueMove(MOVE_RIGHT, TIME_RIGHT, AP_WAIT_TURN);
  } else if (enemyLeft) {
    queueMove(MOVE_LEFT, TIME_LEFT, AP_WAIT_TURN);
  } else {
    queueMove(MOVE_LEFT, 0, AP_DECIDE);   
  }
}
void loop() {
  if (millis() > 5000) isTimeUp = true;

  updateSensors();
  if (!isTimeUp) return;

  runAttackStateMachine(); 
}