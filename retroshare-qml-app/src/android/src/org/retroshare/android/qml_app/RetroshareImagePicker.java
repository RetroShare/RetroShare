package org.retroshare.android.qml_app;

/**
 * Created by Angesoc on 10/07/17.
 */

import org.qtproject.qt5.android.bindings.QtActivity;
import android.content.Intent;
import android.provider.MediaStore;
import android.util.Log;


public class RetroshareImagePicker extends QtActivity{

    public static Intent imagePickerIntent() {

        Log.i("RetroshareImagePicker", "imagePickerIntent()");

        Intent intent = new Intent( Intent.ACTION_PICK, MediaStore.Images.Media.EXTERNAL_CONTENT_URI );
        intent.setType("image/*");
        return Intent.createChooser(intent, "Select Image");
    }
}
