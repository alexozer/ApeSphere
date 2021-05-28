#include "inputdisp.h"

#include <gc/gc.h>
#include <mkb/mkb.h>
#include <cstring>

#include "patch.h"

namespace inputdisp
{

static void (*s_PADRead_tramp)(gc::PADStatus *statuses);

static gc::PADStatus s_raw_inputs[4];

static void draw_ring(u32 pts, Vec2f center, f32 inner_radius, f32 outer_radius, gc::GXColor color)
{
    // "Blank" texture object which seems to let us set a color and draw a poly with it idk??
    gc::GXTexObj *texobj = reinterpret_cast<gc::GXTexObj *>(0x807ad0e0);
    mkb::GXLoadTexObj_cached(texobj, gc::GX_TEXMAP0);
    gc::GXSetTevColor(gc::GX_TEVREG0, color);
    float z = -1.0f / 128.0f;

    gc::GXBegin(gc::GX_QUADS, gc::GX_VTXFMT0, pts * 4);

    for (u32 i = 0; i < pts; i++)
    {
        u16 angle = 0xFFFF * i / pts;
        f32 sin_cos[2];
        mkb::math_sin_cos_v(static_cast<s32>(angle), sin_cos);
        f32 curr_inner_x = sin_cos[0] * inner_radius + center.x;
        f32 curr_inner_y = sin_cos[1] * inner_radius + center.y;
        f32 curr_outer_x = sin_cos[0] * outer_radius + center.x;
        f32 curr_outer_y = sin_cos[1] * outer_radius + center.y;

        // TODO factor this out or something?
        u16 next_angle = 0xFFFF * ((i + 1) % pts) / pts;
        f32 next_sin_cos[2];
        mkb::math_sin_cos_v(static_cast<s32>(next_angle), next_sin_cos);
        f32 next_inner_x = next_sin_cos[0] * inner_radius + center.x;
        f32 next_inner_y = next_sin_cos[1] * inner_radius + center.y;
        f32 next_outer_x = next_sin_cos[0] * outer_radius + center.x;
        f32 next_outer_y = next_sin_cos[1] * outer_radius + center.y;

        gc::GXPosition3f32(next_inner_x, next_inner_y, z);
        gc::GXTexCoord2f32(0, 0);
        gc::GXPosition3f32(next_outer_x, next_outer_y, z);
        gc::GXTexCoord2f32(0, 0);
        gc::GXPosition3f32(curr_outer_x, curr_outer_y, z);
        gc::GXTexCoord2f32(0, 0);
        gc::GXPosition3f32(curr_inner_x, curr_inner_y, z);
        gc::GXTexCoord2f32(0, 0);
    }
}

static void draw_circle(u32 pts, Vec2f center, f32 radius, gc::GXColor color)
{
    // "Blank" texture object which seems to let us set a color and draw a poly with it idk??
    gc::GXTexObj *texobj = reinterpret_cast<gc::GXTexObj *>(0x807ad0e0);
    mkb::GXLoadTexObj_cached(texobj, gc::GX_TEXMAP0);
    gc::GXSetTevColor(gc::GX_TEVREG0, color);
    float z = -1.0f / 128.0f;

    gc::GXBegin(gc::GX_TRIANGLEFAN, gc::GX_VTXFMT0, pts + 2);
    gc::GXPosition3f32(center.x, center.y, z);
    gc::GXTexCoord2f32(0, 0);

    for (s32 i = static_cast<s32>(pts) - 1; i >= -1; i--)
    {
        u16 angle = 0xFFFF * i / pts;
        f32 sin_cos[2];
        mkb::math_sin_cos_v(static_cast<s32>(angle), sin_cos);
        f32 x = sin_cos[0] * radius + center.x;
        f32 y = sin_cos[1] * radius + center.y;
        gc::GXPosition3f32(x, y, z);
        gc::GXTexCoord2f32(0, 0);
    }
}

void init()
{
    // Hook PADRead to give us raw PAD inputs before the game processes them
    s_PADRead_tramp = patch::hook_function(
        gc::PADRead, [](gc::PADStatus *statuses)
        {
            s_PADRead_tramp(statuses);
            memcpy(s_raw_inputs, statuses, sizeof(s_raw_inputs));
        }
    );
}

void tick()
{
}

void disp()
{
    Vec2f center = {100, 100};
    draw_ring(8, center, 54, 60, {0x00, 0x00, 0x00, 0xFF});
    draw_circle(8, center, 54, {0x00, 0x00, 0x00, 0x7F});
    draw_ring(8, center, 50, 58, {0xb1, 0x5a, 0xff, 0xff});

    // Accumulate stick inputs from all controllers since we don't always
    // know which player is active, like in menus
    s32 x = 0, y = 0;
    for (int i = 0; i < 4; i++)
    {
        gc::PADStatus &status = s_raw_inputs[i];
        if (status.err == gc::PAD_ERR_NONE)
        {
            x += status.stickX;
            y += status.stickY;
        }
    }

    Vec2f scaled_input = {
        100.f + static_cast<f32>(x) / 2.8f,
        100.f - static_cast<f32>(y) / 2.8f,
    };

    draw_circle(16, scaled_input, 10, {0xFF, 0xFF, 0xFF, 0xFF});
}

}
