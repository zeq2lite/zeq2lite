#include "g_local.h"

#define RADAR_UPDATE_TIME	5000 // update the radar once every X milliseconds


radar_t g_playerOrigins[MAX_CLIENTS]; //global storage for player positions

void G_RadarUpdateCS(void) {
	int i, valid_count;
	gentity_t *ent;
	playerState_t *ps;
	char cmd[MAX_TOKEN_CHARS];	// make sure our command string is
								// large enough for all the data

	// do we need to update the positions yet?
	if (level.time - level.lastRadarUpdateTime > RADAR_UPDATE_TIME) {
		//store the current time so we know when to update next
		level.lastRadarUpdateTime = level.time;

		//for each possible client
		valid_count = 0;
		
		for (i = 0; i < g_maxclients.integer; i++) {
			//get a pointer to the entity
			ent = g_entities + i;			

			//see if we have a valid entry
			if ( ent->client->pers.connected != CON_CONNECTED ) {
				g_playerOrigins[i].valid = qfalse;
			} else if ( !(ent->inuse) ) {
				g_playerOrigins[i].valid = qfalse;
			} else if( ent->client->ps.powerLevel[plCurrent] <= 0 ) {
				g_playerOrigins[i].valid = qfalse;
			} else {
				// get the client's player info
				ps = &ent->client->ps;

				//get and store the client position and information
				VectorCopy( ps->origin, g_playerOrigins[i].pos );

				g_playerOrigins[i].pl = ps->powerLevel[plCurrent];
				g_playerOrigins[i].plMax = ps->powerLevel[plMaximum];
				g_playerOrigins[i].clientNum = ps->clientNum;

				g_playerOrigins[i].properties = 0;
				if ( ( ps->stats[stChargePercentPrimary] >= 50 ) || ( ps->stats[stChargePercentSecondary] >= 50 ) ) {
					g_playerOrigins[i].properties |= RADAR_WARN;
				}
				if ( ( ps->eFlags & EF_AURA ) || ps->bitFlags & usingBoost ) {
					g_playerOrigins[i].properties |= RADAR_BURST;
				}

				g_playerOrigins[i].team = ps->persistant[ PERS_TEAM ];
				if ( g_playerOrigins[i].team >= TEAM_SPECTATOR ) {
					// mark as invalid entry for a spectator
					g_playerOrigins[i].valid = qfalse;
				
				} else {
					//mark as valid entry
					g_playerOrigins[i].valid = qtrue;

					//increase the valid counter
					valid_count++;
				}
			}
		} 

		//build the command string to send
		Com_sprintf( cmd, sizeof(cmd), "radar %i", valid_count );
		for( i = 0; i < g_maxclients.integer; i++ ) {
			//if weve got a valid entry then add the position to the command string
			if( g_playerOrigins[i].valid ) {
				strcat(cmd, va(" %i,", g_playerOrigins[i].clientNum));
				strcat(cmd, va("%i,",  g_playerOrigins[i].pl));
				strcat(cmd, va("%i,",  g_playerOrigins[i].plMax));
				strcat(cmd, va("%i,",  g_playerOrigins[i].team));
				strcat(cmd, va("%i,",  g_playerOrigins[i].properties));
				strcat(cmd, va("%i,",  (int)ceil(g_playerOrigins[i].pos[0])));
				strcat(cmd, va("%i,",  (int)ceil(g_playerOrigins[i].pos[1])));
				strcat(cmd, va("%i",   (int)ceil(g_playerOrigins[i].pos[2])));
			}
		}

		// broadcast the command seperately to only connected clients.
		
		// FIXME: Does this prevent overflows that otherwise have to
		//        wait for a time-out message from unconnected clients?
		// NOTE:  Yep, seems to fix it.
		for ( i = 0; i < g_maxclients.integer; i++ ) {
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED ) {
				continue;
			}

			if ( ent->inuse ) {
				trap_SendServerCommand( ent-g_entities, cmd );
			}
		}
    
		/*
		//finally broadcast the command
		trap_SendServerCommand( -1, cmd );
		*/
	}
}
