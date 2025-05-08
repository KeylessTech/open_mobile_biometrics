import Foundation

public protocol FaceMatchSDK {

    /// Initializes the FaceMatch SDK with the provided configuration.
    ///
    /// - Parameter configJson: A JSON string containing SDK configuration parameters.
    /// - Returns: `true` if the initialization was successful, `false` otherwise.
    ///
    /// You must call this method once before invoking any other SDK function.
    func initSDK(configJson: String) -> Bool
    
    /// Captures a photo using the front-facing camera, performs liveness detection and face matching
    /// against the provided reference image.
    ///
    /// The camera will remain active until a valid face is detected. This method handles all camera
    /// logic internally and provides the result via the callback.
    ///
    /// - Parameters:
    ///   - referenceImagePath: The path to the reference image file.
    ///   - onResult: A closure that receives a `MatchResult` containing the outcome of the operation.
    ///
    /// You must call `initSDK(configJson:)` before invoking this method.
    func captureAndMatch(referenceImagePath: String, onResult: @escaping (MatchResult) -> Void)
    
    /// Resets the internal state of the FaceMatch SDK.
    ///
    /// This method stops any ongoing camera session and clears any temporary data
    /// or cached state held by the SDK. After calling `reset()`, you must re-initialize
    /// the SDK using `initSDK(configJson:)` before using it again.
    func reset()
}


