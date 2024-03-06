#include "ImChess_Piece_Definitions.h"
#include "ImChess.h"

void piece::set_targetable_points()
{

}

void piece::check_checks()
{

}

void pawn_piece::check_checks()
{
    if (is_pinned.has_value())
    {
        printf("pawn is pinned %d %d from %d %d\n", position.first, position.second, is_pinned.value().first, is_pinned.value().second);

        for (int i = 0; i < tile_moves.size(); i++)
        {
            if (tile_moves[i].type != ATTACK || tile_moves[i].tile != is_pinned.value())
            {
                printf("%d %d\n", tile_moves[i].tile.first, tile_moves[i].tile.second);
                tile_moves.erase(tile_moves.begin() + i);
            }
        }
    }
}

constexpr bool pawn_piece::can_target_point(int row, int column, bool attack_move, bool is_extended_move)
{
    if (!(row >= 0 && row <= 7 && column >= 0 && column <= 7))
        return false;

    auto& tile = chessboard->board_tiles[static_cast<uint8_t>(row)][static_cast<uint8_t>(column)];

    if (tile.piece)
    {
        if (!attack_move)
            return false;

        if (team == tile.piece->team)
        {
            tile_moves.push_back({ COVER, std::make_pair(row, column) });
            return false;
        }

        if (tile.piece->team != team && tile.piece->type == KING)
        {
            checks_enemy_king = true;

            auto* king = static_cast<king_piece*>(tile.piece);
            king->checks.push_back({ std::make_pair(row, column) });

            return false;
        }

        tile_moves.push_back({ ATTACK, std::make_pair(row, column) });
        return true;
    }
    else
    {
        if (is_extended_move)
            tile_moves.push_back({ EXTENDED, std::make_pair(row, column) });
        else if (attack_move)
            tile_moves.push_back({ COVER, std::make_pair(row, column) });
        else
            tile_moves.push_back({ MOVE, std::make_pair(row, column) });
        return true;
    }
}

void pawn_piece::set_targetable_points()
{
    tile_moves.clear();
    checks_enemy_king = false;

    if (!is_alive)
        return;

    const int row = position.first;
    const int column = position.second;

    can_target_point(row + (player->direction == UP ? -1 : 1), column - 1, true);
    can_target_point(row + (player->direction == UP ? -1 : 1), column + 1, true);

    for (uint8_t i = 1; i < 3; i++)
    {
        if (can_target_point(row + (player->direction == UP ? -i : i), column, false, i == 2) == false || did_move_since_spawn)
            break;
    }
}

constexpr void knight_piece::can_target_point(int row, int column)
{
    if (row >= 0 && row <= 7 && column >= 0 && column <= 7)
    {
        auto& tile = chessboard->board_tiles[static_cast<uint8_t>(row)][static_cast<uint8_t>(column)];

        if (tile.piece)
        {
            if (tile.piece->team != team && tile.piece->type == KING)
            {
                checks_enemy_king = true;

                auto* king = static_cast<king_piece*>(tile.piece);
                king->checks.push_back({ std::make_pair(row, column) });

                return;
            }

            if (team == tile.piece->team)
            {
                tile_moves.push_back({ COVER,  std::make_pair(row, column) });
                return;
            }

            tile_moves.push_back({ ATTACK,  std::make_pair(row, column) });
        }
        else
            tile_moves.push_back({ ATTACK,  std::make_pair(row, column) });
    }
}

void knight_piece::set_targetable_points()
{
    tile_moves.clear();
    checks_enemy_king = false;

    if (!is_alive)
        return;

    const int row = position.first;
    const int column = position.second;

    can_target_point(row + 2, column - 1);
    can_target_point(row + 2, column + 1);

    can_target_point(row - 2, column - 1);
    can_target_point(row - 2, column + 1);

    can_target_point(row - 1, column - 2);
    can_target_point(row + 1, column - 2);

    can_target_point(row - 1, column + 2);
    can_target_point(row + 1, column + 2);
}

constexpr bool bishop_piece::can_target_point_ex(int row, int new_row, int column, int new_column)
{
    if (!(new_row >= 0 && new_row <= 7 && new_column >= 0 && new_column <= 7))
        return false;

    auto& tile = chessboard->board_tiles[static_cast<uint8_t>(new_row)][static_cast<uint8_t>(new_column)];

    if (tile.piece && !pinned_piece.has_value())
    {
        if (tile.piece->team != team && tile.piece->type == KING) // checks enemy king.
        {
            if (pinned_piece.has_value())
            {
                pinned_piece.value()->is_pinned = position;
                return false;
            }

            checks_enemy_king = true;

            auto* king = static_cast<king_piece*>(tile.piece);
            const std::pair<int, int> tile_difference = std::make_pair(row - new_row, column - new_column);

            std::vector<std::pair<uint8_t, uint8_t>> check_fields;
            check_fields.push_back(std::make_pair(row, column));

            auto _row = row;
            auto _column = column;

            for (int i = 0; i < abs(tile_difference.first); i++)
            {
                _row -= (tile_difference.first > 0 ? 1 : -1);
                _column -= (tile_difference.second > 0 ? 1 : -1);

                check_fields.push_back(std::make_pair(_row, _column));
            }

            king->checks.push_back(check_fields);

            return true;
        }

        if (team != tile.piece->team)
        {
            if (pinned_piece.has_value())
                return false;

            pinned_piece = tile.piece;
            tile_moves.push_back({ ATTACK, std::make_pair(new_row, new_column) });
            return true;
        }
        else
            tile_moves.push_back({ COVER, std::make_pair(new_row, new_column) });

        return false;
    }

    if (pinned_piece.has_value())
        tile_moves.push_back({ PIN_TRACE, std::make_pair(new_row, new_column) });
    else
        tile_moves.push_back({ ATTACK, std::make_pair(new_row, new_column) });

    return true;
}

constexpr void bishop_piece::can_target_point(int row, int column, uint8_t direction)
{
    switch (direction)
    {
        case 0: // top right
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row + i;
                const int next_column = column + i;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
        case 1: // bottom right
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row - i;
                const int next_column = column + i;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
        case 2: // bottom left
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row - i;
                const int next_column = column - i;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
        case 3: // top left
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row + i;
                const int next_column = column - i;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
    }
}

void bishop_piece::set_targetable_points()
{
    tile_moves.clear();
    checks_enemy_king = false;
    pinned_piece = {};

    if (!is_alive)
        return;

    const int row = position.first;
    const int column = position.second;

    can_target_point(row, column, 0);
    pinned_piece = {};
    can_target_point(row, column, 1);
    pinned_piece = {};
    can_target_point(row, column, 2);
    pinned_piece = {};
    can_target_point(row, column, 3);
}

constexpr bool rook_piece::can_target_point_ex(int row, int new_row, int column, int new_column)
{
    if (!(new_row >= 0 && new_row <= 7 && new_column >= 0 && new_column <= 7))
        return false;

    auto& tile = chessboard->board_tiles[static_cast<uint8_t>(new_row)][static_cast<uint8_t>(new_column)];

    if (tile.piece && !pinned_piece.has_value())
    {
        if (tile.piece->team != team && tile.piece->type == KING) // checks enemy king.
        {
            if (pinned_piece.has_value())
            {
                pinned_piece.value()->is_pinned = position;
                return false;
            }

            checks_enemy_king = true;

            auto* king = static_cast<king_piece*>(tile.piece);
            const std::pair<int, int> tile_difference = std::make_pair(row - new_row, column - new_column);

            std::vector<std::pair<uint8_t, uint8_t>> check_fields;
            check_fields.push_back(std::make_pair(row, column));

            auto _row = row;
            auto _column = column;

            for (int i = 0; i < abs(tile_difference.first); i++)
            {
                _row -= (tile_difference.first > 0 ? 1 : -1);
                _column -= (tile_difference.second > 0 ? 1 : -1);

                check_fields.push_back(std::make_pair(_row, _column));
            }

            king->checks.push_back(check_fields);

            return true;
        }

        if (team != tile.piece->team)
        {
            if (pinned_piece.has_value())
                return false;

            pinned_piece = tile.piece;
            tile_moves.push_back({ ATTACK, std::make_pair(new_row, new_column) });
            return true;
        }
        else
            tile_moves.push_back({ COVER, std::make_pair(new_row, new_column) });

        return false;
    }

    if (pinned_piece.has_value())
        tile_moves.push_back({ PIN_TRACE, std::make_pair(new_row, new_column) });
    else
        tile_moves.push_back({ ATTACK, std::make_pair(new_row, new_column) });

    return true;
}

constexpr void rook_piece::can_target_point(int row, int column, uint8_t direction)
{
    switch (direction)
    {
    case 0: // top
    {
        for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
        {
            const int next_row = row + i;
            const int next_column = column;

            if (!can_target_point_ex(row, next_row, column, next_column))
                break;
        }
    }
    break;
    case 1: // bottom
    {
        for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
        {
            const int next_row = row - i;
            const int next_column = column;

            if (!can_target_point_ex(row, next_row, column, next_column))
                break;
        }
    }
    break;
    case 2: // left
    {
        for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
        {
            const int next_row = row;
            const int next_column = column - i;

            if (!can_target_point_ex(row, next_row, column, next_column))
                break;
        }
    }
    break;
    case 3: // right
    {
        for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
        {
            const int next_row = row;
            const int next_column = column + i;

            if (!can_target_point_ex(row, next_row, column, next_column))
                break;
        }
    }
    break;
    }
}

void rook_piece::set_targetable_points()
{
    tile_moves.clear();
    checks_enemy_king = false;
    pinned_piece = {};

    if (!is_alive)
        return;

    const int row = position.first;
    const int column = position.second;

    can_target_point(row, column, 0);
    pinned_piece = {};
    can_target_point(row, column, 1);
    pinned_piece = {};
    can_target_point(row, column, 2);
    pinned_piece = {};
    can_target_point(row, column, 3);
}

constexpr bool queen_piece::can_target_point_ex(int row, int new_row, int column, int new_column)
{
    if (!(new_row >= 0 && new_row <= 7 && new_column >= 0 && new_column <= 7))
        return false;

    auto& tile = chessboard->board_tiles[static_cast<uint8_t>(new_row)][static_cast<uint8_t>(new_column)];

    if (tile.piece && !pinned_piece.has_value())
    {
        if (tile.piece->team != team && tile.piece->type == KING) // checks enemy king.
        {
            if (pinned_piece.has_value())
            {
                pinned_piece.value()->is_pinned = position;
                return false;
            }

            checks_enemy_king = true;

            auto* king = static_cast<king_piece*>(tile.piece);
            const std::pair<int, int> tile_difference = std::make_pair(row - new_row, column - new_column);

            std::vector<std::pair<uint8_t, uint8_t>> check_fields;
            check_fields.push_back(std::make_pair(row, column));

            auto _row = row;
            auto _column = column;

            for (int i = 0; i < abs(tile_difference.first); i++)
            {
                _row -= (tile_difference.first > 0 ? 1 : -1);
                _column -= (tile_difference.second > 0 ? 1 : -1);

                check_fields.push_back(std::make_pair(_row, _column));
            }

            king->checks.push_back(check_fields);

            return true;
        }

        if (team != tile.piece->team)
        {
            if (pinned_piece.has_value())
                return false;

            pinned_piece = tile.piece;
            tile_moves.push_back({ ATTACK, std::make_pair(new_row, new_column) });
            return true;
        }
        else
            tile_moves.push_back({ COVER, std::make_pair(new_row, new_column) });

        return false;
    }

    if (pinned_piece.has_value())
        tile_moves.push_back({ PIN_TRACE, std::make_pair(new_row, new_column) });
    else
        tile_moves.push_back({ ATTACK, std::make_pair(new_row, new_column) });

    return true;
}

constexpr void queen_piece::can_target_point(int row, int column, uint8_t direction)
{
    switch (direction)
    {
        case 0: // top
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row + i;
                const int next_column = column;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
        case 1: // top right
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row + i;
                const int next_column = column + i;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
        case 2: // right
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row;
                const int next_column = column + i;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
        case 3: // bottom right
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row - i;
                const int next_column = column + i;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
        case 4: // bottom
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row - i;
                const int next_column = column;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
        case 5: // bottom left
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row - i;
                const int next_column = column - i;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
        case 6: // left
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row;
                const int next_column = column - i;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
        case 7: // top left
        {
            for (uint8_t i = 1; i <= static_cast<uint8_t>(7); i++)
            {
                const int next_row = row + i;
                const int next_column = column - i;

                if (!can_target_point_ex(row, next_row, column, next_column))
                    break;
            }
        }
        break;
    }
}

void queen_piece::set_targetable_points()
{
    tile_moves.clear();
    checks_enemy_king = false;
    pinned_piece = {};

    if (!is_alive)
        return;

    const int row = position.first;
    const int column = position.second;

    for (uint8_t i = 0; i <= static_cast<uint8_t>(7); i++)
    {
        can_target_point(row, column, i);
        pinned_piece = {};
    }
}

void king_piece::check_checks()
{
    printf("checks %d\n", checks.size());

    if (checks.size())
        is_checked = true;
    else
        return;

    int possible_moves = 0;

    std::vector<std::pair<piece*, move_type>> blocks;

    for (int i = 0; i < player->pieces.size() -1; i++)
    {
        auto* chess_piece = reinterpret_cast<piece*>(&player->pieces[i]);

        std::vector<std::pair<uint8_t, uint8_t>> piece_blocks;

        if (chess_piece->tile_moves.size())
        {
            for (auto& piece_check : checks)
            {
                for (auto& check_tile : piece_check)
                {
                    if (piece_blocks.size())
                    {
                        for (int i = 0; i < piece_blocks.size(); i++)
                        {
                            if (piece_blocks[i] != check_tile)
                                piece_blocks.erase(piece_blocks.begin() + i);
                        }
                        continue;
                    }
                }

                for (auto& check_tile : piece_check)
                {
                    for (int e = 0; e < chess_piece->tile_moves.size(); e++)
                    {
                        if (chess_piece->tile_moves[e].tile == check_tile && chess_piece->tile_moves[e].type != COVER && chess_piece->tile_moves[e].type != PIN_TRACE)
                        {
                            piece_blocks.push_back(chess_piece->tile_moves[e].tile);
                            printf("%d %d %d\n", chess_piece->type, check_tile.first, check_tile.second);
                        }
                    }
                }

                if (piece_blocks.empty())
                    break;
            }
        }

        chess_piece->tile_moves.clear();

        for (auto& block : piece_blocks)
        {
            possible_moves++;
            chess_piece->tile_moves.push_back({ ATTACK, block });
        }
    }

    if (tile_moves.empty() && possible_moves == 0)
    {
        is_mated = true;
        checks.clear();
        printf("KING MATED\n");
        return;
    }

    checks.clear();
}

constexpr void king_piece::can_target_point_ex(int row, int column)
{
    if (!(row >= 0 && row <= 7 && column >= 0 && column <= 7))
        return;

    std::pair<uint8_t, uint8_t> tile_coord = std::make_pair(static_cast<uint8_t>(row), static_cast<uint8_t>(column));

    auto& tile = chessboard->board_tiles[tile_coord.first][tile_coord.second];

    if (tile.piece && tile.piece->team == team)
    {
        tile_moves.push_back({ COVER, std::make_pair(row, column) });
        return;
    }

    for (auto& chess_player : chessboard->players)
    {
        if (team == chess_player.team)
            continue;

        for (int i = 0; i < chess_player.pieces.size(); i++)
        {
            const auto tile_piece = reinterpret_cast<piece*>(&chess_player.pieces[i]);

            if (tile_piece->position != tile_coord)
            {
                for (auto& piece_move : tile_piece->tile_moves)
                {
                    if (tile_coord == piece_move.tile && (piece_move.type == ATTACK || piece_move.type == COVER))
                        return;
                }
            }
        }
    }

    tile_moves.push_back({ ATTACK, std::make_pair(row, column) });
}

constexpr void king_piece::can_target_point(int row, int column, uint8_t direction)
{
    switch (direction)
    {
    case 0: // top
        can_target_point_ex(row + 1, column);
    break;
    case 1: // top right
        can_target_point_ex(row + 1, column + 1);
    break;
    case 2: // right
        can_target_point_ex(row, column + 1);
    break;
    case 3: // bottom right
        can_target_point_ex(row - 1, column + 1);
    break;
    case 4: // bottom
        can_target_point_ex(row - 1, column);
    break;
    case 5: // bottom left
        can_target_point_ex(row - 1, column - 1);
    break;
    case 6: // left
        can_target_point_ex(row, column - 1);
    break;
    case 7: // top left
        can_target_point_ex(row + 1, column - 1);
    break;
    }
}

void king_piece::set_targetable_points()
{
    tile_moves.clear();
    is_checked = false;
    is_mated = false;

    for (uint8_t i = 0; i <= static_cast<uint8_t>(7); i++)
        can_target_point(position.first, position.second, i);
}
