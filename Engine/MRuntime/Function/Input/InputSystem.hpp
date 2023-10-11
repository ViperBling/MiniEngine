#pragma once

namespace MiniEngine
{
    enum class GameCommand : unsigned int
    {
        Forward  = 1 << 0,                 // W
        Backward = 1 << 1,                 // S
        Left     = 1 << 2,                 // A
        Right    = 1 << 3,                 // D
        Jump     = 1 << 4,                 // SPACE
        Squat    = 1 << 5,                 // not implemented yet
        Sprint   = 1 << 6,                 // LEFT SHIFT
        Fire     = 1 << 7,                 // not implemented yet
        FreeCarema = 1 << 8,              // F
        Invalid  = (unsigned int)(1 << 31) // lost focus
    };

    extern unsigned int kComplementControlCommand;

    class InputSystem
    {
    public:
        void OnKey(int key, int scancode, int action, int mods);
        void OnCursorPos(double current_cursor_x, double current_cursor_y);

        void Initialize();
        void Tick();
        void Clear();

        void         ResetGameCommand() { mGameCommand = 0; }
        unsigned int GetGameCommand() const { return mGameCommand; }

    public:
        int mCursorDeltaX {0};
        int mCursorDeltaY {0};

    private:
        void onKeyInEditMode(int key, int scancode, int action, int mods);

        unsigned int mGameCommand {0};

        int mLastCursorX {0};
        int mLastCursorY {0};
    };
} // namespace MiniEngine
