import SwiftUI
import FaceMatchSDK


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

class ContentViewModel: ObservableObject {
    @Published var referenceUIImage: UIImage?
    @Published var capturedImagePath: String?
    @Published var isLive: Bool?
    @Published var isSameSubject: Bool?
    @Published var showReferenceAlert: Bool = false
    @Published var isShowingCamera: Bool = false

    public var referenceImagePath: String?
    private let sdk: FaceMatchSDK = FaceMatchSDKImpl()

    func initializeSDK() {
        guard let configUrl = Bundle.main.url(forResource: "config", withExtension: "json"),
              let configData = try? Data(contentsOf: configUrl),
              let configString = String(data: configData, encoding: .utf8) else {
            print("Failed to load config.json")
            return
        }

        let success = sdk.initSDK(configJson: configString)
        print(success ? "SDK Initialized" : "SDK Init Failed")
    }

    func takeReferencePicture() {
        isShowingCamera = true
    }

    func handleImagePickerResult(_ image: UIImage) {
        isShowingCamera = false
        // normalize the image so it’s saved upright
        let normalizedImage = image.normalized()

        guard let data = normalizedImage.pngData() else {
            print("Failed to get PNG data")
            return
        }

        let filename = UUID().uuidString + ".png"
        let url = FileManager.default.temporaryDirectory.appendingPathComponent(filename)

        do {
            try data.write(to: url)
            self.referenceImagePath = url.path
            self.referenceUIImage = normalizedImage
            print("Saved reference image at \(url.path)")
        } catch {
            print("Failed to save image: \(error)")
        }
    }

    func captureAndMatch() {
        guard let refPath = referenceImagePath else {
            print("No reference image to match against")
            return
        }

        sdk.captureAndMatch(referenceImagePath: refPath) { matchResult in
            DispatchQueue.main.async {
                self.isLive = matchResult.capturedIsLive
                self.isSameSubject = matchResult.isSameSubject
                self.showReferenceAlert = matchResult.processed ? !matchResult.referenceIsValid : false
                self.capturedImagePath = matchResult.capturedPath
            }
        }
    }
}
