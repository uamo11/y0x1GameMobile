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
        setRequestedOrientation(android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P) {
            getWindow().getAttributes().layoutInDisplayCutoutMode =
                android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }
        getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        hideSystemUI();
        extractAssetsIfNeeded();
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideSystemUI();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            hideSystemUI();
        }
    }

    private void hideSystemUI() {
        try {
            android.view.View decorView = getWindow().getDecorView();
            decorView.setSystemUiVisibility(
                android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | android.view.View.SYSTEM_UI_FLAG_FULLSCREEN
            );
        } catch (Exception e) {
            Log.e(TAG, "Error hiding system UI: ", e);
        }
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
            "c++_shared",
            "zerospades"
        };
    }

    @Override
    protected String getMainSharedObject() {
        return getContext().getApplicationInfo().nativeLibraryDir + "/libzerospades.so";
    }

    @Override
    public void loadLibraries() {
        String[] libs = new String[] { "c++_shared", "zerospades" };
        for (String lib : libs) {
            try {
                System.loadLibrary(lib);
                Log.i(TAG, "Loaded native library: " + lib);
            } catch (UnsatisfiedLinkError e) {
                if ("c++_shared".equals(lib)) {
                    Log.i(TAG, "Optional library " + lib + " not found or already loaded");
                } else {
                    Log.e(TAG, "Failed to load " + lib + ": ", e);
                    throw e;
                }
            }
        }
    }

    @Override
    protected String getMainFunction() {
        return "SDL_main";
    }

    @Override
    protected String[] getArguments() {
        android.content.Intent intent = getIntent();
        if (intent != null) {
            if (intent.getData() != null) {
                String uri = intent.getData().toString();
                if (uri.startsWith("aos://")) {
                    Log.i(TAG, "Starting with intent data URI: " + uri);
                    return new String[] { uri };
                }
            }
            String[] customArgs = intent.getStringArrayExtra("args");
            if (customArgs != null && customArgs.length > 0) {
                Log.i(TAG, "Starting with intent custom args: " + java.util.Arrays.toString(customArgs));
                return customArgs;
            }
            String connect = intent.getStringExtra("connect");
            if (connect != null && !connect.isEmpty()) {
                Log.i(TAG, "Starting with connect extra: " + connect);
                return new String[] { connect };
            }
            String map = intent.getStringExtra("map");
            if (map != null && !map.isEmpty()) {
                Log.i(TAG, "Starting with map extra: " + map);
                return new String[] { "--map", map };
            }
        }
        return new String[0];
    }
}
