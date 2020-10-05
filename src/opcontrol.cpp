#include "main.h"
#include "BruhLibrary/global.hpp"
using namespace pros;
double ang = M_PI/2;

//*******************************************************************************//
//Control scheme configuration
//array format: left-right, forwards-back, clockwise-counterclockwise
/*controller_analog_e_t controlscheme[]{
  ANALOG_LEFT_X,
  ANALOG_LEFT_Y,
  ANALOG_RIGHT_X
};

//Control scheme featureset
//array format: enable relative mode, enable angle hold
bool configoptions[]{
  false,
  true
};*/

//*******************************************************************************//
//The actual code
void opcontrol(){
  //TBD: fix the pointers on these so they actually work
  /*coordcontroller mover(base,bPID); //TEMP FOR AUTON TESTING
  opcontrolcontroller useonlyinopcontrol(base,controlscheme,bPID[2],configoptions);
  //xyaT[0] = 20;
//  xyaT[1] = 10;
//  xyaT[2] = M_PI;
  //Motor b(9);
  //below: testing environment for motorsys, motorsysinterface, and intakes
  Motor g[] = {Motor(9), Motor(10,true)};
  MotorSys gg[] = {Intakes(g,2)};
  while(true){
    gg[0].NC(1,0);
    delay(10);
    if(gg[0].iscomplete) lcd::print(1,"NC WORKS");
    lcd::print(2,"Pot Value: %f", gg[0].OPT);
  }
/*  while(true){
    //NBmotors[0].move();
    b.move(ctrl.get_digital(DIGITAL_UP)*127);
    delay(10);
  }*/
/*  while(true){
    odo.posupdv2();
    odometrycontrollerdebug();
    //useonlyinopcontrol.ssc->vectormove(10, 10, 0, 10);
    //lcd::print(1,"%f",useonlyinopcontrol.ssc->MAP[0].cosV);
    //useonlyinopcontrol.relativemove(ctrl.get_analog(ANALOG_RIGHT_X));
    useonlyinopcontrol.move();
    if (ctrl.get_digital_new_press(DIGITAL_B)) configoptions[0] = !configoptions[0];
    delay(10);
  }; /*//*
//below: test for autonomous
  while(true){
    odo.posupdv2();
    mover.update();
    odometrycontrollerdebug();
    delay(10);
  }*/
//  autonomous();
}
