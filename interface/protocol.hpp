// Type:  ('u8', {'type': 'unsigned'})
// Type:  ('u8', {'width': '8'})
// Type:  ('i8', {'type': 'signed'})
// Type:  ('i8', {'width': '8'})
// Type:  ('OptionalU8', {'type': 'u8'})
// Type:  ('OptionalU8', {'optional': ''})
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
// Enumeration:  ('Piece', ('I', None))
// Enumeration:  ('Piece', ('L', None))
// Enumeration:  ('Piece', ('J', None))
// Enumeration:  ('Piece', ('O', None))
// Enumeration:  ('Piece', ('S', None))
// Enumeration:  ('Piece', ('Z', None))
// Enumeration:  ('Piece', ('T', None))
// Sequence:  CreateGameRequest ('u8', 'boardWidth')
// Sequence:  CreateGameRequest ('u8', 'boardHeight')
// Sequence:  CreateGameRequest ('u16', 'targetChangeTimeoutMs')
// Sequence:  CreateGameRequest ('u16', 'lockingTimeoutMs')
// Sequence:  CreateGameReject ('u8', 'spare')
// Sequence:  CreateGameAccept ('u64', 'gameId')
// Sequence:  GameStartIndication ('u8', 'spare')
// Type:  ('PieceList', {'type': 'Piece'})
// Type:  ('PieceList', {'dynamic_array': '256'})
// Sequence:  PieceRequest ('u8', 'count')
// Sequence:  PieceResponse ('PieceList', 'pieceToAddList')
// Sequence:  Player ('String', 'name')
// Sequence:  Player ('u8', 'id')
// Type:  ('PlayerList', {'type': 'Player'})
// Type:  ('PlayerList', {'dynamic_array': '256'})
// Sequence:  PlayerUpdateNotification ('PlayerList', 'playerToAddList')
// Sequence:  Line ('u8', 'line')
// Sequence:  Line ('u16', 'diff')
// Type:  ('LineList', {'type': 'Line'})
// Type:  ('LineList', {'dynamic_array': ''})
// Sequence:  PiecePosition ('i8', 'x')
// Sequence:  PiecePosition ('i8', 'y')
// Type:  ('PiecePositionOptional', {'type': 'PiecePosition'})
// Type:  ('PiecePositionOptional', {'optional': ''})
// Type:  ('PieceOptional', {'type': 'Piece'})
// Type:  ('PieceOptional', {'optional': ''})
// Sequence:  BoardUpdateNotification ('u8', 'player')
// Sequence:  BoardUpdateNotification ('OptionalU8', 'rotation')
// Sequence:  BoardUpdateNotification ('PieceOptional', 'placement')
// Sequence:  BoardUpdateNotification ('PieceOptional', 'hold')
// Sequence:  BoardUpdateNotification ('PieceList', 'pieceToAddList')
// Sequence:  BoardUpdateNotification ('PiecePositionOptional', 'position')
// Sequence:  BoardUpdateNotification ('LineList', 'linesToReplaceList')
// Sequence:  BoardUpdateNotification ('u8array', 'linesToRemoveList')
// Sequence:  BoardUpdateNotification ('LineList', 'linesToInsertList')
// Sequence:  BoardUpdateNotification ('OptionalU8', 'attackIndicator')
// Sequence:  GameOverNotification ('u8', 'playerId')
// Sequence:  JoinRequest ('u64', 'gameId')
// Sequence:  JoinAccept ('u8', 'playerId')
// Sequence:  JoinAccept ('u8', 'boardWidth')
// Sequence:  JoinAccept ('u8', 'boardHeight')
// Sequence:  JoinReject ('u8', 'spare')
// Sequence:  PlayerActionIndication ('u8', 'player')
// Sequence:  PlayerActionIndication ('u8', 'count')
// Sequence:  PlayerActionIndication ('Action', 'action')
// Choice:  ('TetrisProtocol', 'CreateGameRequest')
// Choice:  ('TetrisProtocol', 'CreateGameAccept')
// Choice:  ('TetrisProtocol', 'CreateGameReject')
// Choice:  ('TetrisProtocol', 'GameStartIndication')
// Choice:  ('TetrisProtocol', 'PieceRequest')
// Choice:  ('TetrisProtocol', 'PieceResponse')
// Choice:  ('TetrisProtocol', 'PlayerUpdateNotification')
// Choice:  ('TetrisProtocol', 'BoardUpdateNotification')
// Choice:  ('TetrisProtocol', 'GameOverNotification')
// Choice:  ('TetrisProtocol', 'JoinRequest')
// Choice:  ('TetrisProtocol', 'JoinAccept')
// Choice:  ('TetrisProtocol', 'JoinReject')
// Choice:  ('TetrisProtocol', 'PlayerActionIndication')
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
using i8 = int8_t;
using OptionalU8 = std::optional<u8>;
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
    I,
    L,
    J,
    O,
    S,
    Z,
    T
};

struct CreateGameRequest
{
    u8 boardWidth;
    u8 boardHeight;
    u16 targetChangeTimeoutMs;
    u16 lockingTimeoutMs;
};

struct CreateGameReject
{
    u8 spare;
};

struct CreateGameAccept
{
    u64 gameId;
};

struct GameStartIndication
{
    u8 spare;
};

using PieceList = cum::vector<Piece, 256>;
struct PieceRequest
{
    u8 count;
};

struct PieceResponse
{
    PieceList pieceToAddList;
};

struct Player
{
    String name;
    u8 id;
};

using PlayerList = cum::vector<Player, 256>;
struct PlayerUpdateNotification
{
    PlayerList playerToAddList;
};

struct Line
{
    u8 line;
    u16 diff;
};

using LineList = cum::vector<Line, 4294967296>;
struct PiecePosition
{
    i8 x;
    i8 y;
};

using PiecePositionOptional = std::optional<PiecePosition>;
using PieceOptional = std::optional<Piece>;
struct BoardUpdateNotification
{
    u8 player;
    OptionalU8 rotation;
    PieceOptional placement;
    PieceOptional hold;
    PieceList pieceToAddList;
    PiecePositionOptional position;
    LineList linesToReplaceList;
    u8array linesToRemoveList;
    LineList linesToInsertList;
    OptionalU8 attackIndicator;
};

struct GameOverNotification
{
    u8 playerId;
};

struct JoinRequest
{
    u64 gameId;
};

struct JoinAccept
{
    u8 playerId;
    u8 boardWidth;
    u8 boardHeight;
};

struct JoinReject
{
    u8 spare;
};

struct PlayerActionIndication
{
    u8 player;
    u8 count;
    Action action;
};

using TetrisProtocol = std::variant<CreateGameRequest,CreateGameAccept,CreateGameReject,GameStartIndication,PieceRequest,PieceResponse,PlayerUpdateNotification,BoardUpdateNotification,GameOverNotification,JoinRequest,JoinAccept,JoinReject,PlayerActionIndication>;
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
    if (Piece::I == pIe) pCtx += "\"I\"";
    if (Piece::L == pIe) pCtx += "\"L\"";
    if (Piece::J == pIe) pCtx += "\"J\"";
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
    encode_per(pIe.targetChangeTimeoutMs, pCtx);
    encode_per(pIe.lockingTimeoutMs, pCtx);
}

inline void decode_per(CreateGameRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.boardWidth, pCtx);
    decode_per(pIe.boardHeight, pCtx);
    decode_per(pIe.targetChangeTimeoutMs, pCtx);
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
    size_t nMandatory = 4;
    str("boardWidth", pIe.boardWidth, pCtx, !(--nMandatory+nOptional));
    str("boardHeight", pIe.boardHeight, pCtx, !(--nMandatory+nOptional));
    str("targetChangeTimeoutMs", pIe.targetChangeTimeoutMs, pCtx, !(--nMandatory+nOptional));
    str("lockingTimeoutMs", pIe.lockingTimeoutMs, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const CreateGameReject& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(CreateGameReject& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
}

inline void str(const char* pName, const CreateGameReject& pIe, std::string& pCtx, bool pIsLast)
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

inline void encode_per(const CreateGameAccept& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.gameId, pCtx);
}

inline void decode_per(CreateGameAccept& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.gameId, pCtx);
}

inline void str(const char* pName, const CreateGameAccept& pIe, std::string& pCtx, bool pIsLast)
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

inline void encode_per(const GameStartIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(GameStartIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
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
    str("spare", pIe.spare, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const PieceRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.count, pCtx);
}

inline void decode_per(PieceRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.count, pCtx);
}

inline void str(const char* pName, const PieceRequest& pIe, std::string& pCtx, bool pIsLast)
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
    str("count", pIe.count, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const PieceResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.pieceToAddList, pCtx);
}

inline void decode_per(PieceResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.pieceToAddList, pCtx);
}

inline void str(const char* pName, const PieceResponse& pIe, std::string& pCtx, bool pIsLast)
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

inline void encode_per(const PlayerUpdateNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.playerToAddList, pCtx);
}

inline void decode_per(PlayerUpdateNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.playerToAddList, pCtx);
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
    str("playerToAddList", pIe.playerToAddList, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const Line& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.line, pCtx);
    encode_per(pIe.diff, pCtx);
}

inline void decode_per(Line& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.line, pCtx);
    decode_per(pIe.diff, pCtx);
}

inline void str(const char* pName, const Line& pIe, std::string& pCtx, bool pIsLast)
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
    if (pIe.rotation)
    {
        set_optional(optionalmask, 0);
    }
    if (pIe.placement)
    {
        set_optional(optionalmask, 1);
    }
    if (pIe.hold)
    {
        set_optional(optionalmask, 2);
    }
    if (pIe.position)
    {
        set_optional(optionalmask, 3);
    }
    if (pIe.attackIndicator)
    {
        set_optional(optionalmask, 4);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    encode_per(pIe.player, pCtx);
    if (pIe.rotation)
    {
        encode_per(*pIe.rotation, pCtx);
    }
    if (pIe.placement)
    {
        encode_per(*pIe.placement, pCtx);
    }
    if (pIe.hold)
    {
        encode_per(*pIe.hold, pCtx);
    }
    encode_per(pIe.pieceToAddList, pCtx);
    if (pIe.position)
    {
        encode_per(*pIe.position, pCtx);
    }
    encode_per(pIe.linesToReplaceList, pCtx);
    encode_per(pIe.linesToRemoveList, pCtx);
    encode_per(pIe.linesToInsertList, pCtx);
    if (pIe.attackIndicator)
    {
        encode_per(*pIe.attackIndicator, pCtx);
    }
}

inline void decode_per(BoardUpdateNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    decode_per(optionalmask, sizeof(optionalmask), pCtx);
    decode_per(pIe.player, pCtx);
    if (check_optional(optionalmask, 0))
    {
        pIe.rotation = decltype(pIe.rotation)::value_type{};
        decode_per(*pIe.rotation, pCtx);
    }
    if (check_optional(optionalmask, 1))
    {
        pIe.placement = decltype(pIe.placement)::value_type{};
        decode_per(*pIe.placement, pCtx);
    }
    if (check_optional(optionalmask, 2))
    {
        pIe.hold = decltype(pIe.hold)::value_type{};
        decode_per(*pIe.hold, pCtx);
    }
    decode_per(pIe.pieceToAddList, pCtx);
    if (check_optional(optionalmask, 3))
    {
        pIe.position = decltype(pIe.position)::value_type{};
        decode_per(*pIe.position, pCtx);
    }
    decode_per(pIe.linesToReplaceList, pCtx);
    decode_per(pIe.linesToRemoveList, pCtx);
    decode_per(pIe.linesToInsertList, pCtx);
    if (check_optional(optionalmask, 4))
    {
        pIe.attackIndicator = decltype(pIe.attackIndicator)::value_type{};
        decode_per(*pIe.attackIndicator, pCtx);
    }
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
    if (pIe.rotation) nOptional++;
    if (pIe.placement) nOptional++;
    if (pIe.hold) nOptional++;
    if (pIe.position) nOptional++;
    if (pIe.attackIndicator) nOptional++;
    size_t nMandatory = 5;
    str("player", pIe.player, pCtx, !(--nMandatory+nOptional));
    if (pIe.rotation)
    {
        str("rotation", *pIe.rotation, pCtx, !(nMandatory+--nOptional));
    }
    if (pIe.placement)
    {
        str("placement", *pIe.placement, pCtx, !(nMandatory+--nOptional));
    }
    if (pIe.hold)
    {
        str("hold", *pIe.hold, pCtx, !(nMandatory+--nOptional));
    }
    str("pieceToAddList", pIe.pieceToAddList, pCtx, !(--nMandatory+nOptional));
    if (pIe.position)
    {
        str("position", *pIe.position, pCtx, !(nMandatory+--nOptional));
    }
    str("linesToReplaceList", pIe.linesToReplaceList, pCtx, !(--nMandatory+nOptional));
    str("linesToRemoveList", pIe.linesToRemoveList, pCtx, !(--nMandatory+nOptional));
    str("linesToInsertList", pIe.linesToInsertList, pCtx, !(--nMandatory+nOptional));
    if (pIe.attackIndicator)
    {
        str("attackIndicator", *pIe.attackIndicator, pCtx, !(nMandatory+--nOptional));
    }
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

inline void encode_per(const JoinAccept& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.playerId, pCtx);
    encode_per(pIe.boardWidth, pCtx);
    encode_per(pIe.boardHeight, pCtx);
}

inline void decode_per(JoinAccept& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.playerId, pCtx);
    decode_per(pIe.boardWidth, pCtx);
    decode_per(pIe.boardHeight, pCtx);
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
    size_t nMandatory = 3;
    str("playerId", pIe.playerId, pCtx, !(--nMandatory+nOptional));
    str("boardWidth", pIe.boardWidth, pCtx, !(--nMandatory+nOptional));
    str("boardHeight", pIe.boardHeight, pCtx, !(--nMandatory+nOptional));
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

inline void encode_per(const PlayerActionIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.player, pCtx);
    encode_per(pIe.count, pCtx);
    encode_per(pIe.action, pCtx);
}

inline void decode_per(PlayerActionIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.player, pCtx);
    decode_per(pIe.count, pCtx);
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
    size_t nMandatory = 3;
    str("player", pIe.player, pCtx, !(--nMandatory+nOptional));
    str("count", pIe.count, pCtx, !(--nMandatory+nOptional));
    str("action", pIe.action, pCtx, !(--nMandatory+nOptional));
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
    else if (12 == type)
    {
        encode_per(std::get<12>(pIe), pCtx);
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
        pIe = CreateGameAccept();
        decode_per(std::get<1>(pIe), pCtx);
    }
    else if (2 == type)
    {
        pIe = CreateGameReject();
        decode_per(std::get<2>(pIe), pCtx);
    }
    else if (3 == type)
    {
        pIe = GameStartIndication();
        decode_per(std::get<3>(pIe), pCtx);
    }
    else if (4 == type)
    {
        pIe = PieceRequest();
        decode_per(std::get<4>(pIe), pCtx);
    }
    else if (5 == type)
    {
        pIe = PieceResponse();
        decode_per(std::get<5>(pIe), pCtx);
    }
    else if (6 == type)
    {
        pIe = PlayerUpdateNotification();
        decode_per(std::get<6>(pIe), pCtx);
    }
    else if (7 == type)
    {
        pIe = BoardUpdateNotification();
        decode_per(std::get<7>(pIe), pCtx);
    }
    else if (8 == type)
    {
        pIe = GameOverNotification();
        decode_per(std::get<8>(pIe), pCtx);
    }
    else if (9 == type)
    {
        pIe = JoinRequest();
        decode_per(std::get<9>(pIe), pCtx);
    }
    else if (10 == type)
    {
        pIe = JoinAccept();
        decode_per(std::get<10>(pIe), pCtx);
    }
    else if (11 == type)
    {
        pIe = JoinReject();
        decode_per(std::get<11>(pIe), pCtx);
    }
    else if (12 == type)
    {
        pIe = PlayerActionIndication();
        decode_per(std::get<12>(pIe), pCtx);
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
        std::string name = "CreateGameAccept";
        str(name.c_str(), std::get<1>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (2 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "CreateGameReject";
        str(name.c_str(), std::get<2>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (3 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "GameStartIndication";
        str(name.c_str(), std::get<3>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (4 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "PieceRequest";
        str(name.c_str(), std::get<4>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (5 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "PieceResponse";
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
        std::string name = "BoardUpdateNotification";
        str(name.c_str(), std::get<7>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (8 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "GameOverNotification";
        str(name.c_str(), std::get<8>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (9 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "JoinRequest";
        str(name.c_str(), std::get<9>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (10 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "JoinAccept";
        str(name.c_str(), std::get<10>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (11 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "JoinReject";
        str(name.c_str(), std::get<11>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (12 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "PlayerActionIndication";
        str(name.c_str(), std::get<12>(pIe), pCtx, true);
        pCtx += "}";
    }
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

#endif //__CUM_MSG_HPP__
