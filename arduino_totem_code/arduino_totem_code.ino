#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <DS3231.h>
#include <Adafruit_NeoPixel.h>
#include <Keypad.h>
#include <EEPROM.h>

// General
#define DELAY_TIME 50
#define WRITE_FREQUENCY 1000
int write_delay_counter = 0;
float max_db = 0.0;
float db_total = 0.0;
int loop_counter = 0;

// LCD
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// SD Utility:
Sd2Card card;
SdVolume volume;
SdFile root;
File myFile;

const int chipSelect = 10;
#define SoundSensorPin A1  //this pin read the analog voltage from the sound level meter
#define VREF 5.0  //voltage on AREF pin,default:operating voltage
float calibration = 1.00;

// GPS
double latitude,longitude;
TinyGPSPlus gps;

// Realtime Clock
DS3231 clock;
RTCDateTime dt;

// LED Panel
#define LED_PIN 6
#define LED_COUNT 256
#define BRIGHTNESS 5
int upper_db_limit = 90;
int lower_db_limit = 45;
#define ROWS 32
#define COLS 8
Adafruit_NeoPixel strip((ROWS * COLS), LED_PIN, NEO_GRB + NEO_KHZ800);

// Keypad
const byte KEY_ROWS = 4; //four rows
const byte KEY_COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[KEY_ROWS][KEY_COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[KEY_ROWS] = {23,25,27,29}; //connect to the row pinouts of the keypad
byte colPins[KEY_COLS] = {31,33,35,37}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, KEY_ROWS, KEY_COLS); 


//EEPROM
#define LOWER_DB_LIMIT_ADDR 69
#define UPPER_DB_LIMIT_ADDR 70
#define CALIBRATION_ADDR 71

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a startup message to the LCD.
  lcd.print("BONNAROO!");

  //  setup_sd_logging();
  Serial.println("Initializaing Real Time Clock");
  clock.begin();
  Serial.println("Real Time Clock up and running!");

  read_from_eeprom();

  strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show(); // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS); // Set BRIGHTNESS to about 1/5 (max = 255)
}

void setup_sd_logging() {
  
//    Serial.print("\nInitializing SD card...");
//
//    // Set the actual ouput pin
//    pinMode(53, OUTPUT);
//
//    // Set a fake pin for our SD card
//    SD.begin(10);
//  
//    // we'll use the initialization code from the utility libraries
//    // since we're just testing if the card is working!
//    if (!card.init(SPI_HALF_SPEED, chipSelect)) {
//      Serial.println("initialization failed. Things to check:");
//      Serial.println("* is a card inserted?");
//      Serial.println("* is your wiring correct?");
//      Serial.println("* did you change the chipSelect pin to match your shield or module?");
//      return;
//    } else {
//      Serial.println("Wiring is correct and a card is present.");
//    }
}

void loop() {

  int triggered = 0;
 
  while (Serial1.available() > 0)
    gps.encode(Serial1.read());
    
  if (gps.location.isUpdated()) {
    latitude = gps.location.lat();
    longitude =  gps.location.lng();
  }
  else if (gps.date.isUpdated() or gps.time.isUpdated()) {
    clock.setDateTime(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
  }

  float voltageValue,dbValue;
  voltageValue = analogRead(SoundSensorPin) / 1024.0 * VREF;
  dbValue = (voltageValue * calibration) * 50.0;  //convert voltage to decibel value
  
  char dateBuffer[19];
  dt = clock.getDateTime();
  sprintf(dateBuffer,"%04u/%02u/%02u %02u:%02u:%02u",dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);

  int board[ROWS][COLS];
  get_board(board);
  display_sound(board, dbValue);

  write_delay_counter += 1;
  smartDelay(DELAY_TIME, dateBuffer, dbValue);
}

// free RAM check for debugging. SRAM for ATmega328p = 2048Kb.
int availableMemory() {
    // Use 1024 with ATmega168
    int size = 2048;
    byte *buf;
    while ((buf = (byte *) malloc(--size)) == NULL);
        free(buf);
    return size;
}

String pad_with_spaces(String output_to_pad) {
  String empty_screen = "                ";
  int whitespace_size = empty_screen.length() - output_to_pad.length();
  return output_to_pad + empty_screen.substring(0, whitespace_size);
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms, char dateBuffer[], float dbValue) {  
  unsigned long start = millis();
  do 
  {
    while (Serial1.available())
      gps.encode(Serial1.read());

    char customKey = customKeypad.getKey();
    if (customKey){
      lcd.setCursor(0, 1);
      switch(customKey){
        case '1': 
          Serial.println("CALIBRATE UP"); 
          calibration += 0.01;
          EEPROM.write(CALIBRATION_ADDR, calibration * 100);
          lcd.print(pad_with_spaces((String) "ADJ: " + calibration));
          break;     
        case '4': 
          Serial.println("CALIBRATE DOWN");
          calibration -= 0.01;
          EEPROM.write(CALIBRATION_ADDR, calibration * 100);
          lcd.print(pad_with_spaces((String) "ADJ: " + calibration));
          break;
        case '3': 
          Serial.println("MAX DB UP");
          upper_db_limit += 1;
          EEPROM.write(UPPER_DB_LIMIT_ADDR, upper_db_limit);
          lcd.print(pad_with_spaces((String) "MAX DB: " + upper_db_limit));
          break;
        case '6': 
          Serial.println("MAX DB DOWN");  
          upper_db_limit -= 1;
          EEPROM.write(UPPER_DB_LIMIT_ADDR, upper_db_limit);
          lcd.print(pad_with_spaces((String) "MAX DB: " + upper_db_limit));
          break;
        case '2': 
          Serial.println("MIN DB UP");
          lower_db_limit += 1;
          EEPROM.write(LOWER_DB_LIMIT_ADDR, lower_db_limit);
          lcd.print(pad_with_spaces((String) "MIN DB: " + lower_db_limit));
          break;
        case '5': 
          Serial.println("MIN DB DOWN");  
          lower_db_limit -= 1;
          EEPROM.write(LOWER_DB_LIMIT_ADDR, lower_db_limit);
          lcd.print(pad_with_spaces((String) "MIN DB: " + lower_db_limit));
          break;
        case 'A':
          Serial.println("Flash them police lights!");  
          police_lights();
          break;
        default: 
          Serial.println(customKey);
      }
    }

    if (dbValue > max_db) {
      max_db = dbValue;
    }

    db_total += dbValue;
    loop_counter += 1;
    
    if (write_delay_counter * DELAY_TIME >= WRITE_FREQUENCY) {

      float average_db = db_total / loop_counter;

      String avg_db = (String) average_db;
      String LAT = (String) latitude; 
      String LON = (String) longitude;

      Serial.println("A," + LAT + "," + LON + "," + dateBuffer + "," + avg_db);
  
      String output = (String) max_db + " dBA";
      lcd.setCursor(0, 0);
      lcd.print(pad_with_spaces(output));
      lcd.setCursor(0, 1);
      output = (String) LAT + ", " + LON;
      lcd.print(pad_with_spaces(output));

//      myFile = SD.open("logging.txt", FILE_WRITE);
////      myFile.print("A,");
////      myFile.print(latitude, 8);
////      myFile.print(",");
////      myFile.print(longitude, 8);
////      myFile.print(",");
////      myFile.print(dateBuffer);
////     myFile.print(",");
////      myFile.println(average_db,1);
//      myFile.println("A," + LAT + "," + LON + "," + dateBuffer + "," + avg_db);
//      myFile.close();  
  
      write_delay_counter = 0;
      db_total = 0;
      max_db = 0;
      loop_counter = 0;
  }
    
  } while (millis() - start < ms);
}

void police_lights() {
  uint32_t red = strip.Color(255, 0, 0);
  uint32_t blue = strip.Color(0, 0, 255);
  int number_of_loops;

  for (int counter = 1; number_of_loops <= 10; counter++) {
    color_entire_matrix(blue);
    delay(250);
    color_entire_matrix(red);
    delay(250);
  }
}

void color_entire_matrix(uint32_t color) {
  for (int x = 0; x < LED_COUNT; x++) {
    strip.setPixelColor(x, color);
  }
  strip.show();
}

void display_sound(int board[ROWS][COLS], float dbValue) {
  strip.clear();
 
  int height = get_height(dbValue);
  
  uint32_t colors[ROWS];
  get_colors(colors);
  
  int cap_size = set_cap(board, height, colors);

  for (int i = 0; i < height - cap_size; i++) {

    for (int x = 0; x <= 7; x++) {
      strip.setPixelColor(board[i][x], colors[i]);
    }
  }
  strip.show(); 
}

int set_cap(int board[ROWS][COLS], int height, uint32_t colors[ROWS]) {

  int cap_size = 2;
  
  if (height >= 10 and height < 20) {
    cap_size = 3;
  }
  else if (height >= 20 and height <= ROWS) {
    cap_size = 4;
  }

  if ((height - cap_size) < 1) {
    cap_size = 0;
  }

  int indent = 1;
  for (int i = height - cap_size; i < height; i++) {
    
    for (int x = 0 + indent; x <= 7 - indent; x++) {
      strip.setPixelColor(board[i][x], colors[i]);
    }
    
    if (indent > 2) {
      indent = 3;
    }
    else {
      indent += 1;
    }
  }
  return cap_size;
}

int get_height(float dbValue) {
  
  if (dbValue < lower_db_limit) {
    dbValue = lower_db_limit;
  }  
  else if (dbValue > upper_db_limit) {
    dbValue = upper_db_limit;
  }
  
  int height = round((dbValue - (float) lower_db_limit)*( (float) ROWS /( (float) upper_db_limit - (float) lower_db_limit)));
  return height;
}

void get_colors(uint32_t colors[ROWS]) {
  colors[0] = strip.Color( 0, 255, 0);
  colors[1] = strip.Color( 18, 255, 0);
  colors[2] = strip.Color( 36, 255, 0);
  colors[3] = strip.Color( 54, 255, 0);
  colors[4] = strip.Color( 72, 255, 0);
  colors[5] = strip.Color( 90, 255, 0);
  colors[6] = strip.Color( 108, 255, 0);
  colors[7] = strip.Color( 126, 255, 0);
  colors[8] = strip.Color( 144, 255, 0);
  colors[9] = strip.Color( 162, 255, 0);
  colors[10] = strip.Color( 180, 255, 0);
  colors[11] = strip.Color( 198, 255, 0);
  colors[12] = strip.Color( 216, 255, 0);
  colors[13] = strip.Color( 234, 255, 0);
  colors[14] = strip.Color( 252, 255, 0);
  colors[15] = strip.Color( 255, 255, 0);
  colors[16] = strip.Color( 255, 252, 0);
  colors[17] = strip.Color( 255, 234, 0);
  colors[18] = strip.Color( 255, 216, 0);
  colors[19] = strip.Color( 255, 198, 0);
  colors[20] = strip.Color( 255, 180, 0);
  colors[21] = strip.Color( 255, 162, 0);
  colors[22] =  strip.Color( 255, 144, 0);
  colors[23] =  strip.Color( 255, 126, 0);
  colors[24] =  strip.Color( 255, 108, 0);
  colors[25] =  strip.Color( 255, 90, 0);
  colors[26] =  strip.Color( 255, 72, 0);
  colors[27] =  strip.Color( 255, 54, 0);
  colors[28] =  strip.Color( 255, 36, 0);
  colors[29] =  strip.Color( 255, 18, 0);
  colors[30] =  strip.Color( 255, 0, 0);
  colors[31] =  strip.Color( 255, 0, 0);
}

void get_board(int board[ROWS][COLS])
{
    board[31][7] = 248;
    board[31][6] = 249;
    board[31][5] = 250;
    board[31][4] = 251;
    board[31][3] = 252;
    board[31][2] = 253;
    board[31][1] = 254;
    board[31][0] = 255;

    board[30][7] = 247;
    board[30][6] = 246;
    board[30][5] = 245;
    board[30][4] = 244;
    board[30][3] = 243;
    board[30][2] = 242;
    board[30][1] = 241;
    board[30][0] = 240;

    board[29][7] = 232;
    board[29][6] = 233;
    board[29][5] = 234;
    board[29][4] = 235;
    board[29][3] = 236;
    board[29][2] = 237;
    board[29][1] = 238;
    board[29][0] = 239;

    board[28][7] = 231;
    board[28][6] = 230;
    board[28][5] = 229;
    board[28][4] = 228;
    board[28][3] = 227;
    board[28][2] = 226;
    board[28][1] = 225;
    board[28][0] = 224;

    board[27][7] = 216;
    board[27][6] = 217;
    board[27][5] = 218;
    board[27][4] = 219;
    board[27][3] = 220;
    board[27][2] = 221;
    board[27][1] = 222;
    board[27][0] = 223;

    board[26][7] = 215;
    board[26][6] = 214;
    board[26][5] = 213;
    board[26][4] = 212;
    board[26][3] = 211;
    board[26][2] = 210;
    board[26][1] = 209;
    board[26][0] = 208;

    board[25][7] = 200;
    board[25][6] = 201;
    board[25][5] = 202;
    board[25][4] = 203;
    board[25][3] = 204;
    board[25][2] = 205;
    board[25][1] = 206;
    board[25][0] = 207;

    board[24][7] = 199;
    board[24][6] = 198;
    board[24][5] = 197;
    board[24][4] = 196;
    board[24][3] = 195;
    board[24][2] = 194;
    board[24][1] = 193;
    board[24][0] = 192;

    board[23][7] = 184;
    board[23][6] = 185;
    board[23][5] = 186;
    board[23][4] = 187;
    board[23][3] = 188;
    board[23][2] = 189;
    board[23][1] = 190;
    board[23][0] = 191;

    board[22][7] = 183;
    board[22][6] = 182;
    board[22][5] = 181;
    board[22][4] = 180;
    board[22][3] = 179;
    board[22][2] = 178;
    board[22][1] = 177;
    board[22][0] = 176;

    board[21][7] = 168;
    board[21][6] = 169;
    board[21][5] = 170;
    board[21][4] = 171;
    board[21][3] = 172;
    board[21][2] = 173;
    board[21][1] = 174;
    board[21][0] = 175;

    board[20][7] = 167;
    board[20][6] = 166;
    board[20][5] = 165;
    board[20][4] = 164;
    board[20][3] = 163;
    board[20][2] = 162;
    board[20][1] = 161;
    board[20][0] = 160;

    board[19][7] = 152;
    board[19][6] = 153;
    board[19][5] = 154;
    board[19][4] = 155;
    board[19][3] = 156;
    board[19][2] = 157;
    board[19][1] = 158;
    board[19][0] = 159;

    board[18][7] = 151;
    board[18][6] = 150;
    board[18][5] = 149;
    board[18][4] = 148;
    board[18][3] = 147;
    board[18][2] = 146;
    board[18][1] = 145;
    board[18][0] = 144;

    board[17][7] = 136;
    board[17][6] = 137;
    board[17][5] = 138;
    board[17][4] = 139;
    board[17][3] = 140;
    board[17][2] = 141;
    board[17][1] = 142;
    board[17][0] = 143;

    board[16][7] = 135;
    board[16][6] = 134;
    board[16][5] = 133;
    board[16][4] = 132;
    board[16][3] = 131;
    board[16][2] = 130;
    board[16][1] = 129;
    board[16][0] = 128;

    board[15][7] = 120;
    board[15][6] = 121;
    board[15][5] = 122;
    board[15][4] = 123;
    board[15][3] = 124;
    board[15][2] = 125;
    board[15][1] = 126;
    board[15][0] = 127;

    board[14][7] = 119;
    board[14][6] = 118;
    board[14][5] = 117;
    board[14][4] = 116;
    board[14][3] = 115;
    board[14][2] = 114;
    board[14][1] = 113;
    board[14][0] = 112;

    board[13][7] = 104;
    board[13][6] = 105;
    board[13][5] = 106;
    board[13][4] = 107;
    board[13][3] = 108;
    board[13][2] = 109;
    board[13][1] = 110;
    board[13][0] = 111;

    board[12][7] = 103;
    board[12][6] = 102;
    board[12][5] = 101;
    board[12][4] = 100;
    board[12][3] = 99;
    board[12][2] = 98;
    board[12][1] = 97;
    board[12][0] = 96;

    board[11][7] = 88;
    board[11][6] = 89;
    board[11][5] = 90;
    board[11][4] = 91;
    board[11][3] = 92;
    board[11][2] = 93;
    board[11][1] = 94;
    board[11][0] = 95;

    board[10][7] = 87;
    board[10][6] = 86;
    board[10][5] = 85;
    board[10][4] = 84;
    board[10][3] = 83;
    board[10][2] = 82;
    board[10][1] = 81;
    board[10][0] = 80;

    board[9][7] = 72;
    board[9][6] = 73;
    board[9][5] = 74;
    board[9][4] = 75;
    board[9][3] = 76;
    board[9][2] = 77;
    board[9][1] = 78;
    board[9][0] = 79;

    board[8][7] = 71;
    board[8][6] = 70;
    board[8][5] = 69;
    board[8][4] = 68;
    board[8][3] = 67;
    board[8][2] = 66;
    board[8][1] = 65;
    board[8][0] = 64;

    board[7][7] = 56;
    board[7][6] = 57;
    board[7][5] = 58;
    board[7][4] = 59;
    board[7][3] = 60;
    board[7][2] = 61;
    board[7][1] = 62;
    board[7][0] = 63;

    board[6][7] = 55;
    board[6][6] = 54;
    board[6][5] = 53;
    board[6][4] = 52;
    board[6][3] = 51;
    board[6][2] = 50;
    board[6][1] = 49;
    board[6][0] = 48;

    board[5][7] = 40;
    board[5][6] = 41;
    board[5][5] = 42;
    board[5][4] = 43;
    board[5][3] = 44;
    board[5][2] = 45;
    board[5][1] = 46;
    board[5][0] = 47;

    board[4][7] = 39;
    board[4][6] = 38;
    board[4][5] = 37;
    board[4][4] = 36;
    board[4][3] = 35;
    board[4][2] = 34;
    board[4][1] = 33;
    board[4][0] = 32;

    board[3][7] = 24;
    board[3][6] = 25;
    board[3][5] = 26;
    board[3][4] = 27;
    board[3][3] = 28;
    board[3][2] = 29;
    board[3][1] = 30;
    board[3][0] = 31;

    board[2][7] = 23;
    board[2][6] = 22;
    board[2][5] = 21;
    board[2][4] = 20;
    board[2][3] = 19;
    board[2][2] = 18;
    board[2][1] = 17;
    board[2][0] = 16;

    board[1][7] = 8;
    board[1][6] = 9;
    board[1][5] = 10;
    board[1][4] = 11;
    board[1][3] = 12;
    board[1][2] = 13;
    board[1][1] = 14;
    board[1][0] = 15;

    board[0][7] = 7;
    board[0][6] = 6;
    board[0][5] = 5;
    board[0][4] = 4;
    board[0][3] = 3;
    board[0][2] = 2;
    board[0][1] = 1;
    board[0][0] = 0;
}

void read_from_eeprom() {

  Serial.println("Searching memory for matrix lower limit value.");
  if (EEPROM.read(LOWER_DB_LIMIT_ADDR) <= 255) {
    Serial.print("Value found! Using value from memory: ");
    lower_db_limit = EEPROM.read(LOWER_DB_LIMIT_ADDR);
    Serial.println(lower_db_limit);
  }
  else {
    Serial.print("No value found in memory, using default value: ");
    Serial.println(lower_db_limit);
  }

  Serial.println("Searching memory for matrix upper limit value.");
  if (EEPROM.read(UPPER_DB_LIMIT_ADDR) <= 255) {
    Serial.print("Value found! Using value from memory: ");
    upper_db_limit = EEPROM.read(UPPER_DB_LIMIT_ADDR);
    Serial.println(upper_db_limit);
  }
  else {
    Serial.print("No value found in memory, using default value: ");
    Serial.println(upper_db_limit);
  }
  
  Serial.println("Searching memory for sound level calibration value.");
  if (EEPROM.read(CALIBRATION_ADDR) <= 255 and EEPROM.read(CALIBRATION_ADDR) >= 50) {
    Serial.print("Value found! Using value from memory: ");
    calibration = (float) EEPROM.read(CALIBRATION_ADDR) / 100;
    Serial.println(calibration);
  }
  else {
    Serial.print("No value found in memory, using default value: ");
    Serial.println(calibration);
  }

  Serial.println("Memory read complete, have a nice day.");
}
