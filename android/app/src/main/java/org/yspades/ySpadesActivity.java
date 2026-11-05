package org.yspades;

import org.libsdl.app.SDLActivity;

public class ySpadesActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "hidapi",
            "SDL2",
            "SDL2_image",
            "ZeroSpades"
        };
    }

    @Override
    protected String getMainFunction() {
        return "SDL_main";
    }
}
