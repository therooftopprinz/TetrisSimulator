#ifndef __SEQUENTIALATTACKER_HPP__
#define __SEQUENTIALATTACKER_HPP__

#include <optional>

#include <tetris_log.hpp>

#include <interface/protocol_export.hpp>

#include <IAttacker.hpp>
#include <ITetrisSimulator.hpp>
#include <PlayerContext.hpp>

namespace tetris
{

class CommonAttacker : public IAttacker
{
public:
    CommonAttacker(const CreateGameRequest& pConfig, std::unordered_map<uint8_t, PlayerContext>& pPlayers, ITetrisSimulator& pSimulator)
        : mConfig(pConfig)
        , mPlayers(pPlayers)
        , mCurrentTarget(pPlayers.end())
        , mSimulator(pSimulator)
    {
        if (0 == mConfig.attackMode.index())
        {
            auto attackMode = std::get<AttackModeEnum>(mConfig.attackMode);
            if (AttackModeEnum::SEQUENTIAL == attackMode)
            {
                mMode = SEQUENTIAL;
            }
        }
    }

    void start()
    {
        if (mRunning || !mPlayers.size())
        {
            return;
        }

        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "CommonAttacker[%p;]: start mode=%u;", this, (unsigned)mMode);

        if (SEQUENTIAL == mMode)
        {
            mCurrentTarget = next(mPlayers.begin());
            if (mPlayers.end() == mCurrentTarget)
            {
                logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
                    "CommonAttacker[%p;]: start cancelled!", this);
                return;
            }

            logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
                "CommonAttacker[%p;]: currentTarget=%u;", this, (unsigned)mCurrentTarget->second.id);
            mCurrentTarget->second.receivedLines = 0;
            mCurrentTarget->second.board->onEvent(board::IncomingAttack{0});
        }

        mRunning = true;
        startTimer();
    }

    void stop()
    {
        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "CommonAttacker[%p;]: stop", this);
        mRunning = false;
        mSimulator.cancelGameTimer(mTimerId);
        mCurrentTarget = mPlayers.end();
    }

    void attack(PlayerContext& pPlayer, uint8_t pLines)
    {
        if (!mRunning)
        {
            return;
        }
        
        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "CommonAttacker[%p;]: attack dmg=%u; src=%u; dst=%u;", this, (unsigned)pLines, (unsigned)pPlayer.id, (unsigned)mCurrentTarget->second.id);

        if (!pLines)
        {
            return;
        }
        auto& player = mCurrentTarget->second;
        if (&pPlayer == &player)
        {
            pPlayer.receivedLines -= pLines;
        }
        else
        {
            player.receivedLines += pLines;
        }

        int8_t lines = player.receivedLines;
        if (lines>0)
        {
            player.board->onEvent(board::IncomingAttack{(uint8_t)lines});
        }
        else
        {
            player.board->onEvent(board::IncomingAttack{0});
        }
    }

private:

    std::unordered_map<uint8_t, PlayerContext>::iterator nextTarget(std::unordered_map<uint8_t, PlayerContext>::iterator pCur)
    {
        while (mPlayers.end() != pCur)
        {
            if (PlayerContext::PLAYING == pCur->second.internalMode)
            {
                return pCur;
            }
            pCur++;
        }
        return pCur;
    }

    void startTimer()
    {
        if (!mRunning)
        {
            return;
        }

        mSimulator.cancelGameTimer(mTimerId);
        auto timediff = std::chrono::nanoseconds(mConfig.attackModeCommon.targetChangeTimeoutMs)*1000*1000;
        mTimerId = mSimulator.scheduleGameTimer(timediff, [this] {
                onTimeout();
            });
    }

    void onTimeout()
    {
        if (!mTimerId)
        {
            return;
        }

        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "CommonAttacker[%p;]: targetChangeTimeout!", this);

        if (SEQUENTIAL == mMode)
        {
            auto& player = mCurrentTarget->second;
            if (player.receivedLines > 0)
            {
                uint8_t lines = player.receivedLines;
                player.board->onEvent(board::Attack{lines});
            }
            player.receivedLines = 0;
            auto prevTarget = mCurrentTarget;
            mCurrentTarget = nextTarget(++mCurrentTarget);

            if (mPlayers.end() == mCurrentTarget)
            {
                mCurrentTarget = nextTarget(mPlayers.begin());
                if (mPlayers.end() == mCurrentTarget || prevTarget == mCurrentTarget)
                {
                    stop();
                    return;
                }
            }

            logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
                "CommonAttacker[%p;]: currentTarget=%u;", this, (unsigned)mCurrentTarget->second.id);
            mCurrentTarget->second.board->onEvent(board::IncomingAttack{0});
        }

        startTimer();
    }

    const CreateGameRequest& mConfig;
    std::unordered_map<uint8_t, PlayerContext>& mPlayers;
    enum AttackMode{NONE, SEQUENTIAL, RANDOM, LEAST, MOST, DIVIDE} mMode = NONE;
    std::unordered_map<uint8_t, PlayerContext>::iterator mCurrentTarget;

    std::optional<GameTimerId> mTimerId;
    bool mRunning = false;

    ITetrisSimulator& mSimulator;
};

} // tetris

#endif // __SEQUENTIALATTACKER_HPP__