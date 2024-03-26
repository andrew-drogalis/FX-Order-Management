pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh 'echo "Building..."'
        sh 'chmod +x build.sh'
        sh 'scripts/build.sh'
        archiveArtifacts(artifacts: 'scripts/teste.out', fingerprint: true)
      }
    }

  }
}
