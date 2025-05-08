import SwiftUI
import FaceMatchSDK

struct ShareableURL: Identifiable {
    let id = UUID()
    let url: URL
}
struct ShareSheet: UIViewControllerRepresentable {
    let items: [Any]

    func makeUIViewController(context: Context) -> UIActivityViewController {
        return UIActivityViewController(activityItems: items, applicationActivities: nil)
    }

    func updateUIViewController(_ uiViewController: UIActivityViewController, context: Context) {}
}

struct ContentView: View {
    @StateObject private var viewModel = ContentViewModel()
    
    @State private var shareURL: ShareableURL? = nil
    
    var body: some View {
        VStack(spacing: 24) {
            
            // Reference section
            VStack(spacing: 8) {
                Text("Reference image")
                    .font(.headline)
                
                if let refImage = viewModel.referenceUIImage {
                    Image(uiImage: refImage)
                        .resizable()
                        .scaledToFit()
                        .frame(height: 300)
                        .cornerRadius(12)
                } else {
                    Rectangle()
                        .fill(Color.gray.opacity(0.2))
                        .frame(height: 300)
                        .cornerRadius(12)
                }

                Button("Take picture") {
                    viewModel.takeReferencePicture()
                }
            }

            // Capture & Match
            Button("Capture and Match") {
                viewModel.captureAndMatch()
            }
            .font(.title2)
            .padding(.top)

            // Results
            VStack(spacing: 12) {
                HStack {
                    Text("Liveness Result:")
                    Spacer()
                    if let live = viewModel.isLive {
                        Text(live ? "Live" : "Spoof")
                            .foregroundColor(live ? .green : .red)
                    }
                }

                HStack {
                    Text("Same Subject:")
                    Spacer()
                    if let match = viewModel.isSameSubject {
                        Text(match ? "True" : "False")
                            .foregroundColor(match ? .green : .red)
                    }
                }
            }
            .padding()
            
            // File paths (debug info) TODO DELME
            if let refPath = viewModel.referenceImagePath {
                Text("Reference: \(refPath)")
                    .font(.footnote)
                    .lineLimit(1)
                    .truncationMode(.middle)

                Button("Share Reference") {
                    shareURL = ShareableURL(url: URL(fileURLWithPath: refPath))
                }
            }

            if let capturedPath = viewModel.capturedImagePath {
                Text("Captured: \(capturedPath)")
                    .font(.footnote)
                    .lineLimit(1)
                    .truncationMode(.middle)

                Button("Share Captured") {
                    shareURL = ShareableURL(url: URL(fileURLWithPath: capturedPath))
                }
            }
        }
        .padding()
        .onAppear {
            viewModel.initializeSDK()
        }
        .sheet(isPresented: $viewModel.isShowingCamera) {
            ImagePicker(sourceType: .camera) { image in
                if let image = image {
                    viewModel.handleImagePickerResult(image)
                }
            }
        }
        .sheet(item: $shareURL) { shareItem in
            ShareSheet(items: [shareItem.url])
        }
        .alert(isPresented: $viewModel.showReferenceAlert) {
            Alert(
                title: Text("Error"),
                message: Text("Unable to extract face from reference image"),
                dismissButton: .default(Text("OK"))
            )
        }
        
    }
}
