plugins {
    alias(libs.plugins.android.library)
    alias(libs.plugins.kotlin.android)
    id("org.jetbrains.kotlin.plugin.compose") version "2.0.21"
    id("maven-publish")
}

android {
    namespace = "kl.open.fmandroid"
    compileSdk = 35

    defaultConfig {
        minSdk = 31

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        consumerProguardFiles("consumer-rules.pro")
        externalNativeBuild {
            cmake {
                cppFlags += ""
            }
        }
    }

    buildFeatures {
        compose = true
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    ndkVersion = "29.0.13113456 rc1"
    buildToolsVersion = "36.0.0"
}

dependencies {
    implementation(libs.androidx.camera.camera2)
    implementation(libs.androidx.camera.lifecycle)
    implementation(libs.androidx.camera.view)

    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.camera.core)
    implementation(libs.camera.lifecycle)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    implementation(libs.ui)
    implementation(libs.material3)
    implementation(libs.ui.tooling.preview)
    debugImplementation(libs.ui.tooling)

    implementation(libs.androidx.activity.compose.v172)

}

afterEvaluate {
    publishing {
        publications {
            create<MavenPublication>("release") {
                from(components["release"])
                groupId = "kl.open"
                artifactId = "facematchsdk"
                version = "0.0.3"
            }
        }

        repositories {
            maven {
                name = "Cloudsmith"

                val org = project.findProperty("cloudsmith.org") as String? ?: System.getenv("CLOUDSMITH_ORG")
                val repo = project.findProperty("cloudsmith.repo") as String? ?: System.getenv("CLOUDSMITH_REPO")

                url = uri("https://maven.cloudsmith.io/$org/$repo/")

                credentials {
                    username = project.findProperty("cloudsmith.user") as String? ?: System.getenv("CLOUDSMITH_USER")
                    password = project.findProperty("cloudsmith.key") as String? ?: System.getenv("CLOUDSMITH_KEY")
                }
            }
        }
    }
}
