package kl.open.fmandroid

object CameraCallbackHolder {
    var referenceResult: ProcessResult? = null
    var decisor: Decisor? = null
    var onFinalResult: ((MatchResult) -> Unit)? = null

    fun reset() {
        referenceResult = null
        decisor = null
        onFinalResult = null
    }
}
