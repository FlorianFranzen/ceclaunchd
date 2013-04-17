/*
 *  ceclaunchd.cpp
 * 
 *  Copyright (c) 2013 by Florian Franzen <Florian.Franzen@gmail.com>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */

#include "unused.h"

#include <libcec/cecc.h> // ToDo: Switch to c library.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>

#define LOG_FILE  "/var/log/ceclaunchd.log"
#define PID_FILE  "/var/run/ceclaunchd.pid"
#define CONF_FILE "/etc/ceclaunchd.conf"

#define PATH_MAX     64 //ToDo: Fix me.
#define MAX_BUFFER   256
#define MAX_SECTION  128

int g_running = 1;

typedef struct {
    cec_user_control_code   key;
    char*                   command;
	unsigned int            duration;
	int 					blocker;
} mapping_t;

typedef struct {
        mapping_t**     list;   
        unsigned int    length;
} map_t;

typedef struct {
	char 		device[PATH_MAX];
	char 		logfile[PATH_MAX];
	char 		pidfile[PATH_MAX];
	int 		daemonize;	
	map_t* 		mappings;
	mapping_t*  last_detected;
} config_t;

typedef struct { 
	const char*				name; 
	cec_user_control_code 	code; 
} codename_t;

static codename_t g_codenames[] = {
	{ "SELECT", 					CEC_USER_CONTROL_CODE_SELECT },
	{ "UP", 						CEC_USER_CONTROL_CODE_UP },
	{ "DOWN", 						CEC_USER_CONTROL_CODE_DOWN },
	{ "LEFT", 						CEC_USER_CONTROL_CODE_LEFT },
	{ "RIGHT", 						CEC_USER_CONTROL_CODE_RIGHT },
	{ "RIGHT_UP", 					CEC_USER_CONTROL_CODE_RIGHT_UP },
	{ "RIGHT_DOWN",					CEC_USER_CONTROL_CODE_RIGHT_DOWN },
	{ "LEFT_UP", 					CEC_USER_CONTROL_CODE_LEFT_UP },
	{ "LEFT_DOWN", 					CEC_USER_CONTROL_CODE_LEFT_DOWN },
	{ "ROOT_MENU", 					CEC_USER_CONTROL_CODE_ROOT_MENU },
	{ "SETUP_MENU", 				CEC_USER_CONTROL_CODE_SETUP_MENU },
	{ "CONTENTS_MENU", 				CEC_USER_CONTROL_CODE_CONTENTS_MENU },
	{ "FAVORITE_MENU", 				CEC_USER_CONTROL_CODE_FAVORITE_MENU },
	{ "EXIT", 						CEC_USER_CONTROL_CODE_EXIT },
	{ "NUMBER0", 					CEC_USER_CONTROL_CODE_NUMBER0 },
	{ "NUMBER1", 					CEC_USER_CONTROL_CODE_NUMBER1 },
	{ "NUMBER2", 					CEC_USER_CONTROL_CODE_NUMBER2 },
	{ "NUMBER3", 					CEC_USER_CONTROL_CODE_NUMBER3 },
	{ "NUMBER4", 					CEC_USER_CONTROL_CODE_NUMBER4 },
	{ "NUMBER5", 					CEC_USER_CONTROL_CODE_NUMBER5 },
	{ "NUMBER6", 					CEC_USER_CONTROL_CODE_NUMBER6 },
	{ "NUMBER7", 					CEC_USER_CONTROL_CODE_NUMBER7 },
	{ "NUMBER8", 					CEC_USER_CONTROL_CODE_NUMBER8 },
	{ "NUMBER9", 					CEC_USER_CONTROL_CODE_NUMBER9 },
	{ "DOT", 						CEC_USER_CONTROL_CODE_DOT },
	{ "ENTER", 						CEC_USER_CONTROL_CODE_ENTER },
	{ "CLEAR", 						CEC_USER_CONTROL_CODE_CLEAR },
	{ "NEXT_FAVORITE", 				CEC_USER_CONTROL_CODE_NEXT_FAVORITE },
	{ "CHANNEL_UP", 				CEC_USER_CONTROL_CODE_CHANNEL_UP },
	{ "CHANNEL_DOWN", 				CEC_USER_CONTROL_CODE_CHANNEL_DOWN },
	{ "PREVIOUS_CHANNEL", 			CEC_USER_CONTROL_CODE_PREVIOUS_CHANNEL },
	{ "SOUND_SELECT", 				CEC_USER_CONTROL_CODE_SOUND_SELECT },
	{ "INPUT_SELECT", 				CEC_USER_CONTROL_CODE_INPUT_SELECT },
	{ "DISPLAY_INFORMATION",		CEC_USER_CONTROL_CODE_DISPLAY_INFORMATION },
	{ "HELP",						CEC_USER_CONTROL_CODE_HELP },
	{ "PAGE_UP", 					CEC_USER_CONTROL_CODE_PAGE_UP },
	{ "PAGE_DOWN", 					CEC_USER_CONTROL_CODE_PAGE_DOWN },
	{ "POWER", 						CEC_USER_CONTROL_CODE_POWER },
	{ "VOLUME_UP", 					CEC_USER_CONTROL_CODE_VOLUME_UP },
	{ "VOLUME_DOWN", 				CEC_USER_CONTROL_CODE_VOLUME_DOWN },
	{ "MUTE", 						CEC_USER_CONTROL_CODE_MUTE },
	{ "PLAY", 						CEC_USER_CONTROL_CODE_PLAY },
	{ "STOP", 						CEC_USER_CONTROL_CODE_STOP },
	{ "PAUSE", 						CEC_USER_CONTROL_CODE_PAUSE },
	{ "RECORD", 					CEC_USER_CONTROL_CODE_RECORD },
	{ "REWIND", 					CEC_USER_CONTROL_CODE_REWIND },
	{ "FAST_FORWARD", 				CEC_USER_CONTROL_CODE_FAST_FORWARD },
	{ "EJECT", 						CEC_USER_CONTROL_CODE_EJECT },
	{ "FORWARD", 					CEC_USER_CONTROL_CODE_FORWARD },
	{ "BACKWARD", 					CEC_USER_CONTROL_CODE_BACKWARD },
	{ "STOP_RECORD", 				CEC_USER_CONTROL_CODE_STOP_RECORD },
	{ "PAUSE_RECORD", 				CEC_USER_CONTROL_CODE_PAUSE_RECORD },
	{ "ANGLE", 						CEC_USER_CONTROL_CODE_ANGLE },
	{ "SUB_PICTURE", 				CEC_USER_CONTROL_CODE_SUB_PICTURE },
	{ "VIDEO_ON_DEMAND", 			CEC_USER_CONTROL_CODE_VIDEO_ON_DEMAND },
	{ "ELECTRONIC_PROGRAM_GUIDE",	CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE },
	{ "TIMER_PROGRAMMING", 			CEC_USER_CONTROL_CODE_TIMER_PROGRAMMING },
	{ "INITIAL_CONFIGURATION",		CEC_USER_CONTROL_CODE_INITIAL_CONFIGURATION },
	{ "PLAY_FUNCTION", 				CEC_USER_CONTROL_CODE_PLAY_FUNCTION },
	{ "PAUSE_PLAY_FUNCTION", 		CEC_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION },
	{ "RECORD_FUNCTION", 			CEC_USER_CONTROL_CODE_RECORD_FUNCTION },
	{ "PAUSE_RECORD_FUNCTION", 		CEC_USER_CONTROL_CODE_PAUSE_RECORD_FUNCTION },
	{ "STOP_FUNCTION", 				CEC_USER_CONTROL_CODE_STOP_FUNCTION },
	{ "MUTE_FUNCTION", 				CEC_USER_CONTROL_CODE_MUTE_FUNCTION },
	{ "RESTORE_VOLUME_FUNCTION", 	CEC_USER_CONTROL_CODE_RESTORE_VOLUME_FUNCTION },
	{ "TUNE_FUNCTION", 				CEC_USER_CONTROL_CODE_TUNE_FUNCTION },
	{ "SELECT_MEDIA_FUNCTION", 		CEC_USER_CONTROL_CODE_SELECT_MEDIA_FUNCTION },
	{ "SELECT_AV_INPUT_FUNCTION", 	CEC_USER_CONTROL_CODE_SELECT_AV_INPUT_FUNCTION },
	{ "SELECT_AUDIO_INPUT_FUNCTION",CEC_USER_CONTROL_CODE_SELECT_AUDIO_INPUT_FUNCTION },
	{ "POWER_TOGGLE_FUNCTION", 		CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION },
	{ "POWER_OFF_FUNCTION", 		CEC_USER_CONTROL_CODE_POWER_OFF_FUNCTION },
	{ "POWER_ON_FUNCTION", 			CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION },
	{ "F1_BLUE", 					CEC_USER_CONTROL_CODE_F1_BLUE },
	{ "F2_RED", 					CEC_USER_CONTROL_CODE_F2_RED },
	{ "F3_GREEN", 					CEC_USER_CONTROL_CODE_F3_GREEN },
	{ "F4_YELLOW", 					CEC_USER_CONTROL_CODE_F4_YELLOW },
	{ "F5", 						CEC_USER_CONTROL_CODE_F5 },
	{ "DATA", 						CEC_USER_CONTROL_CODE_DATA },
	{ "AN_RETURN", 					CEC_USER_CONTROL_CODE_AN_RETURN },
	{ "UNKNOWN", 					CEC_USER_CONTROL_CODE_UNKNOWN } 
};

cec_user_control_code parse_keyname(char* name) {
	int it;
	int max_it = sizeof(g_codenames) / sizeof(codename_t);
	for(it = 0; it < max_it; it++) {
	 	if (strcmp(g_codenames[it].name, name) == 0) {
            return g_codenames[it].code;
        }
	}
	return CEC_USER_CONTROL_CODE_UNKNOWN;
}

void add_mapping(config_t* config, mapping_t* mapping) {
	if(config->mappings == NULL) {
		map_t* map = (map_t*) malloc(sizeof(map_t));
		map->length = 1;
		map->list = (mapping_t**) malloc(sizeof(mapping_t*));
		map->list[0] = mapping;
		config->mappings = map;
	} else {
		config->mappings->length++;
		config->mappings->list = (mapping_t**) realloc(config->mappings->list, config->mappings->length * sizeof(mapping_t*));
		config->mappings->list[config->mappings->length - 1] = mapping;
	}
}

/* Convert a string to uppercase
 */
char* strtoupper(char *str)
{
	char *ptr = str;

	while(*ptr) {
		(*ptr) = toupper(*ptr);
		ptr++;
	}
	return str;
}

/* Trim whitespace and newlines from a string
 */
char* trim(char *str)
{
	char *pch = str;
	while(isspace(*pch)) {
		pch++;
	}
	if(pch != str) {
		memmove(str, pch, (strlen(pch) + 1));
	}
	
	pch = (char*)(str + (strlen(str) - 1));
	while(isspace(*pch)) {
		pch--;
	}
	*++pch = '\0';

	return str;
}


 /* 
	Return 1 if everything is alright!

	@param file is the config file path to load.
 */
int parse_config(config_t* config, char* file) {

	FILE* fid = fopen(file, "r");
	if(fid == NULL) {
		fprintf(stderr, "Could not open file (%s): %s\n", file, strerror(errno));
		return 0;
	}

	mapping_t* mapping;
	char buffer[MAX_BUFFER];
	unsigned int linenum = 0;
	char section[MAX_SECTION] = "";
	
	char* key   = NULL;
	char* value = NULL;

	while(fgets(buffer, MAX_BUFFER, fid)) {
		linenum++;
		trim(buffer);

		if(strlen(buffer) == 0 || buffer[0] == '#') {
			continue;
		}

		if(buffer[0] == '[' && buffer[strlen(buffer)-1] == ']') {
			// Parse new section
			strncpy(section, buffer + 1, sizeof(section));			
			section[strlen(section)-1] = '\0';


			printf("config: new section: '%s'\n", section);
			if(!strlen(section)) {
				fprintf(stderr, "config: line %d: bad section name\n", linenum);
				return 0;
			}
			if(strcmp(section, "options")) {
				// start a new mapping record
				mapping = malloc(sizeof(mapping_t));
				if(mapping == NULL) {
					fprintf(stderr, "Could not malloc: %s\n", strerror(errno));
					return 0;
				}
				mapping->key = parse_keyname(section);
				mapping->command = NULL;
				mapping->duration = 0;
				mapping->blocker  = 0;
				add_mapping(config, mapping);
			}
		} else {
			// Parse parameters
			if(!strlen(section)) {
				fprintf(stderr, "config: line %d: all directives must belong to a section\n", linenum);
				return 0;
			}
			value = buffer;
			key = strsep(&value, "=");
			if(key == NULL) {
				fprintf(stderr, "config: line %d: syntax error\n", linenum);
				return 0;
			}
			trim(key);
			key = strtoupper(key);
			if(value == NULL) {
				// Non value parameter keys
				fprintf(stderr, "config: line %d: syntax error\n", linenum);
				return 0;	
			} else {
				trim(value);
				if(!strcmp(section, "options")) {
					// Parse option parameter 
					if(!strcmp(key, "LOGFILE")) {
						strncpy(config->logfile, value, PATH_MAX-1);
						config->logfile[PATH_MAX-1] = '\0';
						printf("config: log file: %s\n", config->logfile);
					} else if(!strcmp(key, "PIDFILE")) {
						strncpy(config->pidfile, value, PATH_MAX-1);
						config->pidfile[PATH_MAX-1] = '\0';
						printf("config: pid file: %s\n", config->pidfile);
					} else if(!strcmp(key, "DEVICE")) {
						strncpy(config->device, value, PATH_MAX-1);
						config->device[PATH_MAX-1] = '\0';
						printf("config: device: %s\n", config->device);
					} else {
						fprintf(stderr, "config: line %d: syntax error\n", linenum);
						return 0;
					}
				} else {
					// Parse mapping parameter 
					if(!strcmp(key, "COMMAND")) {
						mapping->command = malloc((strlen(value) + 1) * sizeof(char));
						if(mapping->command == NULL) {
							fprintf(stderr, "Could not malloc: %s\n", strerror(errno));
							return 0;
						}
						strcpy(mapping->command, value);
						printf("config: %s: command: %s\n", section, mapping->command);
					} else if(!strcmp(key, "DURATION")) {
						mapping->duration = atoi(value);
						printf("config: %s: duration: %d\n", section, mapping->duration);
					} else if(!strcmp(key, "BLOCKER")) {
						mapping->blocker = atoi(value);
						printf("config: %s: blocker: %d\n", section, mapping->blocker);
					}else {
						fprintf(stderr, "config: line %d: syntax error\n", linenum);
						return 0;
					}
				} // end of mapping value parsing
			} // enf of value parsing
		} // end of section/parameter parsing
		buffer[0] = '\0'; // ToDo: Do I need this?
	} // end of while
	fclose(fid);
	return 1;
}

int alert_callback(void *UNUSED(cbParam), const libcec_alert type, const libcec_parameter UNUSED(param)) {
	switch (type) {
  		case CEC_ALERT_CONNECTION_LOST:
			fprintf(stderr, "Connection to device lost.\n");
    		g_running = 0;
  		break;
  		default:
			fprintf(stderr, "alert: %d received.\n", type);
  			return 0;
  		break;
  }
  
  return 1;
}

void signal_handler(int sigNum) { 
	g_running = 0;
}

/*
 * @return 1 when ok, 0 otherwise.
 */
int key_callback(void* ptr, const cec_keypress key) {
	config_t* config = (config_t*) ptr;

	if(config->last_detected != NULL) {
		fprintf(stderr, "Key press is ignored, command is still processed ('%s').\n", config->last_detected->command);
		return 0;
	}

	if(config->mappings != NULL) {
		unsigned int i;
		for(i = 0; i < config->mappings->length; i++) {
			if(key.keycode == config->mappings->list[i]->key && key.duration > 0) {
				if(config->mappings->list[i]->duration <= key.duration) {
					config->last_detected = config->mappings->list[i];
				 	return 1;
				}
			}

		}
	}
	return 0;
}

int log_callback(void *UNUSED(cbParam), const cec_log_message message) {
	printf("Log received: %s\n", message.message);
	return 0;
}


typedef void (CEC_CDECL* CBCecSourceActivatedType)(void*, const cec_logical_address, const uint8_t);

int main (int argc, char *argv[]) {
	// Init ceclaunchd config
	config_t config;

	memset(config.device, 0, PATH_MAX);
	strncpy(config.logfile, LOG_FILE, sizeof(config.logfile));
	strncpy(config.pidfile, PID_FILE, sizeof(config.pidfile));
	config.daemonize = 0;	
	config.mappings  = NULL;
	config.last_detected = NULL;

	if(!parse_config(&config, CONF_FILE)) {
		fprintf(stderr, "Could not parse config at %s\n", CONF_FILE);
		return 1;
	}

	if(argc > 0 && strcmp(argv[0], "-d")) {
		FILE *pid_fid;
		if(daemon(0, 0) < 0) {
			fprintf(stderr, "Could not deamonize: %s\n", strerror(errno));
			cec_close();
			cec_destroy();
			return 1;
		}
		/* write our PID to the pidfile*/
		if((pid_fid = fopen(config.pidfile, "w"))) {
			fprintf(pid_fid, "%d\n", getpid());
			fclose(pid_fid);
		} else {
			fprintf(stderr, "Could not create pid file %s: %s\n", config.pidfile, strerror(errno));
		}
	}

	// Init libcec callbacks
    ICECCallbacks cec_callbacks;  
	
	cec_callbacks.CBCecLogMessage           = NULL; //&log_callback;
    cec_callbacks.CBCecKeyPress             = &key_callback;
    cec_callbacks.CBCecCommand              = NULL;
    cec_callbacks.CBCecConfigurationChanged = NULL;
    cec_callbacks.CBCecAlert                = &alert_callback;
    cec_callbacks.CBCecMenuStateChanged     = NULL;
    cec_callbacks.CBCecSourceActivated      = NULL;

 	// Init libcec config
	libcec_configuration cec_config;
	
	cec_config.iPhysicalAddress =                CEC_PHYSICAL_ADDRESS_TV;
    cec_config.baseDevice = (cec_logical_address)CEC_DEFAULT_BASE_DEVICE;
    cec_config.iHDMIPort =                       CEC_DEFAULT_HDMI_PORT;
    cec_config.tvVendor =              (uint64_t)CEC_VENDOR_UNKNOWN;
    cec_config.clientVersion =         (uint32_t)CEC_CLIENT_VERSION_CURRENT;
    cec_config.serverVersion =         (uint32_t)CEC_SERVER_VERSION_CURRENT;
    cec_config.bAutodetectAddress =              0;
    cec_config.bGetSettingsFromROM =             CEC_DEFAULT_SETTING_GET_SETTINGS_FROM_ROM;
    cec_config.bUseTVMenuLanguage =              CEC_DEFAULT_SETTING_USE_TV_MENU_LANGUAGE;
    cec_config.bActivateSource =                 0;
    cec_config.bPowerOffScreensaver =            CEC_DEFAULT_SETTING_POWER_OFF_SCREENSAVER;
    cec_config.bPowerOnScreensaver =             CEC_DEFAULT_SETTING_POWER_ON_SCREENSAVER;
    cec_config.bPowerOffOnStandby =              CEC_DEFAULT_SETTING_POWER_OFF_ON_STANDBY;
    cec_config.bShutdownOnStandby =              CEC_DEFAULT_SETTING_SHUTDOWN_ON_STANDBY;
    cec_config.bSendInactiveSource =             CEC_DEFAULT_SETTING_SEND_INACTIVE_SOURCE;
    cec_config.iFirmwareVersion =                CEC_FW_VERSION_UNKNOWN;
    cec_config.bPowerOffDevicesOnStandby =       CEC_DEFAULT_SETTING_POWER_OFF_DEVICES_STANDBY;

	strncpy(cec_config.strDeviceName, "ceclaunchd", sizeof(cec_config.strDeviceName));

    cec_config.iFirmwareBuildDate =              CEC_FW_BUILD_UNKNOWN;
    cec_config.bMonitorOnly =                    0;
    cec_config.cecVersion =         (cec_version)CEC_DEFAULT_SETTING_CEC_VERSION;
    cec_config.adapterType =                     ADAPTERTYPE_UNKNOWN;
    cec_config.iDoubleTapTimeoutMs =             CEC_DOUBLE_TAP_TIMEOUT_MS;
    cec_config.comboKey =                        CEC_USER_CONTROL_CODE_STOP;
    cec_config.iComboKeyTimeoutMs =              0; // Disable combo key 
    
    //cec_config.deviceTypes.types[0] = CEC_DEVICE_TYPE_PLAYBACK_DEVICE;
    cec_config.deviceTypes.types[0] = CEC_DEVICE_TYPE_RECORDING_DEVICE;
	for (unsigned int i = 1; i < 5; i++) {
     	cec_config.deviceTypes.types[i] = CEC_DEVICE_TYPE_RESERVED;
    }

   	cec_config.logicalAddresses.primary = CECDEVICE_UNREGISTERED;
    for (int i = 0; i < 16; i++) {
     	cec_config.logicalAddresses.addresses[i] = 0;
     }

    cec_config.wakeDevices.primary = CECDEVICE_TV;
    for (int i = 0; i < 16; i++) {
    	cec_config.wakeDevices.addresses[i] = 0;
    }
    cec_config.wakeDevices.addresses[CECDEVICE_TV] = 1;

 	cec_config.powerOffDevices.primary = CECDEVICE_BROADCAST;
    for (int i = 0; i < 16; i++){
    	cec_config.powerOffDevices.addresses[i] = 0;
    }
    cec_config.powerOffDevices.addresses[CECDEVICE_BROADCAST] = 1;

    cec_config.callbackParam = &config;
    cec_config.callbacks     = &cec_callbacks;

    //cec_enable_callbacks((void*) &config, cks *callbacks)

    // Initialise libcec
 	if(!cec_initialise(&cec_config)){
		    fprintf(stderr, "Cannot initialise libcec.");
			cec_destroy();
	      	return 1;
	}

	cec_init_video_standalone();
	
    // Run autodectect
    if(!strlen(config.device)) {
    	cec_adapter_descriptor device_list[1];		
	    if (cec_detect_adapters(device_list, 1, NULL, 1) < 1) {
		    fprintf(stderr, "Autodetected failed.");
			cec_destroy();
	      	return 1;
	    }
		strncpy(config.device, device_list[0].strComName, PATH_MAX-1);
		config.device[PATH_MAX-1] = '\0';
	    printf("Autodetected %s (%s).\n", device_list[0].strComName, device_list[0].strComPath);
    }

	if (!cec_open(config.device, 1000)) {
		fprintf(stderr, "Could not open adapter %s.\n", config.device);
		cec_close();
		cec_destroy();
		return 1;
	}

	if(signal(SIGINT,  &signal_handler) == SIG_ERR || signal(SIGTERM,  &signal_handler) == SIG_ERR) {
		fprintf(stderr, "Could not register signal handler: %s\n", strerror(errno));
		return 1;
	}

    printf("Finished init.\n");

	while(g_running) {
		if(config.last_detected != NULL) {
			printf("Running '%s'.\n", config.last_detected->command);
			if(config.last_detected->blocker) {
				printf("Closing CEC device.\n");			
				cec_close();
				
				system(config.last_detected->command);
				
				if (!cec_open(config.device, 1000)) {
					fprintf(stderr, "Could not reopen adapter %s. Set blocker property for processes using libcec.\n", config.device);
					g_running = 0;
				}
			} else {
				pid_t pID = fork();
				if (pID == 0) { //child
					exit(system(config.last_detected->command)); 
				} else if (pID < 0) {
					fprintf(stderr, "Could not fork: %s\n", strerror(errno));
					g_running = 0;
				}        
			}
			config.last_detected = NULL;
		}
		sleep(1);
	}
    printf("Shutting down.\n");

	cec_close();
	cec_destroy();

	return 0;
}
