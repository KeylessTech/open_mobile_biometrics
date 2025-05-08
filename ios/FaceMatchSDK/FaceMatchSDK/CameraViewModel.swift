import AVFoundation
import UIKit


extension UIImage {
    func normalized() -> UIImage {
        if imageOrientation == .up {
            return self
        }

        UIGraphicsBeginImageContextWithOptions(size, false, scale)
        draw(in: CGRect(origin: .zero, size: size))
        let normalized = UIGraphicsGetImageFromCurrentImageContext()!
        UIGraphicsEndImageContext()

        return normalized
    }
}

extension CameraViewModel: AVCaptureVideoDataOutputSampleBufferDelegate {
    public func captureOutput(_ output: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer, from connection: AVCaptureConnection) {
        if !hasStartedStreaming {
            hasStartedStreaming = true
            DispatchQueue.main.async {
                self.sessionStartedCallback?()
                self.sessionStartedCallback = nil // only once
            }
        }
    }
}

public class CameraViewModel: NSObject, ObservableObject, AVCapturePhotoCaptureDelegate {
    private let session = AVCaptureSession()
    private let output = AVCapturePhotoOutput()
    private var captureCompletion: ((URL?) -> Void)?
    private var hasStartedStreaming = false
    private var sessionStartedCallback: (() -> Void)?

    public override init() {
        super.init()
        setupSession()
    }

    private func setupSession() {
        session.beginConfiguration()
        session.sessionPreset = .hd1280x720

        guard let device = AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .front),
              let input = try? AVCaptureDeviceInput(device: device),
              session.canAddInput(input),
              session.canAddOutput(output) else {
            return
        }
        
        let videoOutput = AVCaptureVideoDataOutput()
        videoOutput.setSampleBufferDelegate(self, queue: DispatchQueue(label: "VideoOutputQueue"))
        if session.canAddOutput(videoOutput) {
            session.addOutput(videoOutput)
        }

        session.addInput(input)
        session.addOutput(output)
        session.commitConfiguration()
    }

    func startSession() {
        if !session.isRunning {
            DispatchQueue.global(qos: .userInitiated).async {
                self.session.startRunning()
            }
        }
    }

    func stopSession() {
        if session.isRunning {
            DispatchQueue.global(qos: .userInitiated).async {
                self.session.stopRunning()
            }
        }
    }
    
    public func startAndWaitUntilReady(onReady: @escaping () -> Void) {
        hasStartedStreaming = false
        sessionStartedCallback = onReady
        startSession()
    }

    func getSession() -> AVCaptureSession {
        return session
    }

    public func capturePhoto(completion: @escaping (URL?) -> Void) {
        captureCompletion = completion

        let settings = AVCapturePhotoSettings()
        output.capturePhoto(with: settings, delegate: self)
    }

    public func photoOutput(_ output: AVCapturePhotoOutput,
                            didFinishProcessingPhoto photo: AVCapturePhoto,
                            error: Error?) {
        
        if let error = error {
            print("Photo capture error: \(error.localizedDescription)")
            captureCompletion?(nil)
            return
        }
        
        guard let imageData = photo.fileDataRepresentation(),
              let image = UIImage(data: imageData) else {
            print("Failed to get UIImage from photo data")
            captureCompletion?(nil)
            return
        }

        let normalizedImage = image.normalized()

        guard let pngData = normalizedImage.pngData() else {
            print("Failed to create PNG data")
            captureCompletion?(nil)
            return
        }

        let filename = UUID().uuidString + ".png"
        let fileURL = FileManager.default.temporaryDirectory.appendingPathComponent(filename)

        do {
            try pngData.write(to: fileURL)
            captureCompletion?(fileURL)
        } catch {
            print("Error saving photo: \(error)")
            captureCompletion?(nil)
        }
    }
}
