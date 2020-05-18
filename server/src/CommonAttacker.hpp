#ifndef __SEQUENTIALATTACKER_HPP__
#define __SEQUENTIALATTACKER_HPP__

#include <interface/protocol.hpp>

#include <IAttacker.hpp>
#include <IExecutor.hpp>
#include <PlayerContext.hpp>

namespace tetris
{

class CommonAttacker : public IAttacker
{
public:
    CommonAttacker(IExecutor& pExecutor, const CreateGameRequest& pConfig, std::unordered_map<uint8_t, PlayerContext>& pPlayers, bfc::ThreadPool<>& pTp, bfc::Timer<>& pTimer)
        : mExecutor(pExecutor)
        , mConfig(pConfig)
        , mPlayers(pPlayers)
        , mTp(pTp)
        , mTimer(pTimer)
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

        if (SEQUENTIAL == mMode)
        {
            mCurrentTarget = next(mPlayers.begin());
            if (mPlayers.end() == mCurrentTarget)
            {
                return;
            }
            mCurrentTarget->second.receivedLines = 0;
        }

        mRunning = true;
        startTimer();
    }

    void stop()
    {
        mRunning = false;
        mTimerId = -1;
    }

    void attack(PlayerContext& pPlayer, uint8_t pLines)
    {
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
            if (PlayerContext::ACTIVE == pCur->second.playState)
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

        mTimer.cancel(mTimerId);
        auto timediff = std::chrono::nanoseconds(mConfig.attackModeCommon.targetChangeTimeoutMs)*1000*1000;
        mTimerId = mTimer.schedule(timediff, [this] {
                bfc::LightFn<void()> fn = [this]() -> void {onTimeout();};
                mExecutor.trigger(std::move(fn));
            });
    }

    void onTimeout()
    {
        if (mTimerId<0)
        {
            return;
        }

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
            mCurrentTarget = nextTarget(mCurrentTarget++);

            if (mPlayers.end() == mCurrentTarget)
            {
                mCurrentTarget = nextTarget(mPlayers.begin());
                if (mPlayers.end() == mCurrentTarget || prevTarget == mCurrentTarget)
                {
                    stop();
                }
            }
        }

        startTimer();
    }

    IExecutor& mExecutor;

    const CreateGameRequest& mConfig;
    std::unordered_map<uint8_t, PlayerContext>& mPlayers;
    enum AttackMode{NONE, SEQUENTIAL, RANDOM, LEAST, MOST, DIVIDE} mMode = NONE;
    std::unordered_map<uint8_t, PlayerContext>::iterator mCurrentTarget;

    int mTimerId = -1;
    bool mRunning = true;

    bfc::ThreadPool<>& mTp;
    bfc::Timer<>& mTimer;
};

} // tetris

#endif // __SEQUENTIALATTACKER_HPP__