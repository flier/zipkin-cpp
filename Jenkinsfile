#!groovy

pipeline {
    agent any

    tools {
        cmake 'cmake-3'
    }

    triggers {
        cron('H 4/* 0 0 1-5')
    }

    stages {
        def build_dir = "${env.WORKSPACE}/build"
        def dist_dir = "${env.WORKSPACE}/dist"

        stage('Checkout') {
            steps {
                echo "Checkout branch `${env.GIT_BRANCH}` @ ${env.GIT_URL}"

                checkout scm
            }
        }

        stage('Build') {
            steps {
                echo "Build ${env.JOB_NAME} #${env.BUILD_ID} on ${env.JENKINS_URL}"

                sh "mkdir -p ${build_dir} && cd ${build_dir}"
                sh "cmake .. -DCMAKE_INSTALL_PREFIX=${dist_dir} && make"
            }
        }

        stage('Test') {
            steps {
                echo "Test ${env.JOB_NAME} #${env.BUILD_ID}"

                sh 'make test'
            }
        }

        stage('Deploy') {
            steps {
                echo "Deploy ${env.JOB_NAME} #${env.BUILD_ID}"

                sh 'make install'
            }
        }

        stage('Cleanup') {
            steps {
                echo "Cleanup ${env.JOB_NAME} #${env.BUILD_ID}"
            }
        }
    }
}