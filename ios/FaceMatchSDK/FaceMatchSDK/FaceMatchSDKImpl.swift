import Foundation
import SwiftUI
import AVFoundation
import UIKit

public class FaceMatchSDKImpl: FaceMatchSDK {
    private let cameraVM = CameraViewModel()
    private var previewVC: CameraPreviewViewController?
    private var isInitialized = false
    private let decisor = Decisor(numSamples: 3)
    
    private func dismissCamera() {
        self.cameraVM.stopSession()
        self.previewVC?.dismiss(animated: true)
        self.previewVC = nil
    }
    
    
    public init() {}

    public func initSDK(configJson: String) -> Bool {
        decisor.reset()
        
        // wire up our debug‐dump folder before we initialize C++
        let docs = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true).first!
        FaceMatchBridge.sharedInstance().setDebugSavePath(docs)
        
        
        if let assetPath = Bundle(for: FaceMatchBridge.self).path(forResource: "assets", ofType: nil) {
            isInitialized = FaceMatchBridge.sharedInstance().initWithConfig(configJson, basePath: assetPath)
        } else {
            isInitialized = false
        }
        return isInitialized
    }

    public func captureAndMatch(referenceImagePath: String, onResult: @escaping (MatchResult) -> Void) {
        guard isInitialized else {
            print("SDK not initialized")
            onResult(matchResultErr())
            return
        }

        let referenceResult = FaceMatchBridge.sharedInstance().processImage(atPath: referenceImagePath, skipLiveness: true)
            
        if(!referenceResult.embeddingExtracted){
            print("Failed to extract embedding from reference")
            onResult(
                MatchResult(
                    processed: true,
                    referenceIsValid: false,
                    capturedIsLive: false,
                    isSameSubject: false,
                    capturedPath: nil
                )
            )
            return
        }

        DispatchQueue.main.async {
            guard let windowScene = UIApplication.shared.connectedScenes
                    .first(where: { $0.activationState == .foregroundActive }) as? UIWindowScene,
                  let window = windowScene.windows.first(where: { $0.isKeyWindow }),
                  let rootVC = window.rootViewController else {
                print("No window/rootVC to present camera from")
                onResult(matchResultErr())
                return
            }

            var topVC = rootVC
            while let presented = topVC.presentedViewController {
                topVC = presented
            }

            let preview = CameraPreviewViewController(session: self.cameraVM.getSession())
            preview.modalPresentationStyle = .fullScreen
            self.previewVC = preview

            topVC.present(preview, animated: true) {
                self.cameraVM.startAndWaitUntilReady {
                    captureLoop()
                }
            }
        }

        func captureLoop() {
            self.cameraVM.capturePhoto { capturedURL in
                guard let capturedURL = capturedURL else {
                    print("Capture failed")
                    self.dismissCamera()
                    onResult(matchResultErr())
                    return
                }

                let capturedResult = FaceMatchBridge.sharedInstance().processImage(atPath: capturedURL.path, skipLiveness: false)
                if (!capturedResult.faceDetected){
                    print("No face detected. Retrying...")
                    captureLoop()
                    return
                }

                let isSame = FaceMatchBridge.sharedInstance().matchEmbedding(referenceResult.embedding, withEmbedding: capturedResult.embedding)

                self.decisor.addSample(
                    MatchResult(
                        processed: true,
                        referenceIsValid: true,
                        capturedIsLive: capturedResult.isLive,
                        isSameSubject: isSame,
                        capturedPath: capturedURL.path
                    )
                )

                if self.decisor.isReady() {
                    self.dismissCamera()
                    let final = self.decisor.aggregate()
                    onResult(final)
                } else {
                    captureLoop()
                }
            }
        }
    }

    public func reset() {
        FaceMatchBridge.sharedInstance().reset()
        decisor.reset()
        cameraVM.stopSession()
    }
}
