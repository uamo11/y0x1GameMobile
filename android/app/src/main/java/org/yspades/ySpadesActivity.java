package org.yspades;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.os.Bundle;
import android.util.Log;
import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class ySpadesActivity extends SDLActivity {
    private static final String TAG = "ySpadesActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        extractAssetsIfNeeded();
        super.onCreate(savedInstanceState);
    }

    private void extractAssetsIfNeeded() {
        try {
            PackageInfo pInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
            int currentVersionCode = pInfo.versionCode;
            SharedPreferences prefs = getSharedPreferences("yspades_prefs", Context.MODE_PRIVATE);
            int savedVersionCode = prefs.getInt("version_code", -1);

            File targetDir = new File(getFilesDir(), "Resources");
            if (savedVersionCode != currentVersionCode || !targetDir.exists()) {
                Log.i(TAG, "Extracting game assets to " + targetDir.getAbsolutePath());
                targetDir.mkdirs();
                copyAssetFolder("", targetDir);
                prefs.edit().putInt("version_code", currentVersionCode).apply();
                Log.i(TAG, "Game assets extracted successfully.");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error extracting game assets: ", e);
        }
    }

    private void copyAssetFolder(String srcPath, File destDir) throws IOException {
        String[] files = getAssets().list(srcPath);
        if (files == null || files.length == 0) {
            if (!srcPath.isEmpty()) {
                copyAssetFile(srcPath, new File(destDir, new File(srcPath).getName()));
            }
            return;
        }

        for (String file : files) {
            if (srcPath.isEmpty() && (file.equals("webkit") || file.equals("images"))) {
                continue;
            }
            String subSrc = srcPath.isEmpty() ? file : srcPath + "/" + file;
            String[] subFiles = getAssets().list(subSrc);
            if (subFiles != null && subFiles.length > 0) {
                File subDest = new File(destDir, file);
                subDest.mkdirs();
                copyAssetFolder(subSrc, subDest);
            } else {
                copyAssetFile(subSrc, new File(destDir, file));
            }
        }
    }

    private void copyAssetFile(String srcPath, File destFile) throws IOException {
        File parent = destFile.getParentFile();
        if (parent != null && !parent.exists()) {
            parent.mkdirs();
        }
        try (InputStream in = getAssets().open(srcPath);
             OutputStream out = new FileOutputStream(destFile)) {
            byte[] buffer = new byte[16384];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "ZeroSpades"
        };
    }

    @Override
    public void loadLibraries() {
        String[] libs = new String[] { "SDL2", "SDL2_image", "OpenAL", "ZeroSpades" };
        for (String lib : libs) {
            try {
                System.loadLibrary(lib);
                Log.i(TAG, "Loaded native library: " + lib);
            } catch (UnsatisfiedLinkError e) {
                if ("ZeroSpades".equals(lib)) {
                    Log.e(TAG, "Failed to load ZeroSpades: ", e);
                    throw e;
                } else {
                    Log.i(TAG, "Optional library " + lib + " not found (statically linked)");
                }
            }
        }
    }

    @Override
    protected String getMainFunction() {
        return "SDL_main";
    }
}
