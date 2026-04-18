#ifndef __TETRIS_SINGLETON_HPP__
#define __TETRIS_SINGLETON_HPP__

namespace tetris
{

template <typename T>
struct singleton
{
    static T& get()
    {
        static T instance{};
        return instance;
    }

    static T& instantiate() { return get(); }
};

} // namespace tetris

#endif
