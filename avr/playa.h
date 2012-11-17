
#ifndef playa_h
#define playa_h

void vs_setup_local();
uint8_t ir_decode();
void env_check();
void ui();
void pwr_down();

void get_album_path();
uint8_t play_file();
uint8_t file_find_next();
uint8_t file_find_random();

#endif

