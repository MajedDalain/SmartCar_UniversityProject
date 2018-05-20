#include <Smartcar.h>
#include<math.h>


Car car;

SR04 frontSonar;
SR04 sideSonar;
SR04 backSonar;
GP2D120 backIR;
Gyroscope gyro(-5);


const int fSpeed = 40; //70% of the full speed forward
const int bSpeed = -35; //70% of the full speed backward
const int lDegrees = -75; //degrees to turn left
const int rDegrees = 75; //degrees to turn right

Odometer encoderLeft(210), encoderRight(210);
Servo myservo;

const int frontTrigPin = 6;
const int frontEchoPin = 7;
const int sideTrigPin = A11;
const int sideEchoPin = A12;
const int backTrigPin = A9;
const int backEchoPin = A10;

const int backIrPin = A0;

const int encoderLeftPin = 2;
const int encoderRightPin = 3;


const int SERVO_PIN = 50;

int spotSize;
int backDistanceInCm;
int frontDistanceInCm;
int sideDistanceInCm;
int irDistanceInCm;

int maxBackDistance = 13;
int maxFrontDistance = 20;
int maxSideDistance = 10;
int maxIrbackDistance = 7;

int pos = 0; // variable to store the servo position
int const offset = -5;


void setup() {
  Serial.begin(9600);
  Serial3.begin(9600);
  car.begin(); //initialize the car using the encoders and the gyro
  gyro.attach();
  delay(1500);
  gyro.begin();
  sideSonar.attach(sideTrigPin, sideEchoPin);
  backSonar.attach(backTrigPin, backEchoPin);
  frontSonar.attach(frontTrigPin,frontEchoPin);
  encoderLeft.attach(encoderLeftPin);
  encoderRight.attach(encoderRightPin);
  myservo.attach(SERVO_PIN);
  backIR.attach(backIrPin);
  car.begin(encoderLeft,encoderRight);
  car.begin(gyro);

}
// what is executed here depends on the input of the user
void loop() {
  handleInput();
}

/*
handles input from both the Serial or the usb cable between Pi and Arduino, and the 
input from the blutooth modeule as Serial3
*/
void handleInput() {
  if (Serial3.available()) {
    char input = Serial3.read(); //read everything that has been received so far and log down the last entry
    switch (input) {
      case 'A': //rotate counter-clockwise going forward
        findSpot();
        delay(1000);
        parkInSpot();
        break;

      case 'r': //turn clock-wise
        car.setSpeed(fSpeed);
        car.setAngle(30);
        findSpot();
        break;

      case 's': //go ahead
        car.setSpeed(fSpeed);
        car.setAngle(25);
        findSpot();
        break;

      case 'p': //go back
      parkInSpot();
        break;
        
      default: //if you receive something that you don't know, just stop
        car.setSpeed(0);
        car.setAngle(0);
    }
  }

//  if(Serial.available()){
//    int input  = Serial.read();
//    switch (input) {
//
//      case 1:
//      car.setSpeed(0);
//      break;
//
//      case 2:
//      car.setSpeed(fSpeed);
//      break;
//
//      default:
//      car.setSpeed(0);
//
//    }
//  }
}

/* method to find a sufficiently large spot
where the car can park later on */
void findSpot(){
  const int ENOUGH_SPACE = 40;  
  int spotStartLeft,spotStartRight,spotEndRight,spotEndLeft,rightDistance,totalLengthLeft, totalLengthRight;
  while(car.getSpeed()!= 0) {
     rightDistance = sideSonar.getMedianDistance();
    if(rightDistance == 0 || rightDistance > 35){ // start measuring distance of gap, once a gap is found
       encoderLeft.begin();
       encoderRight.begin();
       Serial3.println(" WOW let me check this Spot!");
       
       while(rightDistance == 0 || rightDistance > 25) {  // keep measuring gap
           rightDistance = sideSonar.getDistance();
           rightDistance = sideSonar.getDistance();
           
        if(encoderLeft.getDistance() >= ENOUGH_SPACE ||  encoderRight.getDistance() >= ENOUGH_SPACE ){
               car.setSpeed(0);
               Serial3.println("ِ ِEnough size for Spot :D");
               break;
           }        
      }

    }
    else {
      Serial3.println(" right Distance is" + rightDistance );
      Serial3.println(" NO spot detected ! "); // if gap is found but is less than set value for minimum spotsize, it is not recognized as a parking spot
    }
  }
}


void driveBackwardOnSpot(){
  irDistanceInCm = backIR.getMedianDistance();
  backDistanceInCm = backSonar.getMedianDistance();
  myservo.write(0);
  delay(500);
    if((irDistanceInCm > maxIrbackDistance  || irDistanceInCm==0) && (backDistanceInCm > maxBackDistance || backDistanceInCm ==0)){
    car.setSpeed(bSpeed);
    car.setAngle(0);
    while(car.getSpeed()< 0){
      irDistanceInCm = backIR.getMedianDistance();
      backDistanceInCm = backSonar.getMedianDistance();
      if((irDistanceInCm < maxIrbackDistance  && irDistanceInCm!=0)||(backDistanceInCm < maxBackDistance && backDistanceInCm !=0)){
        car.setSpeed(0);
        break;
      }
    }
  }
  else {
    car.setSpeed(0);
    Serial3.println(" No enough Distance To drive Backward! ");
  }
  delay(500);
  myservo.write(55);
 }


// method to park the car once i stopped after finding a parking spot
void parkInSpot(){
  int rotateDegree = -35;
  driveBack();
  rotateOnSpot(rotateDegree/2);
  delay(2000);
  rotateOnSpot(rotateDegree/2);
  delay(2000);
  driveBackwardOnSpot();
  delay(1000);
  rotateOnSpot(-rotateDegree/2);
  delay(2000);
  rotateOnSpot(-rotateDegree/2);
  delay(2000);
  middlePark();

}

void rotateOnSpot(int targetDegrees) {
  targetDegrees %= 360;
  if (!targetDegrees){
     return;
  }
  if (targetDegrees > 0) {
    car.setMotorSpeed(fSpeed, 0); // or 0 I am not sure 
  } else {
    car.setMotorSpeed(-fSpeed, fSpeed);
  }

  unsigned int initialHeading = gyro.getAngularDisplacement();
  int degreesTurnedSoFar = 0;

  while (abs(degreesTurnedSoFar) < abs(targetDegrees)) {
    gyro.update();
    int currentHeading = gyro.getAngularDisplacement();

    if ((targetDegrees < 0) && (currentHeading > initialHeading)) {
      currentHeading -= 360;
    } else if ((targetDegrees > 0) && (currentHeading < initialHeading)) {
      currentHeading += 360;
    }

    degreesTurnedSoFar = initialHeading - currentHeading;
  }
 car.setSpeed(0);
}

void middlePark(){
  int frontDistance = frontSonar.getMedianDistance();
  Serial3.println("the front distance is");
  Serial3.println(frontDistance);
  int backDistance = backSonar.getMedianDistance();
  Serial3.println("the back distance is");
  Serial3.println(backDistance);

  int distanceToGo =  frontDistance - backDistance ;
  Serial3.println("the To GO distance is");
  Serial3.println(distanceToGo/2);
  car.go((distanceToGo/4));
  delay(1500);
}


void driveBack(){
  sideDistanceInCm = sideSonar.getMedianDistance();
  Serial3.println(sideDistanceInCm);
  while(sideDistanceInCm < 25 && sideDistanceInCm != 0){
    car.go(-3);
    sideDistanceInCm = sideSonar.getMedianDistance();
    delay(1000);
  }
  sideDistanceInCm = sideSonar.getDistance();
  if(sideDistanceInCm < 25 && sideDistanceInCm != 0)
  car.go(-3);

}

 void straightenCarAngle(){
  int angle;
    gyro.update();
    int gyroAngle = gyro.getAngularDisplacement();

    Serial3.println("initial angle  is ");
    Serial3.println(gyroAngle);
    
    if(gyroAngle > 180 && gyroAngle < 360){
    angle = 360 - gyroAngle;
    Serial3.println("after angle  is ");
    Serial3.println(angle);
   }
   else if(gyroAngle < 180 && gyroAngle > 0){
    angle = gyroAngle;
   }


 car.rotate(angle/2);
 Serial3.println(angle);
 Serial3.println("rotated");
 delay(1000);
  }

 void straightenCar() {
  gyro.update();
  delay(1000);
  int turn = gyro.getAngularDisplacement();
  Serial3.println("car deviated " + turn);
  if(turn > 2 && turn < 180){
    car.setMotorSpeed(fSpeed/1.5,fSpeed);

  }
    else if (turn > 180 && turn < 358) {
      car.setMotorSpeed(fSpeed,fSpeed/1.5);
    }
    else
      car.setMotorSpeed(fSpeed,fSpeed);
  }

