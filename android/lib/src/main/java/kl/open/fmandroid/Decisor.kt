package kl.open.fmandroid

class Decisor(private val numSamples: Int) {

    private val samples = mutableListOf<MatchResult>()

    fun addSample(sample: MatchResult) {
        samples.add(sample)
    }

    fun reset() {
        samples.clear()
    }

    fun isReady(): Boolean {
        return samples.size >= numSamples
    }

    fun aggregate(): MatchResult {
        if (samples.isEmpty()) {
            return MatchResult.error()
        }

        val validSamples = samples.filter { it.processed && it.referenceIsValid }
        if (validSamples.isEmpty()) {
            return MatchResult.error()
        }

        val liveCount = validSamples.count { it.capturedIsLive }
        val sameSubjectCount = validSamples.count { it.capturedIsLive && it.isSameSubject }

        val majorityValid = (validSamples.size / 2) + 1
        val majorityLive = (liveCount / 2) + 1

        val finalLive = liveCount >= majorityValid
        val finalSameSubject = if (!finalLive) false else sameSubjectCount >= majorityLive
        val capturedPath = validSamples.lastOrNull { it.capturedIsLive && it.isSameSubject }?.capturedPath

        return MatchResult(
            processed = true,
            referenceIsValid = true,
            capturedIsLive = finalLive,
            isSameSubject = finalSameSubject,
            capturedPath = capturedPath
        )
    }
}
