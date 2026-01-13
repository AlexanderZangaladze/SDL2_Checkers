#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>

SDL_Window* win = NULL;
SDL_Renderer* ren = NULL;

int win_width = 640;
int win_height = 640;


void DeInit(int error)
{
    if (ren != NULL) SDL_DestroyRenderer(ren);
    if (win != NULL) SDL_DestroyWindow(win);
    SDL_Quit();
    exit(error);
}


void Init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        DeInit(1);
    }

    win = SDL_CreateWindow("Checkers", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_width, win_height, SDL_WINDOW_SHOWN);
    if (!win) {
        printf("Could not create window: %s\n", SDL_GetError());
        DeInit(1);
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        printf("Could not create renderer: %s\n", SDL_GetError());
        DeInit(1);
    }
}

SDL_Rect cell = { 0, 0, 80, 80 };


void draw_field()
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (((j + i) % 2) == 0)
            {

                cell = { 80 * j, i * 80, 80, 80 };
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                SDL_RenderFillRect(ren, &cell);
            }
            else
            {

                cell = { 80 * j, i * 80, 80, 80 };
                SDL_SetRenderDrawColor(ren, 100, 255, 255, 255);
                SDL_RenderFillRect(ren, &cell);
            }
        }
    }


    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    for (int i = 0; i <= 8; i++) {
        SDL_RenderDrawLine(ren, 0, 80 * i, 640, 80 * i);
        SDL_RenderDrawLine(ren, 80 * i, 0, 80 * i, 640);
    }
}


void drawCircle(SDL_Renderer* ren, int cx, int cy, int radius)
{
    for (int w = 0; w < radius * 2; w++)
    {
        for (int h = 0; h < radius * 2; h++)
        {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius))
            {
                SDL_RenderDrawPoint(ren, cx + dx, cy + dy);
            }
        }
    }
}


struct piece
{
    int row;
    int col;
    bool white;
    bool alive;
    SDL_Rect rect;
};


int selected_piece = -1;
bool dragging = false;
int offset_x = 0;
int offset_y = 0;

int logical_field[8][8] = { 0 };
bool white_turn = true;
bool piece_ate = false;

piece pieces[24];


void init_pieces()
{
    int index = 0;


    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 8; col++)
        {

            if ((row + col) % 2 == 1)
            {
                pieces[index].row = row;
                pieces[index].col = col;
                pieces[index].white = false;
                pieces[index].alive = true;
                pieces[index].rect = { col * 80, row * 80, 80, 80 };
                index++;
            }
        }
    }


    for (int row = 5; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            if ((row + col) % 2 == 1)
            {
                pieces[index].row = row;
                pieces[index].col = col;
                pieces[index].white = true;
                pieces[index].alive = true;
                pieces[index].rect = { col * 80, row * 80, 80, 80 };
                index++;
            }
        }
    }


}


void draw_pieces()
{
    for (int i = 0; i < 24; i++)
    {
        if (pieces[i].alive)
        {

            if (selected_piece != i) {
                pieces[i].rect.x = pieces[i].col * 80;
                pieces[i].rect.y = pieces[i].row * 80;
            }


            int center_x = pieces[i].rect.x + 40;
            int center_y = pieces[i].rect.y + 40;


            if (selected_piece == i && !dragging) {
                SDL_SetRenderDrawColor(ren, 255, 255, 0, 255);
                drawCircle(ren, center_x, center_y, 35);
            }


            if (pieces[i].white) {
                SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
            }
            else {
                SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
            }

            drawCircle(ren, center_x, center_y, 30);
        }
    }
}


void rebuild_logical_field()
{

    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            logical_field[i][j] = 0;


    for (int i = 0; i < 24; i++)
    {
        if (pieces[i].alive)
        {
            int code = pieces[i].white ? 1 : 2;
            logical_field[pieces[i].row][pieces[i].col] = code;
        }
    }
}


bool can_place_piece(int target_row, int target_col)
{

    if (target_row < 0 || target_row > 7 || target_col < 0 || target_col > 7) {

        return false;
    }


    if ((target_row + target_col) % 2 == 0) {

        return false;
    }


    if (logical_field[target_row][target_col] != 0) {

        return false;
    }


    return true;
}


bool is_valid_move(int piece_idx, int target_row, int target_col, bool& can_eat)
{
    can_eat = false;

    int orig_row = pieces[piece_idx].row;
    int orig_col = pieces[piece_idx].col;
    bool is_white = pieces[piece_idx].white;

    int row_diff = target_row - orig_row;
    int col_diff = abs(target_col - orig_col);


    int forward_dir = is_white ? -1 : 1;


    if (row_diff == forward_dir && col_diff == 1)
    {
        if (logical_field[target_row][target_col] == 0) {

            return true;
        }
    }


    if (abs(row_diff) == 2 && col_diff == 2)
    {

        if (logical_field[target_row][target_col] != 0) {

            return false;
        }


        int mid_row = (orig_row + target_row) / 2;
        int mid_col = (orig_col + target_col) / 2;


        int enemy_code = is_white ? 2 : 1;


        if (logical_field[mid_row][mid_col] == enemy_code) {

            can_eat = true;
            return true;
        }
        else {

            return false;
        }
    }


    return false;
}


void remove_piece_at(int row, int col)
{
    for (int i = 0; i < 24; i++)
    {
        if (pieces[i].row == row && pieces[i].col == col && pieces[i].alive)
        {
            pieces[i].alive = false;
            logical_field[row][col] = 0;

            return;
        }
    }
}

bool can_piece_hit(int piece_idx)
{
    if (piece_idx < 0 || piece_idx >= 24 || !pieces[piece_idx].alive)
        return false;

    int row = pieces[piece_idx].row;
    int col = pieces[piece_idx].col;
    bool is_white = pieces[piece_idx].white;
    int enemy_code = is_white ? 2 : 1;


    int directions[4][2] = {
        {-1, -1},
        {-1,  1},
        { 1, -1},
        { 1,  1}
    };

    for (int k = 0; k < 4; k++)
    {

        int enemy_row = row + directions[k][0];
        int enemy_col = col + directions[k][1];


        int land_row = row + directions[k][0] * 2;
        int land_col = col + directions[k][1] * 2;


        if (enemy_row < 0 || enemy_row > 7 || enemy_col < 0 || enemy_col > 7)
            continue;
        if (land_row < 0 || land_row > 7 || land_col < 0 || land_col > 7)
            continue;


        if (logical_field[enemy_row][enemy_col] == enemy_code &&
            logical_field[land_row][land_col] == 0)
        {


            return true;
        }
    }

    return false;
}


int main(int argc, char* argv[])
{
    Init();
    init_pieces();
    rebuild_logical_field();

    SDL_Event e;
    bool quit = false;



    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {

            if (e.type == SDL_QUIT) {
                quit = true;
            }


            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                int mouse_x = e.button.x;
                int mouse_y = e.button.y;
                int cell_x = mouse_x / 80;
                int cell_y = mouse_y / 80;


                for (int i = 0; i < 24; i++)
                {
                    if (pieces[i].col == cell_x && pieces[i].row == cell_y && pieces[i].alive)
                    {

                        if (pieces[i].white != white_turn) {


                            break;
                        }


                        selected_piece = i;
                        dragging = true;
                        offset_x = mouse_x - pieces[i].rect.x;
                        offset_y = mouse_y - pieces[i].rect.y;


                        break;
                    }
                }
            }


            if (e.type == SDL_MOUSEMOTION && dragging && selected_piece != -1)
            {
                int mouse_x = e.motion.x;
                int mouse_y = e.motion.y;

                pieces[selected_piece].rect.x = mouse_x - offset_x;
                pieces[selected_piece].rect.y = mouse_y - offset_y;
            }


            if (e.type == SDL_MOUSEBUTTONUP && selected_piece != -1)
            {

                int center_x = pieces[selected_piece].rect.x + 40;
                int center_y = pieces[selected_piece].rect.y + 40;


                int target_col = center_x / 80;
                int target_row = center_y / 80;


                if (target_col < 0) target_col = 0;
                if (target_col > 7) target_col = 7;
                if (target_row < 0) target_row = 0;
                if (target_row > 7) target_row = 7;

                piece_ate = false;
                bool valid_move = false;

                if (can_place_piece(target_row, target_col))
                {
                    bool can_eat = false;


                    if (is_valid_move(selected_piece, target_row, target_col, can_eat))
                    {

                        int orig_row = pieces[selected_piece].row;
                        int orig_col = pieces[selected_piece].col;


                        logical_field[orig_row][orig_col] = 0;


                        if (can_eat)
                        {
                            int mid_row = (orig_row + target_row) / 2;
                            int mid_col = (orig_col + target_col) / 2;
                            remove_piece_at(mid_row, mid_col);
                            piece_ate = true;
                        }


                        int code = pieces[selected_piece].white ? 1 : 2;
                        logical_field[target_row][target_col] = code;
                        pieces[selected_piece].col = target_col;
                        pieces[selected_piece].row = target_row;
                        pieces[selected_piece].rect.x = target_col * 80;
                        pieces[selected_piece].rect.y = target_row * 80;

                        valid_move = true;

                    }
                }


                if (!valid_move)
                {
                    pieces[selected_piece].rect.x = pieces[selected_piece].col * 80;
                    pieces[selected_piece].rect.y = pieces[selected_piece].row * 80;

                }
                else
                {

                    if (piece_ate && can_piece_hit(selected_piece))
                    {


                        dragging = false;
                        continue;
                    }
                    else
                    {

                        white_turn = !white_turn;

                    }
                }

                dragging = false;
                selected_piece = -1;
            }
        }


        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        draw_field();
        draw_pieces();

        SDL_RenderPresent(ren);
        SDL_Delay(1000 / 60);
    }

    DeInit(0);
    return 0;
}