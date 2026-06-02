plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.universalac.host"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.universalac.host"
        minSdk = 23
        targetSdk = 36
        versionCode = 1
        versionName = "1.0.0"
        testInstrumentationRunner = "android.test.InstrumentationTestRunner"
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

kotlin {
    compilerOptions {
        jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_17)
    }
}

dependencies {
    implementation("com.github.mik3y:usb-serial-for-android:3.9.0")
    testImplementation("junit:junit:4.13.2")
}
