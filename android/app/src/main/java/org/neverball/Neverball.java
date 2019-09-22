package org.neverball;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;

import org.libsdl.app.SDLActivity;

public class Neverball extends SDLActivity
{
    private static boolean fileExists(String path)
    {
        return new File(path).exists();
    }
    
    private static void copyFilesRecursive(AssetManager am, String asset, String filesDir) throws IOException
    {
        String outputPath = filesDir + "/" + asset;
        InputStream input = null;

        if (fileExists(outputPath))
            return;

        try
        {
            input = am.open(asset);
        }
        catch (java.io.FileNotFoundException e)
        {
            // This happens if it's a directory
        }
        
        if (input != null)  // Regular file
        {
            Log.d("Neverball", "copy: " + asset + " -> " + outputPath);
            Files.copy(input, Paths.get(outputPath));
            input.close();
        }
        else  // Directory
        {
            Log.d("Neverball", "mkdir: " + outputPath);
            Files.createDirectory(Paths.get(outputPath));
            for (String file : am.list(asset))
                copyFilesRecursive(am, asset + "/" + file, filesDir);
        }
    }

    // Overridden to copy assets to internal storage.
    // This is a stopgap solution until we implement a way to read directly from
    // assets in the C code as well as supporting compressed ZIP archives like
    // Physfs does.
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        
        String storagePath = getFilesDir().getPath();
        
        Log.d("Neverball", "storagePath = " + storagePath);
        
        // Copy all assets to internal storage
        Log.d("Neverball", "onCreate");

        try
        {
            copyFilesRecursive(getAssets(), "data", storagePath);
        }
        catch (IOException e)
        {
            Log.e("Neverball", "Failed to get asset file list.", e);
        }
    }
}