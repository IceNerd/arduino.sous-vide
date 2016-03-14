#include <DallasTemperature.h>
#include <DFR_Key.h>
#include <LiquidCrystal.h>
#include <OneWire.h>

#define DESCRIPTION_LINE_ONE "SOUS VIDE v1.0"
#define DESCRIPTION_LINE_TWO "Alberto Martinez 2013"

// Data wire is plugged into pin 3 on the Arduino
#define CONTROL_UPDATE 1000
#define ONE_WIRE_BUS 3
#define FARENHEIT 'F'
#define CELSIUS 'C'
#define MENU_SET_POINT 1
#define MENU_DIFF 2
#define MENU_UNITS 3
#define MENU_CONTROL_ON 4

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire( ONE_WIRE_BUS );

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature dtSensors( &oneWire );

// Assign the addresses of your 1-Wire temp sensors.
// See the tutorial on how to obtain these addresses:
// http://www.hacktronics.com/Tutorials/arduino-1-wire-address-finder.html
DeviceAddress ovenSensor = { 0x28, 0x44, 0xEC, 0x3D, 0x04, 0x00, 0x00, 0x07 };

// Pin assignments for SainSmart LCD Keypad Shield
LiquidCrystal lcd( 8, 9, 4, 5, 6, 7 ); 
DFR_Key keypad;
int keypress = SAMPLE_WAIT;

// program globals
boolean bUpdate = true;
char cBuffer[8];
int iMenu = 1;
String sPrinter = "";
unsigned long ulUpdate = 0.0;

// control related
boolean bControlOn = false;
boolean bHeatOn = false;
char cUnit = FARENHEIT;
float fCurTemp = 0.0;
float fDiff = 2.0;
int iSetPoint = 100;

void setup( void ) {
  pinMode( 13, HIGH );
  digitalWrite( 13, LOW );
  lcd.begin( 16, 2 );
  lcd.clear();
  lcd.setCursor( 0, 0 );
  lcd.print( DESCRIPTION_LINE_ONE );
  lcd.setCursor( 0,1 );
  lcd.print( DESCRIPTION_LINE_TWO );
  dtSensors.begin();
  dtSensors.setResolution( ovenSensor, 10 );   // set the resolution to 10 bit
  delay( 2000 );
  
  lcd.clear();
  lcd.setCursor( 0, 0 );
  lcd.print( "   Sensor:" );
  mnuSetPoint();
  
  ulUpdate = millis();
  keypad.setRate( 10 );
}

void printTemp( float fTemp, int x, int y ) {
  sPrinter = dtostrf( fTemp, 5, 1, cBuffer );
  sPrinter += cUnit;
  lcd.setCursor( x, y );
  lcd.print( sPrinter );
}

void mnuSetPoint() { 
  switch( keypress ) {
    case UP_KEY:
      iSetPoint += 1;
    break;
    
    case DOWN_KEY:
      iSetPoint -= 1;
    break;
  }
  
  if( bUpdate ) {
      lcd.setCursor( 0, 1 );
      lcd.print( "Set point:" );
      sPrinter = dtostrf( iSetPoint, 5, 0, cBuffer );
      sPrinter += cUnit;
      lcd.setCursor( 10, 1 );
      lcd.print( sPrinter );
      bUpdate = false;
  }
}

void mnuDiff() {
    switch( keypress ) {
      case UP_KEY:
        fDiff += 0.5;
        if( fDiff > 5.0 ) fDiff = 5.0;
      break;
      case DOWN_KEY:
        fDiff -= 0.5;
        if( fDiff < 0.5 ) fDiff = 0.5;
      break;
    }
    
    if( bUpdate ) {
      lcd.setCursor( 0, 1 );
      lcd.print( "     Diff:" );
      printTemp( fDiff, 10, 1 );
      bUpdate = false;
  }
}

void mnuUnits() {
    switch( keypress ) {
      case DOWN_KEY:
      case UP_KEY:
        if( cUnit == CELSIUS )
          { cUnit = FARENHEIT; iSetPoint = DallasTemperature::toFahrenheit( iSetPoint ); }
        else
          { cUnit = CELSIUS; iSetPoint = DallasTemperature::toCelsius( iSetPoint ); }
      break;
    }
    
    if( bUpdate ) {
      lcd.setCursor( 0, 1 );
      lcd.print( "    Units:" );
      lcd.setCursor( 10, 1 );
      lcd.print( "     " );
      lcd.print( cUnit );
      bUpdate = false;
  }
}

void mnuControlOn() {
      switch( keypress ) {
      case DOWN_KEY:
      case UP_KEY:
        bControlOn = !bControlOn;
      break;
    }
    
    if( bUpdate ) {
      lcd.setCursor( 0, 1 );
      lcd.print( "   On/Off:" );
      lcd.setCursor( 10, 1 );
      if( bControlOn ) {
        lcd.print( "    ON" );
      } else {
        lcd.print( "   OFF" );
      }
      bUpdate = false;
  }
}

void loop( void ) {
  
  // Temp Control - ALWAYS RUNNING
  if( millis() - ulUpdate >= CONTROL_UPDATE ) {
    dtSensors.requestTemperatures();
    if( cUnit == CELSIUS ) {
      fCurTemp = dtSensors.getTempC( ovenSensor );
    } else {
      fCurTemp = dtSensors.getTempF( ovenSensor );
    }
    printTemp( fCurTemp, 10, 0 );
    
    if( bControlOn ) {
      if( fCurTemp <= iSetPoint - fDiff ) bHeatOn = true;
      if( fCurTemp >= iSetPoint ) bHeatOn = false;
    } else {
      bHeatOn = false;
    }
    
    lcd.setCursor( 0, 0 );
    if( bHeatOn ) {
      digitalWrite( 13, HIGH );
      lcd.print( '*' );
    } else {
      lcd.print( ' ' );
      digitalWrite( 13, LOW );
    }
    
    ulUpdate = millis();
  }
  ///END Temp Control
  
  keypress = keypad.getKey();
  if( keypress != SAMPLE_WAIT && keypress != NO_KEY ) {
    bUpdate = true;
    switch( keypress ) {
      case LEFT_KEY: --iMenu; if( iMenu < MENU_SET_POINT ) iMenu = MENU_CONTROL_ON; break;
      case RIGHT_KEY: ++iMenu; if( iMenu > MENU_CONTROL_ON ) iMenu = MENU_SET_POINT; break;
    }
    switch( iMenu ) {
      case MENU_SET_POINT: // update set point
        mnuSetPoint();
      break;
      case MENU_DIFF: // update diff
        mnuDiff();
      break;
      case MENU_UNITS: // update units
        mnuUnits();
      break;
      case MENU_CONTROL_ON: // update heat control on/off
        mnuControlOn();
      break;
    }
    delay( 10 );
    while( keypad.getKey() != NO_KEY ) { }
    delay( 10 );
  }
  
} // END MAIN
