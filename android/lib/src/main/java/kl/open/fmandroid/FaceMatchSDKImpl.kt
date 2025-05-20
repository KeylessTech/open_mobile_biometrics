
package kl.open.fmandroid

import android.content.Context
import android.content.Intent
import org.json.JSONObject

class FaceMatchSdkImpl(private val context: Context) : FaceMatchSDK {

    companion object {
        init {
            System.loadLibrary("lib")
        }
    }

    override fun init(configJson: String): Boolean {

        val modelBasePath = context.filesDir.absolutePath
        val parsedConfig = JSONObject(configJson)

        val livenessModel0 = parsedConfig.optString("liveness_model0")
        val livenessModel1 = parsedConfig.optString("liveness_model1")
        val faceDetectorModel = parsedConfig.optString("face_detector_model")
        val embeddingModel = parsedConfig.optString("embedding_extractor_model")

        // Copy models from assets to internal storage
        copyAssetIfNeeded(livenessModel0, modelBasePath)
        copyAssetIfNeeded(livenessModel1, modelBasePath)
        copyAssetIfNeeded(faceDetectorModel, modelBasePath)
        copyAssetIfNeeded(embeddingModel, modelBasePath)

        //TODO it should be called only if the library was built in debug mode
        NativeBridge.jni_setDebugSavePath(context.cacheDir.absolutePath)

        return NativeBridge.jni_init(configJson, modelBasePath)
    }

    override fun captureAndMatch(referenceImagePath: String, onResult: (MatchResult) -> Unit) {
        val referenceResult = NativeBridge.jni_process(referenceImagePath, true)

        if (!referenceResult.embeddingExtracted) {
            // Fail fast if reference image is invalid
            onResult(
                MatchResult(
                    processed = true,
                    referenceIsValid = false,
                    capturedIsLive = false,
                    isSameSubject = false,
                    capturedPath = null
                )
            )
            return
        }

        val decisor = Decisor(numSamples = 3)

        CameraCallbackHolder.referenceResult = referenceResult
        CameraCallbackHolder.decisor = decisor
        CameraCallbackHolder.onFinalResult = onResult

        // Aggregation logic on captured frames is demanded to CameraActivity
        val intent = Intent(context, CameraActivity::class.java)
        context.startActivity(intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
    }

    override fun reset() {
        TODO("Not yet implemented")
    }

    private fun copyAssetIfNeeded(assetName: String, destDir: String) {
        val destFile = java.io.File(destDir, assetName)
        if (!destFile.exists()) {
            context.assets.open(assetName).use { input ->
                destFile.outputStream().use { output ->
                    input.copyTo(output)
                }
            }
        }
    }
}


data class ProcessResult(
    val livenessChecked: Boolean,
    val isLive: Boolean,
    val faceDetected: Boolean,
    val embeddingExtracted: Boolean,
    val embedding: FloatArray
)
