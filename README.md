# FaceMatch Open Source SDK

A cross-platform face recognition & liveness detection SDK with ready-to-use integrations for native C++, Android and iOS.

---

## Features

- **End-to-end face pipeline**  
  - Face detection (MediaPipe/ONNX)  
  - Liveness scoring (Silentface/ONNX)  
  - Embedding extraction (AuraFace/ONNX)  
  - Embedding matching (cosine similarity)  
- **Multi-sample aggregation**  
  - Collect _N_ live samples, majority-vote on liveness & matching  
- **Mobile SDKs**  
  - Android: single `.aar` + camera integration  
  - iOS: `.framework` + SwiftUI/Obj-C bridge  
- **CLI tools**  
  - `fmcore_test` (desktop pipeline)
- **Demo Apps**  
  - Android & iOS sample apps  

---

## Components

| Component                  | Language     | Description                                   |
| -------------------------- | ------------ | --------------------------------------------- |
| **fmcore**                 | C++          | Core pipeline library                         |
| **fmcore_test**            | C++          | Native desktop demo                           |
| **android/lib**            | Kotlin/JNI   | Android SDK + camera & JNI bridge             |
| **ios/FatchMatchSDK**      | Swift/Obj-C  | iOS SDK + camera & Obj-C bridge               |
| **android/demoapp**        | Kotlin       | Sample Android app                            |
| **ios/FaceMatchSDKDemoApp**| SwiftUI      | Sample iOS app                                |

---

## Prerequisites

- **Native**: CMake ≥3.18, C++17 toolchain (macOS/Linux)  
- **Android**: Android NDK r21+, SDK Platform 21+, Java JDK  
- **iOS**: Xcode 13+, Swift 5, iOS 12+ deployment target  


Download your ONNX models here:
https://drive.google.com/drive/folders/1gnDmBuRA2Ad_pDGSzVlMDnYgcCukUDvd?usp=sharing

Place your ONNX models under:  
- **Native**: `fmcore/models/`  
- **Android**: `android/lib/src/main/assets/`  
- **iOS**: `ios/FaceMatchSDK/assets/`

---

## Build & Installation

### Build all targets (native, android, ios)
```bash
./build-all.sh
```
### or build specific:
```bash
./build-all.sh --native
./build-all.sh --android
./build-all.sh --ios
```

Built artifacts will be placed in:
•    build-native/ → native libraries & CLI tools
•    build-android/ → fmandroid.aar
•    build-ios/ → FaceMatchSDK.framework


## Integration Guides


## For all platforms

You will need to pass a path of a configuration json file to initiliaze the sdk.
The `config.json` is like this:

```
{
  "face_detector_model": "mediapipe_long.onnx",
  "embedding_extractor_model": "glintr100.onnx",
  "liveness_model": "liveness.onnx",
  "liveness_threshold": 0.5,
  "matching_threshold": 0.6
}
```
Put it somewhere where it is accessible from your application (needs to be stored on the filesystem, so on Android if you simply put it into `raw` won't work, you then need to copy it to the filesystem to get a working path).


## Android

0. Download the last lib.aar release from github


1.    Add the AAR
```
// settings.gradle
repositories { flatDir { dirs 'libs' } }
// app/build.gradle
dependencies { implementation name: 'fmandroid', ext:'aar' }
```

2.    Initialize & Use

(This assumes you have a `config.json` file stored in `assets` of your app)

```
val sdk: FaceMatchSDK = FaceMatchSdkImpl(context)
val configJson = context.assets.open("config.json").bufferedReader().use { it.readText() }
check(sdk.init(configJson))

sdk.captureAndMatch(referencePath) { result ->
  // result.processed
  // result.referenceIsValid
  // result.capturedIsLive
  // result.isSameSubject
  // result.capturedPath
}
```

3.    Permissions
```
<uses-permission android:name="android.permission.CAMERA"/>
```


## iOS

0. Download the last FaceMatchSDK.framework.zip release from github


1.    Embed the framework
    •    Drag FaceMatchSDK.framework into your Xcode project
    •    In Build Phases → Embed Frameworks, add FaceMatchSDK.framework (Embed & Sign)
    •    Copy config.json & model files into Copy Bundle Resources
    
2.    Initialize & Use

(This assumes you have a `config.json` file stored in the main folder of your app)

```swift
import FaceMatchSDK

private let sdk: FaceMatchSDK = FaceMatchSDKImpl()
guard let configUrl = Bundle.main.url(forResource: "config", withExtension: "json"),
      let configData = try? Data(contentsOf: configUrl),
      let configString = String(data: configData, encoding: .utf8) else {
    print("Failed to load config.json")
    return
}
```

```swift
sdk.captureAndMatch(referenceImagePath: refPath) { result in
  // result.processed
  // result.referenceIsValid
  // result.capturedIsLive
  // result.isSameSubject
  // result.capturedPath
}
```

## API Reference

### Kotlin / Android

```kotlin
interface FaceMatchSDK {
  fun init(configJson: String): Boolean
  fun captureAndMatch(referenceImagePath: String, onResult: (MatchResult)->Unit)
  fun reset()
}
```

### Swift / iOS

```swift
public protocol FaceMatchSDK {
  /// Returns true if initialization succeeded.
  func initSDK(configJson: String) -> Bool

  /// Captures & matches, returns a MatchResult.
  func captureAndMatch(
    referenceImagePath: String,
    onResult: @escaping (MatchResult)->Void
  )

  /// Stops camera & clears state; must re-init to reuse.
  func reset()
}
```

## Demo Apps

    •    Android: see android/demoapp/
    •    iOS: see ios/FaceMatchSDK/FaceMatchSDKDemoApp/

Each demonstrates taking a reference image, invoking captureAndMatch, and displaying liveness & matching results.


## License

Released under MIT License, see LICENSE for details. Third-party licenses are included in `fmcore/deps/*/LICENSE.md` for library dependencies, and in the Google Drive folder for each machine learning model.
