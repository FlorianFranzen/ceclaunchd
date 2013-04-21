/*
 *  ceclaunchd.cpp
 * 
 *  Copyright (c) 2013 by Florian Franzen <Florian.Franzen@gmail.com>=
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
#include "log.h"


FILE*       GLOBAL_LOGFILE_FID = NULL;
log_level_t GLOBAL_LOG_LEVEL   = LOG_LEVEL_DEBUG;

void log_open(char* file) {
    if(GLOBAL_LOGFILE_FID == NULL) {
        GLOBAL_LOGFILE_FID = fopen(file, "a");
    }
}

void set_log_level(log_level_t level) {
    GLOBAL_LOG_LEVEL = level;
}

int log_enabled() {
    if(GLOBAL_LOGFILE_FID != NULL) {
        return 1;
    }
    return 0;
}

void log_close() {
    if(GLOBAL_LOGFILE_FID != NULL) {
        fclose(GLOBAL_LOGFILE_FID);
        GLOBAL_LOGFILE_FID = NULL;
    }
 }

void log_vprint(FILE* fid, char* fmt, va_list args) {
    // Print to log, if openend
    if(GLOBAL_LOGFILE_FID != NULL){
        va_list args_copy;
        va_copy(args_copy, args);
        vfprintf(GLOBAL_LOGFILE_FID, fmt, args_copy);
        va_end(args_copy);
    }
    // Print to supplied file id
    vfprintf(fid, fmt, args);
    fflush(fid);
}

void log_stdout(char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_vprint(stderr, fmt, args);
        va_end(args); 
}

void log_stderr(char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_vprint(stdout, fmt, args);
        va_end(args); 
}

int log_error(char* fmt, ...) {
    if(GLOBAL_LOG_LEVEL >= LOG_LEVEL_ERROR) {
        log_stderr("[ ERROR ] ");
        va_list args;
        va_start(args, fmt);
        log_vprint(stderr, fmt, args);
        va_end(args); 
        return 0;
    }
    return 1;
}

int log_warning(char* fmt, ...) {
    if(GLOBAL_LOG_LEVEL >= LOG_LEVEL_WARNING) {
        log_stderr("[ WARN  ] ");
        va_list args;
        va_start(args, fmt);
        log_vprint(stderr, fmt, args);
        va_end(args);
        return 0;
    }
    return 1;
}

int log_info(char* fmt, ...) {
    if(GLOBAL_LOG_LEVEL >= LOG_LEVEL_INFO) {
        log_stdout("[ INFO  ] ");
        va_list args;
        va_start(args, fmt);
        log_vprint(stdout, fmt, args);
        va_end(args);
        return 0;
    }
    return 1;
}

int log_debug(char* fmt, ...) {
    if(GLOBAL_LOG_LEVEL >= LOG_LEVEL_DEBUG) {
        log_stdout("[ DEBUG ] %d", GLOBAL_LOG_LEVEL);
        va_list args;
        va_start(args, fmt);
        log_vprint(stdout, fmt, args);
        va_end(args);
        return 0;
    }
    return 1;
}

