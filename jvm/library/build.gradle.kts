import groovy.lang.Closure

plugins {
    `java-library`
    `maven-publish`
    kotlin("jvm")
    id("com.palantir.git-version")
}

repositories {
    mavenCentral()
}

dependencies {
    implementation(platform("org.jetbrains.kotlin:kotlin-bom"))
    implementation("org.jetbrains.kotlin:kotlin-stdlib-jdk8")
    implementation("net.java.dev.jna:jna-platform:5.16.0")
    api("net.java.dev.jna:jna:5.16.0")

    testImplementation("org.jetbrains.kotlin:kotlin-test")
    testImplementation("org.jetbrains.kotlin:kotlin-test-junit")
    testImplementation("org.junit.jupiter:junit-jupiter-engine:5.11.0-M2")
}

val javaLanguageVersion = JavaLanguageVersion.of(8)
val javaLanguageVersionTest = JavaLanguageVersion.of(17)

kotlin {
    jvmToolchain {
        languageVersion = javaLanguageVersion
    }
}

tasks {
    test {
        javaLauncher = project.javaToolchains.launcherFor {
            languageVersion = javaLanguageVersionTest
        }
    }

    compileTestKotlin {
        val launcher = project.javaToolchains.launcherFor {
            languageVersion = javaLanguageVersionTest
        }
        kotlinJavaToolchain.toolchain.use(launcher)
    }

    compileTestJava {
        javaCompiler = project.javaToolchains.compilerFor {
            languageVersion = javaLanguageVersionTest
        }
    }
}

val gitVersion: Closure<String> by extra

publishing {
    repositories {
        maven {
            name = "GitHubPackages"
            url = uri("https://maven.pkg.github.com/tira-io/tirex-tracker")
            credentials {
                username = project.findProperty("gpr.user") as String? ?: System.getenv("USERNAME")
                password = project.findProperty("gpr.key") as String? ?: System.getenv("TOKEN")
            }
        }
    }
    publications {
        register<MavenPublication>("gpr") {
            groupId = "io.tira"
            artifactId = "tirex-tracker"
            version = gitVersion()

            from(components["java"])

            pom {
                name = "tirex-tracker"
                description = "Automatic resource and metadata tracking for IR experiments."
                url = "https://github.com/tira-io/tirex-tracker"
                licenses {
                    license {
                        name = "MIT License"
                        url = "https://opensource.org/license/MIT"
                    }
                }
                developers {
                    developer {
                        name = "Jan Heinrich Merker"
                        email = "heinrich.merker@uni-jena.de"
                    }
                    developer {
                        name = "Tim Hagen"
                        email = "tim.hagen@uni-kassel.de"
                    }
                    developer {
                        name = "Maik Fröbe"
                        email = "maik.froebe@uni-jena.de"
                    }
                }
                scm {
                    connection = "scm:git:git://github.com/tira-io/tirex-tracker.git"
                    developerConnection = "scm:git:ssh://git@github.com:tira-io/tirex-tracker.git"
                    url = "https://github.com/tira-io/tirex-tracker"
                }
            }
        }
    }
}
