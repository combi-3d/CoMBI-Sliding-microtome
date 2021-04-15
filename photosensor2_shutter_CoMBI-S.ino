/*
  This sketch is used for CoMBI-S prototype system (low-cost type shown in Suppl.Fig S2).
  
  Arduino Uno R3 controls parts below:
  1. Photosensors (Omron, EE-SY310 or EE-SY410 detect the handle of the microtome.
  2. Optocouplers (Toshiba, TLP785GB) fix focus (half-push), and release shutter, when the handle passing above the sensors.
  3. DIP switch 1 selects photoreflector type (HIGH = Omron EE-SY310, LOW = Omron EE-SY310).
  4. DIP switch 2 selects the direction of the handle when the shutter is released (HIGH = going back(default), LOW = going forward).
  5. Push switches (alt, temp) are used to start serial imaging, or mamual camera control.
  6. LEDs indicate; executing serial imaging, detecting handle, fixing focus (half-push) and releasing shutter. 
 
  This work is licensed under a Creative Commons Attribution 4.0 International License.
  <http://creativecommons.org/licenses/by/4.0/>

  (c) 2016, Tohru Murakami, Gunma University.
*/

// Hardware constants. Adjust these values per installations.

const int latentcy = 100; // msec
const float baselineVoltage = 2.5; // threshold voltage of the photosensor; 0 – 5.0

// Pin assignments

// DIPSW Switch inputs

const int DIPSW_sensorTypeMismatch = 12;
const int DIPSW_sensorDirection = 13;

// Switch inputs
const int SWPin_autoControl = 3; // the auto-control switch
const int SWPin_contHalfPush = 4; // the continuous-half-push switch
const int SWPin_manualRelease = 5; // the manual-shutter-release switch

// Switch indicator outputs
const int ledPin_autoControl = 7; // the auto-control indicator
const int ledPin_contHalfPush = 6; // the continuous-half-push indicator

// Shutter indicator outputs
const int ledPin_halfPush = 8; // the half-push indicator
const int ledPin_release = 9; // the shutter-release indicator

// Shutter outputs
const int optoPin_halfPush = 11; // the half-push optocoupler
const int optoPin_release = 10; // the shutter-release optocoupler

// Sensor inputs (for default installation)
int sensorPin_halfPush = A1; // the half-push photosensor
int sensorPin_release = A0; // the shutter-release photosensor

// Serial communication rate

const int serialSpeed = 9600;

// Shutter status aliases

const int interval = 0;
const int standby = 1;

// DIPSW switch values (0 for default installation)

int sensorTypeMismatch = 0;
int sensorDirection = 0;

// Variables

int releaseCounter = 0; // count shutter release
int shutterStatus = interval;
int autoControl = 0;
int contHalfPush = 0;
int manualRelease = 0;
int sensorVal_halfPush; // 10 bit unsigned integer
int sensorVal_release; // 10 bit unsigned integer
float voltage_halfPush; // 0.0–5.0
float voltage_release; // 0.0–5.0

void setup() {

  int temp;

  // Assign output pins
  pinMode(optoPin_halfPush, OUTPUT);
  pinMode(optoPin_release, OUTPUT);
  pinMode(ledPin_autoControl, OUTPUT);
  pinMode(ledPin_contHalfPush, OUTPUT);
  pinMode(ledPin_halfPush, OUTPUT);
  pinMode(ledPin_release, OUTPUT);

  // Assign switche pins
  pinMode(DIPSW_sensorTypeMismatch, INPUT_PULLUP);
  // HIGH = light-ON sensor for dark signal or dark-ON for light signal; LOW = matched
  pinMode(DIPSW_sensorDirection, INPUT_PULLUP); //HIGH reverse; LOW default
  pinMode(SWPin_autoControl, INPUT_PULLUP); //HIGH automatic; LOW no shutter control
  pinMode(SWPin_contHalfPush, INPUT_PULLUP); //HIGH continuous half push; LOW sensor-controled half push
  pinMode(SWPin_manualRelease, INPUT_PULLUP); //HIGH shutter release

  // initialize output pins
  digitalWrite(ledPin_autoControl, LOW);
  digitalWrite(ledPin_contHalfPush, LOW);
  digitalWrite(optoPin_halfPush, LOW);
  digitalWrite(ledPin_halfPush, LOW);
  digitalWrite(optoPin_release, LOW);
  digitalWrite(ledPin_release, LOW);

  // read the DIP switches
  sensorTypeMismatch = digitalRead(DIPSW_sensorTypeMismatch); // HIGH = default, LOW = mismatch
  sensorDirection = digitalRead(DIPSW_sensorDirection);   // HIGH = default, LOW = reverse

  // adjust the sensor direction
  if (sensorDirection == LOW) {
    temp = sensorPin_halfPush; // Swap the sensor pin assignments
    sensorPin_halfPush = sensorPin_release;
    sensorPin_release = temp;
  }

  // initialize serial communication
  Serial.begin(serialSpeed);

  // print message when done.
  Serial.println("Photosensor-driven shutter controler ready!");
}

void loop() {

  // read the switches

  manualRelease = digitalRead(SWPin_manualRelease);
  autoControl = digitalRead(SWPin_autoControl);
  contHalfPush = digitalRead(SWPin_contHalfPush);

  if (manualRelease == LOW) { // releaes the shutter when the manual shutter release switch is pressed.

    Serial.print("Manual ");
    release();

  } else if (autoControl == LOW) {

    digitalWrite(ledPin_autoControl, HIGH);

    if (contHalfPush == LOW) { // leave the half-push HIGH when the continuous-half-push switch is ON

      digitalWrite(ledPin_contHalfPush, HIGH);
      digitalWrite(optoPin_halfPush, HIGH);

    } else if (shutterStatus == interval) {

      digitalWrite(ledPin_contHalfPush, LOW);
      digitalWrite(optoPin_halfPush, LOW);

    };

    // read the sensor values and convert them to voltage levels

    sensorVal_halfPush = analogRead(sensorPin_halfPush);
    sensorVal_release = analogRead(sensorPin_release);

    voltage_halfPush = (sensorVal_halfPush / 1024.0) * 5.0;
    voltage_release = (sensorVal_release / 1024.0) * 5.0;

    //     print the voltage levels on the serial port

    Serial.print("half push sensor: ");
    Serial.print(voltage_halfPush);
    Serial.print(", release sensor: ");
    Serial.println(voltage_release);

    if (sensorTypeMismatch == LOW) { // invert the sensor voltages when sensorTypeMismatch is bright ON

      voltage_halfPush = 5.0 - voltage_halfPush;
      voltage_release = 5.0 - voltage_release;

    };


    if (voltage_halfPush > baselineVoltage && voltage_release <= baselineVoltage && shutterStatus == interval) {
      // turn half-push HIGH when the half-push sensor detects something.

      digitalWrite(optoPin_halfPush, HIGH);
      digitalWrite(ledPin_halfPush, HIGH);

      shutterStatus = standby;

    };

    if (voltage_release > baselineVoltage && shutterStatus == standby) {
      // release the shutter when the release sensor detects something.

      release();

      shutterStatus = interval;

    };

  } else { // make everything zero when autoControl SW is off.

    Serial.println("OFF");
    digitalWrite(ledPin_autoControl, LOW);
    digitalWrite(ledPin_contHalfPush, LOW);
    digitalWrite(optoPin_halfPush, LOW);
    digitalWrite(ledPin_halfPush, LOW);
    digitalWrite(optoPin_release, LOW);
    digitalWrite(ledPin_release, LOW);

    shutterStatus = interval;

  };
}

void release() {

  digitalWrite(optoPin_halfPush, HIGH);
  digitalWrite(ledPin_halfPush, HIGH);

  delay(latentcy); // give the optocoupler a moment to activate

  digitalWrite(optoPin_release, HIGH);
  digitalWrite(ledPin_release, HIGH);

  delay(latentcy); // give the optocoupler a moment to activate

  digitalWrite(optoPin_release, LOW);
  digitalWrite(ledPin_release, LOW);
  digitalWrite(ledPin_halfPush, LOW);

  if (contHalfPush != LOW) {

    digitalWrite(optoPin_halfPush, LOW);

  };

  Serial.print("# ");
  Serial.println(releaseCounter++);

}
