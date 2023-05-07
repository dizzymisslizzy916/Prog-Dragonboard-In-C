//Liz Crittenden
//Final Project

#include <hidef.h>      /* common defines and macros */
#include <mc9s12dg256.h>     /* derivative information */
#pragma LINK_INFO DERIVATIVE "mc9s12dg256b"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "main_asm.h" /* interface to the assembly module */

#define dSharp 2411  //pitches of notes used
#define fSharp 2028
#define aSharp 1609
#define gSharp 1806
#define cSharp 2706
#define f 2148

int silence1[7] = {dSharp, dSharp, fSharp, fSharp, aSharp, fSharp, gSharp}; 
int silence2[8] = {cSharp, cSharp, cSharp, f, f, gSharp, f, fSharp}; 


void dealIndicate();                //functions used for repetitive code
int deal();
void endGame(int playHand, int dealHand);
void win();
void lose();
void printStuff(char* sentence);

void interrupt 25 hitMe();      //interrupts used
void interrupt 13 sound();
void interrupt 10 handler();

int playHand, dealHand, light, note, card, dealXcard;     //variables
char choice;
int running = 1;
int pressed = 0;
int count = 2;  
int won = 1;
char* line1;
char* line2;


long pulse;
long startcycle;
long distance;

void main(void) {

  PLL_init();        // set system clock frequency to 24 MHz 
  led_enable();       //enabling all used components and interrupts
  seg7_enable();
  lcd_init();
  SW_enable();
  keypad_enable();
  ad0_enable();
  _asm CLI;
  PIEH = 0xFF;
  servo54_init();
  motor0_init();
  SCI0_init(9600);
  
  TIOS = 0x10;       //using channels 4 and 2 for OC/IC
  TIE  = 0x04;       //enable channel 2 interrupts
  TCTL1 = 0x02;      
  TCTL4 = 0x30;    
  TSCR2 = 0x02;     //set timer clock at 6MHz
  
  DDRM = 0xFF;       //enable external led                                                                     
                         
  line1 = "Player: + $50. \nHouse: - $50."; 
  line2 = "Player: - $50. \nHouse: + $50."; 
  
  type_lcd("Let's play"); //This block uses the LCD and delays to show that the game has started
  set_lcd_addr(0x40);
  type_lcd("Blackjack!");
  ms_delay(2000);
  clear_lcd();
  type_lcd("The bet is $50.");
  ms_delay(2000);
  clear_lcd();
  dealIndicate();
  playHand = deal();
  dealHand = deal(); 
  type_lcd("Your hand: ");
  set_lcd_addr(0x40);
  write_int_lcd(playHand);
  ms_delay(2000);
  clear_lcd();
  type_lcd("Dealer's hand: ");
  set_lcd_addr(0x40);
  write_int_lcd(dealHand);
  ms_delay(2000);
  clear_lcd();
         
  while(running == 1){          //this loop runs until the game is over
      type_lcd("Hit or stay?");
      set_lcd_addr(0x00);
      light = ad0conv(4) * 100; //reading light sensor on ad0 channel 4
      
      TSCR1 = 0x80;      //start counter
      PTT = PTT | 0x10;  //make trigger line go high
      TC3 = TCNT + 60;    //interrupt at rise and fall of channel 4
      ms_delay(250);    //repeat attempt every 0.25 second
      distance = ((pulse*17)/600);  //calculate distance

      if (distance < 80 && distance > 30) { //if player is within ~3 inches of distance sensor     
         clear_lcd();
         type_lcd("Too close");      //a warning will be displayed
         set_lcd_addr(0x40);
         type_lcd("To dealer!");
         ms_delay(1000);
         clear_lcd();
         type_lcd("Back up!");
         ms_delay(1000);
         clear_lcd();           
      }
  
      if(light < 1000 || playHand >= 21){  //if player blocks light coming into sensor
         endGame(playHand, dealHand);     //this signals choosing to end the game
      }
      if(pressed == 1){        //if SW5 is pressed this signals choosing another card
        type_lcd("New card:");    //an interrupt is used to update the player's hand
        set_lcd_addr(0x40);
        write_int_lcd(card);
        ms_delay(2000);
        clear_lcd;
        if(playHand < 21){      
          set_lcd_addr(0x00);
          type_lcd("New hand:");
          set_lcd_addr(0x40);
          write_int_lcd(playHand);
          ms_delay(2000);
        }
      pressed = 0;
      set_lcd_addr(0x00);
      clear_lcd(); 
      }
    }     //end of game running loop
    clear_lcd();
    while(choice != 1 && choice != 2){ //user may choose to see how much money was won/lost
      type_lcd("See scores?");         //loop runs until yes or no is chosed
      set_lcd_addr(0x40);
      type_lcd("1 = Y    2 = N");
      choice = getkey();      //wait for button on keypad to be pressed
      wait_keyup();
      clear_lcd();
      if(choice== 1){
        if(won== 1){
          printStuff(line1); //writing scores to miniIDE
        }else{
          printStuff(line2);
        }
        clear_lcd();
        type_lcd("Good game.");
      } else if (choice== 2){
        clear_lcd;
        type_lcd("Goodbye.");
      } else{
        type_lcd("Invalid");  
        ms_delay(2000);
        clear_lcd();
      }
    } 
}
                   
void dealIndicate(){      //function uses the leds, 7seg displays and delays to indicate
  int i;                  //the "cards" are being dealt for smooth gameplay
  clear_lcd();
  type_lcd("Dealing...");
  for(i = 0; i < 3; i++){
    PORTB = PORTB | 0x03;
    ms_delay(200);
    PORTB = PORTB | 0x07;
    ms_delay(200);
    PORTB = PORTB | 0x0F;
    ms_delay(200);
    PORTB = PORTB | 0x1F;
    ms_delay(200);
    PORTB = PORTB | 0x3F;
    ms_delay(200);
    PORTB = PORTB | 0x7F;
    ms_delay(200); 
    PORTB = PORTB | 0xFF; 
    ms_delay(200);
    leds_off();
  }
  clear_lcd();
}

int deal(){             //function deals a random card
  srand(count);
  count++;
  card = rand()%11 + 1;
  return card;
}
  
void interrupt 25 hitMe(){       //interrupt when player pushes SW5 to choose another card
     dealIndicate();
     playHand = playHand + deal();
     pressed = 1;
     PIFH = 0xFF;
}

void interrupt 13 sound(){     //interrupt to play sad song upon losing
  tone(note);   
}


void interrupt 10 handler() { //channel 2 interrupt used for distance sensor
  if(PTT | 0xFD == 0xFF) {
    startcycle = TCNT;        //rising edge interrupt
  }else {
    pulse = TC2 - startcycle;  //falling edge interrupt
    TSCR1 = 0x00;             //stop counter
  }
  TFLG1 = TFLG1 | 0x04;     //reset interrupt flag on channel 2
}

void endGame(int playHand, int dealHand){  //function to end the game and calculate who won
     dealXcard = deal();
     dealHand = dealHand + dealXcard;
     clear_lcd();
     pressed = 0;
     if(playHand == 21){ 
        set_lcd_addr(0x00);
        write_int_lcd(21);
        set_lcd_addr(0x40);
        type_lcd("Blackjack!");
        ms_delay(2000);        
        win();
     }else if (playHand > 21){   
        set_lcd_addr(0x00);
        type_lcd("Your hand:");
        set_lcd_addr(0x40);
        write_int_lcd(playHand);
        ms_delay(2000);
        lose();
     }else if (playHand > dealHand && playHand < 21){
        set_lcd_addr(0x00);
        type_lcd("Your hand:");
        set_lcd_addr(0x40);
        write_int_lcd(playHand);
        ms_delay(2000);
        clear_lcd();
        type_lcd("Dealer's card 2:");
        set_lcd_addr(0x40);
        write_int_lcd(dealXcard);
        ms_delay(2000);
        clear_lcd();
        type_lcd("Dealer's hand:");
        set_lcd_addr(0x40);
        write_int_lcd(dealHand);
        ms_delay(2000);
        win();
     }else if (dealHand >= playHand && dealHand < 21){
        set_lcd_addr(0x00);
        type_lcd("Your hand:");
        set_lcd_addr(0x40);
        write_int_lcd(playHand);
        ms_delay(2000);
        clear_lcd();
        type_lcd("Dealer's card 2:");
        set_lcd_addr(0x40);
        write_int_lcd(dealXcard);
        ms_delay(2000);
        clear_lcd();
        type_lcd("Dealer's hand:");
        set_lcd_addr(0x40);
        write_int_lcd(dealHand);
        ms_delay(2000);
        lose();
     } 
}

void win(){       //function to show the player won
    int z;
    clear_lcd();
    leds_off();
    type_lcd("You won!");
    DDRB = 0xFF;           //motor runs at full speed
    DDRP = 0xFF;
    PORTB = 0x01;
    PTP = 0x01;
    for(z = 0; z < 3; z++){ //flashes external led on and off
      PTM = 0x04;        
      ms_delay(500);
      PTM = 0x00;        
      ms_delay(500);
    }
    PORTB = PORTB & 0xFE;//turn motor off
    running = 0;
}

void lose(){       //function to show the player lost
   int i;
   clear_lcd();
   type_lcd("You lost!");
   set_servo54(2300);          //servo turns within safe range for smaller servo
   for(i = 0; i < 7; i++){     //two for loops, speaker and delays used to play a sad song
    note= silence1[i];             
    sound_init();                
    sound_on();
    ms_delay(400);
   }
   ms_delay(1000);
   set_servo54(6200);        //within safe range for smaller servo
   for(i = 0; i < 8; i++){ 
    note= silence2[i];             
    sound_init();                
    sound_on();
    ms_delay(400);
   }
   ms_delay(500);
   sound_off();
   running = 0;
   won = 0;
} 

void printStuff(char* sentence){    //function to write to miniIDE
  
  int m=0;
  while (sentence[m] != 0 ) { 
      outchar0(sentence[m]);
      m = m + 1;  
  }
}