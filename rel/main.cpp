#include "patch.h"
#include "assembly.h"
#include "heap.h"
#include "savestate.h"
#include "tetris.h"
#include "draw.h"
#include "timer.h"
#include "iw.h"
#include "pad.h"
#include "menu.h"
#include "jump.h"
#include "scratch.h"

#include <mkb/mkb.h>

#include <cstring>

namespace main
{

static void (*s_draw_debug_text_trampoline)();
static void (*s_process_inputs_trampoline)();

static void perform_assembly_patches()
{
    constexpr u32 offset = 0x600;
    // Inject the run function at the start of the main game loop
    patch::write_branch_bl(reinterpret_cast<void *>(reinterpret_cast<u32>(
                                                        heap::heap_data.main_loop_rel_location) + offset),
                           reinterpret_cast<void *>(start_main_loop_assembly));

    /* Remove OSReport call ``PERF : event is still open for CPU!``
    since it reports every frame, and thus clutters the console */
    // Only needs to be applied to the US version
    patch::write_nop(reinterpret_cast<void *>(0x80033E9C));

    // Nop the conditional that guards `draw_debugtext`, enabling it even when debug mode is disabled
    patch::write_nop(reinterpret_cast<void *>(0x80299f54));

    // Titlescreen patches
    strcpy(reinterpret_cast<char *>(0x8047f4ec), "APESPHERE PRACTICE MOD");
    patch::write_branch(reinterpret_cast<void *>(0x8032ad0c),
                        reinterpret_cast<void *>(main::custom_titlescreen_text_color));
}

static void unlock_everything()
{
    // Don't yet know how to unlock the staff credits game from a fresh save...
    mkb::unlock_info.master_unlocked = true;
    mkb::unlock_info.monkeys = 99;
    mkb::unlock_info.staff_credits_game_unlocked = true;
    mkb::unlock_info.play_points = 99999;
    mkb::unlock_info.newest_play_point_record = 99999;
    mkb::unlock_info.movies = 0x0fff;
    mkb::unlock_info.party_games = 0x0001b600;
    mkb::unlock_info.movies_watched = 0x0fff;
    memset(mkb::cm_unlock_entries, 0xff, sizeof(mkb::cm_unlock_entries));
    memset(mkb::storymode_unlock_entries, 0xff, sizeof(mkb::storymode_unlock_entries));
}

void init()
{
    gc::OSReport("[mod] ApeSphere loaded\n");

    perform_assembly_patches();

    heap::init();
    draw::init();
    Tetris::get_instance().init();
    savestate::init();
    timer::init();
    iw::init();
    scratch::init();

    s_draw_debug_text_trampoline = patch::hook_function(
        mkb::draw_debugtext, []()
        {
            // Drawing hook for UI elements.
            // Gets run at the start of smb2's function which draws debug text windows,
            // which is called at the end of smb2's function which draws the UI in general.

            draw::predraw();
            draw::disp();
            timer::disp();
            iw::disp();
            Tetris::get_instance().disp();
            menu::disp();

            s_draw_debug_text_trampoline();
        });

    s_process_inputs_trampoline = patch::hook_function(
        mkb::process_inputs, []()
        {
            s_process_inputs_trampoline();

            // These run after all controller inputs have been processed on the current frame,
            // to ensure lowest input delay
            pad::tick();
            unlock_everything();
            timer::tick();
            iw::tick();
            savestate::tick();
            menu::tick();
            jump::tick();
            scratch::tick();
        });
}

/*
 * This runs at the very start of the main game loop. Most per-frame code runs after
 * controller inputs have been read and processed however, to ensure the lowest input delay.
 */
void tick()
{
    // Enable debug mode (appears to need to be called every frame)
//    mkb::dipSwitches |= mkb::DIP_DEBUG | mkb::DIP_DISP;
    pad::on_frame_start();
}

}
