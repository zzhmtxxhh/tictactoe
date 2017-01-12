/*CMPUT274 project tic tac toe This program contain two part: man vs comp and
man vs man. In addition, we also create a button to regret one step. The button
connect to pin 3 is reset button ( going to main menu) . Another button connect to
13 is regret button.The potential meter is to adjust the difficulty, green light
(light connected to pin 12 ) is smart.The red light (connected to pin 13) is dum.
"x"is computer "o" is human
OK ^o^ ENJOY THE GAME */


#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>
#include <stdlib.h>
//define constant//
#define SD_CS 5
#define TFT_CS 6
#define TFT_DC 7
#define TFT_RST 8
#define JOY_SEL 9
#define JOY_VERT_ANALOG 0
#define JOY_HORIZ_ANALOG 1
#define TFT_WIDTH 128
#define TFT_HEIGHT 160
#define JOY_DEADZONE 64 // Only care about joystick movement if

#define JOY_CENTRE 512
#define JOY_SPEED 5
#define MILLIS_PER_FRAME 50 // 20fps
#define STEPS_PER_PIXEL 32

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// initialize all the varible //
int g_joyX = TFT_WIDTH/2; // X-position of cursor in pixels
int g_joyY = TFT_HEIGHT/2; // Y-position of cursor in pixels
int update = 1;
int mode = 0;
int step = 10 ;
bool max = 0;
int depth;
int a = 0 ;
int first_step_i;//record the first_step i in the fake board
int first_step_j;//record the first step j in the fake board
int game_mode;//0 is p vs p; 1 is com vs p

// initialize struct//
struct chess_board{
  int index;
  int shape;
};
chess_board chess_pieces_info[3][3];
chess_board last_step_of_chess[3][3];
bool user_interface = 1;
bool mode_in_ui = 1;
// print the board on the serial monitor
void testboard(chess_board n[3][3]){
  for(int i = 0 ; i<3 ; i++){ // using a loop print the chess state on serial-mon
      Serial.print(n[i][0].shape);
      Serial.print(" ");
      Serial.print(n[i][1].shape);
      Serial.print(" ");
      Serial.println(n[i][2].shape);
  }
      Serial.println(" ");
}
/** initilize the tft and the SD card*/
void setup() {
  init();

  Serial.begin(9600);
  pinMode(JOY_SEL, INPUT);
  digitalWrite(JOY_SEL, HIGH);
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735R chip, black tab
  tft.fillScreen(ST7735_BLACK);

  pinMode(3, INPUT);//surrending
  pinMode(11, INPUT);//regreting one step
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  digitalWrite(3, HIGH);
  digitalWrite(11, HIGH);

}
// draw chess on  each updated  loacation
void draw_chess_pieces(){
  int center_Y;
  int center_X;
  for(int i=0; i<3; ++i){
    for(int j=0; j<3; ++j){
      center_X = (2*j+1)*21 + j;
      center_Y = (2*i+1)*21 + i;
      if(chess_pieces_info[i][j].index == 1){
        if(chess_pieces_info[i][j].shape == 0){
          tft.drawCircle(center_X, center_Y, 12, ST7735_WHITE);
          tft.drawCircle(center_X, center_Y, 11, ST7735_WHITE);
        }
        else if(chess_pieces_info[i][j].shape == 1){
          tft.drawLine(center_X-11, center_Y-11, center_X+11, center_Y+11, ST7735_WHITE);
          tft.drawLine(center_X+11, center_Y-11, center_X-11, center_Y+11, ST7735_WHITE);
          center_X += 1;
          tft.drawLine(center_X-10, center_Y-10, center_X+10, center_Y+10, ST7735_WHITE);
          tft.drawLine(center_X+10, center_Y-10, center_X-10, center_Y+10, ST7735_WHITE);
        }
      }
    }
  }
}
// update screen on each action
void updateScreen(){
  if(update == 1){
    tft.fillScreen(ST7735_BLACK);
    //draw the board
    tft.drawLine(0, 128, 128, 128, ST7735_WHITE);
    tft.drawLine(0, 43, 128, 43, ST7735_WHITE);
    tft.drawLine(0, 86, 128, 86, ST7735_WHITE);
    tft.drawLine(43, 0, 43, 128, ST7735_WHITE);
    tft.drawLine(86, 0, 86, 128, ST7735_WHITE);
    //draw the cursor
    tft.drawLine(g_joyX-2, g_joyY-2, g_joyX+2, g_joyY-2, ST7735_WHITE);
    tft.drawLine(g_joyX-2, g_joyY-2, g_joyX-2, g_joyY+2, ST7735_WHITE);
    tft.drawLine(g_joyX-2, g_joyY+2, g_joyX+2, g_joyY+2, ST7735_WHITE);
    tft.drawLine(g_joyX+2, g_joyY-2, g_joyX+2, g_joyY+2, ST7735_WHITE);
    //draw chess piece
    draw_chess_pieces();
    tft.setTextWrap(false);
    tft.setTextColor(0xFFFF, 0x0000);
    if(step == 0){
      tft.setCursor(50, 140);// where the characters will be displayed
      tft.print ("draw ");
      tft.print("\n");
    }
    else if(step != 0){
      if (a == 11){
        tft.setCursor(50, 140);
        tft.print("x win");
        tft.print("\n");
      }
      else if (a == 10){
        tft.setCursor(50, 140);
        tft.print("o win");
        tft.print("\n");
      }
      if ( step == -1 ){
        tft.setCursor(50, 140);
        tft.print("user surrended");
        tft.print("\n");
      }
    }
  }
  update = 0;
}
// scan joy stic motion
void scanJoystick(){
  int vert = analogRead(JOY_VERT_ANALOG);
  int horiz = analogRead(JOY_HORIZ_ANALOG);
  int select = digitalRead(JOY_SEL);
  if ((abs(horiz - JOY_CENTRE) > JOY_DEADZONE) /*&& update == 0*/) {
    int delta;
    delta = (horiz - JOY_CENTRE) / STEPS_PER_PIXEL;
    g_joyX = constrain(g_joyX + delta, 0, TFT_WIDTH);
    update = 1;
  }

  if (abs(vert - JOY_CENTRE) > JOY_DEADZONE) {
    int delta = (vert - JOY_CENTRE) / STEPS_PER_PIXEL;
    g_joyY = constrain(g_joyY + delta, 0, TFT_HEIGHT);

    update = 1;
  }
}
// reassign the varible
void fakeboard(chess_board updatefake[3][3]){
  for(int i=0; i<3; ++i){
    for(int j=0; j<3; ++j){
      updatefake[i][j].index = chess_pieces_info[i][j].index;
      updatefake[i][j].shape = chess_pieces_info[i][j].shape;
    }
  }

}
// initialize the chess board
void init_board(){
  for(int i=0; i<3; ++i){
    for(int j=0; j<3; ++j){
      chess_pieces_info[i][j].index = 0;
      chess_pieces_info[i][j].shape = -1;
    }
  }
   Serial.println("initial board ");
}
//  place the chess piece on the different block
void chess_pieces(){
  if(digitalRead(JOY_SEL) == LOW){
    fakeboard(last_step_of_chess);
    update = 1;
    if (g_joyY>=0 && g_joyY<=42){
      if(g_joyX>=0 && g_joyX<=42){
        chess_pieces_info[0][0].index = 1;
        if(mode == 0){
          chess_pieces_info[0][0].shape = 0;
          mode = 1 ;
        }
        else if(mode ==1){
          chess_pieces_info[0][0].shape = 1;
          mode = 0 ;
        }
      }
      else if (g_joyX>=43 && g_joyX<=85){
        chess_pieces_info[0][1].index = 1;
        if(mode == 0){
          chess_pieces_info[0][1].shape = 0;
          mode = 1 ;
        }
        else if(mode ==1){
          chess_pieces_info[0][1].shape = 1;
          mode = 0 ;
        }
      }
      else if (g_joyX>=86 && g_joyX<=128){
        chess_pieces_info[0][2].index = 1;
        if(mode == 0){
          chess_pieces_info[0][2].shape = 0;
          mode = 1 ;
        }
        else if(mode ==1){
          chess_pieces_info[0][2].shape = 1;
          mode = 0 ;
        }
      }
    }
    else if (g_joyY>=43 && g_joyY<=85){
      if(g_joyX>=0 && g_joyX<=42){
        chess_pieces_info[1][0].index = 1;
        if(mode == 0){
          chess_pieces_info[1][0].shape = 0;
          mode = 1;
        }
        else if(mode ==1){
          chess_pieces_info[1][0].shape = 1;
          mode = 0 ;
        }
      }
      else if (g_joyX>=43 && g_joyX<=85){
        chess_pieces_info[1][1].index = 1;
        if(mode == 0){
          chess_pieces_info[1][1].shape = 0;
          mode = 1 ;
        }
        else if(mode ==1){
          chess_pieces_info[1][1].shape = 1;
          mode = 0 ;
        }
      }
      else if (g_joyX>=86 && g_joyX<=128){
        chess_pieces_info[1][2].index = 1;
        if(mode == 0){
          chess_pieces_info[1][2].shape = 0;
          mode = 1 ;
        }
        else if(mode ==1){
          chess_pieces_info[1][2].shape = 1;
          mode = 0 ;
        }
      }
    }
    else if (g_joyY>=86 && g_joyY<=128){
      if(g_joyX>=0 && g_joyX<=42){
        chess_pieces_info[2][0].index = 1;
        if(mode == 0){
          chess_pieces_info[2][0].shape = 0;
          mode = 1 ;
        }
        else if(mode ==1){
          chess_pieces_info[2][0].shape = 1;
          mode = 0 ;
        }
      }
      else if (g_joyX>=43 && g_joyX<=85){
        chess_pieces_info[2][1].index = 1;
        if(mode == 0){
          chess_pieces_info[2][1].shape = 0;
          mode = 1 ;
        }
        else if(mode ==1){
          chess_pieces_info[2][1].shape = 1;
          mode = 0 ;
        }
      }
      else if (g_joyX>=86 && g_joyX<=128){
        chess_pieces_info[2][2].index = 1;
        if(mode == 0){
          chess_pieces_info[2][2].shape = 0;
          mode = 1;
        }
        else if(mode ==1){
          chess_pieces_info[2][2].shape = 1;
          mode = 0 ;
        }
      }
    }
    step = step - 1 ; // count the step remain
    Serial.print("step remaining");
    Serial.println(step);
  }
}
// give the value of the each situation
int evaluation(chess_board input[3][3]){
  int mark = 0;

  for (int j = 0 ; j < 3 ; ++j){
    if ((input[j][0].index == 1) && (input[j][1].index == 1) && (input[j][2].index == 1)){
      if ((input[j][0].shape == 1) && (input[j][1].shape == 1) && (input[j][2].shape == 1)){
        return mark = 10; // "x" win
      }
      if ((input[j][0].shape == 0) && (input[j][1].shape == 0) && (input[j][2].shape == 0)){
        return mark = -10; // "0 " win
      }
    }
  }

  for (int k = 0 ; k < 3 ; ++k){
    if ((input[0][k].index == 1) && (input[1][k].index == 1) && (input[2][k].index == 1)){
      if ((input[0][k].shape == 1) && (input[1][k].shape == 1) && (input[2][k].shape == 1)){
        return mark = 10; // "x" win
      }
      if ((input[0][k].shape == 0) && (input[1][k].shape == 0) && (input[2][k].shape == 0)){
        return mark = -10; // "0 " win
      }
    }
  }

  if ((input[0][0].index == 1) && (input[1][1].index == 1) && (input[2][2].index == 1)){
    if ((input[0][0].shape == 1) && (input[1][1].shape == 1) && (input[2][2].shape == 1)){
      return mark = 10; // "x" win
    }
    if ((input[0][0].shape == 0) && (input[1][1].shape == 0) && (input[2][2].shape == 0)){
      return mark = -10; // "0 " win
    }
  }
  if ((input[2][0].index == 1) && (input[1][1].index == 1) && (input[0][2].index == 1)){
    if ((input[2][0].shape == 1) && (input[1][1].shape == 1) && (input[0][2].shape == 1)){
      return mark = 10; // "x" win
    }
    if ((input[2][0].shape == 0) && (input[1][1].shape == 0) && (input[0][2].shape == 0)){
      return mark = -10; // "0 " win
    }
  }
  return mark = 0;
}
// check the the chess board is empty or not
bool check_terminal(chess_board terminal[3][3]){
  for(int i=0; i<3; ++i){
    for(int j=0; j<3; ++j){
      if(terminal[i][j].index == 0){
        return false;
      }
    }
  }
  return true;
}
// minmax algarithm, to return the best move for computer
int minmax(chess_board node[3][3], int depth, bool truefalse){
  chess_board input_board[3][3];
  //Serial.println("copying input ");
  for(int i=0; i<3; ++i){
    for(int j=0; j<3; ++j){
      input_board[i][j].index = node[i][j].index;
      input_board[i][j].shape = node[i][j].shape;
    }
  }
  int bestValue;
  int v;
  if( depth == 0 || check_terminal(node)){
    int value = evaluation(node);
    return value; //jia dian dong xi
  }

  else if(truefalse == 1 && depth>0){//max
    //count = count_blank();
    bestValue = -1000;
      for(int i=0; i<3; ++i){
        for(int j=0; j<3; ++j){
          if (node[i][j].index == 0){
            node[i][j].index = 1 ;
            node[i][j].shape = 1 ;
            testboard(node);
            v = minmax(node, depth - 1, !truefalse);
            bestValue = max(bestValue, v);
            node[i][j].index = 0;
            node[i][j].shape = -1;

          }
        }
      }

    return bestValue;
  }
  else if(truefalse == 0 && depth>0){//min


    bestValue = 1000;
    for(int i=0; i<3; ++i){
      for(int j=0; j<3; ++j){
        if (node[i][j].index == 0){
          node[i][j].index = 1 ;
          node[i][j].shape = 0 ;
          testboard(node);
          v = minmax(node, depth - 1, !truefalse);
          bestValue = min(bestValue, v);
          node[i][j].index = 0;
          node[i][j].shape = -1;
        }
      }
    }

   return bestValue;
 }
}
// check either man or computer win the game
int winornot(chess_board r[3][3]){
  int result = 0;
  for (int j = 0 ; j < 3 ; ++j){
    if ((r[j][0].index == 1) && (r[j][1].index == 1) && (r[j][2].index == 1)){
      if ((r[j][0].shape == 1) && (r[j][1].shape == 1) && (r[j][2].shape == 1)){
        result = 11; // "x" win
      }
      if ((r[j][0].shape == 0) && (r[j][1].shape == 0) && (r[j][2].shape == 0)){
        result= 10; // "0 " win
      }
    }
  }

  for (int k = 0 ; k < 3 ; ++k){
    if ((r[0][k].index == 1) && (r[1][k].index == 1) && (r[2][k].index == 1)){
      if ((r[0][k].shape == 1) && (r[1][k].shape == 1) && (r[2][k].shape == 1)){
        result = 11; // "x" win
      }
      if ((r[0][k].shape == 0) && (r[1][k].shape == 0) && (r[2][k].shape == 0)){
        result = 10; // "0 " win
      }
    }
  }


  if ((r[0][0].index == 1) && (r[1][1].index == 1) && (r[2][2].index == 1)){
    if ((r[0][0].shape == 1) && (r[1][1].shape == 1) && (r[2][2].shape == 1)){
      result = 11; // "x" win
    }
    if ((r[0][0].shape == 0) && (r[1][1].shape == 0) && (r[2][2].shape == 0)){
      result = 10; // "0 " win
    }
  }


    if ((r[2][0].index == 1) && (r[1][1].index == 1) && (r[0][2].index == 1)){
      if ((r[2][0].shape == 1) && (r[1][1].shape == 1) && (r[0][2].shape == 1)){
        result = 11; // "x" win
      }
      if ((r[2][0].shape == 0) && (r[1][1].shape == 0) && (r[0][2].shape == 0)){
        result = 10; // "0 " win
      }
    }
  return result ;
}
// comuter play chess
int com_play_chess(){
  chess_board fake_chess[3][3];
  fakeboard(fake_chess);
  bool max = 1;
  int m;
  int best = -1000;
  for(int i=0; i<3; ++i){
    for(int j=0; j<3; ++j){
      if(fake_chess[i][j].index == 0){
        fake_chess[i][j].index = 1;
        fake_chess[i][j].shape = 1;

        if (evaluation(fake_chess)== 10){
          first_step_i = i;
          first_step_j = j;
          best = 10;
          return 0 ;
        }
        else if ((depth > 1) && fake_chess[1][1].index == 0){
          if((fake_chess[0][0].index == 1) || (fake_chess[0][2].index == 1 ) || (fake_chess[2][2].index == 1 ) || (fake_chess[2][0].index == 1)){
            first_step_i = 1;
            first_step_j = 1;
            return 0 ;
          }
        }
        //testboard(fake_chess);
        m = minmax(fake_chess, depth - 1, !max);
        // Serial.print("m is ");
        // Serial.println(m);
        if(m>best){
          best = m;
          first_step_i=i;
          first_step_j=j;
        }
        fake_chess[i][j].index = 0;
        fake_chess[i][j].shape = -1;
      }
   }
  }

  return 0 ;
}
// in the mode of computer vs man
void comVSman(){
  if(digitalRead(JOY_SEL) == LOW){
    fakeboard(last_step_of_chess);
    update = 1;
    if (g_joyY>=0 && g_joyY<=42){
      if(g_joyX>=0 && g_joyX<=42){
        chess_pieces_info[0][0].index = 1;
        if(mode == 0){
          chess_pieces_info[0][0].shape = 0;
          mode = 1 ;
        }
      }
      else if (g_joyX>=43 && g_joyX<=85){
        chess_pieces_info[0][1].index = 1;
        if(mode == 0){
          chess_pieces_info[0][1].shape = 0;
          mode = 1 ;
        }
      }
      else if (g_joyX>=86 && g_joyX<=128){
        chess_pieces_info[0][2].index = 1;
        if(mode == 0){
          chess_pieces_info[0][2].shape = 0;
          mode = 1 ;
        }

      }
    }
    else if (g_joyY>=43 && g_joyY<=85){
      if(g_joyX>=0 && g_joyX<=42){
        chess_pieces_info[1][0].index = 1;
        if(mode == 0){
          chess_pieces_info[1][0].shape = 0;
          mode = 1;
        }

      }
      else if (g_joyX>=43 && g_joyX<=85){
        chess_pieces_info[1][1].index = 1;
        if(mode == 0){
          chess_pieces_info[1][1].shape = 0;
          mode = 1 ;
        }

      }
      else if (g_joyX>=86 && g_joyX<=128){
        chess_pieces_info[1][2].index = 1;
        if(mode == 0){
          chess_pieces_info[1][2].shape = 0;
          mode = 1 ;
        }

      }
    }
    else if (g_joyY>=86 && g_joyY<=128){
      if(g_joyX>=0 && g_joyX<=42){
        chess_pieces_info[2][0].index = 1;
        if(mode == 0){
          chess_pieces_info[2][0].shape = 0;
          mode = 1 ;
        }

      }
      else if (g_joyX>=43 && g_joyX<=85){
        chess_pieces_info[2][1].index = 1;
        if(mode == 0){
          chess_pieces_info[2][1].shape = 0;
          mode = 1 ;
        }

      }
      else if (g_joyX>=86 && g_joyX<=128){
        chess_pieces_info[2][2].index = 1;
        if(mode == 0){
          chess_pieces_info[2][2].shape = 0;
          mode = 1;
        }
      }
    }
    step = step - 1 ;

  }
  else if(mode == 1){
    update = 1;
    com_play_chess();
    chess_pieces_info[first_step_i][first_step_j].index = 1;
    chess_pieces_info[first_step_i][first_step_j].shape = 1;

    mode = 0;
    step = step - 1 ;

  }
}
// print the liitle cicle on the menus screen
void mode_selection( bool mode_in_ui ){
  if (mode_in_ui){
    tft.fillCircle(26,84,2,0x07FF);
    tft.fillCircle(26,94,2,ST7735_BLACK);
  }
  else {
    tft.fillCircle(26,84,2,ST7735_BLACK);
    tft.fillCircle(26,94,2,0x07FF);
  }
}
// check joy stick action
bool check_selection_state(){
  int v = analogRead(JOY_VERT_ANALOG);
  // Serial.println("analogRead");
  // Serial.println(v);
  if (abs(v - JOY_CENTRE)> JOY_DEADZONE){
    mode_in_ui = !mode_in_ui;
    delay(300);
  }
  else {
   ;
  }
  return mode_in_ui;
}
// print the title on the tft screen
void title(){

  tft.fillScreen(ST7735_BLACK);

  tft.setCursor(10, 40);
  tft.setTextColor(0xF800);
  tft.setTextSize(2);
  tft.setTextWrap(1);
  tft.print("TICTACTOE");

  tft.setCursor(30,60);
  tft.setTextColor(0xF800);
  tft.setTextSize(1);
  tft.setTextWrap(1);
  tft.print("Plase select");

  tft.setCursor(40,70);
  tft.setTextColor(0xF800);
  tft.setTextSize(1);
  tft.setTextWrap(1);
  tft.print("a   mode   ");

  tft.setCursor(32,90);
  tft.setTextColor(0x07FF);
  tft.setTextSize(1);
  tft.setTextWrap(1);
  tft.print("man vs  man ");

  tft.setCursor(32,80);
  tft.setTextColor(0x07FF);
  tft.setTextSize(1);
  tft.setTextWrap(1);
  tft.print("man vs comp");


  for(int i =0;i<7;i++){
  tft.drawCircle(85,120,13,(0XFFFF-900*i));
  tft.drawCircle(85,120,12,(0XFFFF-900*i));
  tft.drawLine(40+11,120-11,40-11,120+11,(0XFFFF-900*i));
  tft.drawLine(40-11,120-11,40+11,120+11,(0XFFFF-900*i));
  tft.drawLine(39+11,120-11,39-11,120+11,(0XFFFF-900*i));
  tft.drawLine(39-11,120-11,39+11,120+11,(0XFFFF-900*i));
  delay(200);
 }
}
// press button (connect to 3 ) to go to the main menu
void restart_game(){
  while (true) {
    /* code */
    int buttonValue3 = 1;
    buttonValue3 = digitalRead(3);
    if(buttonValue3 == LOW){
      user_interface = !user_interface;
      mode = 0;
      step = 10;
      update = 1;
      a = 0;
      break;
    }
  }
}
// read the potential meter and decide the difficulty of comp
void potentialmeter(){
  int read = analogRead(9);
  if ((read/2)<256){
    digitalWrite(13,HIGH);
    digitalWrite(12,LOW);
    depth = 1 ;

  }
  else if ((read/2)>256){
    digitalWrite(13,LOW);
    digitalWrite(12,HIGH);
    depth = 2;
  }
}
// assemeble all the funciton together
int main(){
  setup();
  while(true){
    if(user_interface == 1){
      bool push;
      title();
      while (true) {
        potentialmeter();
        push = check_selection_state();
        mode_selection(push);
        if(digitalRead(JOY_SEL) == LOW){
          user_interface = !user_interface;
          if(push == 1){
            game_mode = 1;
          }
          else if (push == 0){
            game_mode = 0;
          }
          delay(500);
          break;
        }
      }
    }

    else if(user_interface == 0){
      init_board();

      int buttonValue11 = 1;
      while(true){
        if(game_mode == 0){
          if(update == 1){
            updateScreen();
            scanJoystick();
          }
          else if(update == 0){
            scanJoystick();
            chess_pieces();
            buttonValue11 = digitalRead(11);
            // Serial.print("buttonValue11 is");
            // Serial.println(buttonValue11);
            if(buttonValue11 == LOW){
              for(int i=0; i<3; ++i){
                for(int j=0; j<3; ++j){
                  chess_pieces_info[i][j].index = last_step_of_chess[i][j].index;
                  chess_pieces_info[i][j].shape = last_step_of_chess[i][j].shape;
                }
              }
              step = step + 1;
              update = 1;
              if(mode == 1){
                mode = 0;
              }
              else if(mode == 0){
                mode = 1;
              }
            }
            a = winornot(chess_pieces_info);
            if ( step == 0){
              updateScreen();
              restart_game();
              break;
            }
            else if ( step  != 0){
              if (a == 11){
                updateScreen();
                restart_game();
                break;
              }
              else if (a == 10){
                updateScreen();
                restart_game();
                break;
              }
            }
          }
        }

        else if(game_mode ==1){
          if(update == 1){
            updateScreen();
            scanJoystick();
          }
          else if(update == 0){
            scanJoystick();
            comVSman();
            buttonValue11 = digitalRead(11);
            if(buttonValue11 == LOW){
              for(int i=0; i<3; ++i){
                for(int j=0; j<3; ++j){
                  chess_pieces_info[i][j].index = last_step_of_chess[i][j].index;
                  chess_pieces_info[i][j].shape = last_step_of_chess[i][j].shape;
                }
              }
              step = step + 2;
              update = 1;
              mode = 0;
            }
            a = winornot(chess_pieces_info);

            if ( step == 0){
              updateScreen();
              restart_game();
              break;
            }
            else if ( step  != 0){
              if (a == 11){
                updateScreen();
                restart_game();
                break;
              }
              else if (a == 10){
                updateScreen();
                restart_game();
                break;
              }
            }
          }
        }
      }
    }
  }
  Serial.end();
  return 0;
}
