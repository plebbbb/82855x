#pragma once
#include "global.hpp"
//TBD: Fix headers, also implement the subsystem controls already

//curveS: a single S curve
//Constraints(suggested): range: 0 to 50, upwards.  max height 100;
//TBD, maybe actually calculate proper motion curves later and emulate them with curveS
struct curveS{
double vars[4];
public:
//below: this is stupid and pointless but I cant get the access to work correctly otherwise
curveS(double arr[]){vars[0] = arr[0]; vars[1] = arr[1];vars[2] = arr[2];vars[3] = arr[3];}
double getval(double pos){
  return (vars[0]-vars[3])/(1+pow(M_E,-vars[1]*(pos-vars[2]))) + vars[3];
}
};

//dualScurve: a set of 2 S curves made to aproximate what motion profiling might look like
/*dualScurve should extend curveS, also should be an interface ngl so we can use both one or two
the current pointer access method is ok for these b/c they dont store anything motor-specific,
its literally just a formula*/
//TBD: scale dualScurve to actual time values. an S curve is supposed to be relative to time, not displacement like we use it
//Even right now it still outperforms raw PIDs, but we will get better performance if we adjust it
//To do that, we have to unconvert the area(the displacement) of the curve into its corrospondent speed in the S curve
//this requires some calc stuff so it's gonna take a while
class dualScurve{
  public:
    curveS a;
    curveS b;
  //dualScurve(curveS c, curveS d){a=&c; b=&d;}
  //dualScurve(curveS c){a=&c;}
  //below: direct to curveS system b/c we don't actually use curveS for anything meaningful other than this
  dualScurve(double arr1[], double arr2[]):a(arr1),b(arr2){};
  double getval(double pos){
   // double pos = fabs(ps);
  //  lcd::print(1,"given value: %f", pos);
    if (pos < 50) {
      //lcd::print(2,"return value: %f", a->getval(pos));
      return a.getval(pos);
    }
  //  lcd::print(2,"return value: %f", b->getval(100-pos));
    return b.getval(100-pos);
    //for the part past 50%, we flip the curveS, so that we only need one curve from 0-50 for each different profile
  }
};
  //beziernp: a raw N-point bezier curve
  /*This is a full on proper n-point bezier curve, where we can add
  as many transformations as we want but only 2 garanteed target locations
  */
struct beziernp{
        double (*coords)[2];
        int size; //coordinate size
        //params: Coordinates of NP bezier, amount of coordinates(not an index)
        //and yes, I actually need the size thing cuz pointers dont pass along the actual size
        beziernp(double points[][2], int sie):size(sie){
      	  coords = points; //fancy speed and heading configurations precomputed in motion to reduce load
          }
        //https://www.desmos.com/calculator/xlpbe9bgll
        void getvalFxyaT(double t){
          double x = 0; double y = 0;
          for (int i = 0; i < size; i++){
            double ccfactor = getCCF(t,i);
             x+=ccfactor*coords[i][0];
             y+=ccfactor*coords[i][1];
          }
          xyaT[0] = x; //hardcode cuz I don't wanna deal with any more pointer issues
          xyaT[1] = y;
        }

        void getvalFtangentvals(double t){
          double x = 0; double y = 0;
          for (int i = 0; i < size; i++){
            double ccfactor = getCCF(t,i);
             x+=ccfactor*coords[i][0];
             y+=ccfactor*coords[i][1];
          }
          tangentvals[0] = x; //hardcode cuz I don't wanna deal with any more pointer issues
          tangentvals[1] = y;
        }
        //formatting: t is iteration position, k is index, v is coordinate
        //Ptriangle is a set of precalculated binomial factors, up to degree 10
        double getCCF(double t, double k){
          return (Ptriangle[size-1][k])*pow((1-t),size-1-k)*pow(t,k); //to save performance we calc this once per coord
        };
  };

  //composite N-point bezier curve, currently set up to only go to degree 4. Custom points dataset to be implemented
  //for the moment, active speed and trajectory controls are going to be possible by code design, but we will use precalculated values for now until odometrycontroller's estimations are fixed
  //to implement that, each bezier must be generated at runtime upon completion of the last bezier, using the
  //velocity and heading values as subsitutes for second point of the bezier.
struct compositebezier{
      std::vector<beziernp> genarr; //WARNING: appending to this thing can kill your pointers
      //This constructor is for hard set degree 4 composites
      //data params: x, y, angle, angle scale factor
      //use the length value(raw size() output), not max index for size
      //if intaking a std::vector
      std::vector<beziernp> tanarr;
      compositebezier(std::vector<std::vector<double>> data){
        //size-1 b/c we add one in each array already
        for(int i = 0; i < data.size()-1; i++){ //tbd change to an std::vector.insert if possible
          double (*params)[2] = new double[4][2];
          double (*pslopes)[2] = new double[3][2];
          double pr[][2] = { //TBD throw this directly into the beziernp instead of creating it externally
            {data[i][0],data[i][1]},
            {data[i][3]*cos(data[i][2])+data[i][0],data[i][3]*sin(data[i][2])+data[i][1]}, //Speed configuration will be dealt with externally in the speed controller
            {data[i+1][0]-data[i+1][3]*cos(data[i+1][2]),data[i+1][1]-data[i+1][3]*sin(data[i+1][2])},  //so basically we will access params and multiply these values to do the speed transformation
            {data[i+1][0],data[i+1][1]}
          };
          //now, you may wonder what this seemingly redundent params and pr stuff is for. BezierNP takes a 2d array pointer, and pr is a temp variable. params is used to save the pr values permanently
          //this is b/c I cant figure out how to declare params directly like this, its a cheese solution but it indeed somehow works out
          //and as seemingly ridiculous the below solution is, yes, this is actually how you copy arrays.
          for (int y = 0; y < 2; y++){
            for (int x = 0; x < 4; x++){
              params[x][y] = pr[x][y];
              //printf("PARAMS: %d, %d : %f\n",x,y,params[x][y]);
            }
            for (int xo = 0; xo < 3; xo++){
              pslopes[xo][y] = pr[xo+1][y]-pr[xo][y]; //the slope of a bezier curve is another bezier curve where each point is the difference of each set of 2 points
            }
          }
          genarr.push_back(*new beziernp(params,4));
          tanarr.push_back(*new beziernp(pslopes,3));
        }
      };
      //safe, no deletion mode. In retrospect this is probably sufficient. Each beziernp is like 8 bytes so we shouldn't see any issues given that we have like 40MB of usable memory
      //percent input from 0-100, not 0.00-1.00
      void updvalnd(double pct){
    	  double ival = (pct/100)*genarr.size();
        genarr[(int)floor(ival)].getvalFxyaT(ival-floor(ival));
        tanarr[(int)floor(ival)].getvalFtangentvals(ival-floor(ival));
        tgtangent = fmod(atan2(tangentvals[1], tangentvals[0]),2*M_PI); //compute angle in radians to face to be tangent
      }
    };

//basic data instance which holds a the target and timeframe which to orient the bot
struct orientation{
  	double DR; //direction
  	double AT; //activation threshold
  	double DAT; //deactivation threshold
    //construction params: angle, activation threshold, deactivation threshold
  	orientation(double d, double a, double da){DR = d; AT = a; DAT = da;};
  };

//A combined structure to deal with a set of orientations within a single movement
class orientationscheme{
  	std::vector<orientation> orientationsys;
  	int ind;
  public:
  	orientationscheme(std::vector<std::vector<double>> orientationset){
  		ind = orientationset.size()-1; //-1 as we iterate backwards so we start at the last index, size-1
  		for (int i = ind; i > -1; i--){
  			orientationsys.push_back(
  				*new orientation(
  					orientationset[i][0], //direction
  					orientationset[i][1], //activation threshold
  					orientationset[i][2]  //deactivation threshold
  	        ));
  		}
  	}
  	//below: percent from 0-100, not 0.00-1.00
  	bool orientationset(double percent){
  		if(ind < 0){
  			anglemode = true; //enables automatic angle optimization if no more orientationsys targets left
  		}
      else return true; //if out of orientations that means we are done
  		//above: test first to prevent array out of bound error below if no index 0
  		if(orientationsys[ind].AT <= percent){ //disable automatic angle optimization and set angle target
  			anglemode = false;
  			xyaT[2] = orientationsys[ind].DR;
  		} else anglemode = true;
  		if (orientationsys[ind].DAT < percent){ //delete the current orientation object once it passes threshold
  			ind--;								    //            and reenable angle optimization
  		}
      return false;
  	}
  };

//this structure merges the speed controls, direction controls, and orientation controls into a single instance to access
//TBD: implement motorsysinterface too, will require the headers to not be scuffed as it comes after this hpp file
//potential fix approach - seperate the class definition into parts and pre-define everything at the top so everyone has refrences to everyone else
struct motion{
    double iterationperfac;
    double perc = 0;
  	dualScurve* g;
  	compositebezier* cb;
  	orientationscheme* ob;
  	motion(dualScurve *h, compositebezier *e, orientationscheme *o, double c){
  		g = h; cb = e; ob = o; iterationperfac = c;
  	}
  	bool computepath(){
      perc += iterationperfac;
      if (perc > 100) return true; //if percent > 100 return true
  		GVT = g->getval(perc); //update targeted velocity
  		cb->updvalnd(perc); //update target point
  		//ob->orientationset(perc); //update angle target and rotation mode
      return false;
    }
  };

struct intakecontroller{
    Motor left;
    Motor right;
    Motor bottomR;
    Motor topR;
    controller_digital_e_t Intake;
    controller_digital_e_t Outtake;
    //takes positive and negative values, pos for intake
    void intake_velocity(double velIN, double velOUT){
      left.move_velocity(velIN);
      right.move_velocity(velIN);
      topR.move_velocity(velOUT);
      bottomR.move_velocity(velOUT);
    }
    void input(){
      switch((int)ctrl.get_digital(Intake)-int(ctrl.get_digital(Outtake))){
        case -1: //outtake on, intake offset
          left.move_velocity(-100);
          right.move_velocity(-100);
          topR.move_velocity(-100);
          bottomR.move_velocity(-100);
          return;
        case 0: //both pressed or nothing pressed
          left.move_velocity(0);
          right.move_velocity(0);
          topR.move_velocity(0);
          bottomR.move_velocity(0);
          return;
        case 1: //intake on, outtake off
          left.move_velocity(100);
          right.move_velocity(100);
          topR.move_velocity(100);
          bottomR.move_velocity(100);
          return;
      }
    }
};



struct intakecommandset{
    intakecontroller* intakez;
    std::vector<std::vector<double>>* cmd; //format: 0: intake speed, 1: score speed,  2: start interval, 3: end interval, 4: timeout in units of 10ms per 1
    int index;
    int ticker;
    intakecommandset(std::vector<std::vector<double>>* comm, intakecontroller* intake){cmd = comm; intakez = intake; index = 0; ticker = 0;}
    bool intakeset(double perc){
      if (index >= cmd->size()) {
        intakez->intake_velocity(0,0);
        return true;
      }
      lcd::print(3,"Index: %d", index);
      lcd::print(4,"IN SPD: %f", cmd->at(index)[0]);
      lcd::print(5,"OUT SPD: %f", cmd->at(index)[1]);
      lcd::print(6,"threshold: %d,  ticker: %d", (int)cmd->at(index)[4],ticker);
      if(cmd->at(index)[3] < perc || ticker == (int)(cmd->at(index)[4])){
        ticker = 0;
        intakez->intake_velocity(0,0);
        index++;
        return false;
      }
      if(cmd->at(index)[2] >= perc){
        intakez->intake_velocity(cmd->at(index)[0],cmd->at(index)[1]);
        ticker++;
        return false;
      }
      return false;
    }
};

struct linearmotion{
  double x, y, tang;
  dualScurve* g;
  orientationscheme* ob = NULL;
  intakecommandset* ef = NULL;
  linearmotion(double xa, double ya, double tng){
    tang = tng; x = xa; y = ya;
  }
  linearmotion(double xa, double ya, double tng, intakecommandset *e){
    tang = tng; x = xa; y = ya; ef = e;
  }
  linearmotion(double xa, double ya, orientationscheme* os){
    ob = os; x = xa; y = ya;
  }
  linearmotion(double xa, double ya, orientationscheme* os, intakecommandset *e){
    ob = os; x = xa; y = ya; ef = e;
  }
  void set_tgt(){
    GLOBAL_PERC_COMPLETION = 0;
    xyaT[0] = x;
    xyaT[1] = y;
    tgtangent = fmod(atan2(xyaT[1],xyaT[0]),2*M_PI);
    if (ob != NULL) ob->orientationset(GLOBAL_PERC_COMPLETION);
    else xyaT[2] = tang;
  }
  bool updatesystems(){
    bool c = true;
    if (ef != NULL) c*=ef->intakeset(GLOBAL_PERC_COMPLETION);
    if (ob != NULL) c*=ob->orientationset(GLOBAL_PERC_COMPLETION);
    return c;
  }
};
/*PID: generic PID system*/
//NOTE: DEFAULT TGT = 0
struct PID{
//private:
  /*Integral mode configurations:
  false: Direct I scaling - 100% of I is added each cycle until the I limit. Confirmed working
  true: Asymptope I - I approaches I limit with each iteration. To be tested*/
  bool Imode = 0;

  /*Proportional mode configurations
  false: Raw input
  true: External S curve, percent to target based
  */
  bool Pmode = 0;

  //Izerocutoff: turns I to 0 when target reached
  bool Izerocutoff = true;

  //Pk, Ik, Dk, and I fraction array
  double* ratios;

  //selected dualScurve for Pmode = 1
  dualScurve* Scurve;

  //the max limits for the loop
  double maxIlimit = 0;
  double maxlimit = 0;

  //P, I, D, position, and target array
  double PIDa[4] = {0,0,0,0};

  double lasterror = 0;
//public:
  PID(double scalers[], bool ms[], double limits[]){
    ratios = scalers; Pmode = ms[0]; Imode = ms[1]; Izerocutoff = ms[2]; maxlimit = limits[0]; maxIlimit = limits[1];
  }
  //note: if dualScurve is to be used, input percentage to target values
  PID(double scalers[], bool ms[], double limits[], dualScurve curve){ //note: the dualScurve here may need to be adjusted
    ratios = scalers; Scurve = &curve; Pmode = ms[0]; Imode = ms[1]; Izerocutoff = ms[2]; maxlimit = limits[0]; maxIlimit = limits[1];
  }
  //sets a new target for the loop w/o resetting PIDa
  void set_tgt_soft(double tgt){
    PIDa[3] = tgt;
  }
  //sets a new target for the loop and resets PIDa
  void set_tgt_clean(double tgt){
    PIDa[0]=0;PIDa[1]=0;PIDa[2]=0; //this is stupid
    PIDa[3] = tgt;
  }
  double update(double in){
    double err = (PIDa[3]-in);
    if (Pmode) PIDa[0] = isposorneg(err)*Scurve->getval(err);
    else PIDa[0] = err;
    if (Imode) PIDa[1] += (err-PIDa[1])/ratios[3];
    else PIDa[1] += err;
    if (Izerocutoff == true && round(in*10) == 0) PIDa[1] = 0;
    if (fabs(PIDa[1]) > maxIlimit) PIDa[1] = isposorneg(PIDa[1])*maxIlimit;
    PIDa[2] = err-lasterror;
    lasterror = err;
    if (isnanf(PIDa[0])) PIDa[0] = 0;
    if (isnanf(PIDa[1])) PIDa[1] = 0;
    if (isnanf(PIDa[2])) PIDa[2] = 0;
  //  lcd::print(3,"P: %f", PIDa[0]);
    double final = PIDa[0]*ratios[0] + PIDa[1]*ratios[1] + PIDa[2]*ratios[2];
    //lcd::print(4,"I: %f", PIDa[1]);
    //lcd::print(5,"D: %f", PIDa[2]);
    return isposorneg(final)*determinesmallest(fabs(final),maxlimit);
  }
};
