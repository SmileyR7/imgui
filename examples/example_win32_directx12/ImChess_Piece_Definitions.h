#pragma once

#include <variant>
#include <string>
#include <vector>
#include <optional>


#define MAX_PIECE_AMOUNT 16

class chessboard_instance;
class player_instance;

enum piece_type : uint8_t
{
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK, // THEEEEE ROOOOOOOOOK.
    QUEEN,
    KING
};

enum player_teams : uint8_t
{
    WHITE,
    BLACK,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    MAX_TEAM_AMOUNT
};

enum movement_type : uint8_t
{
    MOVE,
    ATTACK,
    COVER,
    EXTENDED,
    PIN_TRACE
};

class move_type
{
public:
    movement_type type;
    std::pair<uint8_t, uint8_t> tile{};
};

class piece
{
public:
    virtual void set_targetable_points();
    virtual void check_checks();

    std::optional<std::pair<uint8_t,uint8_t>> is_pinned{};
    bool is_alive = { true };
    bool checks_enemy_king{};

    piece_type type{};

    chessboard_instance* chessboard{};
    player_instance* player{};

    player_teams team{};
    std::pair<uint8_t, uint8_t> position{};
    std::vector<move_type> tile_moves{};
    void** texture{};
    //std::optional<ImColor> color_override{}; // 4 player chess? override white texture
};

class pawn_piece : public piece
{
public:
    pawn_piece(const piece& chess_piece)
    {
        type = PAWN;
        is_alive = true;
        texture = nullptr;

        chessboard = chess_piece.chessboard;
        player = chess_piece.player;
        team = chess_piece.team;
        position = chess_piece.position;
    };

public:
    virtual void set_targetable_points() override;
    virtual void check_checks() override;
private:
    void on_promote();
    constexpr bool can_target_point(int row, int column, bool attack_move = false, bool is_extended_move = false);
public:
    bool did_move_since_spawn{};
};

class knight_piece : public piece
{
public:
    knight_piece(const piece& chess_piece)
    {
        type = KNIGHT;
        is_alive = true;
        texture = nullptr;

        chessboard = chess_piece.chessboard;
        player = chess_piece.player;
        team = chess_piece.team;
        position = chess_piece.position;
    };

public:   
    virtual void set_targetable_points() override;
    constexpr void can_target_point(int row, int column);
};

class bishop_piece : public piece
{
public:
    bishop_piece(const piece& chess_piece)
    {
        type = BISHOP;
        is_alive = true;
        texture = nullptr;

        chessboard = chess_piece.chessboard;
        player = chess_piece.player;
        team = chess_piece.team;
        position = chess_piece.position;
    };

public:  
    virtual void set_targetable_points() override;
    constexpr bool can_target_point_ex(int row, int new_row, int column, int new_column);
    constexpr void can_target_point(int row, int column, uint8_t direction);
public:
    std::optional<piece*> pinned_piece{};
};

class rook_piece : public piece
{
public:
    rook_piece(const piece& chess_piece)
    {
        type = ROOK;
        is_alive = true;
        texture = nullptr;

        chessboard = chess_piece.chessboard;
        player = chess_piece.player;
        team = chess_piece.team;
        position = chess_piece.position;
    };

public:   
    virtual void set_targetable_points() override;
    constexpr bool can_target_point_ex(int row, int new_row, int column, int new_column);
    constexpr void can_target_point(int row, int column, uint8_t direction);

public:
    bool can_castle{};
    std::optional<piece*> pinned_piece{};
};

class queen_piece : public piece
{
public:
    queen_piece(const piece& chess_piece)
    {
        type = QUEEN;
        is_alive = true;
        texture = nullptr;

        chessboard = chess_piece.chessboard;
        player = chess_piece.player;
        team = chess_piece.team;
        position = chess_piece.position;
    };

public:  
    virtual void set_targetable_points() override;
    constexpr bool can_target_point_ex(int row, int new_row, int column, int new_column);
    constexpr void can_target_point(int row, int column, uint8_t direction);
public:
    std::optional<piece*> pinned_piece{};
};

class king_piece : public piece
{
public:
    king_piece(const piece& chess_piece)
    {
        type = KING;
        is_alive = true;
        texture = nullptr;

        chessboard = chess_piece.chessboard;
        player = chess_piece.player;
        team = chess_piece.team;
        position = chess_piece.position;
    };

public:
    virtual void set_targetable_points() override;
    virtual void check_checks() override;
    constexpr void can_target_point_ex(int row, int column);
    constexpr void can_target_point(int row, int column, uint8_t direction);

    std::vector<std::vector<std::pair<uint8_t, uint8_t>>> checks;

    bool is_checked{};
    bool is_mated{};
    bool can_castle{};
};

using chess_piece = std::variant<piece, pawn_piece, knight_piece, bishop_piece, rook_piece, queen_piece, king_piece>;
