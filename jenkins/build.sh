docker build -f jenkins/Dockerfile.dev -t replace .
docker run -v $(pwd):/replace replace bash -c "./replace/jenkins/install.sh"