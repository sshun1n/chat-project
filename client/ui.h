#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include "../shared/protocol.h"

#define HISTORY_SIZE 100

extern int sock;
extern bool running;
extern int history_count;
extern int scroll_offset;

typedef struct {
    wchar_t username[MAX_USERNAME_LEN];
    wchar_t message[MAX_MSG_LEN];
    uint8_t color;
} ChatMessage;

typedef struct {
    const wchar_t *name;
    uint8_t code;
} ColorMapEntry;

extern WINDOW *win_history;
extern WINDOW *win_input;
extern ChatMessage history[HISTORY_SIZE];
extern const ColorMapEntry color_map[];
extern uint8_t current_color;
extern wchar_t username[MAX_USERNAME_LEN];

void init_ui();
void display_history();
void add_to_history(const wchar_t *username, const wchar_t *message, uint8_t color);
void show_help();
void input_loop();
void handle_command(const wchar_t* command);
void send_message(int sock, const wchar_t* msg);

#endif