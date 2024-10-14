#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <sys/types.h>
#include <libapi.h>
#include <stdio.h>
#include <stdlib.h>

#define OT_LENGTH 1 
#define PACKETMAX 18 
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define ENEMY_COUNT 10
#define MOVE_DURATION 30
#define PAD_SELECT      1
#define PAD_L3          2
#define PAD_R3          4
#define PAD_START       8
#define PAD_UP          16
#define PAD_RIGHT       32
#define PAD_DOWN        64
#define PAD_LEFT        128
#define PAD_L2          256
#define PAD_R2          512
#define PAD_L1          1024
#define PAD_R1          2048
#define PAD_TRIANGLE    4096
#define PAD_CIRCLE      8192
#define PAD_CROSS       16384
#define PAD_SQUARE      32768

GsOT myOT[2];
GsOT_TAG myOT_TAG[2][1<<OT_LENGTH]; 
PACKET GPUPacketArea[2][PACKETMAX]; 

//int generate_random_pos(int axis);

u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack

short CurrentBuffer = 0; 
u_char padbuff[2][34];
int player_score = 0;
int player_deaths = 9999;
int goal_x = 220;
int goal_y = 100;
int player_x_pos = 25;
int player_y_pos = 25;
int player_high_score = 0;

int enemies_x[ENEMY_COUNT] = {40, 110};
int enemies_y[ENEMY_COUNT] = {40, 50};
int enemy_dir[ENEMY_COUNT];
int move_timer[ENEMY_COUNT] = {0, 0};

//int frame_count = 0;  // Frame counter
//int fps = 0;          // Frames Per Second value

void graphics();
void display();
void game_info(int score, int deaths, int high_score);

int get_rand_dir()
{
	return rand() % 4;
}

void move_enemies()
{
	for(int i = 0; i < ENEMY_COUNT; i++)
	{
		move_timer[i]++;
		if(move_timer[i] >= MOVE_DURATION)
		{
			enemy_dir[i] = get_rand_dir();
			move_timer[i] = 0;
		}

		switch(enemy_dir[i])
		{
			case 0: //up
				enemies_y[i]--;
				if(enemies_y[i] <= 0)
				{
					enemies_y[i] = SCREEN_HEIGHT - 1;
				}
				break;
			case 1: //down
				enemies_y[i]++;
				if(enemies_y[i] >= SCREEN_HEIGHT)
				{
					enemies_y[i] = 1;
				}
				break;
			case 2: //left
				enemies_x[i]--;
				if(enemies_x[i] <= 0)
				{
					enemies_x[i] = SCREEN_WIDTH - 1;
				}
				break;
			case 3: //right
				enemies_x[i]++;
				if(enemies_x[i] >= SCREEN_WIDTH)
				{
					enemies_x[i] = 1;
				}
				break;
		}
	}
}

void render_bg()
{
	TILE bg;
	setTile(&bg);
	setXY0(&bg, 0, 0);
	setWH(&bg, SCREEN_WIDTH, SCREEN_HEIGHT);
	setRGB0(&bg, 0, 0, 0);
	DrawPrim(&bg);
}

int generate_random_pos(int axis)
{
	if(axis == 1)
	{
		//x axis
		return 10 + rand() % (300 - 10 + 1);
	}
	if(axis == 0)
	{
		//y axis
		return 10 + rand() % (220-10 + 1);
	}
}

void render_goal_square(int player_x, int player_y)
{
	TILE goal;
	setTile(&goal);
	setXY0(&goal, goal_x, goal_y);
	setWH(&goal, 12, 12);
	setRGB0(&goal, 191, 255, 0);
	DrawPrim(&goal);

	if(player_x < goal_x + 12 && player_x + 12 > goal_x && player_y < goal_y + 12 && player_y + 12 > goal_y) 
	{
			player_score++;
			if(player_score >= player_high_score)
			{
				player_high_score = player_score;
			}
			goal_x = generate_random_pos(1);
			goal_y = generate_random_pos(0);
	}
}

void render_blue_square(int x, int y)
{
	TILE square;
	setTile(&square);
	setXY0(&square, x, y); //cords
	setWH(&square, 12, 12); //size
	setRGB0(&square, 0, 255, 251); //RGB color
	DrawPrim(&square); //draw the prim
}

void render_enemies(int player_x, int player_y)
{
	for(int i = 0; i < ENEMY_COUNT; i++)
	{
		TILE enemy;
		setTile(&enemy);
		setXY0(&enemy, enemies_x[i], enemies_y[i]);
		setWH(&enemy, 12, 12);
		setRGB0(&enemy, 255, 0, 0);
		DrawPrim(&enemy);

		// Check for collision between player and enemies
		if (player_x < enemies_x[i] + 12 && player_x + 12 > enemies_x[i] &&
		    player_y < enemies_y[i] + 12 && player_y + 12 > enemies_y[i])
		{
			player_deaths++;   // Player dies upon collision
			player_score = 0;
			player_x_pos = 25; // Reset player position
			player_y_pos = 25;
		}
	}
}

int main() 
{
	graphics();
	FntLoad(960, 256); //load font from bios
	SetDumpFnt(FntOpen(5, 20, 320, 240, 0, 512));
	padbuff[0][0] = padbuff[0][1] = 0xff;
	padbuff[1][0] = padbuff[1][1] = 0xff;
	u_short button;

	while (1) 
	{
		button = *((u_short*)(padbuff[0]+2));

		if(padbuff[0][0] == 0)
		{
			if(!(button & PAD_UP))
			{
				player_y_pos--;
				if(player_y_pos == 0)
				{
					player_y_pos = 225;
				}
			}
			if(!(button & PAD_DOWN))
			{
				player_y_pos++;
				if(player_y_pos == 226)
				{
					player_y_pos = 1;
				}
			}
			if(!(button & PAD_LEFT))
			{
				player_x_pos--;
				if(player_x_pos == 0)
				{
					player_x_pos = 306;
				}
			}
			if(!(button & PAD_RIGHT))
			{
				player_x_pos++;
				if(player_x_pos == 307)
				{
					player_x_pos = 1;
				}
			}
		}

		move_enemies(); // Call to move the enemies randomly

		render_bg();
		render_blue_square(player_x_pos, player_y_pos);
		render_goal_square(player_x_pos, player_y_pos);
		render_enemies(player_x_pos, player_y_pos); // Call to render and check enemy collisions
		//update_fps(); // Update FPS counter
		game_info(player_score, player_deaths, player_high_score);
		display();
	}

	return 0;
}

void graphics()
{
	SetVideoMode(0);
	
	GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsNONINTER|GsOFSGPU, 1, 0);
	GsDefDispBuff(0, 0, 0, SCREEN_HEIGHT); 
	
	myOT[0].length = OT_LENGTH;
	myOT[1].length = OT_LENGTH;
	myOT[0].org = myOT_TAG[0];
	myOT[1].org = myOT_TAG[1];
	
	GsClearOt(0,0,&myOT[0]);
	GsClearOt(0,0,&myOT[1]);
	ResetGraph(0);
	InitPAD(padbuff[0], 34, padbuff[1], 34);
	StartPAD();
}


void display()
{
	FntFlush(-1);
	DrawSync(0);
	VSync(0); 
	GsSwapDispBuff(); 
	
	GsSortClear(50, 50, 50, &myOT[CurrentBuffer]);
	GsDrawOt(&myOT[CurrentBuffer]);
	
	CurrentBuffer = !CurrentBuffer;
	GsClearOt(0,0,&myOT[CurrentBuffer]);
}

void game_info(int score, int deaths, int high_score)
{
	char c_score[12];
	char c_deaths[20];
	char c_high_score[12];
	sprintf(c_score, "Score: %d", score);
	sprintf(c_deaths, "Deaths: %d", deaths);
	sprintf(c_high_score, "High Score: %d", high_score);
	//sprintf(c_fps, "\n\n\nFPS: %d", fps);
	FntPrint(c_score);
	FntPrint("\n");
	FntPrint(c_high_score);
	FntPrint("\n");
	FntPrint(c_deaths);
	//FntPrint("\n");
	//FntPrint(c_fps);  // Display FPS
}