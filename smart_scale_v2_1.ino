/*
 Keytracker Scale Sketch
 Setup the scale and start the sketch WITHOUT a weight on the scale
 Once readings are displayed place the weight on the scale
 Press +/- or a/z to adjust the calibration_factor until the output readings match the known weight
 Arduino pin 13 -> HX711 CLK
 Arduino pin 14 -> HX711 DOUT
 Arduino pin 5V -> HX711 VCC
 Arduino pin GND -> HX711 GND 
*/

// this calibration factor is adjusted according to each load cell, this was a close approximation but a proper calibration reading would be required for accuracy
// maybe after a lot of know weight readings...
float calibration_factor = 420.71;
float previous_units = 0;
float units = 0;
float ounces = 0;

// ##################################################################### start HX711  #############################################################
// ************ start declarations for reading from HX711 ************
byte PD_SCK = 13;  // Power Down and Serial Clock Input Pin
byte DOUT = 12;    // Serial Data Output Pin
byte GAIN = 0;    // amplification factor for channel A and B
long OFFSET;  // used for tare weight
float SCALE;  // used to return weight in grams, kg, ounces, whatever
// ************ end declarations for reading from HX711 ************

// define clock and data pin, channel, and gain factor
// channel selection is made by passing the appropriate gain: 128 or 64 for channel A, 32 for channel B
// gain: 128 or 64 for channel A; channel B works with 32 gain factor only
void setup_HX711(){
  pinMode(PD_SCK, OUTPUT);
  pinMode(DOUT, INPUT);
  set_gain(64);
}
// check if HX711 is ready
// from the datasheet: When output data is not ready for retrieval, digital output pin DOUT is high. Serial clock
// input PD_SCK should be low. When DOUT goes to low, it indicates data is ready for retrieval.
bool is_ready(){
  return digitalRead(DOUT) == LOW;
}

// set the gain factor; takes effect only after a call to read()
// channel A can be set for a 128 or 64 gain; channel B has a fixed 32 gain
// depending on the parameter, the channel is also set to either A or B
void set_gain(byte gain){
  switch (gain) {
    case 128:   // channel A, gain factor 128
      GAIN = 1;
      break;
    case 64:    // channel A, gain factor 64
      GAIN = 3;
      break;
    case 32:    // channel B, gain factor 32
      GAIN = 2;
      break;
  }
  digitalWrite(PD_SCK, LOW);
  read();
}

// waits for the chip to be ready and returns a reading
long read(){
  // wait for the chip to become ready
  while (!is_ready());

    unsigned long value = 0;
    byte data[3] = { 0 };
    byte filler = 0x00;

  // pulse the clock pin 24 times to read the data
    data[2] = shiftIn(DOUT, PD_SCK, MSBFIRST);
    data[1] = shiftIn(DOUT, PD_SCK, MSBFIRST);
    data[0] = shiftIn(DOUT, PD_SCK, MSBFIRST);

  // set the channel and the gain factor for the next reading using the clock pin
  for (unsigned int i = 0; i < GAIN; i++) {
    digitalWrite(PD_SCK, HIGH);
    digitalWrite(PD_SCK, LOW);
  }

    // Datasheet indicates the value is returned as a two's complement value
    // Flip all the bits
    data[2] = ~data[2];
    data[1] = ~data[1];
    data[0] = ~data[0];

    // Replicate the most significant bit to pad out a 32-bit signed integer
    if ( data[2] & 0x80 ) {
        filler = 0xFF;
    } else if ((0x7F == data[2]) && (0xFF == data[1]) && (0xFF == data[0])) {
        filler = 0xFF;
    } else {
        filler = 0x00;
    }

    // Construct a 32-bit signed integer
    value = ( static_cast<unsigned long>(filler) << 24
            | static_cast<unsigned long>(data[2]) << 16
            | static_cast<unsigned long>(data[1]) << 8
            | static_cast<unsigned long>(data[0]) );

    // ... and add 1
    return static_cast<long>(++value);
}

// set OFFSET, the value that's subtracted from the actual reading (tare weight)
void set_offset(long offset = 0){
  OFFSET = offset;  
}

// get the current OFFSET
long get_offset(){
  return OFFSET; 
}

// set the SCALE value; this value is used to convert the raw data to "human readable" data (measure units)
void set_scale(float scale = 1.f){
  SCALE = scale;
}

// get the current SCALE
float get_scale(){
  return SCALE;  
}

// returns an average reading; times = how many times to read
long read_average(byte times = 10){
  long sum = 0;
  for (byte i = 0; i < times; i++){
    sum += read();
  }
  return sum / times;
}

// returns (read_average() - OFFSET), that is the current value without the tare weight; times = how many readings to do
double get_value(byte times = 1){
  return read_average(times) - OFFSET;
}

// returns get_value() divided by SCALE, that is the raw value divided by a value obtained via calibration
// times = how many readings to do
float get_units(byte times = 1){
  return get_value(times) / SCALE; 
}

// set the OFFSET value for tare weight; times = how many times to read the tare value
void tare(byte times = 10){
  double sum = read_average(times);
  set_offset(sum);  
}

// puts the chip into power down mode
void power_down(){
  digitalWrite(PD_SCK, LOW);
  digitalWrite(PD_SCK, HIGH);  
}

// wakes up the chip after power down mode
void power_up(){
  digitalWrite(PD_SCK, LOW);  
}
// ##################################################################### end HX711  #############################################################

void setup() {
  Serial.begin(115200);
  setup_HX711();
  set_scale();
  delay(2000); // let the scale settle - may not be needed when correct calibration factor is used
  tare();  //Reset the scale to 0
}

void loop() {
  measure_weight();
  yield();
}

void scale_calibration(){
  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '+' || temp == 'a')
      calibration_factor += 0.01;
    else if(temp == '-' || temp == 'z')
      calibration_factor -= 0.01;
    else if (temp == 'r') // to read on the other application
      Serial.println(units); // change to ounces if we want imperial instead of metric grams
  }
// THESE LINES CAN BE USED FOR DEBUGGING THE CALIBRATION FACTOR, SIMPLY UNCOMMENT TO DISPLAY
  // for debug reasons it is nice to output what calibration factor we are currently on
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.print(", raw weight: ");
  Serial.print(units); // change to ounces if we want imperial instead of metric grams
  Serial.print("g");
  // round the weight 
  Serial.print(", rounded weight: ");
  Serial.print(round(units)); // change to ounces if we want imperial instead of metric grams
  Serial.println("g"); 
}

void measure_weight(){ 
  set_scale(calibration_factor); //Adjust this calibration factor if needed

  previous_units = units;
  delay(1000);
  units = get_units(20); // read the ADC output. Returns the average of 20 readings
  
  if (units < 0){
    units = 0.00;
  }
  ounces = units * 0.035274;
  
   //THESE LINES CAN BE USED FOR DEBUGGING THE CALIBRATION FACTOR, SIMPLY UNCOMMENT TO DISPLAY
   //for debug reasons it is nice to output what calibration factor we are currently on
   Serial.print(" Weight: ");
   Serial.print(units); // change to ounces if we want imperial instead of metric grams
   Serial.println("g");

  // check if the weight has changed. This is done to avoid the variations due to vibration, sensitivity and so on
  if (abs(units - previous_units) > 0.5){
    Serial.println(" The weight has changed ");
    
    Serial.print(" Previous weight: ");
    Serial.print(previous_units);
    Serial.print("g");
    Serial.print(" Current weight: ");
    Serial.print(units);
    Serial.println("g");
  }

}
