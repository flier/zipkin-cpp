#!groovy

pipeline {
    agent any

    triggers {
        cron('H 4/* 0 0 1-5')
    }

    stages {
        stage('Checkout') {
            steps {
                echo "Checkout branch `${env.GIT_BRANCH}` @ ${env.GIT_URL}"

                checkout scm
            }
        }

        stage('Build') {
            steps {
                echo "Build ${env.JOB_NAME} #${env.BUILD_ID} on ${env.JENKINS_URL}"

                sh 'mkdir -p build && cd build'
                sh "cmake .. -DCMAKE_INSTALL_PREFIX=${env.WORKSPACE}/dist && make"
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