#include <stdio.h>
#include <stdlib.h>
#include <libdragon.h>

#define EMPTY 0
#define PLAYER_X 1
#define PLAYER_O 2
#define box_size 60
#define start_x 80
#define start_y 40

static resolution_t res = RESOLUTION_320x240;
static bitdepth_t bit = DEPTH_32_BPP;
static int board[3][3];
int current_player, winner, turns, row=0, col=0;

void draw_x(display_context_t disp, int center_x, int center_y, int size, uint32_t color) {
    // Draw the two diagonal lines of the X
    for (int i = -size / 2; i <= size / 2; i++) {
        // Draw the first diagonal from top-left to bottom-right
        graphics_draw_box(disp, center_x + i, center_y + i, 1, 1, color);
        
        // Draw the second diagonal from top-right to bottom-left
        graphics_draw_box(disp, center_x - i, center_y + i, 1, 1, color);
    }
}

void draw_circle(display_context_t disp, int center_x, int center_y, int radius, uint32_t color) {
	int r2=radius*radius, dif;
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
        	dif = r2 - (x*x+y*y);
            if (dif >= 0 && dif < 50) { // gives the circle some depth
                graphics_draw_box(disp, center_x + x, center_y + y, 1, 1, color);
            }
        }
    }
}

void draw_board(display_context_t disp) {
    uint32_t x_color = graphics_make_color(255, 0, 0, 255); // Color for X
    uint32_t o_color = graphics_make_color(0, 0, 255, 255); // Color for O (adjusted to a brighter blue)
    uint32_t empty_color = graphics_make_color(255, 255, 255, 255); // Color for empty

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int x = start_x + col * box_size;
            int y = start_y + row * box_size;

            // Draw the box outline
            graphics_draw_box(disp, x, y, box_size - 5, box_size - 5, empty_color);

            // Draw X or O based on the board value
            if (board[row][col] == PLAYER_X) {
                draw_x(disp, x + box_size / 2, y + box_size / 2, box_size - 15, x_color);
            } else if (board[row][col] == PLAYER_O) {
                // Draw the 'O' as a circle
                draw_circle(disp, x + box_size / 2, y + box_size / 2, (box_size / 2) - 10, o_color);
            }
        }
    }
}

void draw_cursor(display_context_t disp, int current_row, int current_col) {
    int x = start_x + current_col * box_size;
    int y = start_y + current_row * box_size;
    uint32_t cursor_color = graphics_make_color(0, 255, 0, 255); // Green for cursor
    int thickness = 5; // Thickness of the cursor lines

    // Draw the four sides of the cursor
    // Top side
    graphics_draw_box(disp, x, y, box_size, thickness, cursor_color);
    // Bottom side
    graphics_draw_box(disp, x, y + box_size - thickness, box_size, thickness, cursor_color);
    // Left side
    graphics_draw_box(disp, x, y, thickness, box_size, cursor_color);
    // Right side
    graphics_draw_box(disp, x + box_size - thickness, y, thickness, box_size, cursor_color);
}

int check_winner() {
    // Check rows, columns, and diagonals for a win
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != EMPTY)
            return board[i][0];
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != EMPTY)
            return board[0][i];
    }
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != EMPTY)
        return board[0][0];
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != EMPTY)
        return board[0][2];
    return 0; // No winner yet
}

void reset_board() {
	current_player = PLAYER_X;
	winner = 0;
	turns = 1;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = EMPTY;
        }
    }
}

#define CONTROL_STICK_THRESHOLD 30 // Threshold for stick movement
#define CONTROL_STICK_DEADZONE 5 // Small deadzone for joystick

void update_cursor_position() {
    controller_scan();
    struct controller_data keys = get_keys_down();
    struct controller_data keys_pressed = get_keys_held();

    if (keys.c[0].start && winner) {
        reset_board();
        winner = 0;
    }

    if (!winner) {
        // Set the move when 'A' is pressed
        if (keys.c[0].A) {
            if (board[row][col] == EMPTY) {
                board[row][col] = current_player;
                winner = check_winner();
                current_player = (current_player == PLAYER_X) ? PLAYER_O : PLAYER_X;
                if(!winner && turns++==9)
                	winner = 3;
            }
        }
    }
	int stick_x = keys.c[0].x;
	int stick_y = keys.c[0].y;

	if (stick_y > CONTROL_STICK_THRESHOLD) {
		row=row==0?2:row-1;
	} else if (stick_y < -CONTROL_STICK_THRESHOLD) {
		row=row==2?0:row+1;
	}
	if (stick_x > CONTROL_STICK_THRESHOLD) {
		col=col==2?0:col+1;
	} else if (stick_x < -CONTROL_STICK_THRESHOLD) {
		col=col==0?2:col-1;
	}
}

int main(void) {
    display_init(res, bit, 2, GAMMA_NONE, FILTERS_RESAMPLE);
    controller_init();
    reset_board();

    while (1) {
        static display_context_t disp = 0;
        disp = display_get();
        if (disp) {
            graphics_fill_screen(disp, graphics_make_color(0, 0, 0, 255)); // Black background
			update_cursor_position();
            draw_board(disp);
            draw_cursor(disp, row, col);
            if (winner) {
                char tStr[32];
                if(winner == 3)
                	sprintf(tStr, "It's a tie!");
                else
                	sprintf(tStr, "Player %d wins!", winner);
                graphics_draw_text(disp, 20, 20, tStr);
            } else {
                graphics_draw_text(disp, 20, 20, "Tic-Tac-Toe");
            }

            display_show(disp);
        }
    }
}

