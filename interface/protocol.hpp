// Type:  ('u8', {'type': 'unsigned'})
// Type:  ('u8', {'width': '8'})
// Type:  ('u8array', {'type': 'u8'})
// Type:  ('u8array', {'dynamic_array': '256'})
// Type:  ('u16', {'type': 'unsigned'})
// Type:  ('u16', {'width': '16'})
// Type:  ('u64', {'type': 'unsigned'})
// Type:  ('u64', {'width': '64'})
// Type:  ('String', {'type': 'asciiz'})
// Enumeration:  ('Action', ('LEFT', None))
// Enumeration:  ('Action', ('RIGHT', None))
// Enumeration:  ('Action', ('SOFT_DROP', None))
// Enumeration:  ('Action', ('HARD_DROP', None))
// Enumeration:  ('Action', ('HOLD', None))
// Enumeration:  ('Action', ('ROT_CLOCK', None))
// Enumeration:  ('Action', ('ROT_CCLOCK', None))
// Enumeration:  ('Action', ('ROT_180', None))
// Enumeration:  ('Piece', ('J', None))
// Enumeration:  ('Piece', ('L', None))
// Enumeration:  ('Piece', ('O', None))
// Enumeration:  ('Piece', ('S', None))
// Enumeration:  ('Piece', ('Z', None))
// Enumeration:  ('Piece', ('T', None))
// Sequence:  CreateGameRequest ('u8', 'boardWidth')
// Sequence:  CreateGameRequest ('u8', 'boardHeight')
// Sequence:  CreateGameRequest ('u16', 'lockingTimeoutMs')
// Sequence:  CreateGameRequestReject ('u8', 'spare')
// Sequence:  CreateGameRequestAccept ('u64', 'gameId')
// Sequence:  JoinRequest ('u64', 'gameId')
// Sequence:  Player ('String', 'name')
// Sequence:  Player ('u64', 'id')
// Type:  ('PlayerList', {'type': 'Player'})
// Type:  ('PlayerList', {'dynamic_array': '256'})
// Sequence:  JoinAccept ('u64', 'playerId')
// Sequence:  JoinReject ('u8', 'spare')
// Sequence:  PlayerUpdateNotification ('PlayerList', 'players')
// Sequence:  PlayerActionIndication ('u8', 'repeat')
// Sequence:  PlayerActionIndication ('Action', 'action')
// Type:  ('Lines', {'type': 'unsigned'})
// Type:  ('Lines', {'width': '8'})
// Type:  ('Lines', {'dynamic_array': '256'})
// Enumeration:  ('AddMode', ('INSERT', None))
// Enumeration:  ('AddMode', ('REPLACE', None))
// Sequence:  LineToAdd ('AddMode', 'mode')
// Sequence:  LineToAdd ('u8', 'line')
// Sequence:  LineToAdd ('u64', 'diff')
// Type:  ('LineToAddList', {'type': 'LineToAdd'})
// Type:  ('LineToAddList', {'dynamic_array': ''})
// Type:  ('PieceList', {'type': 'Piece'})
// Type:  ('PieceList', {'dynamic_array': '256'})
// Type:  ('OptionalPiece', {'type': 'Piece'})
// Type:  ('OptionalPiece', {'optional': ''})
// Sequence:  PiecePosition ('u8', 'x')
// Sequence:  PiecePosition ('u8', 'y')
// Type:  ('PiecePositionOptional', {'type': 'PiecePosition'})
// Type:  ('PiecePositionOptional', {'optional': ''})
// Sequence:  BoardUpdateNotification ('u8', 'player')
// Sequence:  BoardUpdateNotification ('PieceList', 'pieceToAddList')
// Sequence:  BoardUpdateNotification ('PiecePositionOptional', 'position')
// Sequence:  BoardUpdateNotification ('u8array', 'linesToRemoveList')
// Sequence:  BoardUpdateNotification ('LineToAddList', 'linesToAddList')
// Sequence:  PushPieceIndication ('PieceList', 'pieceToAddList')
// Sequence:  GameStartIndication ('u64', 'dropRequiredTimeout')
// Sequence:  GameOverNotification ('u64', 'playerId')
// Choice:  ('TetrisProtocol', 'CreateGameRequest')
// Choice:  ('TetrisProtocol', 'CreateGameRequestAccept')
// Choice:  ('TetrisProtocol', 'CreateGameRequestReject')
// Choice:  ('TetrisProtocol', 'JoinRequest')
// Choice:  ('TetrisProtocol', 'JoinAccept')
// Choice:  ('TetrisProtocol', 'JoinReject')
// Choice:  ('TetrisProtocol', 'PlayerUpdateNotification')
// Choice:  ('TetrisProtocol', 'PlayerActionIndication')
// Choice:  ('TetrisProtocol', 'BoardUpdateNotification')
// Choice:  ('TetrisProtocol', 'PushPieceIndication')
// Choice:  ('TetrisProtocol', 'GameStartIndication')
// Choice:  ('TetrisProtocol', 'GameOverNotification')
// Generating for C++
#ifndef __CUM_MSG_HPP__
#define __CUM_MSG_HPP__
#include "cum/cum.hpp"
#include <optional>

/***********************************************
/
/            Message Definitions
/
************************************************/

using u8 = uint8_t;
using u8array = cum::vector<u8, 256>;
using u16 = uint16_t;
using u64 = uint64_t;
using String = std::string;
enum class Action : uint8_t
{
    LEFT,
    RIGHT,
    SOFT_DROP,
    HARD_DROP,
    HOLD,
    ROT_CLOCK,
    ROT_CCLOCK,
    ROT_180
};

enum class Piece : uint8_t
{
    J,
    L,
    O,
    S,
    Z,
    T
};

struct CreateGameRequest
{
    u8 boardWidth;
    u8 boardHeight;
    u16 lockingTimeoutMs;
};

struct CreateGameRequestReject
{
    u8 spare;
};

struct CreateGameRequestAccept
{
    u64 gameId;
};

struct JoinRequest
{
    u64 gameId;
};

struct Player
{
    String name;
    u64 id;
};

using PlayerList = cum::vector<Player, 256>;
struct JoinAccept
{
    u64 playerId;
};

struct JoinReject
{
    u8 spare;
};

struct PlayerUpdateNotification
{
    PlayerList players;
};

struct PlayerActionIndication
{
    u8 repeat;
    Action action;
};

using Lines = cum::vector<uint8_t, 256>;
enum class AddMode : uint8_t
{
    INSERT,
    REPLACE
};

struct LineToAdd
{
    AddMode mode;
    u8 line;
    u64 diff;
};

using LineToAddList = cum::vector<LineToAdd, 4294967296>;
using PieceList = cum::vector<Piece, 256>;
using OptionalPiece = std::optional<Piece>;
struct PiecePosition
{
    u8 x;
    u8 y;
};

using PiecePositionOptional = std::optional<PiecePosition>;
struct BoardUpdateNotification
{
    u8 player;
    PieceList pieceToAddList;
    PiecePositionOptional position;
    u8array linesToRemoveList;
    LineToAddList linesToAddList;
};

struct PushPieceIndication
{
    PieceList pieceToAddList;
};

struct GameStartIndication
{
    u64 dropRequiredTimeout;
};

struct GameOverNotification
{
    u64 playerId;
};

using TetrisProtocol = std::variant<CreateGameRequest,CreateGameRequestAccept,CreateGameRequestReject,JoinRequest,JoinAccept,JoinReject,PlayerUpdateNotification,PlayerActionIndication,BoardUpdateNotification,PushPieceIndication,GameStartIndication,GameOverNotification>;
/***********************************************
/
/            Codec Definitions
/
************************************************/

inline void str(const char* pName, const Action& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (Action::LEFT == pIe) pCtx += "\"LEFT\"";
    if (Action::RIGHT == pIe) pCtx += "\"RIGHT\"";
    if (Action::SOFT_DROP == pIe) pCtx += "\"SOFT_DROP\"";
    if (Action::HARD_DROP == pIe) pCtx += "\"HARD_DROP\"";
    if (Action::HOLD == pIe) pCtx += "\"HOLD\"";
    if (Action::ROT_CLOCK == pIe) pCtx += "\"ROT_CLOCK\"";
    if (Action::ROT_CCLOCK == pIe) pCtx += "\"ROT_CCLOCK\"";
    if (Action::ROT_180 == pIe) pCtx += "\"ROT_180\"";
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void str(const char* pName, const Piece& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (Piece::J == pIe) pCtx += "\"J\"";
    if (Piece::L == pIe) pCtx += "\"L\"";
    if (Piece::O == pIe) pCtx += "\"O\"";
    if (Piece::S == pIe) pCtx += "\"S\"";
    if (Piece::Z == pIe) pCtx += "\"Z\"";
    if (Piece::T == pIe) pCtx += "\"T\"";
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const CreateGameRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.boardWidth, pCtx);
    encode_per(pIe.boardHeight, pCtx);
    encode_per(pIe.lockingTimeoutMs, pCtx);
}

inline void decode_per(CreateGameRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.boardWidth, pCtx);
    decode_per(pIe.boardHeight, pCtx);
    decode_per(pIe.lockingTimeoutMs, pCtx);
}

inline void str(const char* pName, const CreateGameRequest& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 3;
    str("boardWidth", pIe.boardWidth, pCtx, !(--nMandatory+nOptional));
    str("boardHeight", pIe.boardHeight, pCtx, !(--nMandatory+nOptional));
    str("lockingTimeoutMs", pIe.lockingTimeoutMs, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const CreateGameRequestReject& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(CreateGameRequestReject& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
}

inline void str(const char* pName, const CreateGameRequestReject& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("spare", pIe.spare, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const CreateGameRequestAccept& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.gameId, pCtx);
}

inline void decode_per(CreateGameRequestAccept& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.gameId, pCtx);
}

inline void str(const char* pName, const CreateGameRequestAccept& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("gameId", pIe.gameId, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const JoinRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.gameId, pCtx);
}

inline void decode_per(JoinRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.gameId, pCtx);
}

inline void str(const char* pName, const JoinRequest& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("gameId", pIe.gameId, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const Player& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.name, pCtx);
    encode_per(pIe.id, pCtx);
}

inline void decode_per(Player& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.name, pCtx);
    decode_per(pIe.id, pCtx);
}

inline void str(const char* pName, const Player& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 2;
    str("name", pIe.name, pCtx, !(--nMandatory+nOptional));
    str("id", pIe.id, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const JoinAccept& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.playerId, pCtx);
}

inline void decode_per(JoinAccept& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.playerId, pCtx);
}

inline void str(const char* pName, const JoinAccept& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("playerId", pIe.playerId, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const JoinReject& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(JoinReject& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
}

inline void str(const char* pName, const JoinReject& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("spare", pIe.spare, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const PlayerUpdateNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.players, pCtx);
}

inline void decode_per(PlayerUpdateNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.players, pCtx);
}

inline void str(const char* pName, const PlayerUpdateNotification& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("players", pIe.players, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const PlayerActionIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.repeat, pCtx);
    encode_per(pIe.action, pCtx);
}

inline void decode_per(PlayerActionIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.repeat, pCtx);
    decode_per(pIe.action, pCtx);
}

inline void str(const char* pName, const PlayerActionIndication& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 2;
    str("repeat", pIe.repeat, pCtx, !(--nMandatory+nOptional));
    str("action", pIe.action, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void str(const char* pName, const AddMode& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (AddMode::INSERT == pIe) pCtx += "\"INSERT\"";
    if (AddMode::REPLACE == pIe) pCtx += "\"REPLACE\"";
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const LineToAdd& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.mode, pCtx);
    encode_per(pIe.line, pCtx);
    encode_per(pIe.diff, pCtx);
}

inline void decode_per(LineToAdd& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.mode, pCtx);
    decode_per(pIe.line, pCtx);
    decode_per(pIe.diff, pCtx);
}

inline void str(const char* pName, const LineToAdd& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 3;
    str("mode", pIe.mode, pCtx, !(--nMandatory+nOptional));
    str("line", pIe.line, pCtx, !(--nMandatory+nOptional));
    str("diff", pIe.diff, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const PiecePosition& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.x, pCtx);
    encode_per(pIe.y, pCtx);
}

inline void decode_per(PiecePosition& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.x, pCtx);
    decode_per(pIe.y, pCtx);
}

inline void str(const char* pName, const PiecePosition& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 2;
    str("x", pIe.x, pCtx, !(--nMandatory+nOptional));
    str("y", pIe.y, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const BoardUpdateNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    if (pIe.position)
    {
        set_optional(optionalmask, 0);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    encode_per(pIe.player, pCtx);
    encode_per(pIe.pieceToAddList, pCtx);
    if (pIe.position)
    {
        encode_per(*pIe.position, pCtx);
    }
    encode_per(pIe.linesToRemoveList, pCtx);
    encode_per(pIe.linesToAddList, pCtx);
}

inline void decode_per(BoardUpdateNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    decode_per(optionalmask, sizeof(optionalmask), pCtx);
    decode_per(pIe.player, pCtx);
    decode_per(pIe.pieceToAddList, pCtx);
    if (check_optional(optionalmask, 0))
    {
        pIe.position = decltype(pIe.position)::value_type{};
        decode_per(*pIe.position, pCtx);
    }
    decode_per(pIe.linesToRemoveList, pCtx);
    decode_per(pIe.linesToAddList, pCtx);
}

inline void str(const char* pName, const BoardUpdateNotification& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    if (pIe.position) nOptional++;
    size_t nMandatory = 4;
    str("player", pIe.player, pCtx, !(--nMandatory+nOptional));
    str("pieceToAddList", pIe.pieceToAddList, pCtx, !(--nMandatory+nOptional));
    if (pIe.position)
    {
        str("position", *pIe.position, pCtx, !(nMandatory+--nOptional));
    }
    str("linesToRemoveList", pIe.linesToRemoveList, pCtx, !(--nMandatory+nOptional));
    str("linesToAddList", pIe.linesToAddList, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const PushPieceIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.pieceToAddList, pCtx);
}

inline void decode_per(PushPieceIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.pieceToAddList, pCtx);
}

inline void str(const char* pName, const PushPieceIndication& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("pieceToAddList", pIe.pieceToAddList, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const GameStartIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.dropRequiredTimeout, pCtx);
}

inline void decode_per(GameStartIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.dropRequiredTimeout, pCtx);
}

inline void str(const char* pName, const GameStartIndication& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("dropRequiredTimeout", pIe.dropRequiredTimeout, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const GameOverNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.playerId, pCtx);
}

inline void decode_per(GameOverNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.playerId, pCtx);
}

inline void str(const char* pName, const GameOverNotification& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("playerId", pIe.playerId, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const TetrisProtocol& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    using TypeIndex = uint8_t;
    TypeIndex type = pIe.index();
    encode_per(type, pCtx);
    if (0 == type)
    {
        encode_per(std::get<0>(pIe), pCtx);
    }
    else if (1 == type)
    {
        encode_per(std::get<1>(pIe), pCtx);
    }
    else if (2 == type)
    {
        encode_per(std::get<2>(pIe), pCtx);
    }
    else if (3 == type)
    {
        encode_per(std::get<3>(pIe), pCtx);
    }
    else if (4 == type)
    {
        encode_per(std::get<4>(pIe), pCtx);
    }
    else if (5 == type)
    {
        encode_per(std::get<5>(pIe), pCtx);
    }
    else if (6 == type)
    {
        encode_per(std::get<6>(pIe), pCtx);
    }
    else if (7 == type)
    {
        encode_per(std::get<7>(pIe), pCtx);
    }
    else if (8 == type)
    {
        encode_per(std::get<8>(pIe), pCtx);
    }
    else if (9 == type)
    {
        encode_per(std::get<9>(pIe), pCtx);
    }
    else if (10 == type)
    {
        encode_per(std::get<10>(pIe), pCtx);
    }
    else if (11 == type)
    {
        encode_per(std::get<11>(pIe), pCtx);
    }
}

inline void decode_per(TetrisProtocol& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    using TypeIndex = uint8_t;
    TypeIndex type;
    decode_per(type, pCtx);
    if (0 == type)
    {
        pIe = CreateGameRequest();
        decode_per(std::get<0>(pIe), pCtx);
    }
    else if (1 == type)
    {
        pIe = CreateGameRequestAccept();
        decode_per(std::get<1>(pIe), pCtx);
    }
    else if (2 == type)
    {
        pIe = CreateGameRequestReject();
        decode_per(std::get<2>(pIe), pCtx);
    }
    else if (3 == type)
    {
        pIe = JoinRequest();
        decode_per(std::get<3>(pIe), pCtx);
    }
    else if (4 == type)
    {
        pIe = JoinAccept();
        decode_per(std::get<4>(pIe), pCtx);
    }
    else if (5 == type)
    {
        pIe = JoinReject();
        decode_per(std::get<5>(pIe), pCtx);
    }
    else if (6 == type)
    {
        pIe = PlayerUpdateNotification();
        decode_per(std::get<6>(pIe), pCtx);
    }
    else if (7 == type)
    {
        pIe = PlayerActionIndication();
        decode_per(std::get<7>(pIe), pCtx);
    }
    else if (8 == type)
    {
        pIe = BoardUpdateNotification();
        decode_per(std::get<8>(pIe), pCtx);
    }
    else if (9 == type)
    {
        pIe = PushPieceIndication();
        decode_per(std::get<9>(pIe), pCtx);
    }
    else if (10 == type)
    {
        pIe = GameStartIndication();
        decode_per(std::get<10>(pIe), pCtx);
    }
    else if (11 == type)
    {
        pIe = GameOverNotification();
        decode_per(std::get<11>(pIe), pCtx);
    }
}

inline void str(const char* pName, const TetrisProtocol& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    using TypeIndex = uint8_t;
    TypeIndex type = pIe.index();
    if (0 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "CreateGameRequest";
        str(name.c_str(), std::get<0>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (1 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "CreateGameRequestAccept";
        str(name.c_str(), std::get<1>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (2 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "CreateGameRequestReject";
        str(name.c_str(), std::get<2>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (3 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "JoinRequest";
        str(name.c_str(), std::get<3>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (4 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "JoinAccept";
        str(name.c_str(), std::get<4>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (5 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "JoinReject";
        str(name.c_str(), std::get<5>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (6 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "PlayerUpdateNotification";
        str(name.c_str(), std::get<6>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (7 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "PlayerActionIndication";
        str(name.c_str(), std::get<7>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (8 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "BoardUpdateNotification";
        str(name.c_str(), std::get<8>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (9 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "PushPieceIndication";
        str(name.c_str(), std::get<9>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (10 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "GameStartIndication";
        str(name.c_str(), std::get<10>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (11 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "GameOverNotification";
        str(name.c_str(), std::get<11>(pIe), pCtx, true);
        pCtx += "}";
    }
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

#endif //__CUM_MSG_HPP__
