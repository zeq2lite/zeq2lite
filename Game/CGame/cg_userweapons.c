// Copyright (C) 2003-2004 RiO
//
// cg_userweapons.c -- Clientside custom weapons database and parser functions.
//                     Contains the actual loading of the clientside files, and
//                     of the both games files. Passes the pointer to the loaded
//                     string of the latter to bg_userweapons.c. 

#include "cg_local.h"


static cg_userWeapon_t		weaponGraphicsDatabase[MAX_CLIENTS][ALTSPAWN_OFFSET + MAX_PLAYERWEAPONS];
							// * 2 because we also have alternate fires possible for each
							// weapon!

cg_userWeapon_t *CG_FindUserWeaponGraphics( int clientNum, int index ) {
	return &weaponGraphicsDatabase[clientNum][index - 1];
}

void CG_CopyUserWeaponGraphics( int from, int to ) {
	memcpy(&weaponGraphicsDatabase[to], &weaponGraphicsDatabase[from], sizeof(cg_userWeapon_t)*(ALTSPAWN_OFFSET+MAX_PLAYERWEAPONS));
}


