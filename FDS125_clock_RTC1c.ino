 // Woordklok (WordClock)  
 // 25/02/2014 - version 0.2c  
 // For use with a modified FDS-132 led matrixboard board, see http://arduinows.blogspot.com  
 //  
 // The matrix board is controlled by timer0 overflow interrupt. This gives a steady image and the display can be handled like a static type.  
 // The main code does not have to control the board, just set or clear bits in the data array to put some led's on or off.  
 // Logically it's organized in colums, 270 in total. This sofware defines column 0 as the upper left column and 271 is the right bottom column.  
 // This makes it more easy to arrange the text on the board. There is no checking if the text fits on the display.  
 //
 // alse you can see about FDS-132 at  // http://arduinoforum.nl/ 
 // Clock on FDS-125 display by Nicu FLORICA (niq_ro), see http://arduinotehniq.blogspot.com 
 // ver.1.0 (base) - 23.11.2016, with clock and date
 // ver.1.0c - add DHT sensor for humidity and temperature
 // Column layout : 
 //          Line 0 : 0 to 59  
 //          Line 1 : 59 to 119     
 // 
 //  
 #include <avr/pgmspace.h>           // Include pgmspace so data tables can be stored in program memory  
 #include <SPI.h>                    // Include SPI communications, used for high speed clocking to display shiftregisters  
 #include "TimerOne.h"               // Include Timer1 library, used for timer overflow interrupt                    
 #include "RTClib.h"                 // Include library for RTC (Real Time Clock module)  
 #include <Wire.h>                   // Library needed for RTC  
 RTC_DS1307 RTC;                     // RTC module Adafruit DS1307  
 const int strobePin = 10;           // Define I/O pins connected to the FDS-132 display board           
 const int clockPin = 13;                  
 const int dataPin = 11;                  
 const int resredPin = 9;                  
 const int row_a = 5;                     
 const int row_b = 6;                    
 const int row_c = 7;                     
 int rowCount = 0;                   // Counter for the current row, do not change this value in the main code, it's handeld in the interrupt routine   
 #include "font75.h";                // Standard 5x7 font ASCII Table  
 byte columnBytes[8][34];            // This is the data array that is used as buffer. The display data is in this buffer and the timeroverflow routine scans this  
                                     // buffer and shows it on the display. So read and write to this buffer to change display data.  
                                     // No strings are stored here, its the raw bit data that has to be shifted into the display.  
 int an, an01, an10;
 int luna, luna01, luna10;
 int ziua, ziua01, ziua10;
 int ore, ore01, ore10;
 int minuti, minuti01, minuti10;
 int secunde, secunde01, secunde10;
 int zz;
 char daysOfTheWeek[7][12] = {"  Sunday ", "  Monday ", " Tuesday ", " Wednesday", " Thursday", "  Friday ", " Saturday"};
 char ziuadinsaptamana[7][12] = {" Duminica", "   Luni  ", "  Marti  ", " Miercuri", "   Joi   ", "  Vineri ", " Sambata "};
 int limba = 0;
 
#include "DHT.h"
#define DHTPIN 8     // what pin we're connected to
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
// NOTE: For working with a faster chip, like an Arduino Due or Teensy, you
// might need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// Example to initialize DHT sensor for Arduino Due:
//DHT dht(DHTPIN, DHTTYPE, 30);
float temperatura;
int umiditate;
int tzeci, tunit, tzecimi, trest;
int tsemn, ttot;
int hzeci, hunit;

 
 // =========================================================================================  
 // Setup routine, mandatory.  
 // =========================================================================================  
 void setup() {    
   dht.begin(); 
  pinMode (strobePin, OUTPUT);       // Define IO       
  pinMode (clockPin, OUTPUT);   
  pinMode (dataPin, OUTPUT);   
  pinMode (row_c, OUTPUT);   
  pinMode (row_b, OUTPUT);   
  pinMode (row_a, OUTPUT);   
  pinMode (resredPin, OUTPUT);   
  digitalWrite (resredPin, LOW);     // disable display output  
  digitalWrite (strobePin, LOW);     //   
  Serial.begin(9600);                // Include Serial routine, used for debugging and testing  
  SPI.begin();                       // Start SPI comunications  
  SPI.setBitOrder(LSBFIRST);         // Initialize SPI comunications 

                                     // Make use of timer1, thanks to Rolo, see http://arduinoforum.nl/viewtopic.php?f=6&t=336&start=50#p3026 
  Timer1.initialize(1000);           // initialize timer1, overflow timer1 handles the display driver, period (in microseconds)  
  Timer1.attachInterrupt(displayControl);   // attaches timer1 overflow interrupt routine  
  Wire.begin();                      // initialize I2C
  RTC.begin();                       // initialize the clock  
  //RTC.adjust(DateTime(__DATE__, __TIME__)); // load time from computer to RTC (only once if RTC time not set).    
                                     // Let's start   
  clearDisplay();                    // Clear the display and show the credits  
  placeText("FDS-125",10);          // text on first line   
  placeText("display",70);       // second line  
  delay(3000); 
  clearDisplay();                    // Clear the display and show the credits  
  placeText("tested",12);          // text on first line   
  placeText("by niq_ro",63);       // second line  
  delay(3000);                       // two seconds is more than enough  
  clearDisplay();   
 }    
 // =========================================================================================  
 // Main loop.  
 // Take notice when using time crititical code the interrupt is running to control   
 // the display.  
 // =========================================================================================  
 void loop()  
 {
 for (int i = 0; i < 500; i++)
{ 
  getTime();                         // fetch the time from the RTC  
  showDigitalClock();                // shows the time digitally  
  showData();                        // show data
}
  clearDisplay(); 

 for (int i = 0; i < 500; i++)
{ 
  getTime();                         // fetch the time from the RTC  
  showDigitalClock();                // shows the time digitally  
  showZi();                          // show name of day
}

clearDisplay();
 getTeHas(); 
 for (int i = 0; i < 500; i++)
{ 
  getTime();                         // fetch the time from the RTC  
  showDigitalClock();                // shows the time digitally  
  showTe();                          // show name of day
}
  clearDisplay(); 
 for (int i = 0; i < 500; i++)
{ 
  getTime();                         // fetch the time from the RTC  
  showDigitalClock();                // shows the time digitally  
  showHas();                          // show name of day
}
  clearDisplay(); 

  limba = limba +1;

}   
 // =========================================================================================  
 // Timer1 overflow interrupt routine,   
 // Does the Display shifting and muliplexing    
 // =========================================================================================  
 void displayControl()   
 {   
  digitalWrite(strobePin, LOW);      // StrobePin LOW, so led's do not change during clocking  
  digitalWrite (resredPin, LOW);     // Display off to prevent ghosting   
  for (int q = 0; q<34; q++) {       // Shift the bytes in for the current Row  
   SPI.transfer(columnBytes[rowCount][q]);  
  }  
  digitalWrite (row_a, rowCount & 1); // Switch the current column on, will be on until next interrupt  
  digitalWrite (row_b, rowCount & 2);          
  digitalWrite (row_c, rowCount & 4);        
  digitalWrite (strobePin, HIGH);    // Strobe the shiftregisters so data is presented to the LED's   
  digitalWrite (resredPin, HIGH);    // Display back on   
                                     // uncomment next two lines if you want to reduce brightness a bit  
  // digitalWrite (resredPin, LOW);  // reduce brightness straightforwardly by switching the display off  
  // delayMicroseconds(500);         // only 0.5 milliseconds  
  digitalWrite (resredPin, HIGH);  
  rowCount++;                        // Increment Row   
  if (rowCount == 7) rowCount = 0 ;  // Row is 0 to 6  
 }   
 // =========================================================================================  
 // Place text on display, uses the standard 5x7 font  
 // Call this routine with the text to display and the starting column to place the text  
 // =========================================================================================  
 void placeText(String text, int colPos) {  
  byte displayByte;   
  char curChar;  
  int bitPos;  
  int bytePos;  
  int pixelPos;  
  int charCount = text.length();  
  int store = colPos;  
  for (int i = 0; i < charCount; i++) {  // Loop for all the characters in the string  
   curChar = text.charAt(i);         // Read ascii value of current character  
   curChar = curChar - 32;           // Minus 32 to get the right pointer to the ascii table, the first 32 ascii code are not in the table    
   for (int y = 0; y<7; y++) {                                       // y loop is used to handle the 7 rows  
    for (int x = 0; x<5; x++) {                                      // x loop is the pointer to the indiviual bits in the byte from the table   
     displayByte = (pgm_read_word_near(Font75 + (curChar*7)+y));     // Read byte from table   
     pixelPos = abs(colPos - 271);                                   // Calculate start position to place the data on display  
     bytePos = ((pixelPos)/8);                        
     bitPos = (pixelPos) - (bytePos*8);  
     boolean bitStatus = bitRead(displayByte, x);                    // Probe the bits    
     if(bitStatus == 0) bitClear(columnBytes[y][bytePos], bitPos);   // And set or clear the corrosponding bits in de display buffer    
     if(bitStatus == 1) bitSet(columnBytes[y][bytePos], bitPos);      
     colPos++;                                
    }  
    colPos = store;                  // Reset the column pointer so the next byte is shown on the same position   
   }  
   colPos = colPos + 6;              // 6 is used here to give one column spacing between characters (one character is 5 bits wide)  
   store = colPos;                   // For more space between the characters increase this number   
  }  
 }     
 // =========================================================================================  
 // Clear display  
 // All bytes in buffer are set to zero  
 // =========================================================================================  
 void clearDisplay() {  
  for (int y = 0; y<8; y++) {     
   for (int x = 0; x<34; x++) {     
    columnBytes[y][x] = 0;  
   }  
  }     
 }   
 // =========================================================================================  
 // Place a character on display, uses the standard 5x7 font  
 // Call this routine with an integer value to display and the starting column to place the text  
 // =========================================================================================  
 void placeChar(int character, int colPos) {  
  byte displayByte;   
  char curChar;  
  int bitPos;  
  int bytePos;  
  int pixelPos;  
  int store = colPos;  
  curChar = character + 16;          // Read ascii value of character and add 16 in order to match character in font75.h     
  for (int y = 0; y<7; y++) {        // y loop is used to handle the 7 rows  
   for (int x = 0; x<5; x++) {       // x loop is the pointer to the indiviual bits in the byte from the table   
                                     // Read byte from table  
    displayByte = (pgm_read_word_near(Font75 + (curChar*7)+y));   
    pixelPos = abs(colPos - 271);    // Calculate start position to place the data on display  
    bytePos = ((pixelPos)/8);                        
    bitPos = (pixelPos) - (bytePos*8);  
    boolean bitStatus = bitRead(displayByte, x);                     // Probe the bits    
    if(bitStatus == 0) bitClear(columnBytes[y][bytePos], bitPos);    // And set or clear the corresponding bits in de display buffer    
    if(bitStatus == 1) bitSet(columnBytes[y][bytePos], bitPos);      
    colPos++;                                
   }  
   colPos = store;                   // Reset the column pointer so the next byte is shown on the same position   
  }  
  colPos = colPos + 6;               // 6 is used here to give one column spacing between characters (one character is 5 bits wide)  
  store = colPos;                    // For more space between the characters increase this number   
 }  
 // =========================================================================================  
 // get the time from the RealTimeClock  
 // =========================================================================================  
 void getTime()        
 {  
  DateTime now = RTC.now();          // read RTC clock.  
  secunde = now.second();            // get seconds,  
  secunde01 = secunde%10;           // split them into units,  
  secunde10 = secunde/10;           // and tens.  
  minuti = now.minute();            // get minutes  
  minuti01 = minuti%10;  
  minuti10 = minuti/10;  
  ore = now.hour();                // get hours  
  ore01 = ore%10;  
  ore10 = ore/10;  
  ziua = now.day();                  // get day
  ziua01 = ziua%10;
  ziua10 = ziua/10;
  luna = now.month();                // get month
  luna01 = luna%10;
  luna10 = luna/10;
  an = now.year();                   // get year
  an = an -2000;
  an01 = an%10;
  an10 = an/10;
  zz = now.dayOfTheWeek();
 }  
 // =========================================================================================  
 // show digital clock  
 // =========================================================================================  
 void showDigitalClock()  
 {
  placeChar(ore10, 67);            // put it at fixed location (top right)  
  placeChar(ore01, 73);  
  placeText(":",79);                // colons for digital display  
  placeChar(minuti10, 84);  
  placeChar(minuti01, 90);  
  placeText(":",96);  
  placeChar(secunde10, 101);  
  placeChar(secunde01, 107); 
 }  
 // =========================================================================================  
 // show data time  
 // =========================================================================================  
 void showData()  
 {
  placeChar(ziua10, 0);            // put it at fixed location (top right)  
  placeChar(ziua01, 6);
  placeText(".",12);                // colons for digital display  
  placeChar(luna10, 18);            // put it at fixed location (top right)  
  placeChar(luna01, 24);
  placeText(".",30);                // colons for digital display  
  placeText("2",36);
  placeText("0",42);
  placeChar(an10, 48);            // put it at fixed location (top right)  
  placeChar(an01, 54); 
 }  
// =========================================================================================  
// show day name  
// =========================================================================================  
void showZi()  
 {
if (limba%2)   
placeText(daysOfTheWeek[zz],0);        
else
placeText(ziuadinsaptamana[zz],0);        
 }

// =========================================================================================  
// get temperature and humidity  
// =========================================================================================  
void getTeHas()  
 {
temperatura = 10*dht.readTemperature();
umiditate = dht.readHumidity();
//temperatura = random(-150, 350);
//temperatura = 354;
//temperatura = temperatura/10;
//umiditate = random (10, 90);

if (temperatura >= 0) 
{
  tsemn = 1;
  ttot = temperatura;
}
if (temperatura < 0) 
{
  tsemn = -1;
  ttot = -temperatura;
}

tzeci = ttot/100;
trest = ttot%100;
tunit = trest/10;
tzecimi = trest%10;

hzeci = umiditate/10;
hunit = umiditate%10;
 }
// =========================================================================================  
// show temperature 
// =========================================================================================  
void showTe()  
 {
if (tzeci == 0)
{
placeText(" ",8);          
if (tsemn == 1) placeText("+",14);
else placeText("-",14);
}
else
{
if (tsemn == 1) placeText("+",8);
else placeText("-",8);
placeChar(tzeci, 14);
}
placeChar(tunit, 20);
placeText(",",26);
placeChar(tzecimi, 32);
placeText("'C",38);
}
// =========================================================================================  
// show humidity 
// =========================================================================================  
void showHas()  
 {
if (hzeci == 0)
{
placeText(" ",15);          
}
else
{
placeChar(hzeci, 15);
}
placeChar(hunit, 21);
placeText("%RH",27);
}
