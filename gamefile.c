/*C includes*/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
	
//initial setup
#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000
// address for data reg of ps2 keyboard
// add 4 to get to control reg
#define PS2_KEYBOARD_BASE		0xFF200100
/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030

/* Timer addresses */
#define TIMER_BASE            0xFFFEC600
#define INTER1_BASE			  0xFF202000
#define INTER2_BASE		  	  0xFF202020
#define INTER2_STATUS ((volatile short*) (INTER2_BASE))
#define INTER2_CONTROL ((volatile short*) (INTER2_BASE+4))
#define INTER2_LOAD_BOT ((volatile short*) (INTER2_BASE+8))
#define INTER2_LOAD_TOP ((volatile short*) (INTER2_BASE+12))
	
/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
#define BLACK 0x0000
#define DARK_GREY 0x630C
#define LIGHT_GREY 0xAD75
#define LIGHT_BROWN 0xABAB
#define BROWN 0x79E0 

#define ABS(x) (((x) > 0) ? (x) : -(x))

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240
#define CHAR_RES_X 80
#define CHAR_RES_Y 60

/* Constants for animation */
#define BOX_LEN 4
#define WELL_LEN 20
#define WELL_LEN_MAX 25
#define PHONE_LEN 15
#define PHONE_WID 10
#define NUM_BOXES 8
#define	NUM_WELLS 4
#define	NUM_PHONES 3
#define STUDENT_WIDTH 30
#define STUDENT_HEIGHT 50
#define NUM_LIVES 3
#define WATER_WID 6
#define WATER_HEIGHT 9
#define Z_WID 25
#define Z_HEIGHT 24
#define ARROW_KEY_DIM 15
	
/* Constanst for student drawing */
#define HAIR_HEIGHT 10
#define HEAD_HEIGHT 20
#define EYE_WIDTH 5
#define EYE_HEIGHT 6
#define PUPIL_WIDTH 3
#define PUPIL_HEIGHT 2
#define ARM_WIDTH 6
#define ARM_HEIGHT 12
#define BODY_WIDTH 22
#define BODY_HEIGHT 8
#define LEG_WIDTH 8
#define LEG_HEIGHT 10
#define CROTCH_WIDTH 2
#define CROTCH_HEIGHT 5
#define SHOE_WIDTH 8
#define SHOE_HEIGHT 2

/* other constants*/
#define	DELAY 10*200000000	//load value for private timer; 10 second delay for 200MHz clock
#define PHONE_HIT_DELAY_LOW 0xffff	//bottom 16 bits of load value for 1st interval timer
#define PHONE_HIT_DELAY_HIGH 0x0fff	// top 16 bits. total interval delay ~=2.8sec
#define SAD_DELAY_TOP 0x0fff; //top 16 bits of load value for 2nd interval timer
#define SAD_DELAY_BOT 0xffff; //bottom 16 bits. total interval delay ~= 2.8 sec
	
/*subroutine declarations*/
void draw_line(int x1, int x2, int y1, int y2, short int color);
void plot_pixel(int x, int y, short int line_color);
void plot_pixel_well(int x, int y, short int line_color);
void clear_screen();
void clear_char();
void wait_for_vsync();
void drawStudent(int x, int y);
void drawStudentHelper(int x, int y, int width, int height, short int colour);
bool checkWellHit(int xWell, int yWell, const char* type);
bool checkPhoneHit();
void clearStudent();
void drawAPlus(int x, int y);
void drawPhone(int x, int y);
void drawHearts();
void drawGPA();
void plot_char(int x, int y, char letter);
void drawWater(int x, int y);
void drawSleep(int x, int y);
void drawGameOverWin();
void drawGameOverLose();
void drawMainMenu();
void drawArrowKeys();
void drawInstructions();
	
/*global variables-------------------------------------------------*/
//general coordinates
typedef struct coord{
	int x,y;	//all x, y represent top left corner
}Coord;
//wellness icons
typedef struct wellness{
	int x,y;
	bool hit;
	char* type;
}Wellness;
//phone icons
typedef struct phone {
	int x,y;
	int dx,dy;
	bool hit;
}Phone;
//character 
typedef struct characterData {	
	int x, y;
	int numLives;
	double gpa;
}Character;

volatile int pixel_buffer_start;
volatile char char_buffer_start;

//our visible icons
Wellness wells[NUM_WELLS];
Phone phones[NUM_PHONES];
Character student;

//drawn phone locations
Phone drawnPhones[NUM_PHONES];
Phone writtenPhones[NUM_PHONES];
int clearCount = 0; //flag for synchronizing wellness clearing
Wellness wellsPrev[NUM_WELLS]; //previous wellness icon locations

//all pixel locations; TRUE for occupied by background/wellness pixel
bool occupied[RESOLUTION_X][RESOLUTION_Y];

Coord phoneSpawn[4] = {{50,50}, {200, 75}, {120, 150}, {300,180}};
char* wellsType[3] = {"aplus", "water", "sleep"};
bool drawingWells = false;
bool clearing = false;
bool clearingHeart = false;
int clearHeartCount=0;
// initialize front and back drawn pixels
Coord studentFront = {0, 0};
Coord studentBack = {0, 0};

double gpa[11] = {0.0, 0.4, 0.8, 1.2, 1.6, 2.0, 2.4, 2.8, 3.2, 3.6, 4.0};
int gpaCount = 0;
bool sadStudent = false;
bool play = true;

/*MAIN ROUTINE------------------------------------------------------------------*/
int main(void)
{
	///////////////// setting up the pixel buffers /////////////////
    volatile int * pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
	// set front pixel buffer to start of FPGA On-chip memory
    *(pixel_ctrl_ptr + 1) = FPGA_ONCHIP_BASE; // first store the address in the back buffer
    // swap the front/back buffers, to set the front buffer location
     wait_for_vsync();	
    // initialize a pointer to the pixel buffer, used by drawing functions
    pixel_buffer_start = *pixel_ctrl_ptr;
	clear_screen();
	clear_char();
	// main menu of game waiting for user to start
	// drawing on the FRONT buffer
	drawMainMenu();	
	// set back pixel buffer to start of SDRAM memory
    *(pixel_ctrl_ptr + 1) = SDRAM_BASE;
	// draw on the back buffer
    pixel_buffer_start = *(pixel_ctrl_ptr + 1);
	// clear screen here => clear back buffer
	clear_screen();
    
	// seed rand
	srand(time(0));
	
	//////////////// setting up the ps2 keyboard /////////////////
	// pointer pointing to base address of ps2
	volatile int* ps2_ptr = (int*) PS2_KEYBOARD_BASE;	
	*ps2_ptr = 0xFF;	// reset ps2
	// will hold what's written to the ps2 (needed to check which key was pressed)
	int ps2_data;
	unsigned char keyPress = '0'; 	// to check what key was pressed
	
	// check for user input
	// start game when user pressed space bar
	while(keyPress != ')') {
		ps2_data = *ps2_ptr;
		if (ps2_data & 0x00008000)
			keyPress = ps2_data & 0xFF;
	}	
	
	while(play){
		clear_char();
		clear_screen(); // still clearing the back buffer
		////////////set up timer for wells spawning later on/////////////
		volatile int* timer_ctrl_ptr = (int *) TIMER_BASE;
		*timer_ctrl_ptr = DELAY;	//load counter for 5 second delay
		
		wait_for_vsync(); // swap front and back buffers on VGA vertical sync
		pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
		clear_screen();

		//////////// set up interval timer for phone hit delay //////////
		// note that interval timers use 16 bit registers
		volatile short int* intv_timer_ptr = (short int*) INTER1_BASE; 
		*intv_timer_ptr = (short) 0;
		// load delay value -> initially, let it wait for 3 seconds at the start
		*(intv_timer_ptr + 4) = (short int) PHONE_HIT_DELAY_LOW;	// isolate lower bits of delay num
		*(intv_timer_ptr + 6) = (short int) PHONE_HIT_DELAY_HIGH;	// isolate upper bits of delay num
		*(intv_timer_ptr + 2) = (short int) 0;	// make sure timer doesn't reload itself

		// initialize data for drawing
		for(int i=0; i < RESOLUTION_X; i++){
			for(int j=0; j < RESOLUTION_Y; j++){
				occupied[i][j] = false;
			}
		}
		for(int i=0; i < NUM_WELLS; i++){
			//box coordinates represent the top left corner
			wells[i].x = rand()%(RESOLUTION_X-WELL_LEN_MAX +1);
			wells[i].y = rand()%(RESOLUTION_Y-WELL_LEN_MAX - 30 +1)+30;
			wells[i].type = wellsType[rand()%3];
			wells[i].hit = false;
		}
		for(int i=0; i < NUM_PHONES; i++){
			phones[i].x = rand()%(RESOLUTION_X-PHONE_WID+1);
			phones[i].y = rand()%(RESOLUTION_Y-PHONE_LEN+1);
			phones[i].dx = ((rand()%2)*2)-1;
			phones[i].dy = ((rand()%2)*2)-1;
			phones[i].hit = false;
		}
		
		// student will start the game in the middle of the screen
		student.x = RESOLUTION_X/2 - 20;
		student.y = RESOLUTION_Y/2;
		student.gpa = gpa[0];
		gpaCount = 0;
		student.numLives = NUM_LIVES;

		//get wellness icons spawning at random locations
		drawingWells=true;
		for(int i=0; i < NUM_WELLS; i++){
			if(wells[i].type == "aplus")
				drawAPlus(wells[i].x, wells[i].y);
			else if(wells[i].type == "water")
				drawWater(wells[i].x, wells[i].y);
			else 
				drawSleep(wells[i].x, wells[i].y);
		}
		drawingWells = false;
		//lets start the timer now
		*(timer_ctrl_ptr + 2) = 1; 	//set enable

		//initialize "drawn" pixel arrays (arbitrarily to 0 at first)
		for(int i=0; i < NUM_PHONES; i++){
			drawnPhones[i].x = 0;
			drawnPhones[i].y = 0;
			writtenPhones[i].x = 0;
			writtenPhones[i].y = 0;
		}

		drawHearts();
		drawGPA();

		//swap buffers
		wait_for_vsync(); // swap front and back buffers on VGA vertical sync
		pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
		drawHearts();
		
		// can start first interval timer now
		*(intv_timer_ptr + 2) = (short int) 4;

	/*MAIN WHILE LOOP----------------------------------------------------------------*/
	
		while(1){
			// erase the previously drawn box
			clearStudent();
			studentFront.x = studentBack.x;
			studentFront.y = studentBack.y;

			/*WELLNESS------------------------------------------*/
			//deals with previous wellness icons to clear
			if(clearCount == 1) clearCount++;
			if(clearCount == 2){
				//clear
				clearing = true;
				for(int i=0; i < NUM_WELLS; i++){
					//call the draw functions with black here////////////////////
					if(wellsPrev[i].type == "aplus")
						drawAPlus(wellsPrev[i].x, wellsPrev[i].y);
					else if(wellsPrev[i].type == "water")
						drawWater(wellsPrev[i].x, wellsPrev[i].y);
					else 
						drawSleep(wellsPrev[i].x, wellsPrev[i].y);
				}
				clearing = false;
				clearCount = 0;
			}
			//Poll timer for wellness icon respawn
			if(*(timer_ctrl_ptr + 3) == 1){
				//want to clear when count reaches 2
				clearCount = 1;
				//clear
				clearing = true;
				for(int i=0; i < NUM_WELLS; i++){
					if(wells[i].type == "aplus")
						drawAPlus(wells[i].x, wells[i].y);
					else if(wells[i].type == "water")
						drawWater(wells[i].x, wells[i].y);
					else 
						drawSleep(wells[i].x, wells[i].y);
				}
				clearing = false;

				for(int i=0; i < NUM_WELLS; i++){
					//save old locs
					wellsPrev[i].x = wells[i].x;
					wellsPrev[i].y = wells[i].y;
					wellsPrev[i].type = wells[i].type;
					//set new locs
					wells[i].x = rand()%(RESOLUTION_X-WELL_LEN_MAX+1);
					wells[i].y = rand()%(RESOLUTION_Y-WELL_LEN_MAX - 30 +1)+30;
					wells[i].type = wellsType[rand()%3];
					// set hit to false
					wells[i].hit = false;
				}
				// clear the interrupt & set anything else needed
				*(timer_ctrl_ptr) = DELAY; //reload counter
				*(timer_ctrl_ptr + 2) = 1; //set enable
				*(timer_ctrl_ptr + 3) = 1; //write a 1 into the F bit to clear
			}
			//redraw wellness icons
			for(int i=0; i < NUM_WELLS; i++){
				if (wells[i].hit == false) {
					//redraw wellness icons
					drawingWells=true;
					if(wells[i].type == "aplus")
						drawAPlus(wells[i].x, wells[i].y);
					else if(wells[i].type == "water")
						drawWater(wells[i].x, wells[i].y);
					else 
						drawSleep(wells[i].x, wells[i].y);
					drawingWells = false;
				} else {
					// draw black to the current wellness
					clearing = true;
					if(wells[i].type == "aplus")
						drawAPlus(wells[i].x, wells[i].y);
					else if(wells[i].type == "water")
						drawWater(wells[i].x, wells[i].y);
					else 
						drawSleep(wells[i].x, wells[i].y);
					clearing = false;
				}
			}

			/*PHONES-------------------------------------------------*/
			//clear previous phone icons using "memory" array
			for(int i=0; i < NUM_PHONES; i++){
				for (int row = 0; row < PHONE_WID; row++) {
					for (int col = 0; col < PHONE_LEN; col++) {
						plot_pixel(drawnPhones[i].x+row, drawnPhones[i].y+col, BLACK);
					}
				} 
			}
			//update drawnPhones to be writtenPhones
			for(int i=0; i < NUM_PHONES; i++){
				drawnPhones[i].x = writtenPhones[i].x;
				drawnPhones[i].y = writtenPhones[i].y;
			}
			
			//draw phone icons 
			for(int i=0; i < NUM_PHONES; i++){
				//save these locations for 2 cycles later
				writtenPhones[i].x = phones[i].x;
				writtenPhones[i].y = phones[i].y;
				if (phones[i].hit == false) {
					drawPhone(phones[i].x, phones[i].y);
				} else {
					for (int row = 0; row < PHONE_WID; row++) {
						for (int col = 0; col < PHONE_LEN; col++) {
							plot_pixel(drawnPhones[i].x+row, drawnPhones[i].y+col, BLACK);
						}
					} 
				}
			}


			/*CHARACTER--------------------------------------------*/

			// get what's in the ps2 register
			ps2_data = *ps2_ptr;

			// isolate the data reg itself
			keyPress = ps2_data & 0xFF;

			// update position of box accordingly
			if (keyPress == ')') {	// space bar pressed -> stop the box
				drawStudent(student.x, student.y);
			} else if (keyPress == 'k' && student.x != 0) {	// left key pressed
				student.x--;
			} else if (keyPress == 't' && student.x + STUDENT_WIDTH != RESOLUTION_X - 1) {	// right key pressed
				student.x++;
			} else if (keyPress == 'u' && student.y != 0) {	// up key pressed
				student.y--;
			} else if (keyPress == 'r' && student.y + STUDENT_HEIGHT != RESOLUTION_Y - 1) {	// down key pressed
				student.y++;
			} 

			// see if student hits either a wellness icon
			for (int i = 0; i < NUM_WELLS; i++) {
				// only check if it's a hit if it hasn't been hit already
				if (wells[i].hit == false) 
					wells[i].hit = checkWellHit(wells[i].x, wells[i].y, wells[i].type);
				if (wells[i].hit)
					drawGPA();
			}

			// check if student hit a phone, but make sure the hps timer is done counting down
			// (i.e. wait period is over)
			// poll interval timer (T0 bit) -> only start timer again if a phone's been hit
			if (*intv_timer_ptr == 1) {	
				if (checkPhoneHit(student.x, student.y)) {
					// reconfigure timer so that student has some time to get away from the next phone
					*(intv_timer_ptr + 4) = (short int) PHONE_HIT_DELAY_LOW;	
					*(intv_timer_ptr + 6) = (short int) PHONE_HIT_DELAY_HIGH;	
					
					// reset TO bit
					*intv_timer_ptr = (short int) 0;

					// start up timer again
					*(intv_timer_ptr + 2) = (short int) 4;
				}
				
			}	
			
			//update phone data for next write
			for(int i=0; i < NUM_PHONES; i++){
				//respawn if character hit just now or if hit edges
				if((phones[i].hit)){
					phones[i].x = rand()%(RESOLUTION_X-PHONE_WID+1);
					phones[i].y = rand()%(RESOLUTION_Y-PHONE_LEN+1);
					phones[i].dx = ((rand()%2)*2)-1;
					phones[i].dy = ((rand()%2)*2)-1;
					phones[i].hit = false;
				}
				if(phones[i].x == 0 || phones[i].x+PHONE_WID-1 == RESOLUTION_X-1){
					phones[i].dx *= -1;
				}
				if(phones[i].y == 0 || phones[i].y+PHONE_LEN-1 == RESOLUTION_Y-1){
					phones[i].dy *= -1;
				}
				//update box coords using dx, dy
				phones[i].x += phones[i].dx;
				phones[i].y += phones[i].dy;
			}

				
			//also poll the 2nd interval timer
			//if student is still sad (from last phone hit), reset flag
			if(sadStudent && *INTER2_STATUS == 1){
				sadStudent = false;
				*INTER2_STATUS = 0;//reset TO bit
			}
			drawStudent(student.x, student.y);

			/*Other gameplay info----------------------------------------*/
			drawHearts();
			drawGPA();			

			//swap buffers
			wait_for_vsync(); // swap front and back buffers on VGA vertical sync
			pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
			//check if game over
			if(student.numLives == 0){
				drawGameOverLose();
				break;
			}
			if(student.gpa > 3.6){
				drawGameOverWin();
				//swap buffers
				// think we need this if we want to draw the grad cap
				wait_for_vsync(); // swap front and back buffers on VGA vertical sync
				pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
				// front = grad cap, back = last gameplay screen
				break;
			}
		}
		
		play = false;
		while(!play){
			//wait for user input to play again
			ps2_data = *ps2_ptr;
			keyPress = ps2_data & 0xFF;
			if (keyPress == 'Z'){
				play = true;
				clear_screen();
				wait_for_vsync(); // swap front and back buffers on VGA vertical sync
				pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
				// front = cleared screen, back = grad cap
			}
		}
	}
}



/*helper functions called in main------------------------------------------*/

//clears entire screen to black
void clear_screen(){
	//draw black pixels everywhere
	for(int x = 0; x < RESOLUTION_X; x++){
		for(int y = 0; y < RESOLUTION_Y; y++){
			plot_pixel(x, y, BLACK);
		}
	}
}

void clear_char(){
	for(int x=0; x < 80; x++){
		for(int y=0; y < 60; y++){
			plot_char(x,y, ' ');
		}
	}
}
//subroutine to draw line from (x1, y1) to (x2, y2) in specified color
void draw_line(int x1, int y1, int x2, int y2, short int color){
	//determine whether to sweep in x or y direction based on steepness
	bool is_steep = abs(y2-y1) > abs(x2-x1);
	if(is_steep){
		//swap x and y to traverse in optimal direction
		int x1Temp = x1;
		int x2Temp = x2;
		x1 = y1;
		x2 = y2;
		y1 = x1Temp;
		y2 = x2Temp;
	}
	//adjust to traverse in correct direction
	if(x1 > x2){
		int x1Temp = x1;
		int y1Temp = y1;
		x1 = x2;
		y1 = y2;
		x2 = x1Temp;
		y2 = y1Temp;
	}
	//set up error variable to determine incrementing x/y
	int delX = x2 - x1;
	int delY = abs(y2 - y1);
	int error = -(delX/2);
	int yLoc = y1;
	int y_step;
	//get direction of y traversal
	if (y1 < y2)
		y_step = 1;
	else
		y_step = -1;
		
	//loop through start and end x locs and draw 
	for(int xLoc = x1; xLoc <= x2; xLoc ++){
		if(is_steep) {
			if(clearing || drawingWells)
				plot_pixel_well(yLoc, xLoc, color);
			else
				plot_pixel(yLoc, xLoc, color);
		}
		else{
			if(clearing || drawingWells)
				plot_pixel_well(xLoc, yLoc, color);
			else
				plot_pixel(xLoc, yLoc, color);
		}
		
		//increment based on error variable
		error += delY;
		if(error >= 0){
			yLoc += y_step;
			error -= delX;
		}
	}
}

//subroutine to draw one pixel at (x,y) with specified color
void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

//separate subroutine to draw for wellness icons
//so that it does not affect other graphics by accident
void plot_pixel_well(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
	if(clearing)
		occupied[x][y] = false;
	if(drawingWells)
		occupied[x][y] = true;
}
//subroutine to draw one char at (x,y)
void plot_char(int x, int y, char letter)
{
    *(char *)(0xC9000000 + (y << 7) + (x)) = letter;
}
//synchronization & swapping of buffers
void wait_for_vsync(){
	//wait 1/60th of a second
	volatile int* pixel_ctrl_ptr = (int*)PIXEL_BUF_CTRL_BASE;
	//initiate the swap 
	*pixel_ctrl_ptr = 1;
	
	int status = *(pixel_ctrl_ptr + 3);	//status register containing s bit
	while((status & 1)!=0){
		status = *(pixel_ctrl_ptr + 3); //poll s bit
	}
	//return from function when s bit is 0 (swap complete)
}

bool checkWellHit(int xWell, int yWell, const char* type) {
	// determine what "bounds" we should be using for detecting a hit
	int wellW, wellH;
	if (type == "aplus") {
		wellW = WELL_LEN;
		wellH = WELL_LEN;
	} else if (type == "water") {
		wellW = WATER_WID;
		wellH = WATER_HEIGHT;
	} else {
		wellW = Z_WID;
		wellH = Z_HEIGHT;
	}
	
	bool isHit = false;
	if (type == "sleep"){
		//xWell, yWell represent top left corner, but not on zzz itself
		// need to check different bounds for the upper and lower half of body

		if((xWell >= student.x && xWell < student.x + STUDENT_WIDTH) ||
			(xWell + wellW-Z_WID/2 >= student.x && xWell + wellW < student.x + STUDENT_WIDTH)) {
				if ((yWell >= student.y && yWell < student.y + STUDENT_HEIGHT - 8) ||
					(yWell + wellH-Z_HEIGHT/2 >= student.y && yWell + wellH < student.y + STUDENT_HEIGHT - 8)) {
						isHit = true;
				}
		}
		if((xWell >= student.x + ARM_WIDTH && xWell < student.x + STUDENT_WIDTH - ARM_WIDTH) ||
			(xWell + wellW-Z_WID/2 >= student.x + ARM_WIDTH && xWell + wellW < student.x + STUDENT_WIDTH - ARM_WIDTH)) {
				if ((yWell >= student.y + STUDENT_HEIGHT - 8 && yWell < student.y + STUDENT_HEIGHT) ||
					(yWell + wellH-Z_HEIGHT/2 >= student.y + STUDENT_HEIGHT - 8 && yWell + wellH < student.y + STUDENT_HEIGHT)) {
						isHit = true;
				}
		}
	} else {
		if((xWell >= student.x && xWell < student.x + STUDENT_WIDTH) ||
			(xWell + wellW >= student.x && xWell + wellW < student.x + STUDENT_WIDTH)) {
				if ((yWell >= student.y && yWell < student.y + STUDENT_HEIGHT - 8) ||
					(yWell + wellH >= student.y && yWell + wellH < student.y + STUDENT_HEIGHT - 8)) {
						isHit = true;
				}
		}
		if ((xWell >= student.x + ARM_WIDTH && xWell < student.x + STUDENT_WIDTH - ARM_WIDTH) ||
				(xWell + wellW >= student.x + ARM_WIDTH && xWell + wellW < student.x + STUDENT_WIDTH - ARM_WIDTH)) {
				if ((yWell >= student.y + STUDENT_HEIGHT - 8 && yWell < student.y + STUDENT_HEIGHT) ||
					(yWell + wellH >= student.y + STUDENT_HEIGHT - 8 && yWell + wellH < student.y + STUDENT_HEIGHT)) {
						isHit = true;
				}
		}
		
		
	}
	
	if (isHit) {
		gpaCount++;
		student.gpa = gpa[gpaCount];
	}
	
	return isHit;

}

bool checkPhoneHit() {
	bool isHit = false;
	for (int i = 0; i < NUM_PHONES; i++) {
		// check if phone's x and y coordinates fall in the range of the student
		if ((phones[i].x >= student.x && phones[i].x < student.x + STUDENT_WIDTH) || 
			(phones[i].x + PHONE_WID >= student.x && phones[i].x + PHONE_WID < student.x + STUDENT_WIDTH)) {
				if ((phones[i].y >= student.y && phones[i].y < student.y + STUDENT_HEIGHT - 8) || 
					(phones[i].y + PHONE_LEN >= student.y && phones[i].y + PHONE_LEN < student.y + STUDENT_HEIGHT - 8)) {
						phones[i].hit = true;
						isHit = true;
						break;
				}
		}
		if ((phones[i].x >= student.x + ARM_WIDTH && phones[i].x < student.x + STUDENT_WIDTH - ARM_WIDTH) ||
				   (phones[i].x + PHONE_WID >= student.x + ARM_WIDTH && phones[i].x + PHONE_WID < student.x + STUDENT_WIDTH - ARM_WIDTH)) {
					if ((phones[i].y >= student.y + STUDENT_HEIGHT - 8 && phones[i].y < student.y + STUDENT_HEIGHT) ||
						(phones[i].y + PHONE_LEN >= student.y + STUDENT_HEIGHT - 8 && phones[i].y + PHONE_LEN < student.y + STUDENT_HEIGHT)) {
							phones[i].hit = true;
							isHit = true;
							break;
					}
		}
	}
	
	if (isHit) {
		//take away life
		student.numLives--;
		// reset gpa
		if (student.gpa > 2.0) 
			gpaCount = 5;
		else 
			gpaCount = 0;
		student.gpa = gpa[gpaCount];

		//set sad student status & turn on timer
		sadStudent = true;
		// Configure the timeout period
		*(INTER2_LOAD_BOT)=0xffff;
		*(INTER2_LOAD_TOP)=0x0fff;
		// Configure timer to start counting and to stop after
		*(INTER2_CONTROL)=4;
		return true;
	}
	
	return false;

}

void clearStudent() { 
	for (int row = 0; row < STUDENT_WIDTH; row++) {
		for (int col = 0; col < STUDENT_HEIGHT; col++) {
			plot_pixel(studentFront.x + row, studentFront.y + col, BLACK);
		}
	}
}

void drawStudent(int x, int y) {
	// save top left student coordinate being drawn
	// approximate student as a rectangle of width 30px and height 60px
	studentBack.x = x;
	studentBack.y = y;

	Coord hair = {x, y};
	Coord head = {x, y+HAIR_HEIGHT};
	Coord leftEye = {x+8, y+18};
	Coord rightEye = {x+STUDENT_WIDTH-13, y+18};
	Coord leftPupil = {x+9, y+22};	
	Coord rightPupil = {x+STUDENT_WIDTH-12, y+22};
	Coord leftArm = {x, y+HAIR_HEIGHT+HEAD_HEIGHT};	
	Coord rightArm = {x+STUDENT_WIDTH-ARM_WIDTH, y+HAIR_HEIGHT+HEAD_HEIGHT};
	Coord body = {x+ARM_WIDTH, y+HAIR_HEIGHT+HEAD_HEIGHT};	
	Coord leftLeg = {x+ARM_WIDTH, y+HAIR_HEIGHT+HEAD_HEIGHT+BODY_HEIGHT}; 
	Coord crotch = {x+ARM_WIDTH+LEG_WIDTH, y+HAIR_HEIGHT+HEAD_HEIGHT+BODY_HEIGHT};	
	Coord rightLeg = {x+ARM_WIDTH+LEG_WIDTH+CROTCH_WIDTH, y+HAIR_HEIGHT+HEAD_HEIGHT+BODY_HEIGHT};
	Coord leftShoe = {x+ARM_WIDTH, y+STUDENT_HEIGHT-2};
	Coord rightShoe = {x+ARM_WIDTH+LEG_WIDTH+CROTCH_WIDTH, y+STUDENT_HEIGHT-2};
	
	// draw hair
	drawStudentHelper(hair.x, hair.y, STUDENT_WIDTH, HAIR_HEIGHT, BROWN);
	
	// draw head
	drawStudentHelper(head.x, head.y, STUDENT_WIDTH, HEAD_HEIGHT, LIGHT_BROWN);
	
	// draw facial features
	drawStudentHelper(leftEye.x, leftEye.y, EYE_WIDTH, EYE_HEIGHT, BLACK);
	drawStudentHelper(rightEye.x, rightEye.y, EYE_WIDTH, EYE_HEIGHT, BLACK);
	drawStudentHelper(leftPupil.x, leftPupil.y, PUPIL_WIDTH, PUPIL_HEIGHT, WHITE);
	drawStudentHelper(rightPupil.x, rightPupil.y, PUPIL_WIDTH, PUPIL_HEIGHT, WHITE);
	draw_line(x+13, y+27, x+17, y+27, BLACK);
	
	// draw body
	drawStudentHelper(leftArm.x, leftArm.y, ARM_WIDTH, ARM_HEIGHT, RED);
	drawStudentHelper(rightArm.x, rightArm.y, ARM_WIDTH, ARM_HEIGHT, RED);
	drawStudentHelper(body.x, body.y, BODY_WIDTH, BODY_HEIGHT, RED);
	
	// draw legs
	drawStudentHelper(leftLeg.x, leftLeg.y, LEG_WIDTH, LEG_HEIGHT, BLUE);
	drawStudentHelper(crotch.x, crotch.y, CROTCH_WIDTH, CROTCH_HEIGHT, BLUE);
	drawStudentHelper(rightLeg.x, rightLeg.y, LEG_WIDTH, LEG_HEIGHT, BLUE);
	
	// draw shoes
	drawStudentHelper(leftShoe.x, leftShoe.y, SHOE_WIDTH, SHOE_HEIGHT, WHITE);
	drawStudentHelper(rightShoe.x, rightShoe.y, SHOE_WIDTH, SHOE_HEIGHT, WHITE);
	
	//if sad, draw sad brows
	if(sadStudent){
		draw_line(rightEye.x-8,rightEye.y-4, rightEye.x-13, rightEye.y+2, BLACK);
		draw_line(rightEye.x+3,rightEye.y-4, rightEye.x+8, rightEye.y+2, BLACK);
	}
}

void drawStudentHelper(int x, int y, int width, int height, short int colour) {
	// draws individual elements of students
	for (int xInc = 0; xInc < width; xInc++) {
		for (int yInc = 0; yInc < height; yInc++) {
			plot_pixel(x + xInc, y + yInc, colour);
		}
	}
}



void drawAPlus(int x, int y){
	//x, y is top left pixel of 10x10 pixel box
	//hardcoding graphics go brrr
	Coord top = {x+9, y+2};
	Coord botLeft = {x+3, y+18};
	Coord botRight = {x+15, y+18};
	Coord midLeft = {x+5, y+10};
	Coord midRight = {x+13, y+10};
	Coord plusTop = {x+16, y+1};
	Coord plusBot = {x+16, y+5};
	Coord plusRight = {x+18, y+3};
	Coord plusLeft = {x+14, y+3};
	short int color;
	if(clearing) color = BLACK;
	else color = RED;
	
	//draw 5 lines; 3 segs for A, 2 segs for +
	draw_line(top.x, top.y, botLeft.x, botLeft.y, color);
	draw_line(top.x, top.y, botRight.x, botRight.y, color);
	draw_line(top.x-1, top.y, botLeft.x-1, botLeft.y, color);
	draw_line(top.x+1, top.y, botRight.x+1, botRight.y, color);
	draw_line(midLeft.x, midLeft.y, midRight.x, midRight.y, color);
	draw_line(midLeft.x, midLeft.y+1, midRight.x, midRight.y+1, color);
	draw_line(plusTop.x, plusTop.y, plusBot.x, plusBot.y, color);	
	draw_line(plusLeft.x, plusLeft.y, plusRight.x, plusRight.y, color);		
}
void drawWater(int x, int y){
	//for now, x width = 6, y length = 9
	short int outline;
	short int fill;
	if(clearing) {
		outline = BLACK;
		fill = BLACK;
	}
	else {
		outline = WHITE;
		fill = CYAN;
	}
	draw_line(x, y, x, y+8, outline);
	draw_line(x+5, y, x+5, y+8, outline);
	draw_line(x, y+8, x+5, y+8, outline);
	for(int i=1; i<5; i++){
		for(int j=3; j<8; j++){
			plot_pixel_well(x+i, y+j, fill);
		}
	}
}
void drawSleep(int x, int y){
	short int color;
	if(clearing) color = BLACK;
	else color = WHITE;
	
	for(int i=0; i<21; i+=7){
		draw_line(x+i, y-i, x+5+i, y-i, color);
		draw_line(x+i, y+4-i, x+5+i, y+4-i, color);
		draw_line(x+5+i, y-i, x+i, y+4-i, color);
	}
}

void drawPhone(int x, int y){
	//draw grey rectangle 
	for (int row = 0; row < PHONE_WID; row++) {
		for (int col = 0; col < PHONE_LEN; col++) {
			plot_pixel(x + row, y + col, DARK_GREY);
		}
	}  
	//draw smaller, lighter grey rectangle
	for (int col = 2; col < PHONE_WID-2; col++) {
		for (int row = 2; row < PHONE_LEN-4; row++) {
			if((col >= PHONE_WID/2-1 && col <=PHONE_WID/2 && row>=3 && row<=PHONE_LEN-6) &&
			   (row <= PHONE_LEN-9 || row >= PHONE_LEN-7)){
				plot_pixel(x + col, y + row, RED);
			} else {
				plot_pixel(x + col, y + row, LIGHT_GREY);
			}
		}
	}  
}

void drawGPA(){
	for(int xloc = 1; xloc <= 7; xloc++){
		plot_char(xloc, 1, ' ');
	}
	//writing on top left corner for convenience for now
	//'G' at (0,0) of CHAR array (see DE1-SOC manual pg.22)
	plot_char(1, 1, 'G');
	plot_char(2, 1, 'P');
	plot_char(3, 1, 'A');
	plot_char(4, 1, ':');
	
	//get the current gpa and cast the ints into chars
	char gpaTens = (int) student.gpa + '0';
	char gpaTenths = '0';
	if (student.gpa < 1) 
		gpaTenths = student.gpa * 10 + '0';
	else 
		gpaTenths = ((int) (student.gpa * 10)) % 10 + '0';

	plot_char(5, 1, gpaTens);
	plot_char(6, 1, '.');
	plot_char(7, 1, gpaTenths);
}


//heart png array
//if black background desired, change 65535 to 0
const uint16_t pixelHeart[10][10] = {
	{65535,65535,65535,65535,65535,65535,65535,65535,65535,65535},
	{65535,65535,0,0,65535,65535,0,0,65535,65535},
	{65535,61440,64528,65503,61440,61440,63488,63488,61440,65535},
	{65535,63488,65535,63488,63488,63488,63488,63488,63488,65535},
	{65535,63488,63488,63488,63488,63488,63488,63488,63488,65535},
	{65535,65535,63488,63488,63488,63488,63488,18432,65535,65535},
	{65535,65535,65535,0,63488,63488,0,65535,65535,65535},
	{65535,65535,65535,65535,63488,63488,65535,65535,65535,65535},
	{65535,65535,65535,65535,65535,65535,65535,65535,65535,65535},
	{65535,65535,65535,65535,65535,65535,65535,65535,65535,65535},

};

void drawHearts(){
	//cast 16bit unsigned int into short int
	int x = 280+10*(2-student.numLives);
	int y=5;
	if(student.numLives < 3){
		for(int i=0; i<10; i++){
			for(int j=0; j<10; j++){
				plot_pixel(x+i, y+j,BLACK);
			}
		}
		clearHeartCount++;
	}
	x = 280+10*(3-student.numLives);
	for(int n=0; n < student.numLives; n++){
		//draw 1 heart
		for(int i=0; i<10; i++){
			for(int j=0; j<10; j++){
				plot_pixel(x+i, y+j, (short int)pixelHeart[j][i]);
			}
		}
		//update heart coordinate
		x += 10;
	}
}

const uint16_t gradCap[50][50] = {
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2113,4258,6307,4258,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2081,2081,25324,50776,54970,50744,10597,2081,2081,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14791,35985,42324,52857,63454,65535,63454,46518,42324,35953,8484,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6339,23243,29614,42260,61277,65535,65535,65535,65535,65535,65535,65535,59196,38034,29614,21130,4258,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,2081,10565,16872,27437,52889,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,50744,25324,16872,10533,2113,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,4226,6339,12678,42292,61309,61341,65503,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63454,61341,61309,40147,12678,6371,4194,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,31695,52857,54970,61309,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,61309,54970,52857,27501,2081,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,19049,38066,40179,54970,65503,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,52857,40211,40147,16936,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,8484,23243,27469,46486,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,46486,38066,40147,57051,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,44373,27469,23243,8452,0,0,0,0,0},
	{0,0,0,2113,8452,12710,31695,57115,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,59228,52857,50744,14791,32,4194,46518,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,59164,29614,12710,8452,32,0,0},
	{0,0,2081,12678,40147,59196,61309,65503,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63390,59228,38034,10597,6339,2081,2145,8484,57083,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,61309,59196,38066,10565,2081,0},
	{0,0,14791,52857,59228,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63422,40179,14791,8452,2081,0,0,27501,50712,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,59228,52857,14791,0},
	{0,0,14791,57051,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,59164,31727,27469,14791,0,0,4258,29582,38034,52857,63454,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,52825,10597,0},
	{0,0,8420,31695,42260,50744,63422,65535,65535,65535,65535,65535,65535,59196,42260,33808,4226,0,0,10597,23243,29582,57051,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63422,50744,42260,27469,4226,0},
	{0,0,0,0,0,23243,50712,54938,61341,65535,65535,63422,54938,40179,2113,0,2081,6371,12678,35985,63390,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,61277,54938,48631,23211,0,0,0,0},
	{0,0,0,0,0,2113,4226,6339,48599,65535,63454,50744,8452,4194,0,2081,8452,38066,61277,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63422,61309,35953,4258,4226,2113,0,0,0,0},
	{0,0,0,0,0,0,0,29614,59228,65535,25388,12710,32,0,10533,46486,50712,59196,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,48631,16936,16904,8484,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,42260,65535,42292,6339,0,0,4194,31695,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,57051,35921,29614,19049,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,8484,48599,65535,21130,2081,0,0,4226,31695,44405,50712,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,61341,50712,44373,31727,6339,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,29582,59228,63422,14791,0,8452,32,0,2113,4194,16904,48631,54970,59228,65503,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,59196,54970,46486,16936,2145,2113,0,2113,10533,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,35953,65535,55002,12646,0,46486,4258,0,0,0,2113,6307,8420,33808,61341,65535,65535,65535,65535,65535,65535,65535,65535,65535,59228,31727,6339,4258,2113,0,0,0,14791,54970,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,38034,65535,31727,4226,0,57115,46550,46486,35985,2081,0,0,0,8484,19017,19049,50712,65503,65535,65535,65535,65503,48599,19049,16936,8452,0,0,0,6339,35953,46486,50744,63422,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,38034,65535,21130,0,0,61309,65535,65535,59196,33808,31727,21130,2081,0,0,0,21162,31727,33840,52857,35921,31727,21130,0,0,0,2145,19049,31727,35985,59164,65535,65535,65503,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,8452,44373,65535,21162,32,0,54938,65535,65535,65535,65535,65535,48631,21130,19017,8452,32,0,0,32,29582,2113,0,0,32,8452,19017,23243,46518,65535,65535,65535,65535,65535,65503,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,2081,29582,61309,65535,31727,4226,0,33808,63422,65535,65535,65535,65535,65503,63422,63390,33840,10533,6371,2113,0,2081,0,2145,6371,10565,33808,63422,63422,65503,65535,65535,65535,65535,65535,65503,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,8484,44373,65535,65535,59196,12678,0,6339,61277,65535,65535,65535,65535,65535,65535,65535,59228,54970,54938,21130,4194,2113,4194,21162,54970,54970,59228,65535,65535,65535,65535,65535,65535,65535,65535,65503,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,19049,55002,65535,65535,65535,14791,0,0,38034,63454,65535,65535,65535,65535,65535,65535,65535,65535,65535,50744,44373,42292,44373,50744,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,16936,52857,65535,65535,65535,14791,0,0,19017,63390,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,6339,38066,63422,65535,54970,10597,0,0,16936,63390,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,23243,57083,65535,25356,2113,0,0,33808,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63454,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,6339,42292,65535,27469,2113,0,0,54970,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,52889,32,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,10597,46518,65535,40147,6339,0,0,61309,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63390,23211,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,23243,57083,65535,52825,10565,0,0,38034,52857,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,48599,38034,2113,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,31727,65535,65535,61309,12710,0,0,2113,27501,52825,59164,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,57115,52825,19017,2113,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,32,33808,65535,65535,63422,14791,0,0,0,2113,8452,31727,61309,61309,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63422,61309,61309,29582,6371,2113,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,10565,44405,65535,65535,65535,14791,0,0,0,0,32,6371,14791,16872,33840,59196,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,59228,35953,16872,14791,6339,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,21162,57115,65535,65535,65535,14791,0,0,0,0,0,0,0,32,10565,25356,27501,27501,27501,27501,27501,27501,27501,27501,27501,27501,27501,25356,12678,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,8420,40179,65535,65535,65535,63390,14759,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,2081,23275,63422,65535,65535,65535,52857,10565,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,8420,40179,65535,65535,65535,65535,27469,2113,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,19017,59196,65535,65535,65535,65535,19049,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,44373,65535,65535,65535,65535,46518,10565,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,42292,52825,65535,65535,55002,21162,2113,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,21130,57115,57115,31695,4226,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,2145,8452,8452,4226,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},

};
void drawGameOverWin(){
	//first clear all current graphics and characters
	clear_screen();	
	//clear gpa chars by writing ' ';
	//gpa chars are located at x[1,7] and y=1.
	for(int xloc = 1; xloc <= 7; xloc++){
		plot_char(xloc, 1, ' ');
	}
	char* header = "Game Over";
	char* text = "Con-grad-ulations! You made it!";
	char* playAgain = "Press enter to play again";
	for(int i=0; i< strlen(header); i++){
		plot_char((RESOLUTION_X/2)/8+15+i, (RESOLUTION_Y/2)/8, header[i]);
	}
	for(int i=0; i< strlen(text); i++){
		plot_char((RESOLUTION_X/2)/8+5+i, (RESOLUTION_Y/2)/8+2, text[i]);
	}
	for(int i=0; i< strlen(playAgain); i++){
		plot_char((RESOLUTION_X/2)/8+9+i, (RESOLUTION_Y/2)/8+4, playAgain[i]);
	}
	//draw grad cap
	int x= (RESOLUTION_X/2 - 25);
	int y= (RESOLUTION_Y/2);
	for(int i=0; i<50; i++){
		for(int j=0; j<50; j++){
			plot_pixel(x+i, y+j, (short int)gradCap[j][i]);
		}
	}

}
void drawGameOverLose(){
	//first clear all current graphics and characters
	clear_screen();
	for(int xloc = 1; xloc <= 7; xloc++){
		plot_char(xloc, 1, ' ');
	}
	char* header = "Game Over";
	char* text = "Don't give up! Life is not all about grades.";
	char* playAgain = "Press enter to play again";
	
	for(int i=0; i< strlen(header); i++){
		plot_char((RESOLUTION_X/2)/8+15+i, (RESOLUTION_Y/2)/8, header[i]);
	}
	for(int i=0; i< strlen(text); i++){
		plot_char((RESOLUTION_X/2)/8+i, (RESOLUTION_Y/2)/8+2, text[i]);
	}
	for(int i=0; i< strlen(playAgain); i++){
		plot_char((RESOLUTION_X/2)/8+8+i, (RESOLUTION_Y/2)/8+4, playAgain[i]);
	}
	
	// make sure student isn't sad at the start of the next game
	sadStudent = false;
	
}

void drawMainMenu(){
	//all pixels and char should be cleared before this func call
	char* header = "Welcome to 4.0 Student";
	char* text = "Press space to start your journey!";
	for(int i=0; i< strlen(header); i++){
		plot_char((RESOLUTION_X/2)/8+8+i, (RESOLUTION_Y/2)/8, header[i]);
	}
	for(int i=0; i< strlen(text); i++){
		plot_char((RESOLUTION_X/2)/8+2+i, (RESOLUTION_Y/2)/8+2, text[i]);
	}
	// student will start in the middle of the screen
	student.x = RESOLUTION_X / 2 - 70;
	student.y = RESOLUTION_Y / 2 - 20;
	student.gpa = gpa[0];
	student.numLives = NUM_LIVES;
	drawStudent(student.x, student.y);
	
	// visual instructions on how to play the game drawn beside the student
	drawArrowKeys();
	drawInstructions();
}

void drawArrowKeys() {
	// left key
	drawStudentHelper(180, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM, ARROW_KEY_DIM, ARROW_KEY_DIM, WHITE);
	draw_line(180 + 2, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2, 180 + ARROW_KEY_DIM - 3, 
			  student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2, BLACK);
	draw_line(180 + 2, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2, 180 + 5, 
			  student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2 - 3, BLACK);
	draw_line(180 + 2, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2, 180 + 5, 
			  student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2 + 3, BLACK);
	// down key
	drawStudentHelper(200, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM, ARROW_KEY_DIM, ARROW_KEY_DIM, WHITE);
	draw_line(200 + ARROW_KEY_DIM/2, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM + 3, 
			  200 + ARROW_KEY_DIM/2, student.y + STUDENT_HEIGHT - 3, BLACK);
	draw_line(200 + ARROW_KEY_DIM/2, student.y + STUDENT_HEIGHT - 3, 
			  200 + ARROW_KEY_DIM/2 - 3, student.y + STUDENT_HEIGHT - 6, BLACK);
	draw_line(200 + ARROW_KEY_DIM/2, student.y + STUDENT_HEIGHT - 3, 
			  200 + ARROW_KEY_DIM/2 + 3, student.y + STUDENT_HEIGHT - 6, BLACK);
	// right key
	drawStudentHelper(220, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM, ARROW_KEY_DIM, ARROW_KEY_DIM, WHITE);
	draw_line(220 + 2, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2, 220 + ARROW_KEY_DIM - 3, 
			  student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2, BLACK);
	draw_line(220 + ARROW_KEY_DIM - 3, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2,
			  220 + ARROW_KEY_DIM - 6, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2 - 3, BLACK);
	draw_line(220 + ARROW_KEY_DIM - 3, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2,
			  220 + ARROW_KEY_DIM - 6, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM/2 + 3, BLACK);
	// up key
	drawStudentHelper(200, student.y + STUDENT_HEIGHT - 2*ARROW_KEY_DIM - 5, ARROW_KEY_DIM, ARROW_KEY_DIM, WHITE);
	draw_line(200 + ARROW_KEY_DIM/2, student.y + STUDENT_HEIGHT - 2*ARROW_KEY_DIM - 5 + 3, 
			  200 + ARROW_KEY_DIM/2, student.y + STUDENT_HEIGHT - ARROW_KEY_DIM - 5 - 3, BLACK);
	draw_line(200 + ARROW_KEY_DIM/2, student.y + STUDENT_HEIGHT - 2*ARROW_KEY_DIM - 5 + 3,
			  200 + ARROW_KEY_DIM/2 - 3, student.y + STUDENT_HEIGHT - 2*ARROW_KEY_DIM - 5 + 6, BLACK);
	draw_line(200 + ARROW_KEY_DIM/2, student.y + STUDENT_HEIGHT - 2*ARROW_KEY_DIM - 5 + 3,
			  200 + ARROW_KEY_DIM/2 + 3, student.y + STUDENT_HEIGHT - 2*ARROW_KEY_DIM - 5 + 6, BLACK);
	
	const char* instrucs1 = "Press an arrow key to move";
	const char* instrucs2 = "left, right, up, or down";
	for(int i = 0; i < strlen(instrucs1); i++){
		plot_char(CHAR_RES_X/2-1+i, CHAR_RES_Y-36, instrucs1[i]);
	}
	for(int i=0; i < strlen(instrucs2); i++) {
		plot_char(CHAR_RES_X/2+i, CHAR_RES_Y-34, instrucs2[i]);
	}
}

void drawInstructions() {
	// instructions for the wellness icons
	const char* wInst1 = "Collect wellness icons to gain";
	const char* wInst2 = "GPA points. Each are worth 0.4";
	const char* wInst3 = "points, for a total GPA of 4.0!";
	for (int i = 0; i < strlen(wInst1); i++) {
		plot_char(8+i, CHAR_RES_Y-17, wInst1[i]);
	}
	for (int i = 0; i < strlen(wInst2); i++) {
		plot_char(8+i, CHAR_RES_Y-15, wInst2[i]);
	}
	for (int i = 0; i < strlen(wInst3); i++) {
		plot_char(8+i, CHAR_RES_Y-13, wInst3[i]);
	}
	
	drawAPlus(52, student.y + STUDENT_HEIGHT + 50);
	drawWater(88, student.y + STUDENT_HEIGHT + 55);
	drawSleep(110, student.y + STUDENT_HEIGHT + 62);
	
	// tell player to avoid the phones
	const char* phoneInst1 = "Avoid the phones, or your";
	const char* phoneInst2 = "GPA score will fall, and";
	const char* phoneInst3 = "you'll lose a life...";
	for (int i = 0; i < strlen(phoneInst1); i++) {
		plot_char(CHAR_RES_X/2+6+i, CHAR_RES_Y-17, phoneInst1[i]);
	}
	for (int i = 0; i < strlen(phoneInst2); i++) {
		plot_char(CHAR_RES_X/2+6+i, CHAR_RES_Y-15, phoneInst2[i]);
	}
	for (int i = 0; i < strlen(phoneInst3); i++) {
		plot_char(CHAR_RES_X/2+8+i, CHAR_RES_Y-13, phoneInst3[i]);
	}
	
	drawPhone(RESOLUTION_X/2+70, RESOLUTION_Y - 40);
}
