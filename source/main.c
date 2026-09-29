
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC 0x31505741u /* AWP1 */
#define SCREEN_W 400
#define SCREEN_H 240
#define PIXEL_BYTES 3

typedef struct {
    u32 magic;
    u16 width;
    u16 height;
    u16 fps_x100;
    u16 reserved;
    u32 frame_count;
    u32 frame_bytes;
} __attribute__((packed)) AwpHeader;

static void draw_frame_top(const u8 *src, int srcW, int srcH) {
    u16 fbW, fbH;
    u8 *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &fbW, &fbH);
    if (!fb) return;

    // 3DS framebuffer is rotated. src is packed BGR8, row-major, 400x240.
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            const u8 *p = src + (y * srcW + x) * 3;
            u8 *d = fb + ((SCREEN_H - 1 - y) + x * SCREEN_H) * 3;
            d[0] = p[0];
            d[1] = p[1];
            d[2] = p[2];
        }
    }
}

int main(int argc, char **argv) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);

    printf("AnimatedWallpaper3DS playback test\n");
    printf("A: pause/resume   START: exit\n\n");

    FILE *f = fopen("sdmc:/animatedwallpaper/wallpaper.awp", "rb");
    if (!f) {
        printf("Missing:\n/animatedwallpaper/wallpaper.awp\n");
        printf("\nRun the packer on your PC first.\n");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        gfxExit();
        return 0;
    }

    AwpHeader h;
    if (fread(&h, 1, sizeof(h), f) != sizeof(h) ||
        h.magic != MAGIC ||
        h.width != SCREEN_W ||
        h.height != SCREEN_H ||
        h.frame_bytes != SCREEN_W * SCREEN_H * PIXEL_BYTES ||
        h.frame_count == 0) {
        printf("Invalid AWP file.\n");
        fclose(f);
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        gfxExit();
        return 0;
    }

    u8 *frame = (u8*)linearAlloc(h.frame_bytes);
    if (!frame) {
        printf("Not enough memory.\n");
        fclose(f);
        gfxExit();
        return 0;
    }

    const u64 frameTicks = (u64)(268123480.0 * (100.0 / (double)h.fps_x100));
    u64 nextTick = svcGetSystemTick();
    u32 idx = 0;
    bool paused = false;

    printf("%lu frames @ %.2f FPS\n", (unsigned long)h.frame_count, h.fps_x100 / 100.0f);

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        if (down & KEY_START) break;
        if (down & KEY_A) paused = !paused;

        u64 now = svcGetSystemTick();
        bool drewFrame = false;

        if (!paused && now >= nextTick) {
            long off = (long)sizeof(h) + (long)idx * (long)h.frame_bytes;
            if (fseek(f, off, SEEK_SET) != 0 ||
                fread(frame, 1, h.frame_bytes, f) != h.frame_bytes) {
                printf("\nRead error at frame %lu\n", (unsigned long)idx);
                break;
            }

            draw_frame_top(frame, h.width, h.height);

            idx++;
            if (idx >= h.frame_count) idx = 0;
            nextTick = now + frameTicks;
            drewFrame = true;
        }

        // Only present a new top-screen buffer when we actually drew a new GIF frame.
        // Swapping every VBlank while drawing at 10 FPS causes the two buffers to
        // alternate between new/stale images, which looks like intense flashing.
        if (drewFrame) {
            gfxFlushBuffers();
            gfxSwapBuffers();
        }

        gspWaitForVBlank();
    }

    linearFree(frame);
    fclose(f);
    gfxExit();
    return 0;
}
