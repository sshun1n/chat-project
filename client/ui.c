#include "ui.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <ctype.h>

#define HISTORY_SIZE 100
#define INPUT_HEIGHT 3

WINDOW *win_history;
WINDOW *win_input;

ChatMessage history[HISTORY_SIZE];

int history_count = 0;

const ColorMapEntry color_map[] = {
    {L"black", NC_BLACK},
    {L"red", NC_RED},
    {L"green", NC_GREEN},
    {L"yellow", NC_YELLOW},
    {L"blue", NC_BLUE},
    {L"magenta", NC_MAGENTA},
    {L"cyan", NC_CYAN},
    {L"white", NC_WHITE}
};

void init_colors() {
    start_color();
    use_default_colors();
    init_pair(1, COLOR_WHITE, -1);
    for (int i = 1; i < 8; i++) {
        init_pair(i + 1, i, -1);
    }
}

void display_history() {
    werase(win_history);
    int max_y, max_x;
    getmaxyx(win_history, max_y, max_x);
    (void)max_x;
    
    int start_idx = history_count > max_y ? history_count - max_y : 0;
    
    for (int i = start_idx; i < history_count; i++) {
        ChatMessage *msg = &history[i % HISTORY_SIZE];
        
        wattron(win_history, COLOR_PAIR(msg->color + 1));
        waddwstr(win_history, msg->username);
        waddwstr(win_history, L": ");
        
        wattron(win_history, COLOR_PAIR(1));
        waddwstr(win_history, msg->message);
        waddwstr(win_history, L"\n");
    }
    wrefresh(win_history);
}

void add_to_history(const wchar_t *username, const wchar_t *message, uint8_t color) {
    ChatMessage *msg = &history[history_count % HISTORY_SIZE];
    wcsncpy(msg->username, username, MAX_USERNAME_LEN);
    wcsncpy(msg->message, message, MAX_MSG_LEN);
    msg->color = color;
    
    if (history_count < HISTORY_SIZE) history_count++;
}

void init_ui() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    init_colors();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    win_history = newwin(max_y - INPUT_HEIGHT, max_x, 0, 0);
    win_input = newwin(INPUT_HEIGHT, max_x, max_y - INPUT_HEIGHT, 0);
    
    keypad(win_input, TRUE);
    set_escdelay(25);
}

void show_help() {
    WINDOW *win_help = newwin(10, 40, LINES/2-5, COLS/2-20);
    box(win_help, 0, 0);
    mvwprintw(win_help, 1, 2, "Available commands:");
    mvwprintw(win_help, 2, 2, "/color <name> - Change color");
    mvwprintw(win_help, 3, 2, "Available colors:");
    mvwprintw(win_help, 4, 2, "black, red, green, yellow");
    mvwprintw(win_help, 5, 2, "blue, magenta, cyan, white");
    mvwprintw(win_help, 6, 2, "/name <new_name> - Change name");
    mvwprintw(win_help, 7, 2, "/help - Show this help");
    mvwprintw(win_help, 8, 2, "/exit - Quit chat");
    wrefresh(win_help);
    
    wgetch(win_help);
    
    delwin(win_help);
}

void handle_command(const wchar_t* command) {
    wchar_t cmd[20], args[MAX_MSG_LEN] = {0};
    swscanf(command, L"%19ls %l[^\n]", cmd, args);

    if (wcscmp(cmd, L"/color") == 0) {
        for (size_t i = 0; i < sizeof(color_map)/sizeof(color_map[0]); i++) {
            if (wcscasecmp(args, color_map[i].name) == 0) {
                current_color = color_map[i].code;
                add_to_history(L"System", L"Color changed!", NC_CYAN);
                display_history();
                return;
            }
        }
        add_to_history(L"System", L"Invalid color! Use /help", NC_RED);
    }
    else if (wcscmp(cmd, L"/name") == 0) {
        if (wcslen(args) > 0) {
            wcsncpy(username, args, MAX_USERNAME_LEN-1);
            username[MAX_USERNAME_LEN-1] = L'\0';
            add_to_history(L"System", L"Username changed!", NC_CYAN);
        } else {
            add_to_history(L"System", L"Specify new name!", NC_RED);
        }
    }
    else if (wcscmp(cmd, L"/help") == 0) {
        show_help();
    }
    else if (wcscmp(cmd, L"/exit") == 0) {
        running = false;
    }
    else {
        add_to_history(L"System", L"Unknown command", NC_RED);
    }
    
    display_history();
}

void input_loop() {
    wchar_t input_buffer[MAX_MSG_LEN] = {0};
    int cursor_pos = 0;
    
    while (running) {
        werase(win_input);
        box(win_input, 0, 0);
        mvwaddwstr(win_input, 1, 1, L"> ");
        mvwaddwstr(win_input, 1, 3, input_buffer);
        wmove(win_input, 1, 3 + cursor_pos);
        wrefresh(win_input);

        wint_t ch;
        int ret = wget_wch(win_input, &ch);

        switch (ret) {
            case KEY_CODE_YES:
                switch (ch) {
                    case KEY_HOME:
                        show_help();
                        break;
                    case KEY_BACKSPACE:
                        if (cursor_pos > 0) {
                            wmemmove(&input_buffer[cursor_pos-1], 
                                    &input_buffer[cursor_pos], 
                                    wcslen(&input_buffer[cursor_pos]) + 1);
                            cursor_pos--;
                        }
                        break;
                }
                break;

            case OK:
                switch (ch) {
                    case 10: // Enter
                        if (wcslen(input_buffer)) {
                            if (input_buffer[0] == L'/') {
                                handle_command(input_buffer);
                            } else {
                                send_message(sock, input_buffer);
                                add_to_history(username, input_buffer, current_color);
                            }
                            wmemset(input_buffer, 0, MAX_MSG_LEN);
                            cursor_pos = 0;
                            display_history();
                        }
                        break;

                    case 127: // Backspace
                        if (cursor_pos > 0) {
                            wmemmove(&input_buffer[cursor_pos-1], 
                                    &input_buffer[cursor_pos], 
                                    wcslen(&input_buffer[cursor_pos]) + 1);
                            cursor_pos--;
                        }
                        break;

                    default:
                        if (cursor_pos < MAX_MSG_LEN - 1) {
                            input_buffer[cursor_pos++] = ch;
                            input_buffer[cursor_pos] = L'\0';
                        }
                        break;
                }
                break;
        }
    }
}