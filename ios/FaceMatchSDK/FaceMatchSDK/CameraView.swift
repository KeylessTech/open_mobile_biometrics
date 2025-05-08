import SwiftUI
import AVFoundation

public struct CameraView: UIViewRepresentable {
    var viewModel: CameraViewModel
    
    public init(viewModel: CameraViewModel) {
            self.viewModel = viewModel
        }

    public func makeUIView(context: Context) -> UIView {
        let view = UIView()
        let previewLayer = AVCaptureVideoPreviewLayer(session: viewModel.getSession())
        previewLayer.videoGravity = .resizeAspectFill
        previewLayer.frame = view.bounds
        view.layer.addSublayer(previewLayer)

        return view
    }

    public func updateUIView(_ uiView: UIView, context: Context) {
        // do nothing
    }
}
