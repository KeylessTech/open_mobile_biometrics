import UIKit
import AVFoundation


func dismissPresentedViewController() {
    DispatchQueue.main.async {
        if let windowScene = UIApplication.shared.connectedScenes
            .first(where: { $0.activationState == .foregroundActive }) as? UIWindowScene,
           let rootVC = windowScene.windows.first(where: { $0.isKeyWindow })?.rootViewController,
           let presented = rootVC.presentedViewController {
            presented.dismiss(animated: true, completion: nil)
        }
    }
}

class CameraPreviewViewController: UIViewController {
    private let session: AVCaptureSession

    init(session: AVCaptureSession) {
        self.session = session
        super.init(nibName: nil, bundle: nil)
    }

    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }

    override func viewDidLoad() {
        super.viewDidLoad()
        view.backgroundColor = .black

        let previewLayer = AVCaptureVideoPreviewLayer(session: session)
        previewLayer.videoGravity = .resizeAspectFill
        previewLayer.frame = view.bounds
        view.layer.addSublayer(previewLayer)
    }

    override func viewWillLayoutSubviews() {
        super.viewWillLayoutSubviews()
        if let layer = view.layer.sublayers?.first as? AVCaptureVideoPreviewLayer {
            layer.frame = view.bounds
        }
    }
}
