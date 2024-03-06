#pragma once

#include <string>
#include <imgui.h>

#include "ImChess_Piece_Definitions.h"

struct chessboard_tile
{
    piece* piece{};
    ImColor color{};
    float active_tile_state{};
};

enum movement_direction : uint8_t
{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

struct player_instance
{
    bool is_local_player{};
    player_teams team{};
    movement_direction direction{};
    std::pair<uint8_t, uint8_t> move{};
    std::pair<uint8_t, uint8_t> spawn_point{};
    std::vector<chess_piece> pieces;

    void set_pieces(chessboard_instance* instance);
    void spawn_pieces(chessboard_instance* instance);

    void set_spawn_point()
    {
        switch (team)
        {
            case WHITE:
            case BLACK:
                spawn_point = is_local_player ? std::make_pair(7, 0) : std::make_pair(0, 0);
            break;
            case RED:
            {

            }
            break;
            case GREEN:
            {

            }
            case BLUE:
            {

            }
            break;
            case YELLOW:
            {

            }
            break;
        }
    }

    void set_direction()
    {
        switch (team)
        {
        case WHITE:
        case BLACK:
        {
            if (is_local_player)
                direction = UP;
            else
                direction = DOWN;
        }
        break;
        case RED:
            direction = UP;
            break;
        case GREEN:
            direction = LEFT;
            break;
        case BLUE:
            direction = DOWN;
            break;
        case YELLOW:
            direction = RIGHT;
            break;
        }
    }
};

class chessboard_instance
{
public:
    chessboard_instance(const bool free_move = false, int player_count = 2);

    void on_draw();
private:
    void on_move(chessboard_tile& tile, uint8_t row, uint8_t column);
    void check_king_tiles();
    void set_tile_color();
public:
    bool is_offline = { true };
    bool should_delete{};
    std::vector<player_instance> players;
    chessboard_tile board_tiles[static_cast<uint8_t>(8)][static_cast<uint8_t>(8)]{};
private:
    std::string board_name{};
    player_teams team_can_move{};
    player_teams localplayer_team{};
    std::optional<std::pair<uint8_t, uint8_t>> active_tile{};
    std::optional<std::pair<uint8_t, uint8_t>> hovered_tile{};
    std::vector<std::pair<piece_type, piece_type>> killfeed{};
    bool free_move{};
};

class window_instance
{
    chessboard_instance chessboard{};
};

namespace ImChess
{
    void render_window();
    void render_contents();
};
