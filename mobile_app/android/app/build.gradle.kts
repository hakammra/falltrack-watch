plugins {
    id("com.android.application")
    id("kotlin-android")
}

android {
    namespace = "com.example.falldetector_new"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.example.falldetector_new"
        minSdk = 21
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"
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
}

dependencies {
    implementation("org.jetbrains.kotlin:kotlin-stdlib:1.7.20")
}