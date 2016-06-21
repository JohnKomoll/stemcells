/*
   MotorDriveUnoPP sketch
   Drives syringe pumps using arduino Due, with the motors pins in parallel.

   This codes expects a message in the format: 1,4567,890*
   The first field is an indicator of 1 or 0 for direction
   The rest of the fields can be arbitrary size
   This code requires some arbitrary character
   to indicate the end of the command. Here I have used "*"
*/

#define numMtrs 3

//Define parallel pins for all motors
#define MS1 53
#define MS2 51
#define MS3 49
//Define pins for motor 1
#define EN_1  46
#define stp_1 44
#define dir_1 42
//Define pins for motor 2
#define EN_2  52
#define stp_2 50
#define dir_2 48
//Define pins for motor 3
#define EN_3  13
#define stp_3 11
#define dir_3 12



//Declare pins
int pins[numMtrs][6] = {
  {EN_1, MS1, MS2, MS3, stp_1, dir_1},
  {EN_2, MS1, MS2, MS3, stp_2, dir_2},
  {EN_3, MS1, MS2, MS3, stp_3, dir_3}
};
int dpins[6]; //the pins of the currently driven motor

//Declare variables for serial reading
char ui;                        //user serial input
const int NUMBER_OF_FIELDS = 4; //expected # of fields
int fieldIndex = 0;             //to iterate through fields
int values[NUMBER_OF_FIELDS];   //store our fields here
int done = 0;                   //indicator variable

//Declare motor drive variables
long xlim;  //indicate volume (in # of steps)
long spd_t; //delay time between steps (for controlling speed)

//Declare constants (an underscore indicates "per")
float mm_turn = 1.25;
float mcl_mm = (30.0 / 82) * 1000;
int stp_turn = 3200;
float stp_mcl = stp_turn / mcl_mm / mm_turn;
float mcs_stp = 1000000 / stp_mcl;
float errorCorrection = 1.025746;



void setup() {
  //Set pinModes:
  for (int i = 0; i < numMtrs; i++) {
    for (int j = 0; j < 6; j++) {
      pinMode(pins[i][j], OUTPUT);
    }
  }

  //Prepare for motor driving:
  resetBEDpins();     //Set step, direction, microstep and enable pins to default states
  Serial.begin(9600); //Open serial connection for displaying/debugging

  //Print control instructions:
  Serial.println("Begin fluid control\n");
  Serial.println("Enter parameters");
  Serial.println("Usage: <motor><direction>,<microLiters>,<microLiters/sec><*>");
  Serial.println("Example: 1,1,1000,100*");
  Serial.println("Where direction is given by 1 or 0 for in or out, respectively.\n");
}




//Main loop
void loop() {
  //Loop to acquire the serial fields:
  while (done == 0) {
    if (Serial.available()) {
      ui = Serial.read();
      if (ui >= '0' && ui <= '9') //check ascii value
        values[fieldIndex] = (values[fieldIndex] * 10) + (ui - '0');
      else if (ui == ',') {
        if (fieldIndex < NUMBER_OF_FIELDS - 1)
          fieldIndex++;
      }
      else
        //field acquisition is over
        done = 1; //indicator
    }
  }
  done = 0;

  //Diagnostic print statements:
  Serial.print("Motor: "); Serial.println(values[0]);
  Serial.print("Direction: "); Serial.println(values[1]);
  Serial.print("Volume: "); Serial.println(values[2]);
  Serial.print("Speed: "); Serial.println(values[3]);
  Serial.println();

  //Carry out command:
  for (int i = 0; i < 6; i++) {
    dpins[i] = pins[values[0] - 1][i];
  }
  SmallStepMode();

  //Reset for next command:
  for (int i = 0; i <= fieldIndex; i++)
    values[i] = 0;
  fieldIndex = 0;
  resetBEDpins();
  delay(1);
}




//Reset Big Easy Driver pins to default states
void resetBEDpins()
{
  for (int i = 0; i < numMtrs; i++) {
    for (int j = 0; j < 6; j++) {
      if (j == 0)
        digitalWrite(pins[i][j], HIGH);
      else
        digitalWrite(pins[i][j], LOW);
    }
  }
}




// Drive motor @ 1/16th microstep
void SmallStepMode()
{
  //Pull EN down to enable FETs and allow motor to move
  digitalWrite(dpins[0], LOW);
  //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(dpins[1], HIGH); digitalWrite(dpins[2], HIGH); digitalWrite(dpins[3], HIGH);
  //DIRECTION
  if (values[1] == 1)
    digitalWrite(dpins[5], HIGH); //Pull direction pin high to inject
  else
    digitalWrite(dpins[5], LOW); //Pull direction pin low to remove
  //VOLUME
  xlim = ceil(values[2] * stp_mcl * errorCorrection);
  Serial.print("Steps: "); Serial.println(xlim);
  //SPEED
  spd_t = ceil(mcs_stp / values[3] / 2);
  Serial.print("Delay: "); Serial.println(spd_t); Serial.println();
  //DRIVE
  if (spd_t <= 16383) //microsecond delay is only accurate for such values
  {
    for (int x = 1; x <= xlim; x++) //Loop the forward stepping enough times for motion to be visible
    {
      digitalWrite(dpins[4], HIGH); //Trigger one step forward
      delayMicroseconds(spd_t);
      digitalWrite(dpins[4], LOW);  //Pull step pin low so it can be triggered again
      delayMicroseconds(spd_t);
    }
  }
  else //must use millisecond delay
  {
    spd_t = ceil(spd_t / 1000.0);  //convert to milliseconds
    for (int x = 1; x <= xlim; x++) //Loop the forward stepping enough times for motion to be visible
    {
      digitalWrite(dpins[4], HIGH); //Trigger one step forward
      delay(spd_t);
      digitalWrite(dpins[4], LOW);  //Pull step pin low so it can be triggered again
      delay(spd_t);
    }
  }
}
