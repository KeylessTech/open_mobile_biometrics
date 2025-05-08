import Foundation

/// A structure representing the result of the capture and face matching operation.
public struct MatchResult {
    /// Indicates whether the processing job completed, independently by the result.
    public let processed: Bool

    /// Indicates whether the reference image was valid and usable (e.g., face detected).
    public let referenceIsValid: Bool

    /// Indicates whether the captured image passed the liveness check.
    public let capturedIsLive: Bool

    /// Indicates whether the captured face matches the reference face.
    public let isSameSubject: Bool

    /// The file path to the captured image (if available).
    public let capturedPath: String?
}

public func matchResultErr() -> MatchResult {
    return MatchResult(
        processed: false,
        referenceIsValid: false,
        capturedIsLive: false,
        isSameSubject: false,
        capturedPath: nil
    )
}
