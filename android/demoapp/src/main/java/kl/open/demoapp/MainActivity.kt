package kl.open.demoapp

import android.content.Context
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Color
import android.graphics.Matrix
import android.media.ExifInterface
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.FileProvider
import kl.open.fmandroid.FaceMatchSdkImpl
import java.io.File
import java.io.FileOutputStream
import androidx.core.graphics.scale
import androidx.core.graphics.toColorInt


class MainActivity : ComponentActivity() {

    private lateinit var faceMatchSdk: FaceMatchSdkImpl
    private lateinit var referenceImageView: ImageView
    private var referenceImageFile: File? = null
    private var tempCaptureUri: Uri? = null
    private lateinit var requestCameraPermissionLauncher: ActivityResultLauncher<String>
    private lateinit var takePictureLauncher: ActivityResultLauncher<Uri>

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val progressBar = findViewById<ProgressBar>(R.id.progressBar)

        requestCameraPermissionLauncher = registerForActivityResult(
            ActivityResultContracts.RequestPermission()
        ) { isGranted ->
            if (isGranted) {
                launchTakePicture()
            } else {
                Toast.makeText(this, "Camera permission is required", Toast.LENGTH_LONG).show()
            }
        }

        faceMatchSdk = FaceMatchSdkImpl(applicationContext)
        val configJson = loadAsset("config.json")
        faceMatchSdk.init(configJson)

        referenceImageView = findViewById(R.id.referenceImageView)
        val takeReferenceButton = findViewById<Button>(R.id.takeReferenceButton)
        val captureMatchButton = findViewById<Button>(R.id.captureMatchButton)
        captureMatchButton.isEnabled = false

        takePictureLauncher = registerForActivityResult(ActivityResultContracts.TakePicture()) { success ->
            if (success && tempCaptureUri != null) {
                val pngFile = File(getExternalFilesDir(null), "reference_image.png")
                val rotatedBitmap = correctBitmapOrientation(tempCaptureUri!!)
                val resizedBitmap = resizeBitmapMaintainingAspectRatio(rotatedBitmap, 1280, 1280)
                saveAsPng(resizedBitmap, pngFile)
                referenceImageFile = pngFile
                referenceImageView.setImageBitmap(rotatedBitmap)
                captureMatchButton.isEnabled = true
            }
        }

        takeReferenceButton.setOnClickListener {
            if (checkSelfPermission(android.Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
                launchTakePicture()
            } else {
                requestCameraPermissionLauncher.launch(android.Manifest.permission.CAMERA)
            }
        }


        captureMatchButton.setOnClickListener {
            val referenceImage = File(getExternalFilesDir(null), "reference_image.png")

            // Disable button and show spinner
            captureMatchButton.isEnabled = false
            progressBar.visibility = View.VISIBLE
            val livenessResultLabel = findViewById<TextView>(R.id.livenessValue)
            val sameSubjectLabel = findViewById<TextView>(R.id.subjectValue)

            faceMatchSdk.captureAndMatch(referenceImage.absolutePath) { result ->
                runOnUiThread {
                    Log.d("FaceMatch", "Result: $result")

                    // Re-enable button and hide spinner
                    captureMatchButton.isEnabled = true
                    progressBar.visibility = View.GONE

                    if (!result.processed) {
                        livenessResultLabel.text = "Error"
                        livenessResultLabel.setTextColor(Color.GRAY)
                        sameSubjectLabel.text = "Error"
                        sameSubjectLabel.setTextColor(Color.GRAY)
                        return@runOnUiThread
                    }

                    if (!result.referenceIsValid) {
                        Toast.makeText(this, "Unable to extract face from reference image", Toast.LENGTH_LONG).show()
                    }

                    if (!result.capturedIsLive) {
                        livenessResultLabel.text = "Spoof"
                        livenessResultLabel.setTextColor("red".toColorInt())
                        sameSubjectLabel.text = "Not Available"
                        sameSubjectLabel.setTextColor("fuchsia".toColorInt())
                    } else {
                        livenessResultLabel.text = "Live"
                        livenessResultLabel.setTextColor("green".toColorInt())
                        sameSubjectLabel.text = if (result.isSameSubject) "True" else "False"
                        sameSubjectLabel.setTextColor(
                            if (result.isSameSubject) "green".toColorInt() else "red".toColorInt()
                        )
                    }
                }
            }

        }

    }

    private fun launchTakePicture() {
        val tempFile = File(getExternalFilesDir(null), "temp_capture.jpg")
        val uri = FileProvider.getUriForFile(this, "$packageName.provider", tempFile)
        tempCaptureUri = uri
        takePictureLauncher.launch(uri)
    }

    private fun loadAsset(name: String): String {
        return assets.open(name).bufferedReader().use { it.readText() }
    }

    private fun saveAsPng(bitmap: Bitmap, outputFile: File) {
        FileOutputStream(outputFile).use { out ->
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, out)
        }
    }

    private fun correctBitmapOrientation(imageUri: Uri): Bitmap {
        val inputStream = this.contentResolver.openInputStream(imageUri)
        val originalBitmap = BitmapFactory.decodeStream(inputStream)
        inputStream?.close()

        val exif = ExifInterface(this.contentResolver.openInputStream(imageUri)!!)
        val orientation = exif.getAttributeInt(
            ExifInterface.TAG_ORIENTATION,
            ExifInterface.ORIENTATION_NORMAL
        )

        val matrix = Matrix()
        when (orientation) {
            ExifInterface.ORIENTATION_ROTATE_90 -> matrix.postRotate(90f)
            ExifInterface.ORIENTATION_ROTATE_180 -> matrix.postRotate(180f)
            ExifInterface.ORIENTATION_ROTATE_270 -> matrix.postRotate(270f)
        }

        return Bitmap.createBitmap(
            originalBitmap, 0, 0,
            originalBitmap.width, originalBitmap.height,
            matrix, true
        )
    }

    private fun resizeBitmapMaintainingAspectRatio(bitmap: Bitmap, maxWidth: Int, maxHeight: Int): Bitmap {
        val width = bitmap.width
        val height = bitmap.height

        val aspectRatio = width.toFloat() / height.toFloat()

        val targetWidth: Int
        val targetHeight: Int

        if (aspectRatio > 1) {
            // landscape
            targetWidth = maxWidth
            targetHeight = (maxWidth / aspectRatio).toInt()
        } else {
            // portrait
            targetHeight = maxHeight
            targetWidth = (maxHeight * aspectRatio).toInt()
        }

        return bitmap.scale(targetWidth, targetHeight)
    }

}

