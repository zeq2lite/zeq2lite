#include "ui_local.h"
void Menu_Common(int amount){
	int current;
	int x = 16;
	int y = 112;
	current = 1;
	UI_MenuLogo();
	while(current < amount+1){
		if(current == 1){UI_DrawHandlePic(x,y,124,34,uis.menuButton1);}
		if(current == 2){UI_DrawHandlePic(x,y,109,34,uis.menuButton2);}
		if(current == 3){UI_DrawHandlePic(x,y,102,34,uis.menuButton3);}
		if(current == 4){UI_DrawHandlePic(x,y,102,34,uis.menuButton4);}
		if(current == 5){UI_DrawHandlePic(x,y,109,34,uis.menuButton5);}
		if(current == 6){UI_DrawHandlePic(x,y,124,34,uis.menuButton6);}
		y += 38;
		current += 1;
	}
}
void Menu_Frame(void){
	UI_DrawHandlePic(147,113,367,310,uis.menuFrame);
}
void Menu_Side(void){}
