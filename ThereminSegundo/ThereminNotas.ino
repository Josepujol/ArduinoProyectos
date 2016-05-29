/*
Segundo Alvarez 2015.
Arduino Theremin with VS1053M.
Use two ultrasonic sensor. One for volume and another one to note.
This software need to be improved.
Calibrate noise and accuracy of sensors.
Rewrite and clean test lines.
 */

// STL headers
// C headers
#include <avr/pgmspace.h>
// Framework headers
// Library headers
#include <VS1053M.h>
#include <SPI.h>
// Project headers
#include "printf.h"
#include <NewPing.h>
#undef PSTR
#define PSTR(s) (__extension__({static const char __c[] __attribute__ (( section (".progmem") )) = (s); &__c[0];}))


//DEFINES
#define S1_TRIGGER_PIN  A0  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define S1_ECHO_PIN     A1  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define S2_TRIGGER_PIN  A2  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define S2_ECHO_PIN     A3  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar1(S1_TRIGGER_PIN, S1_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonar2(S2_TRIGGER_PIN, S2_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
/**
* MP3 player instance
*/
VS1053M player(/* cs_pin */ 6,/* dcs_pin */ 7,/* dreq_pin */ 2,/* reset_pin */ 8);

/**
 * Real-time MIDI player instance
 */
VS1053M::RtMidi midi(player);

/**
 * selección de instrumentos
 */
 uint8_t instrumentos[7] ={41,1,74,105,117,55,100};
 //violin, piano, flauta,sitar, taiko, voz sintetica, atmosfera
 uint8_t inst_selected = 0;
 

  uint16_t uS_nota, uS_nota_old;
  uint16_t nota_amp, nota_amp_old;
  uint16_t nota_app, nota_app_old;
  uint16_t uS_vol_old;
  uint16_t uS_vol, uS_vol_app, uS_vol_app_old;
  boolean tocando = 0, stop_sound = true, vol_ok=false;
  uint8_t cuenta=0, cuenta2=0, cuenta3 = 0, cuenta4=0;
  long T0 = 0 ;  // Variable global para tiempo
void setup(void)
{
  // Preamble
  Serial.begin(57600);
  printf_begin();
  Serial.println("Starting...\n");
  SPI.begin();
  player.begin();
  midi.begin();
  player.setVolume(0x0);
  midi.selectInstrument(0,41); //BANK AND INSTRUMENT SELECTED
  //midi.selectDrums(0);
  Serial.println("+RADY");
  pinMode(3,INPUT);
  attachInterrupt(1, changeIns, RISING);

}



void loop(void)
{
  //CATCH DISTANCE
  uS_nota = sonar1.ping(); // Send ping, get ping time in microseconds (uS).
  uS_vol = sonar2.ping(); // Send ping, get ping time in microseconds (uS).
  uS_nota = (0.6*uS_nota/ US_ROUNDTRIP_CM + 0.4*uS_nota_old);
  uS_vol = (0.8*uS_vol/ US_ROUNDTRIP_CM + 0.2*uS_vol_old);  
  
  const uint8_t current_note = 60;
  
  //filtro de volumen y parada si quito la mano
  if(abs(uS_vol_old-uS_vol)>10) {uS_vol_app = 0.1* uS_vol + 0.9*uS_vol_old;}
  else {uS_vol_app = 0.5* uS_vol + 0.5*uS_vol_old;}
  if(uS_vol_app>=127 && stop_sound==false) {uS_vol_app= 127; cuenta++;} //protección
  else {cuenta=0;}
  if(uS_vol_app<127) {cuenta2++;}
  if(cuenta>10) {stop_sound = true; vol_ok = false; cuenta = 0;}
  if(cuenta2>20) {stop_sound = false; vol_ok=true; cuenta2 = 0;}
  if(stop_sound) 
   {
     midi.allNotesOff(0); //apaga todas las notas
     player.setVolume(127);
     //midi.sustainOff(0); //quita sostenimiento
     tocando = false;
     vol_ok=false;
     stop_sound = false;
   }
   if(vol_ok)
    {
      //uS_vol_app = (127*uS_vol_app)/20; //amplificacion
      if(abs(uS_vol_app - uS_vol_app_old)<2) {cuenta4++;}
      else{cuenta4=0;}
      if(cuenta4>5) player.setVolume(uS_vol_app); //establece volumen
      uS_vol_app_old = uS_vol_app;
    }
  //player.setVolume(uS_vol);
  //fin del filtro de volumen


  //filtro del nota
  if(uS_nota>120) {nota_app = nota_app_old;}      
  if(abs(uS_nota_old-uS_nota)<2) {cuenta3++; nota_app = nota_app_old;} //solo cambia de nota cuando es estable
  else {cuenta3=0;}
  //else {uS_nota_app = 0.5* uS_nota + 0.5*uS_nota_old;}
  if(cuenta3 > 3) 
    {
      nota_app = uS_nota_old; 
      cuenta3 = 0;
      nota_amp = nota_app/5 +60;//amplificación del nota
    } 
  //fin del filtro del nota
  
 
  
  if((nota_amp>27 && uS_nota<87) && (nota_amp_old !=nota_amp))
  {
    
      midi.noteOff(0, nota_amp_old); 
      midi.noteOn(0,nota_amp,127);//toca nota
      //Serial.println("tocando");
      //midi.sustainOn(0); //mantiene nota canal 0
      
      //stop_sound = false;
      tocando = true;
      nota_amp_old = nota_amp;
      
    
    
   }
  
   uS_nota_old =   uS_nota;
   uS_vol_old = uS_vol;
   nota_app = nota_app_old;

   Serial.print(cuenta3);
   Serial.print("\t");

   Serial.print(uS_nota);
   Serial.print("\t");
   Serial.print(nota_app);
   Serial.print("\t");
   Serial.print(nota_amp);
   Serial.print("\t");
   Serial.print(uS_vol);
   Serial.print("\t");

   Serial.println(uS_vol_app);
   
}


void changeIns()
{
  detachInterrupt(1);
  if ( millis() > T0  + 250)
    {   
      
      if(inst_selected<128) inst_selected++;
      else inst_selected=0;
      midi.selectInstrument(0,inst_selected); //BANK AND INSTRUMENT SELECTED
      T0 = millis();
    }

  
  attachInterrupt(1, changeIns, RISING);
}


// vim:cin:ai:sts=2 sw=2 ft=cpp
