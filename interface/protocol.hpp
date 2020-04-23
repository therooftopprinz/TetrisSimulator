// Type:  ('u8', {'type': 'unsigned'})
// Type:  ('u8', {'width': '8'})
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
// Enumeration:  ('Piece', ('L', None))
// Enumeration:  ('Piece', ('L_MIRRORED', None))
// Enumeration:  ('Piece', ('I', None))
// Enumeration:  ('Piece', ('S', None))
// Enumeration:  ('Piece', ('S_MIRRORED', None))
// Enumeration:  ('Piece', ('BOX', None))
// Sequence:  CreateGameRequest ('u8', 'boardWidth')
// Sequence:  CreateGameRequest ('u8', 'boardHeight')
// Sequence:  CreateGameResponse ('u64', 'gameId')
// Sequence:  JoinRequest ('u64', 'gameId')
// Sequence:  Player ('String', 'name')
// Sequence:  Player ('u64', 'id')
// Sequence:  JoinAccept ('u8', 'spare')
// Sequence:  JoinReject ('u8', 'spare')
// Sequence:  PlayerUpdateNotification ('PlayerList', 'players')
// Type:  ('ActionList', {'type': 'Action'})
// Type:  ('ActionList', {'dynamic_array': '256'})
// Sequence:  PlayerActionIndication ('ActionList', 'action')
// Type:  ('Lines', {'type': 'unsigned'})
// Type:  ('Lines', {'width': '8'})
// Type:  ('Lines', {'dynamic_array': '256'})
// Type:  ('LinesDiff', {'type': 'unsigned'})
// Type:  ('LinesDiff', {'width': '8'})
// Type:  ('LinesDiff', {'dynamic_array': '256'})
// Type:  ('PieceList', {'type': 'Piece'})
// Type:  ('PieceList', {'dynamic_array': '256'})
// Type:  ('OptionalPiece', {'type': 'Piece'})
// Type:  ('OptionalPiece', {'optional': ''})
// Sequence:  BoardUpdateNotification ('u8', 'player')
// Sequence:  BoardUpdateNotification ('Piece', 'current')
// Sequence:  BoardUpdateNotification ('optional', 'hold')
// Sequence:  BoardUpdateNotification ('PieceList', 'next')
// Sequence:  BoardUpdateNotification ('u8', 'x')
// Sequence:  BoardUpdateNotification ('u8', 'y')
// Sequence:  BoardUpdateNotification ('u8', 'upShift')
// Sequence:  BoardUpdateNotification ('Lines', 'affectedLines')
// Sequence:  BoardUpdateNotification ('LinesDiff', 'lineDiff')
// Sequence:  GameStartIndication ('u8', 'spare')
// Sequence:  GameStartNotification ('u8', 'spare')
// Sequence:  GameEndNotification ('u8', 'spare')
// Choice:  ('TetrisProtocol', 'CreateGameRequest')
// Choice:  ('TetrisProtocol', 'CreateGameResponse')
// Choice:  ('TetrisProtocol', 'JoinRequest')
// Choice:  ('TetrisProtocol', 'JoinAccept')
// Choice:  ('TetrisProtocol', 'JoinReject')
// Choice:  ('TetrisProtocol', 'GameStartNotification')
// Choice:  ('TetrisProtocol', 'GameEndNotification')
// Choice:  ('TetrisProtocol', 'PlayerUpdateNotification')
// Choice:  ('TetrisProtocol', 'PlayerActionIndication')
// Choice:  ('TetrisProtocol', 'BoardUpdateNotification')
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
    L,
    L_MIRRORED,
    I,
    S,
    S_MIRRORED,
    BOX
};

struct CreateGameRequest
{
    u8 boardWidth;
    u8 boardHeight;
};

struct CreateGameResponse
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

struct JoinAccept
{
    u8 spare;
};

struct JoinReject
{
    u8 spare;
};

struct PlayerUpdateNotification
{
    PlayerList players;
};

using ActionList = cum::vector<Action, 256>;
struct PlayerActionIndication
{
    ActionList action;
};

using Lines = cum::vector<uint8_t, 256>;
using LinesDiff = cum::vector<uint8_t, 256>;
using PieceList = cum::vector<Piece, 256>;
using OptionalPiece = std::optional<Piece>;
struct BoardUpdateNotification
{
    u8 player;
    Piece current;
    optional hold;
    PieceList next;
    u8 x;
    u8 y;
    u8 upShift;
    Lines affectedLines;
    LinesDiff lineDiff;
};

struct GameStartIndication
{
    u8 spare;
};

struct GameStartNotification
{
    u8 spare;
};

struct GameEndNotification
{
    u8 spare;
};

using TetrisProtocol = std::variant<CreateGameRequest,CreateGameResponse,JoinRequest,JoinAccept,JoinReject,GameStartNotification,GameEndNotification,PlayerUpdateNotification,PlayerActionIndication,BoardUpdateNotification>;
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
    if (Piece::L == pIe) pCtx += "\"L\"";
    if (Piece::L_MIRRORED == pIe) pCtx += "\"L_MIRRORED\"";
    if (Piece::I == pIe) pCtx += "\"I\"";
    if (Piece::S == pIe) pCtx += "\"S\"";
    if (Piece::S_MIRRORED == pIe) pCtx += "\"S_MIRRORED\"";
    if (Piece::BOX == pIe) pCtx += "\"BOX\"";
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
}

inline void decode_per(CreateGameRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.boardWidth, pCtx);
    decode_per(pIe.boardHeight, pCtx);
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
    size_t nMandatory = 2;
    str("boardWidth", pIe.boardWidth, pCtx, !(--nMandatory+nOptional));
    str("boardHeight", pIe.boardHeight, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const CreateGameResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.gameId, pCtx);
}

inline void decode_per(CreateGameResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.gameId, pCtx);
}

inline void str(const char* pName, const CreateGameResponse& pIe, std::string& pCtx, bool pIsLast)
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
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(JoinAccept& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
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
    str("spare", pIe.spare, pCtx, !(--nMandatory+nOptional));
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
    encode_per(pIe.action, pCtx);
}

inline void decode_per(PlayerActionIndication& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
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
    size_t nMandatory = 1;
    str("action", pIe.action, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const BoardUpdateNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.player, pCtx);
    encode_per(pIe.current, pCtx);
    encode_per(pIe.hold, pCtx);
    encode_per(pIe.next, pCtx);
    encode_per(pIe.x, pCtx);
    encode_per(pIe.y, pCtx);
    encode_per(pIe.upShift, pCtx);
    encode_per(pIe.affectedLines, pCtx);
    encode_per(pIe.lineDiff, pCtx);
}

inline void decode_per(BoardUpdateNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.player, pCtx);
    decode_per(pIe.current, pCtx);
    decode_per(pIe.hold, pCtx);
    decode_per(pIe.next, pCtx);
    decode_per(pIe.x, pCtx);
    decode_per(pIe.y, pCtx);
    decode_per(pIe.upShift, pCtx);
    decode_per(pIe.affectedLines, pCtx);
    decode_per(pIe.lineDiff, pCtx);
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
    size_t nMandatory = 9;
    str("player", pIe.player, pCtx, !(--nMandatory+nOptional));
    str("current", pIe.current, pCtx, !(--nMandatory+nOptional));
    str("hold", pIe.hold, pCtx, !(--nMandatory+nOptional));
    str("next", pIe.next, pCtx, !(--nMandatory+nOptional));
    str("x", pIe.x, pCtx, !(--nMandatory+nOptional));
    str("y", pIe.y, pCtx, !(--nMandatory+nOptional));
    str("upShift", pIe.upShift, pCtx, !(--nMandatory+nOptional));
    str("affectedLines", pIe.affectedLines, pCtx, !(--nMandatory+nOptional));
    str("lineDiff", pIe.lineDiff, pCtx, !(--nMandatory+nOptional));
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

inline void encode_per(const GameStartNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(GameStartNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
}

inline void str(const char* pName, const GameStartNotification& pIe, std::string& pCtx, bool pIsLast)
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

inline void encode_per(const GameEndNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(GameEndNotification& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
}

inline void str(const char* pName, const GameEndNotification& pIe, std::string& pCtx, bool pIsLast)
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
        pIe = CreateGameResponse();
        decode_per(std::get<1>(pIe), pCtx);
    }
    else if (2 == type)
    {
        pIe = JoinRequest();
        decode_per(std::get<2>(pIe), pCtx);
    }
    else if (3 == type)
    {
        pIe = JoinAccept();
        decode_per(std::get<3>(pIe), pCtx);
    }
    else if (4 == type)
    {
        pIe = JoinReject();
        decode_per(std::get<4>(pIe), pCtx);
    }
    else if (5 == type)
    {
        pIe = GameStartNotification();
        decode_per(std::get<5>(pIe), pCtx);
    }
    else if (6 == type)
    {
        pIe = GameEndNotification();
        decode_per(std::get<6>(pIe), pCtx);
    }
    else if (7 == type)
    {
        pIe = PlayerUpdateNotification();
        decode_per(std::get<7>(pIe), pCtx);
    }
    else if (8 == type)
    {
        pIe = PlayerActionIndication();
        decode_per(std::get<8>(pIe), pCtx);
    }
    else if (9 == type)
    {
        pIe = BoardUpdateNotification();
        decode_per(std::get<9>(pIe), pCtx);
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
        std::string name = "CreateGameResponse";
        str(name.c_str(), std::get<1>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (2 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "JoinRequest";
        str(name.c_str(), std::get<2>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (3 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "JoinAccept";
        str(name.c_str(), std::get<3>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (4 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "JoinReject";
        str(name.c_str(), std::get<4>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (5 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "GameStartNotification";
        str(name.c_str(), std::get<5>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (6 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "GameEndNotification";
        str(name.c_str(), std::get<6>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (7 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "PlayerUpdateNotification";
        str(name.c_str(), std::get<7>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (8 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "PlayerActionIndication";
        str(name.c_str(), std::get<8>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (9 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "BoardUpdateNotification";
        str(name.c_str(), std::get<9>(pIe), pCtx, true);
        pCtx += "}";
    }
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

#endif //__CUM_MSG_HPP__
