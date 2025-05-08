package kl.open.fmandroid

import android.Manifest
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Matrix
import android.os.Bundle
import android.util.Size
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.*
import androidx.camera.core.resolutionselector.ResolutionSelector
import androidx.camera.core.resolutionselector.ResolutionStrategy
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.content.ContextCompat
import java.io.File

class CameraActivity : AppCompatActivity() {

    private lateinit var imageCapture: ImageCapture
    private lateinit var previewView: PreviewView

    private val requestPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted: Boolean ->
        if (isGranted) {
            startCamera()
        } else {
            Toast.makeText(this, "Camera permission denied", Toast.LENGTH_SHORT).show()
            CameraCallbackHolder.onFinalResult?.invoke(MatchResult.error())
            finish()
        }
    }

    private fun imageProxyToBitmap(imageProxy: ImageProxy): Bitmap? {
        val planeProxy = imageProxy.planes[0]
        val buffer = planeProxy.buffer
        val bytes = ByteArray(buffer.remaining())
        buffer.get(bytes)
        return BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        previewView = PreviewView(this)
        previewView.scaleX = -1f
        setContentView(previewView)

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
            == PackageManager.PERMISSION_GRANTED) {
            startCamera()
        } else {
            requestPermissionLauncher.launch(Manifest.permission.CAMERA)
        }
    }

    private fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)

        cameraProviderFuture.addListener({
            val cameraProvider = cameraProviderFuture.get()

            val preview = Preview.Builder().build().also {
                it.setSurfaceProvider(previewView.surfaceProvider)
            }

            val resolutionSelector = ResolutionSelector.Builder()
                .setResolutionStrategy(
                    ResolutionStrategy(
                        Size(1280, 720),
                        ResolutionStrategy.FALLBACK_RULE_CLOSEST_LOWER_THEN_HIGHER
                    )
                )
                .build()

            imageCapture = ImageCapture.Builder()
                .setResolutionSelector(resolutionSelector)
                .build()

            val cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA

            cameraProvider.unbindAll()
            cameraProvider.bindToLifecycle(this, cameraSelector, preview, imageCapture)

            // Start the first frame capture
            captureFrame()

        }, ContextCompat.getMainExecutor(this))
    }

    private fun captureFrame() {
        val finalFile = File(getExternalFilesDir(null), "captured_frame_${System.currentTimeMillis()}.png")

        val imageCaptureCallback = object : ImageCapture.OnImageCapturedCallback() {
            override fun onCaptureSuccess(imageProxy: ImageProxy) {
                // Convert raw ImageProxy to Bitmap
                val bitmap = imageProxyToBitmap(imageProxy)
                // Read the correct rotation from the ImageProxy metadata
                val rotationDegrees = imageProxy.imageInfo.rotationDegrees.toFloat()
                // Close the ImageProxy early to free camera resources
                imageProxy.close()

                // If we got a valid Bitmap, rotate and save as PNG
                if (bitmap != null) {
                    // Rotate the bitmap to upright orientation
                    val matrix = Matrix().apply { if (rotationDegrees != 0f) postRotate(rotationDegrees) }
                    val rotatedBitmap = Bitmap.createBitmap(
                        bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true
                    )

                    // Save as lossless PNG
                    finalFile.outputStream().use { out ->
                        rotatedBitmap.compress(Bitmap.CompressFormat.PNG, 100, out)
                    }

                    // Pass the saved file path into the processing pipeline
                    processCapturedImage(finalFile.absolutePath)
                } else {
                    // On error, report a failure result and clean up
                    CameraCallbackHolder.onFinalResult?.invoke(MatchResult.error())
                    CameraCallbackHolder.reset()
                    finish()
                }
            }

            override fun onError(exception: ImageCaptureException) {
                CameraCallbackHolder.onFinalResult?.invoke(MatchResult.error())
                CameraCallbackHolder.reset()
                finish()
            }
        }

        imageCapture.takePicture(
            ContextCompat.getMainExecutor(this),
            imageCaptureCallback
        )
    }


    private fun processCapturedImage(imagePath: String) {
        try {
            val capturedResult = NativeBridge.jni_process(imagePath, false)

            if (capturedResult.faceDetected) {
                val isSameSubject = CameraCallbackHolder.referenceResult?.embedding?.let {
                    NativeBridge.jni_match(it, capturedResult.embedding)
                } ?: false

                val sample = MatchResult(
                    processed = true,
                    referenceIsValid = true,
                    capturedIsLive = capturedResult.isLive,
                    isSameSubject = isSameSubject,
                    capturedPath = imagePath
                )

                CameraCallbackHolder.decisor?.addSample(sample)

                if (CameraCallbackHolder.decisor?.isReady() == true) {
                    finishCapture()
                } else {
                    captureFrame()
                }

            } else {
                // No face detected, just retry another capture
                captureFrame()
            }
        } catch (e: Exception) {
            CameraCallbackHolder.onFinalResult?.invoke(MatchResult.error())
            CameraCallbackHolder.reset()
            finish()
        }
    }

    private fun finishCapture() {
        val finalResult = CameraCallbackHolder.decisor?.aggregate() ?: MatchResult.error()
        CameraCallbackHolder.onFinalResult?.invoke(finalResult)
        CameraCallbackHolder.reset()
        finish()
    }
}
