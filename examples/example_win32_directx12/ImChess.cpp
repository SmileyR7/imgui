#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>
#include <imgui_internal.h>
#include <random>
#include "ImChess.h"
#include "ImChess_debug_timer.h"

chessboard_instance::chessboard_instance(const bool is_free_move, int player_count)
{
    free_move = is_free_move;
    player_count = std::clamp(player_count, 2, 4);

    players.reserve(player_count);

    const bool is_2v2_chess = player_count == 2;

    team_can_move = (is_2v2_chess ? WHITE : RED);

    std::random_device random_device;
    std::mt19937 generate(random_device());

    std::uniform_int_distribution<int> char_distribution(65, 90);

    board_name.reserve(12);
    board_name = "##";

    for (int i = 0; i < 10; i++)
        board_name.append(1, static_cast<char>(char_distribution(generate)));

    if (is_offline)
    {
        std::uniform_int_distribution<int> team_distribution((is_2v2_chess ? 0 : 2), (is_2v2_chess ? 1 : player_count));

        player_teams team = static_cast<player_teams>(team_distribution(generate));

        player_instance player = {};

        for (int i = 0; i < player_count; i++)
        {
            player.is_local_player = i == 0;
            player.team = team;

            if (player.is_local_player)
                localplayer_team = team;

            player.set_direction();
            player.set_spawn_point();
            player.set_pieces(this);

            if (player_count == 2)
            {
                if (team == WHITE)
                    team = BLACK;
                else
                    team = WHITE;
            }
            else
            {
                int team_int = static_cast<int>(team);
                if (team_int++ >= MAX_TEAM_AMOUNT)
                    team = RED;
                else
                    team = static_cast<player_teams>(team_int);
            }

            players.push_back(player);
        }
    }

    for (auto& player : players)
    {
        for (int i = 0; i < player.pieces.size(); i++)
        {
            auto* tile_piece = reinterpret_cast<piece*>(&player.pieces[i]);
            board_tiles[tile_piece->position.first][tile_piece->position.second].piece = tile_piece;
        }

        for (int i = 0; i < player.pieces.size(); i++)
        {
            auto* tile_piece = reinterpret_cast<piece*>(&player.pieces[i]);
            tile_piece->player = &player;
            tile_piece->chessboard = this;
            tile_piece->set_targetable_points();
        }
    }

    set_tile_color();
}

void player_instance::set_pieces(chessboard_instance* instance)
{
    pieces.clear();
    pieces.reserve(MAX_PIECE_AMOUNT);
    spawn_pieces(instance);
}

void player_instance::spawn_pieces(chessboard_instance* chessboard)
{
    piece piece;
    piece.player = this;
    piece.chessboard = chessboard;
    piece.team = team;

    for (uint8_t i = 0; i <= static_cast<uint8_t>(7); i++)
    {
        piece.position = std::make_pair(spawn_point.first + (direction == UP ? -1 : + 1), i);
        pieces.push_back(pawn_piece{ piece });
    }

    piece.position = std::make_pair(spawn_point.first, spawn_point.second + 1);
    pieces.push_back(knight_piece{ piece });
    piece.position = std::make_pair(spawn_point.first, spawn_point.second + 6);
    pieces.push_back(knight_piece{ piece });

    piece.position = std::make_pair(spawn_point.first, spawn_point.second + 2);
    pieces.push_back(bishop_piece{ piece });
    piece.position = std::make_pair(spawn_point.first, spawn_point.second + 5);
    pieces.push_back(bishop_piece{ piece });

    piece.position = std::make_pair(spawn_point.first, spawn_point.second);
    pieces.push_back(rook_piece{ piece });
    piece.position = std::make_pair(spawn_point.first, spawn_point.second + 7);
    pieces.push_back(rook_piece{ piece });

    switch (team)
    {
        case WHITE:
        case BLACK:
        {
            piece.position = std::make_pair(spawn_point.first, spawn_point.second + (is_local_player ? 3 : 4));
            pieces.push_back(queen_piece{ piece });

            piece.position = std::make_pair(spawn_point.first, spawn_point.second + (is_local_player ? 4 : 3));
            pieces.push_back(king_piece{ piece });
        }
        break;
    }
}

void chessboard_instance::on_draw()
{
    if (should_delete)
        return;

    bool window_open = true;

    ImGui::Begin(board_name.c_str(), &window_open, ImGuiWindowFlags_AlwaysAutoResize);

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems)
    {
        ImGui::End();
        return;
    }

    if (!window_open)
    {
        should_delete = true;
        ImGui::End();
        return;
    }

    constexpr float tile_size = 128.f;
    constexpr float half_tile_size = tile_size * 0.5f;
    constexpr ImVec2 tile_vec2 = ImVec2(tile_size, tile_size);

    ImVec2 region_start = window->WorkRect.Min;
    const ImVec2 region_start_backup = region_start;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.f, 0.f));

    for (uint8_t row = 0; row <= static_cast<uint8_t>(7); row++)
    {
        region_start.x = region_start_backup.x;

        for (uint8_t column = 0; column <= static_cast<uint8_t>(7); column++)
        {
            if (column > 0)
                ImGui::SameLine(0.0f, 0.0f);

            chessboard_tile& tile = board_tiles[row][column];

            const ImRect bb(region_start, region_start + tile_vec2);
            ImGui::ItemSize(tile_vec2, ImGui::GetStyle().FramePadding.y);

            const auto tile_name = board_name + "_" + std::to_string(row) + std::to_string(column);
            const auto id = ImGui::GetID(tile_name.c_str());

            if (ImGui::ItemAdd(bb, id))
            {
                bool hovered, held;
                bool pressed = tile.piece && ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnClickReleaseAnywhere);

                if (pressed)
                    on_move(tile, row, column);

                if (active_tile.has_value() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
                    hovered_tile = std::make_pair(row, column);

                ImGui::GetWindowDrawList()->AddRectFilled(region_start, region_start + tile_vec2, tile.color);

                if (active_tile.has_value() && active_tile.value().first == row && active_tile.value().second == column)
                {
                    if (tile.active_tile_state < 1.f)
                    {
                        tile.active_tile_state += 1.f / 0.1f * ImGui::GetIO().DeltaTime;
                        tile.active_tile_state = std::min(tile.active_tile_state, 1.f);
                    }
                }
                else
                {
                    if (tile.active_tile_state > 0.f)
                    {
                        tile.active_tile_state -= 1.f / 0.3f * ImGui::GetIO().DeltaTime;
                        tile.active_tile_state = std::max(tile.active_tile_state, 0.f);
                    }
                }

                if (tile.piece && tile.piece->type == KING)
                {
                    const auto* king = reinterpret_cast<king_piece*>(tile.piece);
                    if (king->is_mated)
                        ImGui::GetWindowDrawList()->AddRectFilled(region_start, region_start + tile_vec2, ImColor(0.9f, 0.1f, 0.1f, 1.f), tile_size * 0.1f);
                    else if (king->is_checked)
                        ImGui::GetWindowDrawList()->AddRectFilled(region_start, region_start + tile_vec2, ImColor(0.7f, 0.1f, 0.1f, 1.f), tile_size * 0.1f);
                }

                if (tile.active_tile_state > 0.f)
                {
                    const auto active_adjust = tile_vec2 * 0.15f * tile.active_tile_state;
                    ImGui::GetWindowDrawList()->AddRectFilled(region_start + active_adjust , region_start + tile_vec2 - active_adjust, ImColor(0.8f, 0.8f, 0.8f, 1.f * tile.active_tile_state), tile_size * 0.1f);
                }

                if (tile.piece && tile.piece->is_alive)
                {
                    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(region_start.x + half_tile_size, region_start.y + half_tile_size), tile_size * 0.17f, tile.piece->team == WHITE ? ImColor(255, 255, 255, 255) : ImColor(70, 70, 70, 255));
                    ImGui::GetWindowDrawList()->AddCircle(ImVec2(region_start.x + half_tile_size, region_start.y + half_tile_size), tile_size * 0.17f, ImColor(0, 0, 0, 255));

                    const char* name = tile.piece->type == PAWN ? "P" : tile.piece->type == KNIGHT ? "K" : tile.piece->type == BISHOP ? "B" : tile.piece->type == ROOK ? "R" : tile.piece->type == QUEEN ? "Q" : "KK";
                    const auto offset = ImGui::CalcTextSize(name);

                    ImGui::GetWindowDrawList()->AddText(ImVec2(region_start.x + half_tile_size - offset.x * 0.5f, region_start.y + half_tile_size - offset.y * 0.5f), tile.piece->team == BLACK ? ImColor(255, 255, 255, 255) : ImColor(0, 0, 0, 255), name);
                    if (tile.active_tile_state > 0.f && !tile.piece->is_pinned)
                    {
                        for (const auto& tile_move : tile.piece->tile_moves)
                        {
                            if (tile_move.type == COVER || tile_move.type == PIN_TRACE)
                                continue;

                            const float row = static_cast<float>(tile_move.tile.first);
                            const float column = static_cast<float>(tile_move.tile.second);

                            const float x = region_start_backup.x + (column * tile_size) + half_tile_size;
                            const float y = region_start_backup.y + (row * tile_size) + half_tile_size;

                            ImGui::GetForegroundDrawList()->AddCircleFilled({ x, y }, tile_size * 0.1f * tile.active_tile_state, ImColor(0.2f, 0.2f, 0.2f, 1.f * tile.active_tile_state));
                            ImGui::GetForegroundDrawList()->AddCircle({ x, y }, tile_size * 0.1f * tile.active_tile_state, ImColor(0.f, 0.f, 0.f, 1.f * tile.active_tile_state));
                        }
                    }
                }

                region_start.x += tile_size;
            }
        }

        region_start.y += tile_size;
    }

    ImGui::PopStyleVar(2);

    ImGui::End();
}

void chessboard_instance::on_move(chessboard_tile& tile, uint8_t row, uint8_t column)
{
    debug_timer timer;

    if (active_tile.has_value())
    {
        if (hovered_tile.has_value())
        {
            if (active_tile.value() != hovered_tile.value() && (free_move || team_can_move == tile.piece->team))
            {
                bool found_target = false;

                if (!tile.piece->is_pinned)
                {
                    for (auto& target : tile.piece->tile_moves)
                    {
                        if (target.type != COVER && target.tile == hovered_tile)
                        {
                            found_target = true;
                            break;
                        }
                    }
                }

                if (found_target)
                {
                    if (tile.piece->type == PAWN)
                    {
                        const auto pawn = reinterpret_cast<pawn_piece*>(tile.piece);
                        pawn->did_move_since_spawn = true;
                    }

                    auto& board_tile = board_tiles[hovered_tile.value().first][hovered_tile.value().second];

                    if (board_tile.piece)
                        board_tile.piece->is_alive = false;

                    board_tile.piece = tile.piece;
                    board_tile.piece->position = std::make_pair(hovered_tile.value().first, hovered_tile.value().second);

                    tile.piece = nullptr;

                    for (auto& player : players)
                    {
                        for (int i = 0; i < player.pieces.size(); i++)
                        {
                            const auto chess_piece = reinterpret_cast<piece*>(&player.pieces[i]);
                            chess_piece->is_pinned = {};
                        }
                    }

                    for (auto& player : players)
                    {
                        for (int i = 0; i < player.pieces.size() - 1; i++)
                        {
                            const auto chess_piece = reinterpret_cast<piece*>(&player.pieces[i]);

                            chess_piece->chessboard = this;
                            chess_piece->player = &player;
                            chess_piece->set_targetable_points();
                            chess_piece->check_checks();
                        }
                    }

                    // now after every piece except kings have set their calcs, now lets check the kings.

                    for (auto& player : players)
                    {
                        if (player.pieces.size() == MAX_PIECE_AMOUNT)
                        {
                            const auto king = reinterpret_cast<king_piece*>(&player.pieces[player.pieces.size() - 1]);

                            king->chessboard = this;
                            king->player = &player;
                            king->set_targetable_points();
                        }
                    }

                    for (auto& player : players)
                    {
                        if (player.pieces.size() == MAX_PIECE_AMOUNT)
                        {
                            const auto king = reinterpret_cast<king_piece*>(&player.pieces[player.pieces.size() - 1]);
                            king->check_checks();
                        }
                    }

                    if (team_can_move == WHITE)
                        team_can_move = BLACK;
                    else
                        team_can_move = WHITE;

                    printf("move submitted row %d, column %d\n", hovered_tile.value().first, hovered_tile.value().second);
                }
            }
        }
        active_tile = {};
    }
    else
        active_tile = std::make_pair(row, column);
}

void chessboard_instance::set_tile_color()
{
    bool change = localplayer_team == WHITE ? true : false;

    for (uint8_t row = 0; row <= static_cast<uint8_t>(7); row++)
    {
        change = !change;

        for (uint8_t column = 0; column <= static_cast<uint8_t>(7); column++)
        {
             change = !change;

            chessboard_tile& tile = board_tiles[row][column];

            tile.color = change ? ImColor(220, 220, 220, 255) : ImColor(120, 120, 120, 255);
        }
    }
}

void chessboard_instance::check_king_tiles()
{

}
