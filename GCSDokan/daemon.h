#pragma once
#include <string>

/* 
* This function will start a new thread which
* is responsible to kill the process when a terminating
* signal is received.
*/
bool start_deamon();


// Kill a particular GCSDokan process given
// by the key
void kill_process();


//
bool list_instances();
