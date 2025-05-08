import Foundation

public class Decisor {
    private var samples: [MatchResult] = []
    private var numSamples: Int = 1

    public init(numSamples: Int) {
        self.numSamples = numSamples
    }

    public func addSample(_ result: MatchResult) {
        samples.append(result)
    }
    
    public func reset() {
        samples.removeAll()
    }

    public func isReady() -> Bool {
        return samples.count >= numSamples
    }

    public func aggregate() -> MatchResult {
        guard !samples.isEmpty else {
            return matchResultErr()
        }

        let validSamples = samples.filter { $0.processed && $0.referenceIsValid }
        guard !validSamples.isEmpty else {
            return matchResultErr()
        }

        let liveCount = validSamples.filter { $0.capturedIsLive }.count
        let sameSubjectCount = validSamples.filter { $0.capturedIsLive && $0.isSameSubject }.count
        let majorityValid = (validSamples.count / 2) + 1
        let majorityLive = (liveCount / 2) + 1
        let finalLive = liveCount >= majorityValid
        let finalSameSubject = !finalLive ? false : sameSubjectCount >= majorityLive
        let capturedPath = validSamples.filter { $0.capturedIsLive && $0.isSameSubject }.last?.capturedPath

        return MatchResult(
            processed: true,
            referenceIsValid: true,
            capturedIsLive: finalLive,
            isSameSubject: finalSameSubject,
            capturedPath: capturedPath
        )
    }
}
